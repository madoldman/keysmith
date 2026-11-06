/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2020 Johan Ouwerkerk <jm.ouwerkerk@gmail.com>
 */
#include "hmac.h"
#include "../logging_p.h"

#include <memory>
#include <optional>

KEYSMITH_LOGGER(logger, ".hmac")

namespace
{
/*
 * 复用 QCryptographicHash 实例（性能修复）：
 * 实测在本机 OpenSSL 4（libcrypto.so.4）下，每次构造/析构 QCryptographicHash 都会触发
 * EVP_MD_fetch / OSSL_PROVIDER_unload 与大范围 CRYPTO_zalloc（逐页 mprotect），
 * 且单次成本随进程累计构造次数线性退化：微基准每轮构造/析构 500 次，
 * 第 0 轮约 0.064ms/次，第 7 轮退化到约 0.828ms/次（涨 13 倍）。
 * keysmith 的工作线程长期常驻，每个 TOTP token 经两次 hmac_stage，每 30 秒全量重算，
 * 数百万次累计构造后退化到每次数百毫秒，把 Dispatcher 工作线程占满一整个核。
 * 对照实测：复用同一实例并 reset() 后 8 轮 x 500 次全部约 0ms、零退化，
 * 且 reset()+addData()+result() 与新建对象的结果逐字节一致，令牌值不受影响。
 */
class ReusableHash
{
public:
    /*
     * 取得可复用实例：算法不变时 reset() 后与新构造对象等价；
     * 算法变化（跨账号切换 SHA1/SHA256/SHA512）时销毁重建，频率极低。
     */
    QCryptographicHash &acquire(QCryptographicHash::Algorithm requested)
    {
        if (!m_hash || m_algorithm != requested) {
            m_hash = std::make_unique<QCryptographicHash>(requested);
            m_algorithm = requested;
        } else {
            m_hash->reset();
        }
        return *m_hash;
    }

private:
    std::unique_ptr<QCryptographicHash> m_hash;
    std::optional<QCryptographicHash::Algorithm> m_algorithm;
};

// 按工作线程各持一份缓存：工作线程是单一 Dispatcher 线程，thread_local 天然线程安全。
// hmac_stage 与长 key 摘要（ksize > blockSize 分支）分开持有，避免两处中间状态互相影响。
thread_local ReusableHash stageHashInstance;
thread_local ReusableHash keyHashInstance;
}

static QByteArray hmac_stage(QCryptographicHash::Algorithm algorithm,
                             char *const keyBuf,
                             int ksize,
                             int blockSize,
                             const char fillPad,
                             const char xorKey,
                             const QByteArray &message)
{
    // 复用而非重新构造：见 ReusableHash 类注释（构造/析构成本随累计调用次数线性退化）
    QCryptographicHash &hash = stageHashInstance.acquire(algorithm);
    int count;
    for (count = 0; count < ksize; ++count) {
        keyBuf[count] ^= xorKey;
    }

    hash.addData(QByteArrayView(keyBuf, count));

    if (ksize < blockSize) {
        QByteArray pad(blockSize - ksize, fillPad);
        hash.addData(pad);
    }

    hash.addData(message);

    return hash.result();
}

namespace hmac
{
std::optional<int> blockSize(QCryptographicHash::Algorithm algorithm)
{
    switch (algorithm) {
    case QCryptographicHash::Md4:
    case QCryptographicHash::Md5:
    case QCryptographicHash::Sha1:
    case QCryptographicHash::Sha224:
    case QCryptographicHash::Sha256:
        return std::optional<int>(64ULL);
    case QCryptographicHash::Sha384:
    case QCryptographicHash::Sha512:
        return std::optional<int>(128ULL);
    case QCryptographicHash::RealSha3_224:
    case QCryptographicHash::Keccak_224:
        return std::optional<int>(144ULL);
    case QCryptographicHash::RealSha3_256:
    case QCryptographicHash::Keccak_256:
        return std::optional<int>(136ULL);
    case QCryptographicHash::RealSha3_384:
    case QCryptographicHash::Keccak_384:
        return std::optional<int>(104ULL);
    case QCryptographicHash::RealSha3_512:
    case QCryptographicHash::Keccak_512:
        return std::optional<int>(72ULL);
    default:
        return std::nullopt;
    }
}

std::optional<uint> outputSize(QCryptographicHash::Algorithm algorithm)
{
    switch (algorithm) {
    case QCryptographicHash::Md4:
    case QCryptographicHash::Md5:
        return std::optional<uint>(16ULL);
    case QCryptographicHash::Sha1:
        return std::optional<uint>(20ULL);
    case QCryptographicHash::Sha224:
    case QCryptographicHash::RealSha3_224:
    case QCryptographicHash::Keccak_224:
        return std::optional<int>(28UL);
    case QCryptographicHash::Sha256:
    case QCryptographicHash::RealSha3_256:
    case QCryptographicHash::Keccak_256:
        return std::optional<int>(32UL);
    case QCryptographicHash::Sha384:
    case QCryptographicHash::RealSha3_384:
    case QCryptographicHash::Keccak_384:
        return std::optional<int>(48UL);
    case QCryptographicHash::Sha512:
    case QCryptographicHash::RealSha3_512:
    case QCryptographicHash::Keccak_512:
        return std::optional<uint>(64UL);
    default:
        return std::nullopt;
    }
}

bool validateKeySize(QCryptographicHash::Algorithm algorithm, int ksize, bool requireSaneKeyLength)
{
    std::optional<int> maybeOutputSize = outputSize(algorithm);
    return maybeOutputSize && ksize >= 0 && (ksize >= (*maybeOutputSize) || !requireSaneKeyLength);
}

std::optional<QByteArray> compute(QCryptographicHash::Algorithm algorithm, char *rawKey, int ksize, const QByteArray &message, bool requireSaneKeyLength)
{
    if (!rawKey) {
        return std::nullopt;
    }

    std::optional<int> maybeBlockSize = blockSize(algorithm);
    if (!maybeBlockSize) {
        qCDebug(logger) << "Unable to determine block size for hashing algorithm:" << algorithm;
        return std::nullopt;
    }

    if (!validateKeySize(algorithm, ksize, requireSaneKeyLength)) {
        qCDebug(logger) << "Invalid HMAC key size:" << ksize << "for hashing algorithm:" << algorithm
                        << "Sane key length requirements apply:" << requireSaneKeyLength;
        return std::nullopt;
    }

    int bs = *maybeBlockSize;
    QByteArray hashedKey;

    if (ksize > bs) {
        // 长 key 摘要同样复用实例（独立于 stage 用的实例），理由见 ReusableHash 类注释
        QCryptographicHash &khash = keyHashInstance.acquire(algorithm);
        khash.addData(QByteArrayView(rawKey, ksize));
        hashedKey = khash.result();
        rawKey = hashedKey.data();
        ksize = hashedKey.size();
    }

    QByteArray inner = hmac_stage(algorithm, rawKey, ksize, bs, '\x36', '\x36', message);
    QByteArray result = hmac_stage(algorithm, rawKey, ksize, bs, '\x5c', '\x36' ^ '\x5c', inner);

    hashedKey.fill('\x0');
    return std::optional<QByteArray>(result);
}
}

/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2020-2021 Johan Ouwerkerk <jm.ouwerkerk@gmail.com>
 */
#include "oath/oath.h"

#include <QDateTime>
#include <QTest>
#include <QtDebug>

#include <limits>

class TimeStepCountConversionTest : public QObject // clazy:exclude=ctor-missing-parent-argument
{
    Q_OBJECT
private Q_SLOTS:
    void invalidCount(void);
    void invalidCount_data(void);
    void validCount(void);
    void validCount_data(void);
};

/*
 * Probe the largest number of milliseconds since epoch which QDateTime can cope with on
 * this (version of the) Qt build, e.g. as epoch of a token.
 *
 * 这里的边界值依赖 Qt 的 QDateTime 实现：QDateTime 文档明确其无法覆盖完整的 qint64
 * 宽度，但未写明具体限制，且该限制会随 Qt 版本变化。早期测试曾硬编码探测出的魔数
 * （0x7fffffffff921fd8，见 3c0fc62 "test: bump limits to trigger confusion in QDateTime"），
 * Qt 升级后该魔数超出了新版 QDateTime 的表示能力，fromMSecsSinceEpoch() 返回 invalid，
 * 用例随之失效。为避免测试再次被 Qt 版本升级"漂移"击穿，这里改为运行时探测当前
 * QDateTime 实际支持的最大毫秒值（二分查找；其正确性依赖"有效区间向下连续、自上限
 * 起单调"这一边界形状，实测各 Qt 版本均如此）。
 */
static qint64 probe_upper_datetime_limit(void)
{
    static const auto isValidAt = [](qint64 msecs) {
        const QDateTime datetime = QDateTime::fromMSecsSinceEpoch(msecs);
        return datetime.isValid() && datetime.toMSecsSinceEpoch() == msecs;
    };

    qint64 low = 0LL; // the epoch itself is well within range
    qint64 high = std::numeric_limits<qint64>::max();
    if (isValidAt(high)) {
        return high;
    }
    while (high - low > 1LL) {
        const qint64 mid = low + (high - low) / 2LL;
        if (isValidAt(mid)) {
            low = mid;
        } else {
            high = mid;
        }
    }
    return low;
}

static void define_valid_test_case(const QDateTime &epoch, uint timeStep, quint64 count, const QDateTime &expected)
{
    static const QString testCase(QStringLiteral("%1 + %2 * 1000 * %3 ... %4"));
    QTest::newRow(qPrintable(testCase.arg(epoch.toMSecsSinceEpoch()).arg(timeStep).arg(count).arg(expected.toMSecsSinceEpoch())))
        << epoch << timeStep << count << expected;
}

static void define_invalid_test_case(const QDateTime &epoch, uint timeStep, quint64 count)
{
    static const QString testCase(QStringLiteral("%1 + %2 * 1000 * %3"));
    QTest::newRow(qPrintable(testCase.arg(epoch.toMSecsSinceEpoch()).arg(timeStep).arg(count))) << epoch << timeStep << count;
}

void TimeStepCountConversionTest::validCount(void)
{
    QFETCH(QDateTime, epoch);
    QFETCH(uint, timeStep);
    QFETCH(quint64, count);

    auto result = oath::fromCounter(count, epoch, timeStep);
    QVERIFY2(result, "should be able to determine the datetime associated with the counter");
    QTEST(*result, "expected");
}

void TimeStepCountConversionTest::validCount_data(void)
{
    QTest::addColumn<QDateTime>("epoch");
    QTest::addColumn<uint>("timeStep");
    QTest::addColumn<quint64>("count");
    QTest::addColumn<QDateTime>("expected");

    const qint64 min = std::numeric_limits<qint64>::min();
    const qint64 s2038 = std::numeric_limits<qint32>::max();

    // few basic cases
    define_valid_test_case(QDateTime::fromMSecsSinceEpoch(0LL), 30UL, 0ULL, QDateTime::fromMSecsSinceEpoch(0LL));
    define_valid_test_case(QDateTime::fromMSecsSinceEpoch(0LL), 30UL, 1ULL, QDateTime::fromMSecsSinceEpoch(30'000LL));
    define_valid_test_case(QDateTime::fromMSecsSinceEpoch(-30'000LL), 30UL, 1ULL, QDateTime::fromMSecsSinceEpoch(0LL));

    // checking max 'classic' 32bit unix time for counting utc seconds
    define_valid_test_case(QDateTime::fromMSecsSinceEpoch(0LL), 30UL, s2038 / 30LL, QDateTime::fromMSecsSinceEpoch(30'000LL * (s2038 / 30LL)));
    define_valid_test_case(QDateTime::fromMSecsSinceEpoch(0LL), 1UL, s2038, QDateTime::fromMSecsSinceEpoch(1'000LL * s2038));

    // finding the actual upper limit(s) which QDateTime can cope with
    //
    // 上限在运行时探测（见 probe_upper_datetime_limit 的注释），不硬编码会随 Qt
    // 版本失效的魔数。由此派生上限附近的三个边界用例（保留原用例意图：正的 epoch
    // 偏移、count 减 1、以及 fromCounter 在这些点应返回有效结果）：
    // - epoch 偏移取自上限对 1000 的余数：当前 Qt 6.12.0 实测上限为
    //   9223372036825975807（余数 807），此时偏移恰为 807/808，与旧硬编码一致。
    // - 第二例 (remainder + 1) + 1000 * (limit/1000 - 1) = limit - 999，恒在上限之下；
    // - 第三例 remainder + 1000 * (limit/1000) = limit，恰好命中最上限本身。
    const qint64 upperLimit = probe_upper_datetime_limit();
    QVERIFY2(upperLimit > 0LL, "could not probe the maximum datetime (msecs since epoch) supported by QDateTime");
    const qint64 upperRemainder = upperLimit % 1000LL;
    const quint64 upperCount = (quint64)(upperLimit / 1000LL);

    define_valid_test_case(QDateTime::fromMSecsSinceEpoch(0LL), 1UL, upperCount, QDateTime::fromMSecsSinceEpoch(1000LL * (qint64)upperCount));
    define_valid_test_case(QDateTime::fromMSecsSinceEpoch(upperRemainder + 1LL), 1UL, upperCount - 1ULL, QDateTime::fromMSecsSinceEpoch(upperLimit - 999LL));
    define_valid_test_case(QDateTime::fromMSecsSinceEpoch(upperRemainder), 1UL, upperCount, QDateTime::fromMSecsSinceEpoch(upperLimit));

    // finding the lower limit(s) which QDateTime can cope with
    define_valid_test_case(QDateTime::fromMSecsSinceEpoch(min), 1UL, 0ULL, QDateTime::fromMSecsSinceEpoch(min));
}

void TimeStepCountConversionTest::invalidCount(void)
{
    QFETCH(QDateTime, epoch);
    QFETCH(uint, timeStep);
    QFETCH(quint64, count);

    QVERIFY2(!oath::fromCounter(count, epoch, timeStep), "should not be able to associate a datetime with the counter");
}

void TimeStepCountConversionTest::invalidCount_data(void)
{
    QTest::addColumn<QDateTime>("epoch");
    QTest::addColumn<uint>("timeStep");
    QTest::addColumn<quint64>("count");

    const quint64 max = std::numeric_limits<quint64>::max();

    define_invalid_test_case(QDateTime::fromMSecsSinceEpoch(0LL), 1UL, max / 2);
    define_invalid_test_case(QDateTime::fromMSecsSinceEpoch(0LL), 1UL, max);
    define_invalid_test_case(QDateTime::fromMSecsSinceEpoch(std::numeric_limits<qint64>::min()), 1UL, max);
    define_invalid_test_case(QDateTime::fromMSecsSinceEpoch(std::numeric_limits<qint64>::min()), 1UL, max / 1000ULL + 1ULL);

    // fits within qint64 ultimately but QDateTime gets confused about it
    define_invalid_test_case(QDateTime::fromMSecsSinceEpoch(0LL), 1ULL, (quint64)(max / 1000LL));
    define_invalid_test_case(QDateTime::fromMSecsSinceEpoch(0LL), 1ULL, (quint64)(max >> 9));
    define_invalid_test_case(QDateTime::fromMSecsSinceEpoch(0LL), 1ULL, (quint64)(max >> 10) + (max >> 11L));
    define_invalid_test_case(QDateTime::fromMSecsSinceEpoch(808LL), 1ULL, 0x7fffffffffffffffULL / 1000ULL);
}

QTEST_APPLESS_MAIN(TimeStepCountConversionTest)

#include "convert-timestep-counter.moc"

/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 Keysmith Contributors
 */
#include "output.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

using namespace Qt::Literals::StringLiterals;

namespace model
{

ExportOutput::ExportOutput(QObject *parent)
    : QObject(parent)
    , m_format(ExportFormat::AndOTPPlainJSON)
{
}

void ExportOutput::reset(void)
{
    setFile(QString());
    setFormat(ExportFormat::AndOTPPlainJSON);
}

QString ExportOutput::file(void) const
{
    return m_file;
}

void ExportOutput::setFile(const QString &file)
{
    QUrl url(file);
    if ((url.isLocalFile() && m_file != url.toLocalFile()) || m_file != file) {
        m_file = url.isLocalFile() ? url.toLocalFile() : file;
        Q_EMIT fileChanged();
    }
}

ExportOutput::ExportFormat ExportOutput::format(void) const
{
    return m_format;
}

void ExportOutput::setFormat(model::ExportOutput::ExportFormat format)
{
    if (m_format != format) {
        m_format = format;
        Q_EMIT formatChanged();
    }
}

static QJsonArray writeAndOTP(accounts::AccountStorage *storage)
{
    QJsonArray array;
    QList<QString> names = storage->accounts();
    accounts::AccountSecret *secret = storage->secret();

    for (const QString &name : names) {
        accounts::Account *account = storage->get(name);
        if (!account) {
            continue;
        }

        QJsonObject otp;

        QString decrypted = account->decryptedSecret(secret);
        otp["secret"_L1] = decrypted;

        QString issuer = account->issuer();
        if (!issuer.isEmpty()) {
            otp["issuer"_L1] = issuer;
        } else {
            otp["issuer"_L1] = QString();
        }

        // andOTP label format: "Issuer:Name" if issuer is present, else "Name"
        if (!issuer.isEmpty()) {
            otp["label"_L1] = QString(issuer + QLatin1Char(':') + account->name());
        } else {
            otp["label"_L1] = account->name();
        }

        otp["digits"_L1] = account->tokenLength();

        if (account->algorithm() == accounts::Account::Hotp) {
            otp["type"_L1] = "HOTP"_L1;
            otp["counter"_L1] = static_cast<qint64>(account->counter());
        } else {
            otp["type"_L1] = "TOTP"_L1;
            otp["period"_L1] = static_cast<int>(account->timeStep());
        }

        switch (account->hash()) {
        case accounts::Account::Sha1:
            otp["algorithm"_L1] = "SHA1"_L1;
            break;
        case accounts::Account::Sha256:
            otp["algorithm"_L1] = "SHA256"_L1;
            break;
        case accounts::Account::Sha512:
            otp["algorithm"_L1] = "SHA512"_L1;
            break;
        }

        otp["tags"_L1] = QJsonArray();

        array.append(otp);
    }

    return array;
}

static QJsonObject writeAegis(accounts::AccountStorage *storage)
{
    QJsonArray entries;
    QList<QString> names = storage->accounts();
    accounts::AccountSecret *secret = storage->secret();

    for (const QString &name : names) {
        accounts::Account *account = storage->get(name);
        if (!account) {
            continue;
        }

        QJsonObject entry;

        if (account->algorithm() == accounts::Account::Hotp) {
            entry["type"_L1] = "hotp"_L1;
        } else {
            entry["type"_L1] = "totp"_L1;
        }

        entry["name"_L1] = account->name();
        entry["issuer"_L1] = account->issuer();

        QJsonObject info;

        QString decrypted = account->decryptedSecret(secret);
        info["secret"_L1] = decrypted;
        info["digits"_L1] = account->tokenLength();

        switch (account->hash()) {
        case accounts::Account::Sha1:
            info["algo"_L1] = "SHA1"_L1;
            break;
        case accounts::Account::Sha256:
            info["algo"_L1] = "SHA256"_L1;
            break;
        case accounts::Account::Sha512:
            info["algo"_L1] = "SHA512"_L1;
            break;
        }

        if (account->algorithm() == accounts::Account::Hotp) {
            info["counter"_L1] = static_cast<qint64>(account->counter());
        } else {
            info["period"_L1] = static_cast<int>(account->timeStep());
        }

        entry["info"_L1] = info;
        entries.append(entry);
    }

    QJsonObject db;
    db["entries"_L1] = entries;

    QJsonObject root;
    root["db"_L1] = db;

    return root;
}

bool ExportOutput::exportAccounts(accounts::AccountStorage *storage) const
{
    if (!storage || m_file.isEmpty()) {
        return false;
    }

    QJsonDocument doc;
    switch (m_format) {
    case AndOTPPlainJSON:
        doc = QJsonDocument(writeAndOTP(storage));
        break;
    case AegisPlainJSON:
        doc = QJsonDocument(writeAegis(storage));
        break;
    default:
        return false;
    }

    QFile file(m_file);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    file.write(doc.toJson());
    file.close();
    return true;
}
}

#include "moc_output.cpp"

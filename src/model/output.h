/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 Keysmith Contributors
 */
#ifndef MODEL_OUTPUT_H
#define MODEL_OUTPUT_H

#include "../account/account.h"

#include <QObject>
#include <QString>

namespace model
{
class ExportOutput : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString file READ file WRITE setFile NOTIFY fileChanged)
    Q_PROPERTY(model::ExportOutput::ExportFormat format READ format WRITE setFormat NOTIFY formatChanged)
public:
    enum ExportFormat {
        AndOTPPlainJSON,
        AegisPlainJSON
    };
    Q_ENUM(ExportFormat)
    ExportOutput(QObject *parent = nullptr);
    Q_INVOKABLE void reset(void);

public:
    QString file(void) const;
    void setFile(const QString &file);
    ExportFormat format(void) const;
    void setFormat(model::ExportOutput::ExportFormat format);
    bool exportAccounts(accounts::AccountStorage *storage) const;
Q_SIGNALS:
    void fileChanged(void);
    void formatChanged(void);

private:
    QString m_file;
    ExportFormat m_format;
};
}

#endif

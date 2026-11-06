#ifndef KEYSMITH_APPLET_H
#define KEYSMITH_APPLET_H

#include <QAbstractListModel>
#include <QElapsedTimer>
#include <QObject>
#include <QQmlEngine>
#include <QTimer>

#include "app/keysmith.h"
#include "model/accounts.h"
#include "model/input.h"
#include "model/output.h"
#include "model/password.h"

namespace accounts
{
class AccountSecret;
}

class AccountsModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        TokenRole,
        IsTotpRole,
        IssuerRole,
        TimeStepRole
    };

    explicit AccountsModel(QObject *parent = nullptr)
        : QAbstractListModel(parent)
    {
        // 合并窗口结束时的真正全量重算；触发时 m_accounts 已是最新列表，
        // 启动期连续多次 setSourceModel 只产生一次全量重算。
        m_recomputeCoalescer.setSingleShot(true);
        m_recomputeCoalescer.setInterval(50);
        QObject::connect(&m_recomputeCoalescer, &QTimer::timeout, this, [this]() {
            for (auto *account : m_accounts) {
                Q_EMIT account->recompute();
            }
        });
    }

    void setSourceModel(model::SimpleAccountListModel *source);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        Q_UNUSED(parent);
        return m_accounts.count();
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    QHash<int, QByteArray> roleNames() const override
    {
        QHash<int, QByteArray> roles;
        roles[NameRole] = "name";
        roles[TokenRole] = "token";
        roles[IsTotpRole] = "isTotp";
        roles[IssuerRole] = "issuer";
        roles[TimeStepRole] = "timeStep";
        return roles;
    }

    Q_INVOKABLE qint64 millisecondsLeftForToken(int row) const;
    Q_INVOKABLE void recompute(int row);

private Q_SLOTS:
    void onTokenChanged();

public:
    void triggerRecompute();

    QVector<model::AccountView *> m_accounts;
    model::SimpleAccountListModel *m_source = nullptr;

private:
    // 合并触发窗口：启动/加载期 loadedChanged/added/removed/keyAvailable 会在极短时间内
    // 连续多次调用 refreshModel() -> setSourceModel()（每次无条件全量重算 N 个账号，
    // 形成 O(N²) 突发）。用单次 QTimer 把窗口内的多次全量重算合并为一次。
    QTimer m_recomputeCoalescer;
    // in-flight 去重：记录最近一次单行 recompute 请求的行号与时间，
    // 同一行在短窗口内重复请求直接忽略，避免相同 job 在 Dispatcher 队列里堆积。
    int m_lastRecomputeRow = -1;
    QElapsedTimer m_lastRecomputeAt;
};

class KeysmithApplet : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Keysmith)
    QML_SINGLETON
    Q_PROPERTY(AccountsModel *accountsModel READ accountsModel CONSTANT)
    Q_PROPERTY(bool locked READ locked NOTIFY lockedChanged)
    Q_PROPERTY(bool needsSetup READ needsSetup NOTIFY needsSetupChanged)
    Q_PROPERTY(bool passwordRequestResolved READ passwordRequestResolved NOTIFY passwordRequestResolvedChanged)

public:
    ~KeysmithApplet() override;

    static KeysmithApplet *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);

    AccountsModel *accountsModel() const
    {
        return m_accountsModel;
    }
    app::Keysmith *keysmith() const
    {
        return m_keysmith;
    }
    bool locked() const
    {
        return m_locked;
    }
    bool needsSetup() const
    {
        return m_needsSetup;
    }
    bool passwordRequestResolved() const
    {
        return m_passwordRequestResolved;
    }

    Q_INVOKABLE void addAccount(const QString &name, const QString &secret);
    Q_INVOKABLE void addAccountEx(const QString &name,
                                  const QString &issuer,
                                  const QString &secret,
                                  int type,
                                  int timeStep,
                                  int algorithm,
                                  int tokenLength,
                                  const QString &epoch,
                                  int counter,
                                  bool checksum,
                                  bool useFixedTruncation,
                                  int truncationOffset);
    Q_INVOKABLE void importAccountsFromFile(const QString &filePath, int format, const QString &password);
    Q_INVOKABLE bool exportAccountsToFile(const QString &filePath, int format);
    Q_INVOKABLE void removeAccount(int row);
    Q_INVOKABLE void copyToClipboard(const QString &text);
    Q_INVOKABLE bool providePassword(const QString &password);
    Q_INVOKABLE bool provideNewPassword(const QString &password, const QString &confirm);
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void recomputeAll();

Q_SIGNALS:
    void lockedChanged(void);
    void needsSetupChanged(void);
    void passwordRequestResolvedChanged(void);

private:
    explicit KeysmithApplet(QObject *parent = nullptr);
    void init(QQmlEngine *engine);
    void setLocked(bool locked);
    void setNeedsSetup(bool needsSetup);
    void setPasswordRequestResolved(bool resolved);
    void refreshModel(void);
    void tryAutoUnlock(void);
    void checkInitialState(void);

    app::Keysmith *m_keysmith;
    app::Navigation *m_navigation;
    AccountsModel *m_accountsModel;
    model::PasswordRequest *m_passwordRequest;
    accounts::AccountSecret *m_secret;
    bool m_locked;
    bool m_needsSetup;
    bool m_passwordRequestResolved;
};

#endif

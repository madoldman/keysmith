#include "keysmithapplet.h"

#include <QClipboard>
#include <QFile>
#include <QGuiApplication>
#include <QQmlContext>
#include <QTextStream>
#include <QUrl>
#include <QUrlQuery>

#include "account/keys.h"
#include "app/keysmith.h"
#include "model/accounts.h"
#include "model/input.h"
#include "model/output.h"

KeysmithApplet::KeysmithApplet(QObject *parent)
    : QObject(parent)
    , m_keysmith(nullptr)
    , m_navigation(nullptr)
    , m_accountsModel(new AccountsModel(this))
    , m_passwordRequest(nullptr)
    , m_secret(nullptr)
    , m_locked(true)
    , m_needsSetup(false)
    , m_passwordRequestResolved(false)
{
}

KeysmithApplet::~KeysmithApplet()
{
}

KeysmithApplet *KeysmithApplet::create(QQmlEngine *qmlEngine, QJSEngine *jsEngine)
{
    Q_UNUSED(jsEngine);
    auto *applet = new KeysmithApplet(qmlEngine);
    applet->init(qmlEngine);
    return applet;
}

void KeysmithApplet::init(QQmlEngine *engine)
{
    m_navigation = new app::Navigation(engine);
    m_keysmith = new app::Keysmith(m_navigation, this);
    m_keysmith->setAutoUnlockEnabled(true);

    model::SimpleAccountListModel *model = m_keysmith->accountListModel();
    m_secret = m_keysmith->store().accounts()->secret();

    // Create a PasswordRequest for the providePassword/provideBothPasswords methods
    m_passwordRequest = new model::PasswordRequest(m_secret, this);

    // When accounts are loaded, refresh the accounts model
    connect(model, &model::SimpleAccountListModel::loadedChanged, this, [this]() {
        refreshModel();
    });

    // When accounts are added/removed asynchronously, refresh the accounts model
    connect(m_keysmith->store().accounts(), &accounts::AccountStorage::added, this, [this]() {
        refreshModel();
    });
    connect(m_keysmith->store().accounts(), &accounts::AccountStorage::removed, this, [this]() {
        refreshModel();
    });

    // Directly connect to AccountSecret signals for reliable cross-thread delivery
    connect(m_secret, &accounts::AccountSecret::keyAvailable, this, [this]() {
        setLocked(false);
        refreshModel();
    });

    connect(m_secret, &accounts::AccountSecret::existingPasswordNeeded, this, [this]() {
        setPasswordRequestResolved(true);
        // Delay auto-unlock slightly to ensure keyParams/salt are set
        QTimer::singleShot(0, this, &KeysmithApplet::tryAutoUnlock);
    });

    connect(m_secret, &accounts::AccountSecret::newPasswordNeeded, this, [this]() {
        setPasswordRequestResolved(true);
        setNeedsSetup(true);
    });

    // Also connect via PasswordRequest as a backup signal path
    connect(m_passwordRequest, &model::PasswordRequest::passwordAccepted, this, [this]() {
        setLocked(false);
        refreshModel();
    });

    connect(m_passwordRequest, &model::PasswordRequest::passwordRequestChanged, this, [this]() {
        if (m_passwordRequest->previouslyDefined() && !m_passwordRequestResolved) {
            setPasswordRequestResolved(true);
            QTimer::singleShot(0, this, &KeysmithApplet::tryAutoUnlock);
        } else if (m_passwordRequest->firstRun() && !m_passwordRequestResolved) {
            setPasswordRequestResolved(true);
            setNeedsSetup(true);
        }
    });

    // Check initial state in case signals were already emitted
    checkInitialState();

    // Use a timer to poll for state changes as a fallback
    // This handles any race conditions with cross-thread signal delivery
    QTimer *pollTimer = new QTimer(this);
    pollTimer->setInterval(200);
    connect(pollTimer, &QTimer::timeout, this, &KeysmithApplet::checkInitialState);
    pollTimer->start();

    // Stop polling once we're unlocked
    connect(this, &KeysmithApplet::lockedChanged, this, [pollTimer]() {
        pollTimer->stop();
    });

    // Initial model setup
    m_accountsModel->setSourceModel(model);
}

void KeysmithApplet::checkInitialState(void)
{
    if (!m_secret)
        return;

    if (m_secret->isKeyAvailable()) {
        setLocked(false);
        setPasswordRequestResolved(true);
        return;
    }

    if (m_secret->isExistingPasswordRequested()) {
        if (!m_passwordRequestResolved) {
            setPasswordRequestResolved(true);
        }
        tryAutoUnlock();
        return;
    }

    if (m_secret->isNewPasswordRequested()) {
        if (!m_passwordRequestResolved) {
            setPasswordRequestResolved(true);
        }
        setNeedsSetup(true);
        return;
    }
}

void KeysmithApplet::tryAutoUnlock(void)
{
    if (!m_keysmith->isAutoUnlockEnabled()) {
        return;
    }

    if (!m_secret || !m_secret->isExistingPasswordRequested()) {
        return;
    }

    if (m_secret->isKeyAvailable()) {
        setLocked(false);
        return;
    }

    if (m_secret->autoUnlockFromWallet()) {
        setLocked(false);
        refreshModel();
    }
}

void KeysmithApplet::setLocked(bool locked)
{
    if (m_locked != locked) {
        m_locked = locked;
        Q_EMIT lockedChanged();
    }
}

void KeysmithApplet::setNeedsSetup(bool needsSetup)
{
    if (m_needsSetup != needsSetup) {
        m_needsSetup = needsSetup;
        Q_EMIT needsSetupChanged();
    }
}

void KeysmithApplet::setPasswordRequestResolved(bool resolved)
{
    if (m_passwordRequestResolved != resolved) {
        m_passwordRequestResolved = resolved;
        Q_EMIT passwordRequestResolvedChanged();
    }
}

void AccountsModel::setSourceModel(model::SimpleAccountListModel *source)
{
    beginResetModel();
    m_source = source;
    m_accounts.clear();

    if (source) {
        for (int i = 0; i < source->rowCount(); ++i) {
            QVariant data = source->data(source->index(i), model::SimpleAccountListModel::AccountRole);
            model::AccountView *account = qvariant_cast<model::AccountView *>(data);
            if (account) {
                m_accounts.append(account);
                connect(account, &model::AccountView::tokenChanged, this, &AccountsModel::onTokenChanged);
            }
        }
        std::sort(m_accounts.begin(), m_accounts.end(), [](const model::AccountView *a, const model::AccountView *b) {
            const QString aIssuer = a->issuer();
            const QString bIssuer = b->issuer();
            if (aIssuer.isNull() && !bIssuer.isNull())
                return true;
            if (!aIssuer.isNull() && bIssuer.isNull())
                return false;
            if (!aIssuer.isNull() && !bIssuer.isNull()) {
                int cmp = aIssuer.localeAwareCompare(bIssuer);
                if (cmp != 0)
                    return cmp < 0;
            }
            return a->name().localeAwareCompare(b->name()) < 0;
        });
    }

    endResetModel();
    triggerRecompute();
}

QVariant AccountsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_accounts.count()) {
        return QVariant();
    }

    model::AccountView *account = m_accounts[index.row()];

    switch (role) {
    case NameRole:
        return account->name();
    case TokenRole:
        return account->token();
    case IsTotpRole:
        return account->isTotp();
    case IssuerRole:
        return account->issuer();
    case TimeStepRole:
        return account->timeStep();
    default:
        return QVariant();
    }
}

qint64 AccountsModel::millisecondsLeftForToken(int row) const
{
    if (row < 0 || row >= m_accounts.count()) {
        return -1;
    }
    return m_accounts[row]->millisecondsLeftForToken();
}

void AccountsModel::recompute(int row)
{
    if (row < 0 || row >= m_accounts.count()) {
        return;
    }
    Q_EMIT m_accounts[row]->recompute();
}

void AccountsModel::onTokenChanged()
{
    model::AccountView *account = qobject_cast<model::AccountView *>(sender());
    if (!account)
        return;
    int idx = m_accounts.indexOf(account);
    if (idx < 0)
        return;
    QModelIndex mi = index(idx);
    Q_EMIT dataChanged(mi, mi, {TokenRole});
}

void AccountsModel::triggerRecompute()
{
    for (auto *account : m_accounts) {
        Q_EMIT account->recompute();
    }
}

void KeysmithApplet::refreshModel(void)
{
    if (m_keysmith) {
        m_accountsModel->setSourceModel(m_keysmith->accountListModel());
    }
}

void KeysmithApplet::addAccount(const QString &name, const QString &secret)
{
    if (!m_keysmith) {
        return;
    }

    model::AccountInput input;
    input.setName(name);
    input.setSecret(secret);
    input.setType(model::AccountInput::Totp);

    m_keysmith->accountListModel()->addAccount(&input);
}

void KeysmithApplet::addAccountEx(const QString &name,
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
                                  int truncationOffset)
{
    if (!m_keysmith) {
        return;
    }

    model::AccountInput input;
    input.setName(name);
    input.setIssuer(issuer);
    input.setSecret(secret);
    input.setType(type == 0 ? model::AccountInput::Totp : model::AccountInput::Hotp);
    input.setTimeStep(timeStep);
    input.setTokenLength(tokenLength);
    input.setAlgorithm(static_cast<model::AccountInput::TOTPAlgorithm>(algorithm));
    if (!epoch.isEmpty()) {
        input.setEpoch(epoch);
    }
    input.setCounter(counter);
    if (checksum) {
        input.setChecksum(checksum);
    }
    if (useFixedTruncation) {
        input.setTruncationOffset(truncationOffset);
    } else {
        input.setDynamicTruncation();
    }

    m_keysmith->accountListModel()->addAccount(&input);
}

void KeysmithApplet::importAccountsFromFile(const QString &filePath, int format, const QString &password)
{
    if (!m_keysmith) {
        return;
    }

    model::ImportInput input;
    input.setFile(filePath);
    input.setFormat(static_cast<model::ImportInput::ImportFormat>(format));
    input.setPassword(password);

    std::vector<std::unique_ptr<model::AccountInput>> accounts = input.importAccounts();
    for (std::unique_ptr<model::AccountInput> &accInput : accounts) {
        m_keysmith->accountListModel()->addAccount(accInput.get());
    }
}

bool KeysmithApplet::exportAccountsToFile(const QString &filePath, int format)
{
    if (!m_keysmith) {
        return false;
    }

    model::ExportOutput output;
    output.setFile(filePath);
    output.setFormat(static_cast<model::ExportOutput::ExportFormat>(format));

    return output.exportAccounts(m_keysmith->store().accounts());
}

void KeysmithApplet::removeAccount(int row)
{
    if (row < 0 || row >= m_accountsModel->m_accounts.count()) {
        return;
    }
    Q_EMIT m_accountsModel->m_accounts[row]->remove();
}

void KeysmithApplet::refresh(void)
{
    if (!m_keysmith) {
        return;
    }

    // Remove accounts from the model that no longer exist in storage
    // (e.g. deleted in the desktop app), then reload to pick up new ones.
    accounts::AccountStorage *storage = m_keysmith->store().accounts();
    model::SimpleAccountListModel *sourceModel = m_keysmith->accountListModel();

    QList<QString> diskAccounts = storage->accounts();
    QSet<QString> diskSet(diskAccounts.constBegin(), diskAccounts.constEnd());

    // Trigger removal for accounts in the model that are missing from disk
    for (int i = 0; i < sourceModel->rowCount(); ++i) {
        QVariant data = sourceModel->data(sourceModel->index(i), model::SimpleAccountListModel::AccountRole);
        model::AccountView *account = qvariant_cast<model::AccountView *>(data);
        if (account) {
            QString issuer = account->issuer();
            QString fullName = issuer.isNull() ? account->name() : issuer + QLatin1Char(':') + account->name();
            if (!diskSet.contains(fullName)) {
                Q_EMIT account->remove();
            }
        }
    }

    storage->reload();
    refreshModel();
}

void KeysmithApplet::recomputeAll(void)
{
    m_accountsModel->triggerRecompute();
}

void KeysmithApplet::copyToClipboard(const QString &text)
{
    if (!m_keysmith) {
        return;
    }
    m_keysmith->copyToClipboard(text);
}

bool KeysmithApplet::providePassword(const QString &password)
{
    if (!m_passwordRequest) {
        return false;
    }

    return m_passwordRequest->providePassword(password);
}

bool KeysmithApplet::provideNewPassword(const QString &password, const QString &confirm)
{
    if (!m_passwordRequest) {
        return false;
    }

    bool result = m_passwordRequest->provideBothPasswords(password, confirm);
    if (result) {
        setNeedsSetup(false);
    }
    return result;
}

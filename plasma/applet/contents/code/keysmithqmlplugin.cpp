#include <QQmlExtensionPlugin>
#include <QtQml>

#include "app/cli.h"
#include "app/keysmith.h"
#include "app/vms.h"
#include "model/input.h"
#include "stateconfig.h"
#include "validators/countervalidator.h"
#include "validators/datetimevalidator.h"
#include "validators/issuervalidator.h"
#include "validators/namevalidator.h"
#include "validators/secretvalidator.h"

class KeysmithQmlPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QQmlExtensionInterface/1.0")

public:
    void registerTypes(const char *uri) override
    {
        Q_UNUSED(uri);

        qmlRegisterUncreatableType<app::AddAccountViewModel>(
            "Keysmith.Application",
            1,
            0,
            "AddAccountViewModel",
            QStringLiteral("Should be automatically provided through Keysmith.Application.Navigation signals"));

        qmlRegisterUncreatableType<app::ImportAccountViewModel>(
            "Keysmith.Application",
            1,
            0,
            "ImportAccountViewModel",
            QStringLiteral("Should be automatically provided through Keysmith.Application.Navigation signals"));

        qmlRegisterUncreatableType<app::RenameAccountViewModel>("Keysmith.Application",
                                                                1,
                                                                0,
                                                                "RenameAccountViewModel",
                                                                QStringLiteral("Should be automatically provided through Keysmith.Navigation signals"));

        qmlRegisterUncreatableType<app::ErrorViewModel>("Keysmith.Application",
                                                        1,
                                                        0,
                                                        "ErrorViewModel",
                                                        QStringLiteral("Should be automatically provided through Keysmith.Navigation signals"));

        qmlRegisterUncreatableType<app::SetupPasswordViewModel>("Keysmith.Application",
                                                                1,
                                                                0,
                                                                "SetupPasswordViewModel",
                                                                QStringLiteral("Should be automatically provided through Keysmith.Navigation signals"));

        qmlRegisterUncreatableType<app::UnlockAccountsViewModel>("Keysmith.Application",
                                                                 1,
                                                                 0,
                                                                 "UnlockAccountsViewModel",
                                                                 QStringLiteral("Should be automatically provided through Keysmith.Navigation signals"));

        qmlRegisterUncreatableType<app::AccountsOverviewViewModel>("Keysmith.Application",
                                                                   1,
                                                                   0,
                                                                   "AccountsOverviewViewModel",
                                                                   QStringLiteral("Should be automatically provided through Keysmith.Navigation signals"));

        qmlRegisterUncreatableType<app::ScanQRViewModel>("Keysmith.Application",
                                                         1,
                                                         0,
                                                         "ScanQRViewModel",
                                                         QStringLiteral("Should be automatically provided through Keysmith.Navigation signals"));

        qmlRegisterUncreatableType<app::Navigation>("Keysmith.Application",
                                                    1,
                                                    0,
                                                    "Navigation",
                                                    QStringLiteral("Use the Keysmith singleton to obtain a Navigation"));

        qmlRegisterUncreatableType<model::SimpleAccountListModel>("Keysmith.Models",
                                                                  1,
                                                                  0,
                                                                  "AccountListModel",
                                                                  QStringLiteral("Use the Keysmith singleton to obtain an AccountListModel"));

        qmlRegisterUncreatableType<model::PasswordRequest>("Keysmith.Models",
                                                           1,
                                                           0,
                                                           "PasswordRequestModel",
                                                           QStringLiteral("Use the Keysmith singleton to obtain an PasswordRequestModel"));

        qmlRegisterUncreatableType<model::AccountView>("Keysmith.Models",
                                                       1,
                                                       0,
                                                       "Account",
                                                       QStringLiteral("Use an AccountListModel from the Keysmith singleton to obtain an Account"));

        qmlRegisterType<model::AccountInput>("Keysmith.Models", 1, 0, "ValidatedAccountInput");
        qmlRegisterType<model::ImportInput>("Keysmith.Models", 1, 0, "ValidatedImportInput");
        qmlRegisterType<model::SortedAccountsListModel>("Keysmith.Models", 1, 0, "SortedAccountListModel");
        qmlRegisterType<model::AccountNameValidator>("Keysmith.Validators", 1, 0, "AccountNameValidator");
        qmlRegisterType<validators::EpochValidator>("Keysmith.Validators", 1, 0, "TOTPEpochValidator");
        qmlRegisterType<validators::NameValidator>("Keysmith.Validators", 1, 0, "AccountIssuerValidator");
        qmlRegisterType<validators::Base32Validator>("Keysmith.Validators", 1, 0, "Base32SecretValidator");
        qmlRegisterType<validators::UnsignedLongValidator>("Keysmith.Validators", 1, 0, "HOTPCounterValidator");

        qmlRegisterSingletonType<app::Keysmith>("Keysmith.Application", 1, 0, "Keysmith", [](QQmlEngine *qml, QJSEngine *js) -> QObject * {
            Q_UNUSED(js);
            app::Keysmith *keysmith = new app::Keysmith(new app::Navigation(qml));
            keysmith->setAutoUnlockEnabled(true);
            return keysmith;
        });

        qmlRegisterSingletonType<app::CommandLineOptions>("Keysmith.Application", 1, 0, "CommandLine", [](QQmlEngine *qml, QJSEngine *js) -> QObject * {
            Q_UNUSED(qml);
            Q_UNUSED(js);
            QCommandLineParser parser;
            return new app::CommandLineOptions(parser, true);
        });

        qmlRegisterSingletonType<StateConfig>("Keysmith.Application", 1, 0, "StateConfig", [](QQmlEngine *qml, QJSEngine *js) -> QObject * {
            Q_UNUSED(qml);
            Q_UNUSED(js);
            return StateConfig::self();
        });
    }
};

#include "keysmithqmlplugin.moc"
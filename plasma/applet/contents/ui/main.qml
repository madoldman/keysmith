import QtQuick
import QtQuick.Layouts
import QtQuick.Controls 2.0 as QQC2
import QtQuick.Dialogs as Dialogs

import org.kde.plasma.plasmoid
import org.kde.plasma.extras as PlasmaExtras
import org.kde.kirigami as Kirigami
import org.kde.kirigami.delegates as KirigamiDelegates

import org.kde.plasma.private.keysmith

PlasmoidItem {
    id: root

    fullRepresentation: PlasmaExtras.Representation {
        collapseMarginsHint: true

        Kirigami.ScrollablePage {
            id: page
            anchors.fill: parent
            title: i18n("OTP Accounts")
            supportsRefreshing: false

            ColumnLayout {
                anchors.fill: parent
                spacing: Kirigami.Units.smallSpacing

                // Password setup dialog shown when no password has been set yet
                QQC2.Dialog {
                    id: setupPasswordDialog
                    title: i18n("Set Up Password")
                    width: Math.min(380, page.width - Kirigami.Units.largeSpacing * 4)
                    modal: true
                    visible: Keysmith.needsSetup && Keysmith.passwordRequestResolved
                    closePolicy: QQC2.Popup.NoAutoClose

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: Kirigami.Units.largeSpacing
                        spacing: Kirigami.Units.smallSpacing

                        QQC2.Label {
                            text: i18n("Set a master password to encrypt your accounts:")
                            Layout.fillWidth: true
                            wrapMode: Text.WordWrap
                        }

                        QQC2.Label {
                            text: i18n("Password:")
                            font.bold: true
                        }

                        QQC2.TextField {
                            id: setupPasswordField
                            Layout.fillWidth: true
                            placeholderText: i18n("Enter password")
                            echoMode: QQC2.TextField.Password
                        }

                        QQC2.Label {
                            text: i18n("Confirm Password:")
                            font.bold: true
                        }

                        QQC2.TextField {
                            id: setupPasswordConfirmField
                            Layout.fillWidth: true
                            placeholderText: i18n("Confirm password")
                            echoMode: QQC2.TextField.Password
                        }

                        QQC2.Label {
                            id: setupPasswordError
                            visible: false
                            color: Kirigami.Theme.negativeTextColor
                            Layout.fillWidth: true
                            wrapMode: Text.WordWrap
                        }
                    }

                    footer: QQC2.DialogButtonBox {
                        QQC2.Button {
                            text: i18n("Set Password")
                            highlighted: true
                            enabled: setupPasswordField.text.length > 0 && setupPasswordConfirmField.text.length > 0
                            QQC2.DialogButtonBox.buttonRole: QQC2.DialogButtonBox.AcceptRole
                        }
                    }

                    onAccepted: {
                        if (setupPasswordField.text !== setupPasswordConfirmField.text) {
                            setupPasswordError.text = i18n("Passwords do not match")
                            setupPasswordError.visible = true
                            setupPasswordConfirmField.text = ""
                            open()
                            return
                        }
                        if (!Keysmith.provideNewPassword(setupPasswordField.text, setupPasswordConfirmField.text)) {
                            setupPasswordError.text = i18n("Failed to set password")
                            setupPasswordError.visible = true
                            setupPasswordField.text = ""
                            setupPasswordConfirmField.text = ""
                            open()
                            return
                        }
                        setupPasswordError.visible = false
                        setupPasswordField.text = ""
                        setupPasswordConfirmField.text = ""
                    }
                }

                // Unlock dialog shown when storage is locked and needs password
                QQC2.Dialog {
                    id: unlockDialog
                    title: i18n("Unlock Accounts")
                    width: Math.min(380, page.width - Kirigami.Units.largeSpacing * 4)
                    modal: true
                    visible: Keysmith.locked && !Keysmith.needsSetup && Keysmith.passwordRequestResolved
                    closePolicy: QQC2.Popup.NoAutoClose

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: Kirigami.Units.largeSpacing
                        spacing: Kirigami.Units.smallSpacing

                        QQC2.Label {
                            text: i18n("Enter your master password to unlock your accounts:")
                            Layout.fillWidth: true
                            wrapMode: Text.WordWrap
                        }

                        QQC2.TextField {
                            id: unlockPasswordField
                            Layout.fillWidth: true
                            placeholderText: i18n("Enter password")
                            echoMode: QQC2.TextField.Password
                            onAccepted: unlockDialog.accepted()
                        }

                        QQC2.Label {
                            id: unlockPasswordError
                            visible: false
                            color: Kirigami.Theme.negativeTextColor
                            Layout.fillWidth: true
                            wrapMode: Text.WordWrap
                        }
                    }

                    footer: QQC2.DialogButtonBox {
                        QQC2.Button {
                            text: i18n("Unlock")
                            highlighted: true
                            enabled: unlockPasswordField.text.length > 0
                            QQC2.DialogButtonBox.buttonRole: QQC2.DialogButtonBox.AcceptRole
                        }
                    }

                    onAccepted: {
                        if (!Keysmith.providePassword(unlockPasswordField.text)) {
                            unlockPasswordError.text = i18n("Incorrect password")
                            unlockPasswordError.visible = true
                            unlockPasswordField.text = ""
                            open()
                            return
                        }
                        unlockPasswordError.visible = false
                        unlockPasswordField.text = ""
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.smallSpacing

                    QQC2.Button {
                        text: i18n("Add Account")
                        icon.name: "list-add"
                        onClicked: addAccountDialog.open()
                        Layout.fillWidth: true
                        enabled: !Keysmith.locked && !Keysmith.needsSetup
                    }

                    QQC2.Button {
                        text: i18n("Import")
                        icon.name: "document-import"
                        onClicked: importDialog.open()
                        Layout.fillWidth: true
                        enabled: !Keysmith.locked && !Keysmith.needsSetup
                    }
                }

                ListView {
                    id: accountList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    model: Keysmith.accountsModel

                    delegate: QQC2.ItemDelegate {
                        id: listItem
                        required property string name
                        required property string token
                        required property bool isTotp
                        required property string issuer
                        required property int timeStep
                        required property int index

                        property real healthIndicator: 0
                        property real totpInterval: isTotp ? 1000 * timeStep : 0

                        width: ListView.view.width - ListView.view.leftMargin - ListView.view.rightMargin
                        implicitHeight: nameLbl.implicitHeight + tokenLbl.implicitHeight + topPadding + bottomPadding + (isTotp ? 3 : 0)

                        onClicked: {
                            if (token.length > 0) {
                                Keysmith.copyToClipboard(token)
                            }
                        }

                        contentItem: Column {
                            id: col
                            spacing: 2

                            QQC2.Label {
                                id: nameLbl
                                text: issuer.length > 0 ? issuer + " (" + name + ")" : name
                                elide: Text.ElideRight
                                width: col.width
                            }

                            QQC2.Label {
                                id: tokenLbl
                                text: token.length > 0 ? token : "..."
                                font.bold: true
                                font.pointSize: Math.round(Kirigami.Theme.defaultFont.pointSize * 1.4)
                                color: Kirigami.Theme.textColor
                                opacity: 0.95
                                width: col.width
                            }
                        }

                        // Countdown progress bar for TOTP accounts
                        Rectangle {
                            visible: listItem.isTotp
                            y: listItem.height - height
                            x: 0
                            width: {
                                var lvWidth = listItem.ListView.view ? listItem.ListView.view.width : listItem.width
                                var barWidth = lvWidth - listItem.leftPadding - listItem.rightPadding
                                return listItem.isTotp && listItem.totpInterval > 0
                                    ? barWidth * listItem.healthIndicator / listItem.totpInterval
                                    : 0
                            }
                            height: 3
                            radius: height / 2
                            color: Kirigami.Theme.positiveTextColor
                            opacity: countdownAnim.running ? 0.5 : 0
                        }

                        NumberAnimation {
                            id: countdownAnim
                            target: listItem
                            property: "healthIndicator"
                            from: listItem.totpInterval
                            to: 0
                            duration: listItem.totpInterval
                            running: listItem.isTotp && listItem.visible
                        }

                        Timer {
                            id: totpTimer
                            running: listItem.isTotp && listItem.visible
                            interval: listItem.isTotp ? Keysmith.accountsModel.millisecondsLeftForToken(listItem.index) : 0
                            onTriggered: {
                                if (listItem.isTotp) {
                                    totpTimer.stop()
                                    countdownAnim.stop()
                                    Keysmith.accountsModel.recompute(listItem.index)
                                    var phase = Keysmith.accountsModel.millisecondsLeftForToken(listItem.index)
                                    totpTimer.interval = phase
                                    listItem.healthIndicator = phase
                                    countdownAnim.duration = phase
                                    countdownAnim.from = phase
                                    totpTimer.restart()
                                    countdownAnim.restart()
                                }
                            }
                        }
                    }

                    Kirigami.PlaceholderMessage {
                        anchors.centerIn: parent
                        width: parent.width - Kirigami.Units.largeSpacing * 4
                        visible: accountList.count === 0
                        text: Keysmith.locked ? i18n("Accounts are locked") : i18n("No accounts added")
                        icon.name: Keysmith.locked ? "lock" : "unlock"
                    }
                }
            }

            QQC2.Dialog {
                id: importDialog
                title: i18n("Import Accounts")
                width: Math.min(380, page.width - Kirigami.Units.largeSpacing * 4)
                height: 300
                modal: true
                visible: false
                closePolicy: QQC2.Popup.CloseOnEscape | QQC2.Popup.CloseOnPressOutside

                property int selectedFormat: 0
                property bool passwordRequired: selectedFormat === 0

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: Kirigami.Units.largeSpacing
                    spacing: Kirigami.Units.smallSpacing

                    QQC2.Label {
                        text: i18n("Import Format:")
                        font.bold: true
                        Layout.fillWidth: true
                    }

                    QQC2.ComboBox {
                        id: formatCombo
                        Layout.fillWidth: true
                        currentIndex: 0
                        model: [
                            i18n("andOTP Encrypted JSON"),
                            i18n("andOTP Plain JSON"),
                            i18n("Aegis Plain JSON"),
                            i18n("FreeOTP URIs")
                        ]
                        onCurrentIndexChanged: {
                            importDialog.selectedFormat = currentIndex
                        }
                    }

                    QQC2.Label {
                        text: i18n("Select file:")
                        font.bold: true
                        Layout.fillWidth: true
                    }

                    QQC2.TextField {
                        id: filePathField
                        Layout.fillWidth: true
                        readOnly: true
                        placeholderText: i18n("No file selected")
                    }

                    QQC2.Button {
                        text: i18n("Browse...")
                        Layout.fillWidth: true
                        onClicked: fileDialog.open()
                    }

                    Dialogs.FileDialog {
                        id: fileDialog
                        title: i18n("Select file to import")
                        onAccepted: {
                            var path = selectedFile.toString()
                            if (path.startsWith("file://")) {
                                path = path.substring(7)
                            }
                            filePathField.text = path
                        }
                    }

                    QQC2.Label {
                        text: i18n("Password:")
                        font.bold: true
                        Layout.fillWidth: true
                        visible: importDialog.passwordRequired
                    }

                    QQC2.TextField {
                        id: passwordField
                        Layout.fillWidth: true
                        placeholderText: i18n("Enter decryption password")
                        visible: importDialog.passwordRequired
                        echoMode: QQC2.TextField.Password
                    }
                }

                footer: QQC2.DialogButtonBox {
                    QQC2.Button {
                        text: i18n("Cancel")
                        QQC2.DialogButtonBox.buttonRole: QQC2.DialogButtonBox.RejectRole
                    }
                    QQC2.Button {
                        text: i18n("Import")
                        highlighted: true
                        enabled: filePathField.text.length > 0 &&
                                 (!importDialog.passwordRequired || passwordField.text.length > 0)
                        QQC2.DialogButtonBox.buttonRole: QQC2.DialogButtonBox.AcceptRole
                    }
                }

                onAccepted: {
                    var formatMap = [2, 1, 3, 0]
                    Keysmith.importAccountsFromFile(filePathField.text, formatMap[formatCombo.currentIndex], passwordField.text)
                    filePathField.text = ""
                    passwordField.text = ""
                }

                onRejected: {
                    filePathField.text = ""
                    passwordField.text = ""
                }
            }

            QQC2.Dialog {
                id: addAccountDialog
                title: i18n("Add Account")
                width: Math.min(380, page.width - Kirigami.Units.largeSpacing * 4)
                height: Math.min(650, page.height - Kirigami.Units.largeSpacing * 4)
                modal: true
                visible: false
                closePolicy: QQC2.Popup.CloseOnEscape | QQC2.Popup.CloseOnPressOutside

                Flickable {
                    anchors.fill: parent
                    clip: true
                    contentHeight: addAccountColumn.height

                    ColumnLayout {
                        id: addAccountColumn
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.margins: Kirigami.Units.largeSpacing
                        spacing: Kirigami.Units.smallSpacing

                        QQC2.Label {
                            text: i18n("Account Name:")
                            font.bold: true
                            Layout.fillWidth: true
                        }

                        QQC2.TextField {
                            id: accountNameInput
                            Layout.fillWidth: true
                            placeholderText: i18n("Enter account name")
                        }

                        QQC2.Label {
                            text: i18n("Issuer:")
                            font.bold: true
                            Layout.fillWidth: true
                        }

                        QQC2.TextField {
                            id: issuerInput
                            Layout.fillWidth: true
                            placeholderText: i18n("Enter issuer (optional)")
                        }

                        QQC2.Label {
                            text: i18n("Secret Key (Base32):")
                            font.bold: true
                            Layout.fillWidth: true
                        }

                        QQC2.TextField {
                            id: secretKeyInput
                            Layout.fillWidth: true
                            placeholderText: i18n("Enter base32 secret key")
                            echoMode: QQC2.TextField.Password
                        }

                        QQC2.Label {
                            text: i18n("Token Type:")
                            font.bold: true
                            Layout.fillWidth: true
                        }

                        RowLayout {
                            Layout.fillWidth: true

                            QQC2.RadioButton {
                                id: totpRadio
                                text: i18n("TOTP (Time-based)")
                                checked: true
                            }

                            QQC2.RadioButton {
                                id: hotpRadio
                                text: i18n("HOTP (Counter-based)")
                            }
                        }

                        ColumnLayout {
                            id: totpDetails
                            Layout.fillWidth: true
                            visible: totpRadio.checked
                            spacing: Kirigami.Units.smallSpacing

                            QQC2.Label {
                                text: i18n("TOTP Settings:")
                                font.bold: true
                                font.underline: true
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                QQC2.Label { text: i18n("Time Step (seconds):") }
                                QQC2.TextField {
                                    id: timeStepInput
                                    text: "30"
                                    validator: IntValidator { bottom: 1 }
                                    Layout.preferredWidth: 80
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                QQC2.Label { text: i18n("Token Length:") }
                                QQC2.SpinBox {
                                    id: totpTokenLengthInput
                                    from: 6
                                    to: 10
                                    value: 6
                                    Layout.preferredWidth: 100
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                QQC2.Label { text: i18n("Algorithm:") }
                                QQC2.ComboBox {
                                    id: algorithmInput
                                    model: ["SHA-1", "SHA-256", "SHA-512"]
                                    Layout.preferredWidth: 150
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                QQC2.Label { text: i18n("Epoch (optional):") }
                                QQC2.TextField {
                                    id: epochInput
                                    placeholderText: i18n("Leave empty for Unix epoch")
                                    Layout.preferredWidth: 200
                                }
                            }
                        }

                        ColumnLayout {
                            id: hotpDetails
                            Layout.fillWidth: true
                            visible: hotpRadio.checked
                            spacing: Kirigami.Units.smallSpacing

                            QQC2.Label {
                                text: i18n("HOTP Settings:")
                                font.bold: true
                                font.underline: true
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                QQC2.Label { text: i18n("Counter:") }
                                QQC2.TextField {
                                    id: counterInput
                                    validator: IntValidator { bottom: 0 }
                                    placeholderText: i18n("Enter counter value")
                                    Layout.preferredWidth: 150
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                QQC2.Label { text: i18n("Token Length:") }
                                QQC2.SpinBox {
                                    id: hotpTokenLengthInput
                                    from: 6
                                    to: 10
                                    value: 6
                                    Layout.preferredWidth: 100
                                }
                            }

                            QQC2.CheckBox {
                                id: checksumInput
                                text: i18n("Add checksum digit")
                            }

                            QQC2.CheckBox {
                                id: useFixedTruncationInput
                                text: i18n("Use custom truncation offset")
                                onCheckedChanged: {
                                    truncationOffsetInput.enabled = checked
                                }
                            }

                            QQC2.SpinBox {
                                id: truncationOffsetInput
                                from: 0
                                to: 16
                                value: 0
                                enabled: false
                                Layout.preferredWidth: 100
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.topMargin: Kirigami.Units.largeSpacing
                            spacing: Kirigami.Units.smallSpacing

                            QQC2.Button {
                                text: i18n("Cancel")
                                onClicked: addAccountDialog.close()
                                Layout.fillWidth: true
                            }

                            QQC2.Button {
                                text: i18n("Add")
                                highlighted: true
                                enabled: accountNameInput.text.length > 0 && secretKeyInput.text.length > 0
                                onClicked: {
                                    var algorithmMap = {0: 0, 1: 1, 2: 2}
                                    Keysmith.addAccountEx(
                                        accountNameInput.text,
                                        issuerInput.text,
                                        secretKeyInput.text,
                                        totpRadio.checked ? 0 : 1,
                                        totpRadio.checked ? parseInt(timeStepInput.text) : 0,
                                        totpRadio.checked ? algorithmMap[algorithmInput.currentIndex] : 0,
                                        totpRadio.checked ? totpTokenLengthInput.value : hotpTokenLengthInput.value,
                                        epochInput.text,
                                        hotpRadio.checked ? parseInt(counterInput.text) : 0,
                                        hotpRadio.checked ? checksumInput.checked : false,
                                        hotpRadio.checked ? useFixedTruncationInput.checked : false,
                                        hotpRadio.checked ? truncationOffsetInput.value : 0
                                    )
                                    addAccountDialog.close()
                                    accountNameInput.text = ""
                                    issuerInput.text = ""
                                    secretKeyInput.text = ""
                                    totpRadio.checked = true
                                    timeStepInput.text = "30"
                                    totpTokenLengthInput.value = 6
                                    algorithmInput.currentIndex = 0
                                    epochInput.text = ""
                                    counterInput.text = ""
                                    hotpTokenLengthInput.value = 6
                                    checksumInput.checked = false
                                    useFixedTruncationInput.checked = false
                                    truncationOffsetInput.value = 0
                                }
                                Layout.fillWidth: true
                            }
                        }
                    }
                }
            }
        }
    }
}
/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 Keysmith Contributors
 */

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import QtQuick.Dialogs as Dialogs
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard 1 as FormCard

import Keysmith.Application as Application
import Keysmith.Models as Models

FormCard.FormCardPage {
    id: root

    title: i18nc("@title:window", "Export Accounts")

    required property Application.ExportAccountViewModel vm

    readonly property bool formatComboboxAcceptable: formatCombobox.currentIndex !== -1
    readonly property bool accountsFileAcceptable: accountsFile.selectedFile.toString() !== ""
    readonly property bool acceptable: formatComboboxAcceptable && accountsFileAcceptable

    topPadding: Kirigami.Units.gridUnit
    bottomPadding: Kirigami.Units.gridUnit

    onBackRequested: event => {
        event.accepted = true;
        vm.cancelled();
    }

    Component.onCompleted: formatCombobox.forceActiveFocus()

    FormCard.FormCard {
        id: requiredDetails

        FormCard.FormComboBoxDelegate {
            id: formatCombobox
            text: i18n("Export format")

            currentIndex: -1
            model: ListModel {
                Component.onCompleted: {
                    // ListModel doesn't support i18n strings
                    append({name: i18nc("@item:inlistbox", "andOTP Plain JSON"), value: Models.ExportOutput.AndOTPPlainJSON});
                    append({name: i18nc("@item:inlistbox", "Aegis Plain JSON"), value: Models.ExportOutput.AegisPlainJSON});

                    formatCombobox.currentIndex = 0;
                }
            }

            textRole: "name"
            valueRole: "value"

            onCurrentValueChanged: vm.output.format = currentValue;
        }

        FormCard.FormDelegateSeparator {}

        FormCard.FormButtonDelegate {
            id: openFileDialog
            text: i18nc("@label:chooser", "Export file:")
            enabled: formatComboboxAcceptable
            onClicked: accountsFile.open();
            description: vm.output.file.toString().length > 0 ? vm.output.file.toString().substring(7) : i18nc("@info:placeholder", "No file selected")

            Dialogs.FileDialog {
                id: accountsFile
                title: i18nc("@title:window", "Select export file")
                fileMode: Dialogs.FileDialog.SaveFile
                onAccepted: {
                    vm.output.file = accountsFile.selectedFile;
                }
            }
        }
    }

    FormCard.FormCard {
        Layout.topMargin: Kirigami.Units.gridUnit

        FormCard.FormButtonDelegate {
            text: i18nc("@action:button", "Export")
            icon.name: "document-export"
            enabled: acceptable
            onClicked: {
                vm.accepted();
            }
        }
    }
}

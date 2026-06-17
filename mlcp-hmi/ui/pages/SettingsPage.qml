import QtQuick 2.15

Item {
    id: root
    width: 1280
    height: 720

    signal backRequested()

    readonly property int pageMargin: 24
    readonly property int panelPadding: 22
    readonly property int cardRadius: 8
    property bool fanAutoControl: true
    property bool configResultVisible: false
    property bool configResultSuccess: false
    property string configResultMessage: ""

    function fanSettingsAvailable() {
        return typeof fanSettingsViewModel !== "undefined" && fanSettingsViewModel
    }

    function loadFanSettings() {
        if (!fanSettingsAvailable()) {
            return
        }

        fanSettingsViewModel.refresh()
        root.fanAutoControl = fanSettingsViewModel.temperatureAutoControl
        fanDutyInput.inputText = String(fanSettingsViewModel.manualFanDuty)
        targetTempInput.inputText = Number(fanSettingsViewModel.targetCelsius).toFixed(1)
        maxDutyInput.inputText = String(fanSettingsViewModel.maxFanDuty)
    }

    function parsedNumber(text, fallbackValue) {
        var parsedValue = Number(text)
        return isNaN(parsedValue) ? fallbackValue : parsedValue
    }

    function parsedInt(text, fallbackValue) {
        var parsedValue = parseInt(text)
        return isNaN(parsedValue) ? fallbackValue : parsedValue
    }

    function parsedValueFromText(text) {
        var matchedValue = String(text).match(/-?\d+(\.\d+)?/)
        return matchedValue ? Number(matchedValue[0]) : NaN
    }

    function validNumber(value) {
        return !isNaN(value) && isFinite(value)
    }

    function showConfigResult(success, message) {
        root.configResultSuccess = success
        root.configResultMessage = message
        root.configResultVisible = true
        configResultCloseTimer.restart()
    }

    function applyLiquidPidSettings() {
        var kpValue = root.parsedValueFromText(liquidKpInput.inputText)
        var kiValue = root.parsedValueFromText(liquidKiInput.inputText)
        var kdValue = root.parsedValueFromText(liquidKdInput.inputText)
        var valid = root.validNumber(kpValue) &&
            root.validNumber(kiValue) &&
            root.validNumber(kdValue)

        root.showConfigResult(valid, valid ? qsTr("Configuration saved")
                                           : qsTr("Invalid PID parameter"))
    }

    function applyAlarmThresholdSettings() {
        var tempHighValue = root.parsedValueFromText(tempHighInput.inputText)
        var liquidLowValue = root.parsedValueFromText(liquidLowInput.inputText)
        var liquidHighValue = root.parsedValueFromText(liquidHighInput.inputText)
        var valid = root.validNumber(tempHighValue) &&
            root.validNumber(liquidLowValue) &&
            root.validNumber(liquidHighValue) &&
            liquidLowValue < liquidHighValue

        root.showConfigResult(valid, valid ? qsTr("Configuration saved")
                                           : qsTr("Invalid alarm threshold"))
    }

    function applyFanSettings() {
        if (!root.fanSettingsAvailable()) {
            root.showConfigResult(false, qsTr("Temp service unavailable"))
            return
        }

        var targetTempValue = root.parsedValueFromText(targetTempInput.inputText)
        var fanDutyValue = root.parsedValueFromText(fanDutyInput.inputText)
        var maxDutyValue = root.parsedValueFromText(maxDutyInput.inputText)
        var valid = root.validNumber(targetTempValue) &&
            root.validNumber(fanDutyValue) &&
            root.validNumber(maxDutyValue)
        if (!valid) {
            root.showConfigResult(false, qsTr("Invalid fan setting"))
            return
        }

        var saved = fanSettingsViewModel.applySettings(
            root.fanAutoControl,
            targetTempValue,
            Math.round(fanDutyValue),
            Math.round(maxDutyValue))

        root.showConfigResult(saved, saved ? qsTr("Configuration saved")
                                           : root.configFailureMessage())
        if (saved) {
            root.loadFanSettings()
        }
    }

    function configFailureMessage() {
        return fanSettingsViewModel.errorText.length > 0
            ? fanSettingsViewModel.errorText
            : qsTr("Configuration failed")
    }

    Component.onCompleted: loadFanSettings()
    onVisibleChanged: {
        if (visible) {
            loadFanSettings()
        }
    }

    Rectangle {
        anchors.fill: parent
        color: "#eef3f6"
    }

    Rectangle {
        id: headerPanel
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.leftMargin: root.pageMargin
        anchors.rightMargin: root.pageMargin
        anchors.topMargin: root.pageMargin
        height: 84
        radius: root.cardRadius
        color: "#163542"

        Rectangle {
            id: backButton
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: root.panelPadding
            width: 92
            height: 44
            radius: 5
            color: "#0f5b78"

            Text {
                anchors.centerIn: parent
                text: qsTr("Back")
                color: "#ffffff"
                font.pixelSize: 18
                font.bold: true
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: root.backRequested()
            }
        }

        Text {
            anchors.left: backButton.right
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.leftMargin: 24
            anchors.rightMargin: root.panelPadding
            text: qsTr("Settings")
            color: "#ffffff"
            font.pixelSize: 30
            font.bold: true
            verticalAlignment: Text.AlignVCenter
        }
    }

    Rectangle {
        id: pidPanel
        anchors.left: parent.left
        anchors.top: headerPanel.bottom
        anchors.leftMargin: root.pageMargin
        anchors.topMargin: 16
        width: 596
        height: 418
        radius: root.cardRadius
        color: "#ffffff"
        border.color: "#cad7dd"

        Text {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.leftMargin: root.panelPadding
            anchors.topMargin: 18
            text: qsTr("Liquid PID")
            color: "#163542"
            font.pixelSize: 22
            font.bold: true
        }

        SettingInput {
            id: liquidKpInput
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.leftMargin: root.panelPadding
            anchors.rightMargin: root.panelPadding
            anchors.topMargin: 76
            labelText: "Kp"
            valueText: "1.20"
        }

        SettingInput {
            id: liquidKiInput
            anchors.left: liquidKpInput.left
            anchors.right: liquidKpInput.right
            anchors.top: liquidKpInput.bottom
            anchors.topMargin: 18
            labelText: "Ki"
            valueText: "0.08"
        }

        SettingInput {
            id: liquidKdInput
            anchors.left: liquidKpInput.left
            anchors.right: liquidKpInput.right
            anchors.top: liquidKiInput.bottom
            anchors.topMargin: 18
            labelText: "Kd"
            valueText: "0.30"
        }

        Rectangle {
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.rightMargin: root.panelPadding
            anchors.bottomMargin: root.panelPadding
            width: 128
            height: 44
            radius: 5
            color: "#0f5b78"

            Text {
                anchors.centerIn: parent
                text: qsTr("Set")
                color: "#ffffff"
                font.pixelSize: 18
                font.bold: true
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: root.applyLiquidPidSettings()
            }
        }
    }

    Rectangle {
        id: alarmPanel
        anchors.left: pidPanel.right
        anchors.right: parent.right
        anchors.top: pidPanel.top
        anchors.leftMargin: 20
        anchors.rightMargin: root.pageMargin
        height: pidPanel.height
        radius: root.cardRadius
        color: "#ffffff"
        border.color: "#cad7dd"

        Text {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.leftMargin: root.panelPadding
            anchors.topMargin: 18
            text: qsTr("Alarm Threshold")
            color: "#163542"
            font.pixelSize: 22
            font.bold: true
        }

        SettingInput {
            id: tempHighInput
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.leftMargin: root.panelPadding
            anchors.rightMargin: root.panelPadding
            anchors.topMargin: 76
            labelText: "Temperature High"
            valueText: "39.0 C"
        }

        SettingInput {
            id: liquidLowInput
            anchors.left: tempHighInput.left
            anchors.right: tempHighInput.right
            anchors.top: tempHighInput.bottom
            anchors.topMargin: 18
            labelText: "Liquid Low"
            valueText: "20 %"
        }

        SettingInput {
            id: liquidHighInput
            anchors.left: tempHighInput.left
            anchors.right: tempHighInput.right
            anchors.top: liquidLowInput.bottom
            anchors.topMargin: 18
            labelText: "Liquid High"
            valueText: "95 %"
        }

        Rectangle {
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.rightMargin: root.panelPadding
            anchors.bottomMargin: root.panelPadding
            width: 128
            height: 44
            radius: 5
            color: "#c47a20"

            Text {
                anchors.centerIn: parent
                text: qsTr("Set")
                color: "#ffffff"
                font.pixelSize: 18
                font.bold: true
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: root.applyAlarmThresholdSettings()
            }
        }
    }

    Rectangle {
        id: fanPanel
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: pidPanel.bottom
        anchors.bottom: parent.bottom
        anchors.leftMargin: root.pageMargin
        anchors.rightMargin: root.pageMargin
        anchors.topMargin: 16
        anchors.bottomMargin: root.pageMargin
        radius: root.cardRadius
        color: "#ffffff"
        border.color: "#cad7dd"

        Text {
            id: fanTitle
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.leftMargin: root.panelPadding
            anchors.topMargin: 14
            text: qsTr("Fan Settings")
            color: "#163542"
            font.pixelSize: 22
            font.bold: true
        }

        Text {
            anchors.left: fanTitle.right
            anchors.right: saveFanButton.left
            anchors.verticalCenter: fanTitle.verticalCenter
            anchors.leftMargin: 22
            anchors.rightMargin: 16
            text: fanSettingsAvailable()
                  ? qsTr("Current %1  Duty %2/%3  %4").arg(
                        fanSettingsViewModel.latestTemperatureText).arg(
                        fanSettingsViewModel.latestFanDuty).arg(
                        fanSettingsViewModel.maxFanDuty).arg(
                        fanSettingsViewModel.statusText)
                  : qsTr("Temp service unavailable")
            color: fanSettingsAvailable() && fanSettingsViewModel.available ? "#4b5b63" : "#a33d2d"
            font.pixelSize: 16
            elide: Text.ElideRight
        }

        Rectangle {
            id: autoSwitch
            anchors.left: parent.left
            anchors.top: fanTitle.bottom
            anchors.leftMargin: root.panelPadding
            anchors.topMargin: 18
            width: 210
            height: 48
            radius: 5
            color: root.fanAutoControl ? "#0f5b78" : "#f7fafb"
            border.color: root.fanAutoControl ? "#0f5b78" : "#d7e1e5"

            Text {
                anchors.centerIn: parent
                text: root.fanAutoControl ? qsTr("Auto by Temp") : qsTr("Manual Duty")
                color: root.fanAutoControl ? "#ffffff" : "#163542"
                font.pixelSize: 18
                font.bold: true
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: root.fanAutoControl = !root.fanAutoControl
            }
        }

        SettingInput {
            id: fanDutyInput
            anchors.left: autoSwitch.right
            anchors.top: autoSwitch.top
            anchors.leftMargin: 20
            width: 280
            labelText: "Fan Duty"
            valueText: "0"
            labelWidth: 92
            editorLeftMargin: 116
        }

        SettingInput {
            id: targetTempInput
            anchors.left: fanDutyInput.right
            anchors.top: autoSwitch.top
            anchors.leftMargin: 20
            width: 300
            labelText: "Target Temp"
            valueText: "37.0"
            labelWidth: 116
            editorLeftMargin: 140
        }

        SettingInput {
            id: maxDutyInput
            anchors.left: targetTempInput.right
            anchors.right: saveFanButton.left
            anchors.top: autoSwitch.top
            anchors.leftMargin: 20
            anchors.rightMargin: 18
            labelText: "Max Duty"
            valueText: "255"
            labelWidth: 86
            editorLeftMargin: 110
        }

        Rectangle {
            id: saveFanButton
            anchors.right: parent.right
            anchors.top: fanTitle.bottom
            anchors.rightMargin: root.panelPadding
            anchors.topMargin: 18
            width: 128
            height: 48
            radius: 5
            color: "#0f5b78"

            Text {
                anchors.centerIn: parent
                text: qsTr("Set")
                color: "#ffffff"
                font.pixelSize: 18
                font.bold: true
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: root.applyFanSettings()
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        visible: root.configResultVisible
        z: 20
        color: "#66000000"

        MouseArea {
            anchors.fill: parent
            onClicked: root.configResultVisible = false
        }

        Rectangle {
            anchors.centerIn: parent
            width: 420
            height: 180
            radius: root.cardRadius
            color: "#ffffff"
            border.color: root.configResultSuccess ? "#17835b" : "#b94c3a"

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                height: 58
                radius: root.cardRadius
                color: root.configResultSuccess ? "#17835b" : "#b94c3a"

                Text {
                    anchors.centerIn: parent
                    text: root.configResultSuccess ? qsTr("Configuration Successful")
                                                   : qsTr("Configuration Failed")
                    color: "#ffffff"
                    font.pixelSize: 22
                    font.bold: true
                }
            }

            Text {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.bottom: closeConfigResultButton.top
                anchors.leftMargin: 28
                anchors.rightMargin: 28
                anchors.topMargin: 72
                anchors.bottomMargin: 12
                text: root.configResultMessage
                color: "#163542"
                font.pixelSize: 18
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                wrapMode: Text.WordWrap
            }

            Rectangle {
                id: closeConfigResultButton
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 18
                width: 104
                height: 40
                radius: 5
                color: "#0f5b78"

                Text {
                    anchors.centerIn: parent
                    text: qsTr("OK")
                    color: "#ffffff"
                    font.pixelSize: 17
                    font.bold: true
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.configResultVisible = false
                }
            }
        }
    }

    Timer {
        id: configResultCloseTimer
        interval: 2200
        repeat: false
        onTriggered: root.configResultVisible = false
    }

    component SettingInput: Item {
        property string labelText: ""
        property string valueText: ""
        property int labelWidth: 190
        property int editorLeftMargin: 214
        property alias inputText: valueEditor.text

        height: 58

        Text {
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            width: labelWidth
            text: labelText
            color: "#4b5b63"
            font.pixelSize: 18
            elide: Text.ElideRight
        }

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: editorLeftMargin
            height: 48
            radius: 5
            color: "#f7fafb"
            border.color: "#d7e1e5"

            TextInput {
                id: valueEditor
                anchors.fill: parent
                anchors.leftMargin: 14
                anchors.rightMargin: 14
                text: valueText
                color: "#163542"
                font.pixelSize: 20
                verticalAlignment: TextInput.AlignVCenter
                selectByMouse: true
                inputMethodHints: Qt.ImhFormattedNumbersOnly
            }
        }
    }
}

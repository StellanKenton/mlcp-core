import QtQuick 2.15

Item {
    id: root
    width: 1280
    height: 720

    signal faultHistoryPageRequested(var historyModel)
    signal informationPageRequested()
    signal maintenancePageRequested()
    signal settingsPageRequested()

    readonly property int pageMargin: 24
    readonly property int panelGap: 20
    readonly property int panelPadding: 22
    readonly property int headerPadding: 24
    readonly property int leftColumnWidth: 400
    readonly property int headerHeight: 84
    readonly property int actionButtonHeight: 64
    readonly property int cardRadius: 8
    readonly property int titleFontSize: 30
    readonly property int sectionTitleFontSize: 20
    readonly property int bodyFontSize: 17
    readonly property int valueFontSize: 28
    readonly property int secondaryValueFontSize: 21
    readonly property int faultHistoryPreviewCount: 6
    readonly property int faultHistoryRowHeight: 48
    readonly property int faultHistoryRowSpacing: 8
    readonly property int faultHistoryPanelHeight: (
        110
        + faultHistoryPreviewCount * faultHistoryRowHeight
        + (faultHistoryPreviewCount - 1) * faultHistoryRowSpacing
    )
    readonly property int fanPercent: 0
    readonly property int pushPercent: 0
    property bool alarmHistoryOpen: false
    readonly property bool hasActiveFault: false
    readonly property string currentFaultCode: hasActiveFault ? qsTr("ALM-203") : qsTr("ALM-000")
    readonly property string currentFaultTitle: hasActiveFault ? qsTr("High pressure") : qsTr("Normal")
    readonly property string currentFaultDetail: hasActiveFault
        ? qsTr("Outlet pressure exceeds threshold")
        : qsTr("No active fault")
    readonly property string lifecycleStateText: typeof lifecycleViewModel !== "undefined"
            && lifecycleViewModel
        ? lifecycleViewModel.stateText
        : qsTr("STANDBY")

    ListModel {
        id: faultHistoryModel

        ListElement {
            timeText: "10:42:18"
            codeText: "ALM-203"
            titleText: "High pressure"
            detailText: "Recovered after pressure relief"
            levelColor: "#b6422a"
        }

        ListElement {
            timeText: "09:58:04"
            codeText: "DIA-112"
            titleText: "Sensor timeout"
            detailText: "Pressure sensor sample delayed"
            levelColor: "#c47a20"
        }

        ListElement {
            timeText: "08:21:37"
            codeText: "ALM-071"
            titleText: "Low fluid level"
            detailText: "Reservoir refilled and confirmed"
            levelColor: "#0f5b78"
        }

        ListElement {
            timeText: "07:36:12"
            codeText: "ALM-152"
            titleText: "Outlet blocked"
            detailText: "Tubing obstruction cleared by operator"
            levelColor: "#b6422a"
        }

        ListElement {
            timeText: "06:14:49"
            codeText: "DIA-084"
            titleText: "Fan tachometer drift"
            detailText: "Fan speed returned to expected range"
            levelColor: "#c47a20"
        }

        ListElement {
            timeText: "05:48:03"
            codeText: "ALM-045"
            titleText: "Leak detected"
            detailText: "Pressure decay exceeded monitoring threshold"
            levelColor: "#b6422a"
        }

        ListElement {
            timeText: "04:22:31"
            codeText: "DIA-130"
            titleText: "Temperature warning"
            detailText: "Thermal control recovered after fan adjustment"
            levelColor: "#c47a20"
        }

        ListElement {
            timeText: "03:09:55"
            codeText: "ALM-071"
            titleText: "Low fluid level"
            detailText: "Reservoir level restored before therapy start"
            levelColor: "#0f5b78"
        }
    }

    Rectangle {
        id: background
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
        height: root.headerHeight
        radius: root.cardRadius
        color: "#163542"

        Text {
            id: titleText
            anchors.left: parent.left
            anchors.right: stateBadge.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.leftMargin: root.headerPadding
            anchors.rightMargin: root.headerPadding
            text: qsTr("MLCP")
            color: "#ffffff"
            font.pixelSize: root.titleFontSize
            font.bold: true
            verticalAlignment: Text.AlignVCenter
        }

        Rectangle {
            id: stateBadge
            width: 132
            height: 44
            anchors.right: parent.right
            anchors.rightMargin: root.headerPadding
            anchors.verticalCenter: parent.verticalCenter
            radius: 22
            color: "#2e7d32"

            Text {
                anchors.centerIn: parent
                text: root.lifecycleStateText
                color: "#ffffff"
                font.pixelSize: 20
                font.bold: true
            }
        }
    }

    Rectangle {
        id: alarmIsland
        anchors.top: headerPanel.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: 15
        width: 456
        height: 54
        radius: 27
        color: root.hasActiveFault ? "#6f241e" : "#102832"
        border.color: root.hasActiveFault ? "#f0a39a" : "#3d6170"
        z: 10

        Behavior on color {
            ColorAnimation {
                duration: 160
            }
        }

        Rectangle {
            id: faultIndicator
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: 18
            width: 12
            height: 12
            radius: 6
            color: root.hasActiveFault ? "#ff6b5e" : "#42b883"
        }

        Text {
            id: currentFaultText
            anchors.left: faultIndicator.right
            anchors.right: historyChevron.left
            anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: 12
            anchors.rightMargin: 12
            text: qsTr("%1 %2 · %3")
                .arg(root.currentFaultCode)
                .arg(root.currentFaultTitle)
                .arg(root.currentFaultDetail)
            color: "#ffffff"
            elide: Text.ElideRight
            font.pixelSize: 18
            font.bold: true
            verticalAlignment: Text.AlignVCenter
        }

        Text {
            id: historyChevron
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.rightMargin: 18
            text: root.alarmHistoryOpen ? qsTr("⌃") : qsTr("⌄")
            color: "#d9edf4"
            font.pixelSize: 22
            font.bold: true
        }

        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: root.alarmHistoryOpen = !root.alarmHistoryOpen
        }
    }

    Rectangle {
        id: alarmHistoryPanel
        anchors.top: alarmIsland.bottom
        anchors.horizontalCenter: alarmIsland.horizontalCenter
        anchors.topMargin: 10
        width: 520
        height: root.alarmHistoryOpen ? root.faultHistoryPanelHeight : 0
        radius: root.cardRadius
        color: "#ffffff"
        border.color: "#cad7dd"
        opacity: root.alarmHistoryOpen ? 1.0 : 0.0
        visible: root.alarmHistoryOpen || opacity > 0.0
        clip: true
        z: 9

        Behavior on height {
            NumberAnimation {
                duration: 180
                easing.type: Easing.OutCubic
            }
        }

        Behavior on opacity {
            NumberAnimation {
                duration: 120
            }
        }

        Text {
            id: alarmHistoryTitle
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.leftMargin: 20
            anchors.topMargin: 16
            text: qsTr("Fault History")
            color: "#163542"
            font.pixelSize: 19
            font.bold: true
        }

        Text {
            anchors.right: parent.right
            anchors.top: alarmHistoryTitle.top
            anchors.rightMargin: 20
            text: qsTr("%1 records").arg(faultHistoryModel.count)
            color: "#6f8088"
            font.pixelSize: 15
        }

        ListView {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: alarmHistoryTitle.bottom
            anchors.bottom: moreFaultHistoryButton.top
            anchors.leftMargin: 16
            anchors.rightMargin: 16
            anchors.topMargin: 12
            anchors.bottomMargin: 10
            spacing: root.faultHistoryRowSpacing
            interactive: false
            model: faultHistoryModel

            delegate: Rectangle {
                width: ListView.view.width
                height: root.faultHistoryRowHeight
                radius: 6
                color: "#f7fafb"
                border.color: "#d7e1e5"

                Rectangle {
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    width: 4
                    radius: 2
                    color: levelColor
                }

                Text {
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.leftMargin: 16
                    anchors.topMargin: 7
                    text: codeText
                    color: "#163542"
                    font.pixelSize: 15
                    font.bold: true
                }

                Text {
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.leftMargin: 108
                    anchors.topMargin: 7
                    text: titleText
                    color: "#163542"
                    font.pixelSize: 15
                    font.bold: true
                }

                Text {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.leftMargin: 108
                    anchors.rightMargin: 14
                    anchors.topMargin: 28
                    text: detailText
                    color: "#4b5b63"
                    elide: Text.ElideRight
                    font.pixelSize: 13
                }

                Text {
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.rightMargin: 14
                    anchors.topMargin: 7
                    text: timeText
                    color: "#6f8088"
                    font.pixelSize: 13
                }
            }
        }

        Rectangle {
            id: moreFaultHistoryButton
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.leftMargin: 16
            anchors.rightMargin: 16
            anchors.bottomMargin: 14
            height: 36
            radius: 5
            color: "#0f5b78"

            Text {
                anchors.centerIn: parent
                text: qsTr("More")
                color: "#ffffff"
                font.pixelSize: 16
                font.bold: true
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    root.alarmHistoryOpen = false
                    root.faultHistoryPageRequested(faultHistoryModel)
                }
            }
        }
    }

    Rectangle {
        id: livePanel
        anchors.right: parent.right
        anchors.top: headerPanel.bottom
        anchors.rightMargin: root.pageMargin
        anchors.topMargin: 16
        width: root.leftColumnWidth
        height: 260
        radius: root.cardRadius
        color: "#ffffff"
        border.color: "#cad7dd"

        Text {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.leftMargin: root.panelPadding
            anchors.topMargin: 18
            text: qsTr("Live Status")
            color: "#163542"
            font.pixelSize: root.sectionTitleFontSize
            font.bold: true
        }

        Text {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.leftMargin: root.panelPadding
            anchors.topMargin: 66
            text: qsTr("Current Pressure")
            color: "#4b5b63"
            font.pixelSize: root.bodyFontSize
        }

        Text {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.leftMargin: 182
            anchors.topMargin: 56
            text: qsTr("25.0 kPa")
            color: "#0f5b78"
            font.pixelSize: root.valueFontSize
            font.bold: true
        }

        Text {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.leftMargin: root.panelPadding
            anchors.topMargin: 118
            text: qsTr("Fluid Level")
            color: "#4b5b63"
            font.pixelSize: root.bodyFontSize
        }

        Rectangle {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.leftMargin: 182
            anchors.topMargin: 120
            width: 172
            height: 20
            radius: 10
            color: "#d7e1e5"

            Rectangle {
                width: 116
                height: parent.height
                radius: 10
                color: "#2e7d32"
            }
        }

        Text {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.leftMargin: root.panelPadding
            anchors.topMargin: 158
            text: qsTr("Temperature")
            color: "#4b5b63"
            font.pixelSize: root.bodyFontSize
        }

        Text {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.leftMargin: 182
            anchors.topMargin: 152
            text: qsTr("36.8 ℃")
            color: "#4b5b63"
            font.pixelSize: root.secondaryValueFontSize
        }

        Text {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.leftMargin: root.panelPadding
            anchors.topMargin: 198
            text: qsTr("Fan")
            color: "#4b5b63"
            font.pixelSize: root.bodyFontSize
        }

        Text {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.leftMargin: 182
            anchors.topMargin: 194
            text: qsTr("%1%").arg(root.fanPercent)
            color: "#2e7d32"
            font.pixelSize: 19
        }

        Text {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.leftMargin: root.panelPadding
            anchors.topMargin: 232
            text: qsTr("Push")
            color: "#4b5b63"
            font.pixelSize: root.bodyFontSize
        }

        Text {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.leftMargin: 182
            anchors.topMargin: 228
            text: qsTr("%1%").arg(root.pushPercent)
            color: "#0f5b78"
            font.pixelSize: 19
        }
    }

    Rectangle {
        id: controlPanel
        anchors.left: livePanel.left
        anchors.top: livePanel.bottom
        anchors.topMargin: root.panelGap
        width: root.leftColumnWidth
        height: 212
        radius: root.cardRadius
        color: "#ffffff"
        border.color: "#cad7dd"

        Text {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.leftMargin: root.panelPadding
            anchors.topMargin: 18
            text: qsTr("Parameter Adjustment")
            color: "#163542"
            font.pixelSize: root.sectionTitleFontSize
            font.bold: true
        }

        Text {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.leftMargin: root.panelPadding
            anchors.topMargin: 70
            text: qsTr("Target Pressure")
            color: "#4b5b63"
            font.pixelSize: root.bodyFontSize
        }

        Text {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.leftMargin: 212
            anchors.topMargin: 70
            text: qsTr("25 kPa")
            color: "#163542"
            font.pixelSize: 18
        }

        Text {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.leftMargin: root.panelPadding
            anchors.topMargin: 114
            text: qsTr("Target Flow Rate")
            color: "#4b5b63"
            font.pixelSize: root.bodyFontSize
        }

        Text {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.leftMargin: 212
            anchors.topMargin: 114
            text: qsTr("45 L/min")
            color: "#163542"
            font.pixelSize: 18
        }

        Text {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.leftMargin: root.panelPadding
            anchors.topMargin: 158
            text: qsTr("Alarm Threshold")
            color: "#4b5b63"
            font.pixelSize: root.bodyFontSize
        }

        Text {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.leftMargin: 212
            anchors.topMargin: 158
            text: qsTr("75%")
            color: "#163542"
            font.pixelSize: 18
        }
    }

    Row {
        id: actionButtonRow
        anchors.left: controlPanel.left
        anchors.top: controlPanel.bottom
        anchors.topMargin: 16
        width: root.leftColumnWidth
        height: root.actionButtonHeight
        spacing: 10

        Rectangle {
            width: (actionButtonRow.width - actionButtonRow.spacing * 2) / 3
            height: parent.height
            radius: 5
            color: "#0f5b78"

            Text {
                anchors.centerIn: parent
                text: qsTr("Info")
                color: "#ffffff"
                font.pixelSize: 20
                font.bold: true
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: root.informationPageRequested()
            }
        }

        Rectangle {
            width: (actionButtonRow.width - actionButtonRow.spacing * 2) / 3
            height: parent.height
            radius: 5
            color: "#2e7d32"

            Text {
                anchors.centerIn: parent
                text: qsTr("Maint.")
                color: "#ffffff"
                font.pixelSize: 20
                font.bold: true
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: root.maintenancePageRequested()
            }
        }

        Rectangle {
            width: (actionButtonRow.width - actionButtonRow.spacing * 2) / 3
            height: parent.height
            radius: 5
            color: "#c47a20"

            Text {
                anchors.centerIn: parent
                text: qsTr("Setup")
                color: "#ffffff"
                font.pixelSize: 20
                font.bold: true
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: root.settingsPageRequested()
            }
        }
    }

    Rectangle {
        id: trendPanel
        anchors.left: parent.left
        anchors.right: livePanel.left
        anchors.top: livePanel.top
        anchors.bottom: actionButtonRow.bottom
        anchors.leftMargin: root.pageMargin
        anchors.rightMargin: root.panelGap
        radius: root.cardRadius
        color: "#ffffff"
        border.color: "#cad7dd"

        Text {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.leftMargin: root.panelPadding
            anchors.topMargin: 18
            text: qsTr("Trend Curves")
            color: "#163542"
            font.pixelSize: root.sectionTitleFontSize
            font.bold: true
        }

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.leftMargin: root.panelPadding
            anchors.rightMargin: root.panelPadding
            anchors.topMargin: 58
            anchors.bottomMargin: root.panelPadding
            radius: 4
            color: "#f7fafb"
            border.color: "#d7e1e5"

            Text {
                anchors.centerIn: parent
                text: qsTr("Reserved area for pressure / fluid level / temperature trends")
                color: "#6f8088"
                font.pixelSize: 19
            }
        }
    }

}

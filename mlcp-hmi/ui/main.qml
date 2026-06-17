import QtQuick 2.15
import QtQuick.Window 2.15
import "pages"

Window {
    id: root
    width: 1280
    height: 720
    visible: true
    title: qsTr("MLCP Main Screen")

    property bool faultHistoryPageOpen: false
    property string activeToolPage: ""
    property var selectedFaultHistoryModel

    MainScreen {
        anchors.fill: parent
        visible: !root.faultHistoryPageOpen && root.activeToolPage === ""

        onFaultHistoryPageRequested: function(historyModel) {
            root.selectedFaultHistoryModel = historyModel
            root.faultHistoryPageOpen = true
        }

        onInformationPageRequested: root.activeToolPage = "information"
        onMaintenancePageRequested: root.activeToolPage = "maintenance"
        onSettingsPageRequested: root.activeToolPage = "settings"
    }

    FaultHistoryPage {
        anchors.fill: parent
        visible: root.faultHistoryPageOpen
        historyModel: root.selectedFaultHistoryModel

        onBackRequested: root.faultHistoryPageOpen = false
    }

    InformationPage {
        anchors.fill: parent
        visible: root.activeToolPage === "information"

        onBackRequested: root.activeToolPage = ""
    }

    MaintenancePage {
        anchors.fill: parent
        visible: root.activeToolPage === "maintenance"

        onBackRequested: root.activeToolPage = ""
    }

    SettingsPage {
        anchors.fill: parent
        visible: root.activeToolPage === "settings"

        onBackRequested: root.activeToolPage = ""
    }
}

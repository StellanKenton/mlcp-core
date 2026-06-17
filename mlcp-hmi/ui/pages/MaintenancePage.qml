import QtQuick 2.15

Item {
    id: root
    width: 1280
    height: 720

    signal backRequested()

    readonly property int pageMargin: 24
    readonly property int panelPadding: 22
    readonly property int cardRadius: 8
    property bool selfTestCompleted: false

    function runSelfTest() {
        selfTestCompleted = true
        selfTestModel.setProperty(0, "statusText", "PASS")
        selfTestModel.setProperty(1, "statusText", "PASS")
        selfTestModel.setProperty(2, "statusText", "PASS")
        selfTestModel.setProperty(3, "statusText", "PASS")
        selfTestModel.setProperty(4, "statusText", "PASS")
        selfTestModel.setProperty(5, "statusText", "PASS")
    }

    ListModel {
        id: selfTestModel

        ListElement {
            itemText: "Pressure Sensor"
            statusText: "WAIT"
        }

        ListElement {
            itemText: "Liquid Level Sensor"
            statusText: "WAIT"
        }

        ListElement {
            itemText: "Temperature Sensor"
            statusText: "WAIT"
        }

        ListElement {
            itemText: "Fan Control"
            statusText: "WAIT"
        }

        ListElement {
            itemText: "OLED Display"
            statusText: "WAIT"
        }

        ListElement {
            itemText: "Database Access"
            statusText: "WAIT"
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
            anchors.right: selfTestButton.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.leftMargin: 24
            anchors.rightMargin: 24
            text: qsTr("Maintenance")
            color: "#ffffff"
            font.pixelSize: 30
            font.bold: true
            verticalAlignment: Text.AlignVCenter
        }

        Rectangle {
            id: selfTestButton
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.rightMargin: root.panelPadding
            width: 136
            height: 44
            radius: 5
            color: "#2e7d32"

            Text {
                anchors.centerIn: parent
                text: qsTr("Test")
                color: "#ffffff"
                font.pixelSize: 18
                font.bold: true
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: root.runSelfTest()
            }
        }
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: headerPanel.bottom
        anchors.bottom: parent.bottom
        anchors.leftMargin: root.pageMargin
        anchors.rightMargin: root.pageMargin
        anchors.topMargin: 16
        anchors.bottomMargin: root.pageMargin
        radius: root.cardRadius
        color: "#ffffff"
        border.color: "#cad7dd"

        ListView {
            anchors.fill: parent
            anchors.margins: root.panelPadding
            spacing: 10
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            model: selfTestModel

            delegate: Rectangle {
                width: ListView.view.width
                height: 70
                radius: 6
                color: "#f7fafb"
                border.color: "#d7e1e5"

                Text {
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.leftMargin: 20
                    text: itemText
                    color: "#163542"
                    font.pixelSize: 20
                    font.bold: true
                }

                Rectangle {
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.rightMargin: 20
                    width: 92
                    height: 36
                    radius: 18
                    color: statusText === "PASS" ? "#2e7d32" : "#6f8088"

                    Text {
                        anchors.centerIn: parent
                        text: statusText
                        color: "#ffffff"
                        font.pixelSize: 17
                        font.bold: true
                    }
                }
            }
        }
    }
}

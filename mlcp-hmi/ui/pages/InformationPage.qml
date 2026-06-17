import QtQuick 2.15

Item {
    id: root
    width: 1280
    height: 720

    signal backRequested()

    readonly property int pageMargin: 24
    readonly property int panelPadding: 22
    readonly property int cardRadius: 8

    ListModel {
        id: informationModel

        ListElement {
            labelText: "Software Version"
            valueText: "0.1.0"
        }

        ListElement {
            labelText: "Hardware Version"
            valueText: "HW-A1"
        }

        ListElement {
            labelText: "SN"
            valueText: "MLCP-20260617-001"
        }

        ListElement {
            labelText: "Storage"
            valueText: "18.6 GB / 32 GB"
        }

        ListElement {
            labelText: "Runtime"
            valueText: "126 h"
        }

        ListElement {
            labelText: "Build"
            valueText: "Qt HMI"
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
            text: qsTr("Information")
            color: "#ffffff"
            font.pixelSize: 30
            font.bold: true
            verticalAlignment: Text.AlignVCenter
        }
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: headerPanel.bottom
        anchors.leftMargin: root.pageMargin
        anchors.rightMargin: root.pageMargin
        anchors.topMargin: 16
        height: 420
        radius: root.cardRadius
        color: "#ffffff"
        border.color: "#cad7dd"

        GridView {
            anchors.fill: parent
            anchors.margins: root.panelPadding
            cellWidth: (width - 20) / 2
            cellHeight: 104
            interactive: false
            model: informationModel

            delegate: Rectangle {
                width: GridView.view.cellWidth - 10
                height: 86
                radius: 6
                color: "#f7fafb"
                border.color: "#d7e1e5"

                Text {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.leftMargin: 18
                    anchors.rightMargin: 18
                    anchors.topMargin: 14
                    text: labelText
                    color: "#4b5b63"
                    font.pixelSize: 17
                    elide: Text.ElideRight
                }

                Text {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.leftMargin: 18
                    anchors.rightMargin: 18
                    anchors.bottomMargin: 14
                    text: valueText
                    color: "#163542"
                    font.pixelSize: 24
                    font.bold: true
                    elide: Text.ElideRight
                }
            }
        }
    }
}

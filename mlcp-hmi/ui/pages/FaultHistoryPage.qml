import QtQuick 2.15

Item {
    id: root
    width: 1280
    height: 720

    property var historyModel
    signal backRequested()

    readonly property int pageMargin: 24
    readonly property int panelPadding: 22
    readonly property int cardRadius: 8

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
            anchors.right: recordCountText.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.leftMargin: 24
            anchors.rightMargin: 24
            text: qsTr("Fault History")
            color: "#ffffff"
            font.pixelSize: 30
            font.bold: true
            verticalAlignment: Text.AlignVCenter
        }

        Text {
            id: recordCountText
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.rightMargin: root.panelPadding
            text: qsTr("%1 records").arg(root.historyModel ? root.historyModel.count : 0)
            color: "#d9edf4"
            font.pixelSize: 18
        }
    }

    Rectangle {
        id: historyPanel
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
        clip: true

        ListView {
            id: historyListView
            anchors.fill: parent
            anchors.margins: root.panelPadding
            spacing: 10
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            model: root.historyModel

            delegate: Rectangle {
                width: ListView.view.width
                height: 72
                radius: 6
                color: "#f7fafb"
                border.color: "#d7e1e5"

                Rectangle {
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    width: 5
                    radius: 2
                    color: levelColor
                }

                Text {
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.leftMargin: 20
                    anchors.topMargin: 12
                    text: codeText
                    color: "#163542"
                    font.pixelSize: 17
                    font.bold: true
                }

                Text {
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.leftMargin: 148
                    anchors.topMargin: 12
                    text: titleText
                    color: "#163542"
                    font.pixelSize: 17
                    font.bold: true
                }

                Text {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.leftMargin: 148
                    anchors.rightMargin: 120
                    anchors.topMargin: 40
                    text: detailText
                    color: "#4b5b63"
                    elide: Text.ElideRight
                    font.pixelSize: 15
                }

                Text {
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.rightMargin: 18
                    anchors.topMargin: 14
                    text: timeText
                    color: "#6f8088"
                    font.pixelSize: 15
                }
            }
        }
    }
}

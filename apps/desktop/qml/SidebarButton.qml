import QtQuick
import QtQuick.Controls

Button {
    id: control
    property string iconText: ""

    flat: true
    height: 46
    leftPadding: 12
    rightPadding: 12
    font.pixelSize: 14

    contentItem: Item {
        implicitHeight: 32
        Text {
            id: iconLabel
            text: control.iconText
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            font.pixelSize: 16
            color: control.checked ? "#07111f" : "#6b7f99"
            width: 26
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
        Text {
            anchors.left: iconLabel.right
            anchors.leftMargin: 8
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            text: control.text
            color: control.checked ? "#07111f" : "#455a73"
            font.pixelSize: 14
            font.weight: control.checked ? Font.DemiBold : Font.Normal
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
            maximumLineCount: 1
        }
    }

    background: Rectangle {
        radius: 10
        color: control.checked ? "#bfff2e" : control.hovered ? "#e9fff8" : "transparent"
        border.color: control.checked ? "#97e217" : control.hovered ? "#b9f5e7" : "transparent"
        border.width: control.checked || control.hovered ? 1 : 0
        Rectangle {
            visible: control.checked
            width: 3
            radius: 1
            color: "#ff6aa7"
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.topMargin: 8
            anchors.bottomMargin: 8
        }
        Behavior on color { ColorAnimation { duration: 140 } }
    }
}

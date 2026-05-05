import QtQuick
import QtQuick.Controls

Button {
    id: control
    property string iconText: ""
    property bool active: false

    flat: true
    height: 46
    leftPadding: 14
    rightPadding: 14
    font.pixelSize: 14

    contentItem: Item {
        implicitHeight: 34
        Text {
            id: iconLabel
            text: control.iconText
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            font.pixelSize: 16
            font.weight: Font.DemiBold
            color: control.active ? "white" : "#315a7d"
            width: 28
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
        Text {
            anchors.left: iconLabel.right
            anchors.leftMargin: 9
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            text: control.text
            color: control.active ? "white" : "#244968"
            font.pixelSize: 14
            font.weight: control.active ? Font.DemiBold : Font.Normal
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
            maximumLineCount: 1
        }
    }

    background: Rectangle {
        radius: AwaTheme.radiusMd
        color: control.active ? "#1677df" : control.hovered ? "#e7f4ff" : "transparent"
        border.color: control.active ? "#0757ad" : control.hovered ? "#9dccff" : "transparent"
        border.width: control.active || control.hovered ? 1 : 0
        Rectangle {
            visible: control.active
            width: 3
            radius: 1
            color: "#c8e4ff"
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.topMargin: 9
            anchors.bottomMargin: 9
        }
        Behavior on color { ColorAnimation { duration: 140 } }
    }
}

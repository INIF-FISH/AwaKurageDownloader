import QtQuick
import QtQuick.Controls

Button {
    id: control
    property string iconText: ""

    flat: true
    height: 44
    leftPadding: 10
    rightPadding: 10
    font.pixelSize: 14

    contentItem: Item {
        implicitHeight: 32
        Text {
            id: iconLabel
            text: control.iconText
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            font.pixelSize: 16
            color: control.checked ? "#0f766e" : "#64748b"
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
            color: control.checked ? "#0f172a" : "#475569"
            font.pixelSize: 14
            font.weight: control.checked ? Font.DemiBold : Font.Normal
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
            maximumLineCount: 1
        }
    }

    background: Rectangle {
        radius: 8
        color: control.checked ? "#ccfbf1" : control.hovered ? "#f1f5f9" : "transparent"
        Behavior on color { ColorAnimation { duration: 140 } }
    }
}

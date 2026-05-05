import QtQuick
import QtQuick.Controls

ToolButton {
    id: control

    property string tone: "neutral"

    implicitWidth: 34
    implicitHeight: 34
    font.pixelSize: 14
    font.weight: Font.DemiBold
    padding: 0

    readonly property color baseColor: {
        if (!enabled) return "#eef2f7"
        if (tone === "primary") return "#bfff2e"
        if (tone === "danger") return "#ff5fa2"
        return "#e8fff7"
    }
    readonly property color hoverColor: {
        if (!enabled) return "#eef2f7"
        if (tone === "primary") return "#adf51f"
        if (tone === "danger") return "#ff4b96"
        return "#d9fff1"
    }

    contentItem: Text {
        text: control.text
        font: control.font
        color: control.enabled ? "#0f172a" : "#94a3b8"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    background: Rectangle {
        radius: 9
        color: control.down ? "#c7fae7" : control.hovered ? control.hoverColor : control.baseColor
        border.color: control.enabled ? "#00d1a7" : "#cbd5e1"
        border.width: 1
        Behavior on color { ColorAnimation { duration: 120 } }
    }
}

import QtQuick
import QtQuick.Controls
import AwaKurageDownloader 1.0

ToolButton {
    id: control

    property string tone: "neutral"

    implicitWidth: 36
    implicitHeight: 36
    font.pixelSize: 14
    font.weight: Font.DemiBold
    padding: 0
    hoverEnabled: enabled

    readonly property color baseColor: {
        if (!enabled) return "#dce5ee"
        if (tone === "primary") return "#1677df"
        if (tone === "danger") return "#ffe9ee"
        return "#d9ecff"
    }
    readonly property color hoverColor: {
        if (!enabled) return "#dce5ee"
        if (tone === "primary") return "#0969cf"
        if (tone === "danger") return "#ffd5df"
        return "#c6e2ff"
    }
    readonly property color textColor: {
        if (!enabled) return "#5c6d7c"
        if (tone === "primary") return "white"
        if (tone === "danger") return "#9d2238"
        return "#1677df"
    }

    contentItem: Text {
        text: control.text
        font: control.font
        color: control.textColor
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        radius: AwaTheme.radiusMd
        color: control.down ? "#add5ff" : control.hovered ? control.hoverColor : control.baseColor
        border.color: control.enabled ? (control.tone === "danger" ? "#ee91a3" : "#75b7f5") : "#9fb0bf"
        border.width: 1
        Behavior on color { ColorAnimation { duration: 120 } }
    }
}

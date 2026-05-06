import QtQuick
import QtQuick.Controls
import AwaKurageDownloader 1.0

Button {
    id: control

    property string tone: "neutral"

    implicitHeight: 40
    implicitWidth: Math.max(96, contentItem.implicitWidth + leftPadding + rightPadding)
    leftPadding: 18
    rightPadding: 18
    topPadding: 8
    bottomPadding: 8
    font.pixelSize: 13
    font.weight: Font.DemiBold
    flat: true
    hoverEnabled: enabled

    readonly property color baseColor: {
        if (!enabled) return "#dce5ee"
        if (tone === "primary") return "#1677df"
        if (tone === "danger") return "#ffe9ee"
        if (tone === "ghost") return "transparent"
        return "#d9ecff"
    }
    readonly property color hoverColor: {
        if (!enabled) return "#dce5ee"
        if (tone === "primary") return "#0969cf"
        if (tone === "danger") return "#ffd5df"
        if (tone === "ghost") return AwaTheme.primaryPale
        return "#c6e2ff"
    }
    readonly property color pressColor: {
        if (!enabled) return "#dce5ee"
        if (tone === "primary") return "#0757ad"
        if (tone === "danger") return "#ffc4d1"
        if (tone === "ghost") return AwaTheme.primarySoft
        return "#add5ff"
    }
    readonly property color textColor: {
        if (!enabled) return "#5c6d7c"
        if (tone === "primary") return "white"
        if (tone === "danger") return "#9d2238"
        return "#12314f"
    }
    readonly property color lineColor: {
        if (!enabled) return "#9fb0bf"
        if (tone === "primary") return "#1677df"
        if (tone === "danger") return "#ee91a3"
        return "#75b7f5"
    }

    contentItem: Text {
        text: control.text
        font: control.font
        color: control.textColor
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
        maximumLineCount: 1
    }

    background: Rectangle {
        radius: AwaTheme.radiusMd
        color: control.down ? control.pressColor : control.hovered ? control.hoverColor : control.baseColor
        border.color: control.tone === "primary" ? "#0757ad" : control.lineColor
        border.width: 1

        Behavior on color { ColorAnimation { duration: 120 } }
    }
}

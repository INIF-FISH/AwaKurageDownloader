import QtQuick
import QtQuick.Controls

Button {
    id: control

    property string tone: "neutral"

    implicitHeight: 36
    implicitWidth: Math.max(86, contentItem.implicitWidth + leftPadding + rightPadding)
    leftPadding: 16
    rightPadding: 16
    topPadding: 7
    bottomPadding: 7
    font.pixelSize: 13
    font.weight: Font.DemiBold
    flat: true

    readonly property color baseColor: {
        if (!enabled) return "#eef2f7"
        if (tone === "primary") return "#bfff2e"
        if (tone === "danger") return "#ff6aa7"
        if (tone === "ghost") return "transparent"
        return "#e9fff8"
    }
    readonly property color hoverColor: {
        if (!enabled) return "#eef2f7"
        if (tone === "primary") return "#adf51f"
        if (tone === "danger") return "#ff579d"
        if (tone === "ghost") return "#eef5f6"
        return "#dcfff4"
    }
    readonly property color pressColor: {
        if (!enabled) return "#eef2f7"
        if (tone === "primary") return "#9fe817"
        if (tone === "danger") return "#f43f8a"
        if (tone === "ghost") return "#e2e8f0"
        return "#c7fae7"
    }
    readonly property color textColor: {
        if (!enabled) return "#94a3b8"
        if (tone === "danger") return "#1f1020"
        return "#0f172a"
    }
    readonly property color lineColor: {
        if (!enabled) return "#cbd5e1"
        if (tone === "primary") return "#07111f"
        if (tone === "danger") return "#0f172a"
        if (tone === "ghost") return "#cbd5e1"
        return "#00d1a7"
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
        radius: 9
        color: control.down ? control.pressColor : control.hovered ? control.hoverColor : control.baseColor
        border.color: control.tone === "ghost" ? control.lineColor : "transparent"
        border.width: control.tone === "ghost" ? 1 : 0

        Rectangle {
            visible: control.enabled && control.tone !== "ghost"
            width: parent.width
            height: 3
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            radius: 1
            color: control.lineColor
            opacity: control.down ? 0.75 : 0.45
        }

        Behavior on color { ColorAnimation { duration: 120 } }
    }
}

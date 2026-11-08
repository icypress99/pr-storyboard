import QtQuick
import QtQuick.Controls

import GitEase_Style

/*! ***********************************************************************************************
 * StudioFlatButton — always uses GitEase Style text colors (works in light and dark).
 * ************************************************************************************************/
Button {
    id: control

    flat: true
    font.family: Style.fontTypes.inter
    font.pixelSize: Style.dp(12)

    property bool primary: false
    property bool danger: false
    property color textColor: {
        if (!enabled)
            return Style.colors.secondaryText
        if (primary)
            return Style.colors.onAccentText
        if (danger)
            return Style.colors.error
        if (checked)
            return Style.colors.accent
        return Style.colors.foreground
    }

    contentItem: Label {
        text: control.text
        font: control.font
        color: control.textColor
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
        // Keep disabled text readable in dark mode (no extra fade on top of muted color).
        opacity: control.enabled ? 1.0 : 0.72
    }

    background: Rectangle {
        implicitHeight: Style.dp(30)
        radius: Style.dp(6)
        color: {
            if (control.primary && control.enabled)
                return Style.colors.accent
            if (control.checked)
                return Style.colors.pluginSidebarRowActiveBg
            if (control.hovered && control.enabled)
                return Style.colors.pluginSidebarRowHoverBg
            return "transparent"
        }
        border.width: control.primary && control.enabled ? 0 : (control.flat ? 0 : 1)
        border.color: Style.colors.pluginBtnSecondaryBorder
    }
}

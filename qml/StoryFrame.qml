import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import GitEase_Style

/*! ***********************************************************************************************
 * StoryFrame — one storyboard card (commit or cluster).
 * ************************************************************************************************/
Rectangle {
    id: root

    property int index: 0
    property var frame: ({})
    property bool selected: false

    signal clicked()

    readonly property string kind: (frame && frame.kind) ? frame.kind : "commit"
    readonly property int additions: (frame && frame.additions) ? frame.additions : 0
    readonly property int deletions: (frame && frame.deletions) ? frame.deletions : 0
    readonly property string title: (frame && frame.title) ? frame.title : ""
    readonly property string subtitle: {
        const t = (frame && frame.type) ? frame.type : ""
        const p = (frame && frame.primaryPath) ? frame.primaryPath : ""
        if (t && p)
            return t + " · " + p
        return t || p || kind
    }
    readonly property var files: (frame && frame.files) ? frame.files : []

    width: Style.dp(200)
    height: Style.dp(148)
    radius: Style.dp(10)
    color: selected ? Style.colors.pluginSidebarRowActiveBg : Style.colors.pluginCardBackground
    border.width: selected ? 2 : 1
    border.color: selected ? Style.colors.accent
                           : (kind === "merge" ? Style.colors.mutedText : Style.colors.pluginCardBorder)
    opacity: kind === "merge" ? 0.72 : 1.0

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Style.dp(10)
        spacing: Style.dp(6)

        RowLayout {
            Layout.fillWidth: true
            Label {
                text: (root.index + 1) < 10 ? ("0" + (root.index + 1)) : String(root.index + 1)
                font.family: Style.fontTypes.inter
                font.pixelSize: Style.dp(11)
                font.bold: true
                color: Style.colors.accent
            }
            Item { Layout.fillWidth: true }
            Label {
                text: "+" + root.additions + " / −" + root.deletions
                font.family: Style.fontTypes.inter
                font.pixelSize: Style.dp(10)
                font.bold: true
                color: Style.colors.secondaryText
            }
        }

        Label {
            Layout.fillWidth: true
            text: root.title
            font.family: Style.fontTypes.inter
            font.pixelSize: Style.dp(12)
            font.bold: true
            color: Style.colors.foreground
            wrapMode: Text.WordWrap
            maximumLineCount: 3
            elide: Text.ElideRight
        }

        Label {
            Layout.fillWidth: true
            text: root.subtitle
            font.family: Style.fontTypes.inter
            font.pixelSize: Style.dp(10)
            color: Style.colors.mutedText
            elide: Text.ElideRight
        }

        Item { Layout.fillHeight: true }

        Row {
            Layout.fillWidth: true
            spacing: Style.dp(3)
            Repeater {
                model: Math.min(5, (root.files || []).length)
                delegate: Rectangle {
                    width: Style.dp(14)
                    height: Style.dp(18)
                    radius: Style.dp(3)
                    color: Style.colors.secondaryBackground
                    readonly property real ratio: {
                        const files = root.files || []
                        let max = 1
                        for (let i = 0; i < Math.min(5, files.length); ++i)
                            max = Math.max(max, (files[i].add || 0) + (files[i].del || 0))
                        const h = (files[index].add || 0) + (files[index].del || 0)
                        return h / max
                    }
                    Rectangle {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        height: Math.max(Style.dp(3), parent.height * parent.ratio)
                        radius: Style.dp(3)
                        color: Style.colors.accent
                        opacity: 0.75
                    }
                }
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}

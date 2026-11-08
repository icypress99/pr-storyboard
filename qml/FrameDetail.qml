import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import GitEase_Style

/*! ***********************************************************************************************
 * FrameDetail — selected frame metadata and file heat list.
 * ************************************************************************************************/
Rectangle {
    id: root

    property var frame: ({})
    property int index: -1

    signal copyDiffCommand()

    radius: Style.dp(10)
    color: Style.colors.pluginPanelBackground
    border.width: 1
    border.color: Style.colors.pluginPanelBorder
    clip: true

    readonly property bool hasFrame: !!(frame && frame.id)

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Style.dp(12)
        spacing: Style.dp(8)

        RowLayout {
            Layout.fillWidth: true
            spacing: Style.dp(8)
            Label {
                text: root.hasFrame
                      ? qsTr("Frame %1").arg((root.index + 1) < 10 ? ("0" + (root.index + 1)) : String(root.index + 1))
                      : qsTr("Frame detail")
                font.family: Style.fontTypes.inter
                font.pixelSize: Style.dp(12)
                font.bold: true
                color: Style.colors.foreground
                Layout.fillWidth: true
            }
            StudioFlatButton {
                text: qsTr("Copy diff cmd")
                enabled: root.hasFrame
                onClicked: root.copyDiffCommand()
            }
        }

        Label {
            visible: root.hasFrame
            Layout.fillWidth: true
            text: (frame && frame.title) ? frame.title : ""
            font.family: Style.fontTypes.inter
            font.pixelSize: Style.dp(14)
            font.bold: true
            color: Style.colors.foreground
            wrapMode: Text.WordWrap
        }

        Label {
            visible: root.hasFrame
            Layout.fillWidth: true
            text: {
                if (!frame)
                    return ""
                const bits = []
                if (frame.author)
                    bits.push(frame.author)
                if (frame.authoredAt)
                    bits.push(String(frame.authoredAt).slice(0, 19).replace("T", " "))
                if (frame.kind)
                    bits.push(frame.kind)
                if (frame.hashes && frame.hashes.length)
                    bits.push(frame.hashes.map(function(h) { return String(h).slice(0, 7) }).join(", "))
                return bits.join(" · ")
            }
            font.family: Style.fontTypes.inter
            font.pixelSize: Style.dp(11)
            color: Style.colors.mutedText
            wrapMode: Text.WordWrap
        }

        Label {
            visible: root.hasFrame && frame.body
            Layout.fillWidth: true
            text: frame.body || ""
            font.family: Style.fontTypes.inter
            font.pixelSize: Style.dp(11)
            color: Style.colors.secondaryText
            wrapMode: Text.WordWrap
            maximumLineCount: 4
            elide: Text.ElideRight
        }

        Label {
            text: qsTr("Files")
            font.family: Style.fontTypes.inter
            font.pixelSize: Style.dp(11)
            font.bold: true
            color: Style.colors.pluginSidebarLabel
        }

        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: Style.dp(4)
            model: (frame && frame.files) ? frame.files : []

            delegate: Rectangle {
                width: ListView.view.width
                height: Style.dp(36)
                radius: Style.dp(6)
                color: Style.colors.secondaryBackground

                readonly property int heat: (modelData.add || 0) + (modelData.del || 0)

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: Style.dp(8)
                    spacing: Style.dp(8)
                    Label {
                        text: modelData.path || ""
                        font.family: Style.fontTypes.inter
                        font.pixelSize: Style.dp(11)
                        color: Style.colors.foreground
                        elide: Text.ElideMiddle
                        Layout.fillWidth: true
                    }
                    Label {
                        text: modelData.binary ? qsTr("binary")
                                               : ("+" + (modelData.add || 0) + " / −" + (modelData.del || 0))
                        font.family: Style.fontTypes.inter
                        font.pixelSize: Style.dp(10)
                        color: Style.colors.mutedText
                    }
                }
            }

            Label {
                anchors.centerIn: parent
                visible: parent.count === 0
                text: root.hasFrame ? qsTr("No file stats") : qsTr("Select a frame")
                color: Style.colors.mutedText
                font.pixelSize: Style.dp(11)
            }
        }
    }
}

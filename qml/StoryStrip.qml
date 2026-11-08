import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import GitEase_Style

/*! ***********************************************************************************************
 * StoryStrip — horizontal scrubbable frame strip.
 * ************************************************************************************************/
Item {
    id: root

    property var frames: []
    property int selectedIndex: -1
    property bool playing: false

    signal frameClicked(int index)
    signal playToggled()

    function ensureVisible(index) {
        if (index < 0 || index >= repeater.count)
            return
        const item = repeater.itemAt(index)
        if (!item)
            return
        const left = item.x - Style.dp(16)
        const right = item.x + item.width + Style.dp(16)
        if (left < flick.contentX)
            flick.contentX = Math.max(0, left)
        else if (right > flick.contentX + flick.width)
            flick.contentX = Math.max(0, right - flick.width)
    }

    onSelectedIndexChanged: Qt.callLater(function() { root.ensureVisible(root.selectedIndex) })

    Timer {
        id: playTimer
        interval: 1200
        repeat: true
        running: root.playing && root.frames && root.frames.length > 1
        onTriggered: {
            if (root.selectedIndex >= root.frames.length - 1) {
                root.playing = false
                return
            }
            root.frameClicked(root.selectedIndex + 1)
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: Style.dp(8)

        RowLayout {
            Layout.fillWidth: true
            Label {
                text: qsTr("Storyboard")
                font.family: Style.fontTypes.inter
                font.pixelSize: Style.dp(12)
                font.bold: true
                color: Style.colors.pluginSidebarLabel
                Layout.fillWidth: true
            }
            StudioFlatButton {
                text: root.playing ? qsTr("Pause") : qsTr("Play")
                checked: root.playing
                checkable: true
                enabled: root.frames && root.frames.length > 1
                onClicked: {
                    root.playing = !root.playing
                    root.playToggled()
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: Style.dp(10)
            color: Style.colors.secondaryBackground
            border.width: 1
            border.color: Style.colors.pluginPanelBorder
            clip: true

            Flickable {
                id: flick
                anchors.fill: parent
                anchors.margins: Style.dp(12)
                contentWidth: row.implicitWidth
                contentHeight: height
                flickableDirection: Flickable.HorizontalFlick
                clip: true
                boundsBehavior: Flickable.StopAtBounds

                Row {
                    id: row
                    spacing: Style.dp(12)
                    height: parent.height
                    anchors.verticalCenter: parent.verticalCenter

                    Repeater {
                        id: repeater
                        model: root.frames
                        delegate: StoryFrame {
                            anchors.verticalCenter: parent.verticalCenter
                            index: index
                            frame: modelData
                            selected: index === root.selectedIndex
                            onClicked: root.frameClicked(index)
                        }
                    }
                }
            }

            Label {
                anchors.centerIn: parent
                visible: !root.frames || root.frames.length === 0
                text: qsTr("No frames yet")
                color: Style.colors.mutedText
                font.family: Style.fontTypes.inter
                font.pixelSize: Style.dp(12)
            }
        }
    }
}

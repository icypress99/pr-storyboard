import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

import GitEase
import GitEase_Style
import GitEase_Style_Impl
import GitEasePrStoryboard

/*! ***********************************************************************************************
 * StoryboardPage — scrub branch intent, draft PR from frames.
 * ************************************************************************************************/
Rectangle {
    id: root

    property var repositoryController: null
    property var appModel: null
    property var pluginManager: null
    property var notificationController: null
    property var eventBus: null
    property string pluginId: "com.gitease.pr-storyboard"
    property int _branchToken: -1

    readonly property string repoPath: {
        if (repositoryController && repositoryController.activeRepositoryPath)
            return repositoryController.activeRepositoryPath
        if (appModel && appModel.activeRepositoryPath)
            return appModel.activeRepositoryPath
        return ""
    }

    function notify(ok, message) {
        if (!notificationController) {
            console.log(message)
            return
        }
        if (ok)
            notificationController.success(message, qsTr("Storyboard"), 3500)
        else
            notificationController.error(message, qsTr("Storyboard"), 5000)
    }

    function onPageActivated() {
        engine.setRepoPath(root.repoPath)
        engine.refresh()
    }

    color: Style.colors.pluginPageBackground
    Material.theme: Style.isDarkTheme ? Material.Dark : Material.Light
    Material.foreground: Style.colors.foreground
    Material.background: Style.colors.pluginPageBackground
    Material.accent: Style.colors.accent

    StoryboardEngine {
        id: engine
        onActionFinished: function(ok, message) {
            if (message)
                root.notify(ok, message)
        }
    }

    Connections {
        target: root
        function onRepoPathChanged() {
            engine.setRepoPath(root.repoPath)
        }
    }

    Component.onCompleted: {
        forceActiveFocus()
        engine.detectTools()
        engine.setRepoPath(root.repoPath)
        if (root.eventBus) {
            root._branchToken = root.eventBus.subscribeEvent("branch.changed", function() {
                engine.refresh()
            })
            root.eventBus.subscribeEvent("pr-storyboard.open", function() {
                engine.refresh()
            })
        }
    }

    Component.onDestruction: {
        if (root.eventBus && root._branchToken >= 0)
            root.eventBus.unsubscribeEvent(root._branchToken)
    }

    Keys.onPressed: function(event) {
        if (event.key === Qt.Key_Left) {
            engine.selectPrev()
            event.accepted = true
        } else if (event.key === Qt.Key_Right) {
            engine.selectNext()
            event.accepted = true
        } else if (event.key === Qt.Key_Space) {
            strip.playing = !strip.playing
            event.accepted = true
        }
    }
    focus: true

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Style.dp(14)
        spacing: Style.dp(12)

        // Header
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: headerCol.implicitHeight + Style.dp(20)
            radius: Style.dp(10)
            color: Style.colors.pluginPanelBackground
            border.width: 1
            border.color: Style.colors.pluginPanelBorder

            ColumnLayout {
                id: headerCol
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: Style.dp(12)
                spacing: Style.dp(8)

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Style.dp(8)
                    Label {
                        text: "\uf008"
                        font.family: Style.fontTypes.font6Pro
                        font.pixelSize: Style.dp(16)
                        color: Style.colors.accent
                    }
                    Label {
                        text: qsTr("PR Storyboard")
                        font.family: Style.fontTypes.inter
                        font.pixelSize: Style.dp(15)
                        font.bold: true
                        color: Style.colors.foreground
                    }
                    Label {
                        text: engine.headBranch || engine.headRef.slice(0, 7) || qsTr("no branch")
                        font.family: Style.fontTypes.inter
                        font.pixelSize: Style.dp(12)
                        color: Style.colors.accent
                        leftPadding: Style.dp(8)
                        rightPadding: Style.dp(8)
                        topPadding: Style.dp(3)
                        bottomPadding: Style.dp(3)
                        background: Rectangle {
                            radius: Style.dp(8)
                            color: Qt.rgba(Style.colors.accent.r, Style.colors.accent.g, Style.colors.accent.b, 0.14)
                        }
                    }
                    Item { Layout.fillWidth: true }
                    Label {
                        text: engine.statusMessage
                        font.family: Style.fontTypes.inter
                        font.pixelSize: Style.dp(11)
                        color: Style.colors.mutedText
                        elide: Text.ElideRight
                        Layout.maximumWidth: Style.dp(320)
                    }
                    StudioFlatButton {
                        text: qsTr("Refresh")
                        enabled: root.repoPath.length > 0 && !engine.busy
                        onClicked: engine.refresh()
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Style.dp(8)
                    Label {
                        text: qsTr("Base")
                        font.family: Style.fontTypes.inter
                        font.pixelSize: Style.dp(11)
                        color: Style.colors.secondaryText
                    }
                    ComboBox {
                        id: baseBox
                        Layout.preferredWidth: Style.dp(200)
                        model: engine.baseCandidates
                        onActivated: engine.setBaseRef(currentText)
                        function syncIndex() {
                            const list = engine.baseCandidates || []
                            let i = 0
                            for (; i < list.length; ++i) {
                                if (list[i] === engine.baseRef)
                                    break
                            }
                            currentIndex = i < list.length ? i : 0
                        }
                        Component.onCompleted: syncIndex()
                        Connections {
                            target: engine
                            function onBaseRefChanged() { baseBox.syncIndex() }
                            function onBaseCandidatesChanged() { baseBox.syncIndex() }
                        }
                    }
                    StudioFlatButton {
                        text: qsTr("Cluster")
                        checkable: true
                        checked: engine.clusteringEnabled
                        onClicked: engine.clusteringEnabled = !engine.clusteringEnabled
                    }
                    Label {
                        visible: engine.hiddenCommitCount > 0
                        text: qsTr("+%1 older commits hidden").arg(engine.hiddenCommitCount)
                        font.pixelSize: Style.dp(10)
                        color: Style.colors.warning
                    }
                    Item { Layout.fillWidth: true }
                    Label {
                        text: qsTr("%1 frames · %2 commits").arg(engine.frameCount).arg(engine.commitCount)
                        font.family: Style.fontTypes.inter
                        font.pixelSize: Style.dp(11)
                        color: Style.colors.mutedText
                    }
                }
            }
        }

        // Strip + detail
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: 1
            spacing: Style.dp(12)

            StoryStrip {
                id: strip
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 3
                frames: engine.frames
                selectedIndex: engine.selectedIndex
                onFrameClicked: function(index) {
                    strip.playing = false
                    engine.selectedIndex = index
                }
            }

            FrameDetail {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 2
                frame: engine.selectedFrame
                index: engine.selectedIndex
                onCopyDiffCommand: engine.copyFrameDiffCommand(engine.selectedIndex)
            }
        }

        // PR draft footer
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: Style.dp(220)
            radius: Style.dp(10)
            color: Style.colors.pluginPanelBackground
            border.width: 1
            border.color: Style.colors.pluginPanelBorder

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Style.dp(12)
                spacing: Style.dp(8)

                RowLayout {
                    Layout.fillWidth: true
                    Label {
                        text: qsTr("Draft pull request")
                        font.family: Style.fontTypes.inter
                        font.pixelSize: Style.dp(12)
                        font.bold: true
                        color: Style.colors.foreground
                        Layout.fillWidth: true
                    }
                    StudioFlatButton {
                        text: qsTr("Regenerate")
                        enabled: engine.frameCount > 0
                        onClicked: engine.regenerateDraft()
                    }
                    StudioFlatButton {
                        text: qsTr("Copy")
                        enabled: engine.prTitle.length > 0
                        onClicked: engine.copyDraft()
                    }
                    StudioFlatButton {
                        text: engine.existingPrUrl.length ? qsTr("Open PR") : qsTr("Create PR")
                        primary: true
                        enabled: engine.frameCount > 0 && (engine.existingPrUrl.length > 0 || engine.ghAvailable)
                        onClicked: {
                            if (engine.existingPrUrl.length)
                                engine.openExistingPr()
                            else
                                engine.createPullRequest()
                        }
                    }
                }

                TextField {
                    Layout.fillWidth: true
                    text: engine.prTitle
                    placeholderText: qsTr("PR title")
                    font.family: Style.fontTypes.inter
                    font.pixelSize: Style.dp(12)
                    color: Style.colors.foreground
                    onTextEdited: engine.prTitle = text
                    background: Rectangle {
                        radius: Style.dp(6)
                        color: Style.colors.secondaryBackground
                        border.width: 1
                        border.color: Style.colors.pluginPanelBorder
                    }
                }

                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    TextArea {
                        text: engine.prBody
                        placeholderText: qsTr("PR body (Markdown from frames)")
                        wrapMode: TextEdit.Wrap
                        font.family: Style.fontTypes.inter
                        font.pixelSize: Style.dp(11)
                        color: Style.colors.foreground
                        onTextChanged: {
                            if (text !== engine.prBody)
                                engine.prBody = text
                        }
                        background: Rectangle {
                            radius: Style.dp(6)
                            color: Style.colors.secondaryBackground
                            border.width: 1
                            border.color: Style.colors.pluginPanelBorder
                        }
                    }
                }

                Label {
                    visible: !engine.ghAvailable
                    text: qsTr("Create PR needs the GitHub CLI (gh) authenticated on PATH.")
                    font.pixelSize: Style.dp(10)
                    color: Style.colors.warning
                }
            }
        }
    }

    // Empty overlay
    Rectangle {
        anchors.fill: parent
        visible: root.repoPath.length === 0
        color: Style.colors.pluginPageBackground
        z: 10
        Column {
            anchors.centerIn: parent
            spacing: Style.dp(10)
            width: Style.dp(360)
            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "\uf008"
                font.family: Style.fontTypes.font6Pro
                font.pixelSize: Style.dp(32)
                color: Style.colors.accent
            }
            Label {
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                text: qsTr("Open a repository")
                font.family: Style.fontTypes.inter
                font.pixelSize: Style.dp(16)
                font.bold: true
                color: Style.colors.foreground
            }
            Label {
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                text: qsTr("PR Storyboard turns commits ahead of your base into a scrubbable story, then drafts a pull request.")
                font.family: Style.fontTypes.inter
                font.pixelSize: Style.dp(12)
                color: Style.colors.secondaryText
            }
        }
    }
}

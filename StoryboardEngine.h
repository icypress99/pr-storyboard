#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

class StoryboardEngine : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString repoPath READ repoPath WRITE setRepoPath NOTIFY repoPathChanged)
    Q_PROPERTY(QString baseRef READ baseRef WRITE setBaseRef NOTIFY baseRefChanged)
    Q_PROPERTY(QString headRef READ headRef NOTIFY headRefChanged)
    Q_PROPERTY(QString headBranch READ headBranch NOTIFY headBranchChanged)
    Q_PROPERTY(bool clusteringEnabled READ clusteringEnabled WRITE setClusteringEnabled NOTIFY clusteringEnabledChanged)
    Q_PROPERTY(QVariantList frames READ frames NOTIFY framesChanged)
    Q_PROPERTY(int selectedIndex READ selectedIndex WRITE setSelectedIndex NOTIFY selectedIndexChanged)
    Q_PROPERTY(QVariantMap selectedFrame READ selectedFrame NOTIFY selectedIndexChanged)
    Q_PROPERTY(QString prTitle READ prTitle WRITE setPrTitle NOTIFY prDraftChanged)
    Q_PROPERTY(QString prBody READ prBody WRITE setPrBody NOTIFY prDraftChanged)
    Q_PROPERTY(QString existingPrUrl READ existingPrUrl NOTIFY existingPrUrlChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(bool ghAvailable READ ghAvailable NOTIFY toolsChanged)
    Q_PROPERTY(bool gitAvailable READ gitAvailable NOTIFY toolsChanged)
    Q_PROPERTY(int commitCount READ commitCount NOTIFY framesChanged)
    Q_PROPERTY(int frameCount READ frameCount NOTIFY framesChanged)
    Q_PROPERTY(int hiddenCommitCount READ hiddenCommitCount NOTIFY framesChanged)
    Q_PROPERTY(QStringList baseCandidates READ baseCandidates NOTIFY baseCandidatesChanged)

public:
    explicit StoryboardEngine(QObject* parent = nullptr);

    QString repoPath() const { return m_repoPath; }
    void setRepoPath(const QString& path);

    QString baseRef() const { return m_baseRef; }
    void setBaseRef(const QString& ref);

    QString headRef() const { return m_headRef; }
    QString headBranch() const { return m_headBranch; }

    bool clusteringEnabled() const { return m_clusteringEnabled; }
    void setClusteringEnabled(bool enabled);

    QVariantList frames() const { return m_frames; }
    int selectedIndex() const { return m_selectedIndex; }
    void setSelectedIndex(int index);
    QVariantMap selectedFrame() const;

    QString prTitle() const { return m_prTitle; }
    void setPrTitle(const QString& title);
    QString prBody() const { return m_prBody; }
    void setPrBody(const QString& body);

    QString existingPrUrl() const { return m_existingPrUrl; }
    bool busy() const { return m_busy; }
    QString statusMessage() const { return m_statusMessage; }
    bool ghAvailable() const { return m_ghAvailable; }
    bool gitAvailable() const { return m_gitAvailable; }
    int commitCount() const { return m_commitCount; }
    int frameCount() const { return m_frames.size(); }
    int hiddenCommitCount() const { return m_hiddenCommitCount; }
    QStringList baseCandidates() const { return m_baseCandidates; }

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void detectTools();
    Q_INVOKABLE void selectNext();
    Q_INVOKABLE void selectPrev();
    Q_INVOKABLE void regenerateDraft();
    Q_INVOKABLE void copyDraft();
    Q_INVOKABLE void createPullRequest();
    Q_INVOKABLE void openExistingPr();
    Q_INVOKABLE QString frameDiffCommand(int index) const;
    Q_INVOKABLE void copyFrameDiffCommand(int index);

signals:
    void repoPathChanged();
    void baseRefChanged();
    void headRefChanged();
    void headBranchChanged();
    void clusteringEnabledChanged();
    void framesChanged();
    void selectedIndexChanged();
    void prDraftChanged();
    void existingPrUrlChanged();
    void busyChanged();
    void statusMessageChanged();
    void toolsChanged();
    void baseCandidatesChanged();
    void actionFinished(bool ok, const QString& message);

private:
    struct CommitInfo {
        QString hash;
        QString subject;
        QString body;
        QString author;
        QString authoredAt;
        QString type;
        QString primaryPath;
        bool isMerge = false;
        int additions = 0;
        int deletions = 0;
        QVariantList files;
    };

    void setBusy(bool busy);
    void setStatus(const QString& msg);
    QString runGit(const QStringList& args, int* exitCode = nullptr) const;
    QString runGh(const QStringList& args, int* exitCode = nullptr) const;
    QString resolveBaseRef() const;
    void refreshBaseCandidates();
    QList<CommitInfo> loadCommits(const QString& base) const;
    CommitInfo loadCommit(const QString& hash) const;
    static QString conventionalType(const QString& subject);
    static QString topLevelPath(const QString& path);
    QVariantList buildFrames(const QList<CommitInfo>& commits) const;
    QVariantMap commitToMap(const CommitInfo& c, const QString& kind,
                            const QStringList& hashes, const QString& title) const;
    void deriveDraft(const QVariantList& frames);
    void refreshExistingPr();

    QString m_repoPath;
    QString m_baseRef;
    QString m_headRef;
    QString m_headBranch;
    bool m_clusteringEnabled = true;
    QVariantList m_frames;
    int m_selectedIndex = -1;
    QString m_prTitle;
    QString m_prBody;
    QString m_existingPrUrl;
    bool m_busy = false;
    QString m_statusMessage;
    bool m_ghAvailable = false;
    bool m_gitAvailable = false;
    int m_commitCount = 0;
    int m_hiddenCommitCount = 0;
    QStringList m_baseCandidates;
    static constexpr int kMaxCommits = 80;
};

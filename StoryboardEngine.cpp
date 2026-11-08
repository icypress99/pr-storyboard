#include "StoryboardEngine.h"

#include <QClipboard>
#include <QDesktopServices>
#include <QGuiApplication>
#include <QProcess>
#include <QRegularExpression>
#include <QUrl>

namespace {

QString trimNl(QString s)
{
    while (s.endsWith(QLatin1Char('\n')) || s.endsWith(QLatin1Char('\r')))
        s.chop(1);
    return s;
}

} // namespace

StoryboardEngine::StoryboardEngine(QObject* parent)
    : QObject(parent)
{
    detectTools();
}

void StoryboardEngine::setRepoPath(const QString& path)
{
    if (m_repoPath == path)
        return;
    m_repoPath = path;
    emit repoPathChanged();
    refresh();
}

void StoryboardEngine::setBaseRef(const QString& ref)
{
    if (m_baseRef == ref)
        return;
    m_baseRef = ref;
    emit baseRefChanged();
    refresh();
}

void StoryboardEngine::setClusteringEnabled(bool enabled)
{
    if (m_clusteringEnabled == enabled)
        return;
    m_clusteringEnabled = enabled;
    emit clusteringEnabledChanged();
    refresh();
}

void StoryboardEngine::setSelectedIndex(int index)
{
    if (m_frames.isEmpty()) {
        if (m_selectedIndex != -1) {
            m_selectedIndex = -1;
            emit selectedIndexChanged();
        }
        return;
    }
    const int clamped = qBound(0, index, m_frames.size() - 1);
    if (m_selectedIndex == clamped)
        return;
    m_selectedIndex = clamped;
    emit selectedIndexChanged();
}

QVariantMap StoryboardEngine::selectedFrame() const
{
    if (m_selectedIndex < 0 || m_selectedIndex >= m_frames.size())
        return {};
    return m_frames.at(m_selectedIndex).toMap();
}

void StoryboardEngine::setPrTitle(const QString& title)
{
    if (m_prTitle == title)
        return;
    m_prTitle = title;
    emit prDraftChanged();
}

void StoryboardEngine::setPrBody(const QString& body)
{
    if (m_prBody == body)
        return;
    m_prBody = body;
    emit prDraftChanged();
}

void StoryboardEngine::setBusy(bool busy)
{
    if (m_busy == busy)
        return;
    m_busy = busy;
    emit busyChanged();
}

void StoryboardEngine::setStatus(const QString& msg)
{
    if (m_statusMessage == msg)
        return;
    m_statusMessage = msg;
    emit statusMessageChanged();
}

void StoryboardEngine::detectTools()
{
    auto which = [](const QString& name) {
        QProcess p;
        p.start(name, { QStringLiteral("--version") });
        if (!p.waitForFinished(3000)) {
            p.kill();
            return false;
        }
        return p.exitCode() == 0;
    };
    const bool git = which(QStringLiteral("git"));
    const bool gh = which(QStringLiteral("gh"));
    if (git != m_gitAvailable || gh != m_ghAvailable) {
        m_gitAvailable = git;
        m_ghAvailable = gh;
        emit toolsChanged();
    }
}

QString StoryboardEngine::runGit(const QStringList& args, int* exitCode) const
{
    QProcess p;
    p.setWorkingDirectory(m_repoPath);
    p.start(QStringLiteral("git"), args);
    if (!p.waitForFinished(60000)) {
        p.kill();
        if (exitCode)
            *exitCode = -1;
        return {};
    }
    if (exitCode)
        *exitCode = p.exitCode();
    return QString::fromUtf8(p.readAllStandardOutput());
}

QString StoryboardEngine::runGh(const QStringList& args, int* exitCode) const
{
    QProcess p;
    p.setWorkingDirectory(m_repoPath);
    p.start(QStringLiteral("gh"), args);
    if (!p.waitForFinished(90000)) {
        p.kill();
        if (exitCode)
            *exitCode = -1;
        return {};
    }
    if (exitCode)
        *exitCode = p.exitCode();
    return QString::fromUtf8(p.readAllStandardOutput())
         + QString::fromUtf8(p.readAllStandardError());
}

QString StoryboardEngine::resolveBaseRef() const
{
    if (!m_baseRef.isEmpty()) {
        int code = 1;
        runGit({ QStringLiteral("rev-parse"), QStringLiteral("--verify"), m_baseRef }, &code);
        if (code == 0)
            return m_baseRef;
    }

    int code = 1;
    const QString upstream = trimNl(runGit(
        { QStringLiteral("rev-parse"), QStringLiteral("--abbrev-ref"),
          QStringLiteral("--symbolic-full-name"), QStringLiteral("@{upstream}") },
        &code));
    if (code == 0 && !upstream.isEmpty() && upstream != QLatin1String("@{upstream}"))
        return upstream;

    const QStringList fallbacks = {
        QStringLiteral("origin/main"),
        QStringLiteral("origin/master"),
        QStringLiteral("main"),
        QStringLiteral("master"),
    };
    for (const QString& cand : fallbacks) {
        code = 1;
        runGit({ QStringLiteral("rev-parse"), QStringLiteral("--verify"), cand }, &code);
        if (code == 0)
            return cand;
    }
    return {};
}

void StoryboardEngine::refreshBaseCandidates()
{
    QStringList out;
    auto add = [&](const QString& ref) {
        if (ref.isEmpty() || out.contains(ref))
            return;
        int code = 1;
        runGit({ QStringLiteral("rev-parse"), QStringLiteral("--verify"), ref }, &code);
        if (code == 0)
            out.append(ref);
    };
    add(QStringLiteral("origin/main"));
    add(QStringLiteral("origin/master"));
    add(QStringLiteral("main"));
    add(QStringLiteral("master"));
    int code = 1;
    const QString upstream = trimNl(runGit(
        { QStringLiteral("rev-parse"), QStringLiteral("--abbrev-ref"),
          QStringLiteral("--symbolic-full-name"), QStringLiteral("@{upstream}") },
        &code));
    if (code == 0)
        add(upstream);

    if (out != m_baseCandidates) {
        m_baseCandidates = out;
        emit baseCandidatesChanged();
    }
}

QString StoryboardEngine::conventionalType(const QString& subject)
{
    static const QRegularExpression re(
        QStringLiteral(R"(^([a-z]+)(?:\([^)]*\))?!?:)"),
        QRegularExpression::CaseInsensitiveOption);
    const auto m = re.match(subject.trimmed());
    if (!m.hasMatch())
        return {};
    return m.captured(1).toLower();
}

QString StoryboardEngine::topLevelPath(const QString& path)
{
    const int slash = path.indexOf(QLatin1Char('/'));
    if (slash <= 0)
        return path;
    return path.left(slash);
}

StoryboardEngine::CommitInfo StoryboardEngine::loadCommit(const QString& hash) const
{
    CommitInfo info;
    info.hash = hash;

    int code = 1;
    const QString meta = runGit(
        { QStringLiteral("show"), QStringLiteral("-s"),
          QStringLiteral("--format=%s%x1f%b%x1f%an%x1f%aI%x1f%P"), hash },
        &code);
    if (code != 0)
        return info;

    const QStringList parts = meta.split(QChar(0x1f));
    info.subject = parts.value(0).trimmed();
    info.body = trimNl(parts.value(1));
    info.author = parts.value(2).trimmed();
    info.authoredAt = parts.value(3).trimmed();
    const QString parents = parts.value(4).trimmed();
    info.isMerge = parents.split(QLatin1Char(' '), Qt::SkipEmptyParts).size() > 1;
    info.type = conventionalType(info.subject);

    code = 1;
    const QString numstat = runGit(
        { QStringLiteral("show"), QStringLiteral("--numstat"), QStringLiteral("--format="), hash },
        &code);
    QHash<QString, int> dirHeat;
    for (const QString& line : numstat.split(QLatin1Char('\n'), Qt::SkipEmptyParts)) {
        const QStringList cols = line.split(QLatin1Char('\t'));
        if (cols.size() < 3)
            continue;
        const bool binary = cols[0] == QLatin1String("-") || cols[1] == QLatin1String("-");
        const int add = binary ? 0 : cols[0].toInt();
        const int del = binary ? 0 : cols[1].toInt();
        const QString path = cols[2].trimmed();
        info.additions += add;
        info.deletions += del;
        info.files.append(QVariantMap{
            { QStringLiteral("path"), path },
            { QStringLiteral("add"), add },
            { QStringLiteral("del"), del },
            { QStringLiteral("binary"), binary },
        });
        const QString top = topLevelPath(path);
        dirHeat[top] += add + del;
    }

    QString bestPath;
    int best = -1;
    for (auto it = dirHeat.constBegin(); it != dirHeat.constEnd(); ++it) {
        if (it.value() > best) {
            best = it.value();
            bestPath = it.key();
        }
    }
    info.primaryPath = bestPath;
    return info;
}

QList<StoryboardEngine::CommitInfo> StoryboardEngine::loadCommits(const QString& base) const
{
    QList<CommitInfo> list;
    int code = 1;
    const QString out = runGit(
        { QStringLiteral("rev-list"), QStringLiteral("--reverse"), base + QStringLiteral("..HEAD") },
        &code);
    if (code != 0)
        return list;

    const QStringList hashes = out.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    for (const QString& hash : hashes)
        list.append(loadCommit(hash.trimmed()));
    return list;
}

QVariantMap StoryboardEngine::commitToMap(const CommitInfo& c, const QString& kind,
                                          const QStringList& hashes, const QString& title) const
{
    return QVariantMap{
        { QStringLiteral("id"), hashes.join(QLatin1Char('+')) },
        { QStringLiteral("kind"), kind },
        { QStringLiteral("hashes"), hashes },
        { QStringLiteral("title"), title },
        { QStringLiteral("body"), c.body },
        { QStringLiteral("type"), c.type },
        { QStringLiteral("primaryPath"), c.primaryPath },
        { QStringLiteral("additions"), c.additions },
        { QStringLiteral("deletions"), c.deletions },
        { QStringLiteral("files"), c.files },
        { QStringLiteral("authoredAt"), c.authoredAt },
        { QStringLiteral("author"), c.author },
        { QStringLiteral("isMerge"), c.isMerge },
    };
}

QVariantList StoryboardEngine::buildFrames(const QList<CommitInfo>& commits) const
{
    QVariantList frames;
    if (!m_clusteringEnabled) {
        for (const CommitInfo& c : commits) {
            const QString kind = c.isMerge ? QStringLiteral("merge") : QStringLiteral("commit");
            frames.append(commitToMap(c, kind, { c.hash }, c.subject));
        }
        return frames;
    }

    int i = 0;
    while (i < commits.size()) {
        CommitInfo agg = commits.at(i);
        QStringList hashes{ agg.hash };
        QString title = agg.subject;
        QString kind = agg.isMerge ? QStringLiteral("merge") : QStringLiteral("commit");
        int j = i + 1;
        while (j < commits.size()) {
            const CommitInfo& next = commits.at(j);
            if (next.isMerge || agg.isMerge)
                break;
            const bool sameType = !agg.type.isEmpty() && agg.type == next.type;
            const bool samePath = !agg.primaryPath.isEmpty() && agg.primaryPath == next.primaryPath;
            if (!sameType && !samePath)
                break;
            hashes.append(next.hash);
            agg.additions += next.additions;
            agg.deletions += next.deletions;
            agg.files.append(next.files);
            if (agg.body.isEmpty())
                agg.body = next.body;
            ++j;
        }
        if (hashes.size() > 1) {
            kind = QStringLiteral("cluster");
            const QString label = !agg.type.isEmpty()
                                      ? agg.type
                                      : (!agg.primaryPath.isEmpty() ? agg.primaryPath
                                                                    : QStringLiteral("changes"));
            title = QStringLiteral("%1 commits · %2").arg(hashes.size()).arg(label);
        }
        frames.append(commitToMap(agg, kind, hashes, title));
        i = j;
    }
    return frames;
}

void StoryboardEngine::deriveDraft(const QVariantList& frames)
{
    if (frames.isEmpty()) {
        m_prTitle.clear();
        m_prBody.clear();
        emit prDraftChanged();
        return;
    }

    // Title: prefer last non-merge frame subject / cluster label
    QString title;
    for (int i = frames.size() - 1; i >= 0; --i) {
        const QVariantMap f = frames.at(i).toMap();
        if (f.value(QStringLiteral("kind")).toString() == QLatin1String("merge"))
            continue;
        title = f.value(QStringLiteral("title")).toString();
        break;
    }
    if (title.isEmpty())
        title = frames.last().toMap().value(QStringLiteral("title")).toString();
    if (frames.size() > 1 && !title.contains(QStringLiteral(" · ")))
        title = QStringLiteral("%1 (%2 frames)").arg(title).arg(frames.size());

    QStringList bodyParts;
    const QString headLabel = m_headBranch.isEmpty() ? m_headRef.left(7) : m_headBranch;
    bodyParts << QStringLiteral("## Summary")
              << QStringLiteral("")
              << QStringLiteral("Storyboard of `%1` → `%2` (%3 frames).")
                     .arg(m_baseRef, headLabel)
                     .arg(frames.size())
              << QStringLiteral("");
    for (int i = 0; i < frames.size(); ++i) {
        const QVariantMap f = frames.at(i).toMap();
        bodyParts << QStringLiteral("### %1. %2")
                         .arg(i + 1, 2, 10, QLatin1Char('0'))
                         .arg(f.value(QStringLiteral("title")).toString());
        const QString frameBody = f.value(QStringLiteral("body")).toString().trimmed();
        if (!frameBody.isEmpty())
            bodyParts << frameBody << QStringLiteral("");
        bodyParts << QStringLiteral("-%1 / +%2 · `%3`")
                         .arg(f.value(QStringLiteral("deletions")).toInt())
                         .arg(f.value(QStringLiteral("additions")).toInt())
                         .arg(f.value(QStringLiteral("primaryPath")).toString());
        const QVariantList files = f.value(QStringLiteral("files")).toList();
        const int limit = qMin(8, files.size());
        for (int fi = 0; fi < limit; ++fi) {
            const QVariantMap file = files.at(fi).toMap();
            bodyParts << QStringLiteral("- `%1` (+%2 / -%3)")
                             .arg(file.value(QStringLiteral("path")).toString())
                             .arg(file.value(QStringLiteral("add")).toInt())
                             .arg(file.value(QStringLiteral("del")).toInt());
        }
        if (files.size() > limit)
            bodyParts << QStringLiteral("- … %1 more files").arg(files.size() - limit);
        bodyParts << QStringLiteral("");
    }

    m_prTitle = title;
    m_prBody = bodyParts.join(QLatin1Char('\n'));
    emit prDraftChanged();
}

void StoryboardEngine::refreshExistingPr()
{
    m_existingPrUrl.clear();
    emit existingPrUrlChanged();
    if (!m_ghAvailable || m_repoPath.isEmpty())
        return;
    int code = 1;
    const QString out = runGh(
        { QStringLiteral("pr"), QStringLiteral("view"),
          QStringLiteral("--json"), QStringLiteral("url,title") },
        &code);
    if (code != 0)
        return;
    // Minimal parse: "url":"https://..."
    static const QRegularExpression urlRe(QStringLiteral("\"url\"\\s*:\\s*\"([^\"]+)\""));
    const auto m = urlRe.match(out);
    if (m.hasMatch()) {
        m_existingPrUrl = m.captured(1);
        emit existingPrUrlChanged();
    }
}

void StoryboardEngine::refresh()
{
    detectTools();
    m_frames.clear();
    m_commitCount = 0;
    m_hiddenCommitCount = 0;
    m_selectedIndex = -1;
    emit framesChanged();
    emit selectedIndexChanged();

    if (m_repoPath.isEmpty()) {
        setStatus(QStringLiteral("Open a repository to build a storyboard."));
        return;
    }
    if (!m_gitAvailable) {
        setStatus(QStringLiteral("git was not found on PATH."));
        return;
    }

    setBusy(true);
    setStatus(QStringLiteral("Loading storyboard…"));

    int code = 1;
    m_headRef = trimNl(runGit({ QStringLiteral("rev-parse"), QStringLiteral("HEAD") }, &code));
    emit headRefChanged();
    m_headBranch = trimNl(runGit(
        { QStringLiteral("rev-parse"), QStringLiteral("--abbrev-ref"), QStringLiteral("HEAD") },
        &code));
    emit headBranchChanged();

    refreshBaseCandidates();
    const QString base = resolveBaseRef();
    if (base.isEmpty()) {
        setBusy(false);
        setStatus(QStringLiteral("Could not resolve a base branch (tried origin/main, master, upstream)."));
        return;
    }
    if (m_baseRef != base) {
        m_baseRef = base;
        emit baseRefChanged();
    }

    QList<CommitInfo> commits = loadCommits(base);
    m_commitCount = commits.size();
    if (commits.size() > kMaxCommits) {
        m_hiddenCommitCount = commits.size() - kMaxCommits;
        commits = commits.mid(commits.size() - kMaxCommits);
    }

    m_frames = buildFrames(commits);
    emit framesChanged();
    setSelectedIndex(m_frames.isEmpty() ? -1 : 0);
    deriveDraft(m_frames);
    refreshExistingPr();

    setBusy(false);
    if (m_frames.isEmpty()) {
        setStatus(QStringLiteral("No commits ahead of %1.").arg(m_baseRef));
    } else {
        QString msg = QStringLiteral("%1 frame(s) · %2 commit(s) ahead of %3")
                          .arg(m_frames.size())
                          .arg(m_commitCount)
                          .arg(m_baseRef);
        if (m_hiddenCommitCount > 0)
            msg += QStringLiteral(" · showing last %1").arg(kMaxCommits);
        setStatus(msg);
    }
}

void StoryboardEngine::selectNext()
{
    setSelectedIndex(m_selectedIndex + 1);
}

void StoryboardEngine::selectPrev()
{
    setSelectedIndex(m_selectedIndex - 1);
}

void StoryboardEngine::regenerateDraft()
{
    deriveDraft(m_frames);
    emit actionFinished(true, QStringLiteral("Draft regenerated from frames."));
}

void StoryboardEngine::copyDraft()
{
    if (auto* clip = QGuiApplication::clipboard()) {
        clip->setText(m_prTitle + QLatin1String("\n\n") + m_prBody);
        emit actionFinished(true, QStringLiteral("PR title and body copied."));
    }
}

void StoryboardEngine::createPullRequest()
{
    if (!m_ghAvailable) {
        emit actionFinished(false, QStringLiteral("gh CLI not available. Install and run gh auth login."));
        return;
    }
    if (m_prTitle.trimmed().isEmpty()) {
        emit actionFinished(false, QStringLiteral("PR title is empty."));
        return;
    }
    setBusy(true);
    setStatus(QStringLiteral("Creating pull request…"));
    int code = 1;
    QStringList args{
        QStringLiteral("pr"), QStringLiteral("create"),
        QStringLiteral("--title"), m_prTitle,
        QStringLiteral("--body"), m_prBody,
    };
    if (!m_baseRef.isEmpty()) {
        // Prefer short base name for --base (origin/main → main)
        QString base = m_baseRef;
        if (base.startsWith(QLatin1String("origin/")))
            base = base.mid(7);
        args << QStringLiteral("--base") << base;
    }
    const QString out = runGh(args, &code);
    setBusy(false);
    if (code != 0) {
        setStatus(QStringLiteral("Create PR failed."));
        emit actionFinished(false, trimNl(out).isEmpty()
                                       ? QStringLiteral("gh pr create failed.")
                                       : trimNl(out));
        return;
    }
    static const QRegularExpression urlRe(QStringLiteral(R"(https://github\.com/[^\s]+)"));
    const auto m = urlRe.match(out);
    if (m.hasMatch()) {
        m_existingPrUrl = m.captured(0);
        emit existingPrUrlChanged();
    } else {
        refreshExistingPr();
    }
    setStatus(QStringLiteral("Pull request created."));
    emit actionFinished(true, m_existingPrUrl.isEmpty()
                                  ? QStringLiteral("Pull request created.")
                                  : QStringLiteral("Pull request created: %1").arg(m_existingPrUrl));
}

void StoryboardEngine::openExistingPr()
{
    if (m_existingPrUrl.isEmpty())
        return;
    QDesktopServices::openUrl(QUrl(m_existingPrUrl));
}

QString StoryboardEngine::frameDiffCommand(int index) const
{
    if (index < 0 || index >= m_frames.size())
        return {};
    const QVariantMap f = m_frames.at(index).toMap();
    const QStringList hashes = f.value(QStringLiteral("hashes")).toStringList();
    if (hashes.isEmpty())
        return {};
    if (hashes.size() == 1)
        return QStringLiteral("git show %1").arg(hashes.first());
    return QStringLiteral("git diff %1^..%2").arg(hashes.first(), hashes.last());
}

void StoryboardEngine::copyFrameDiffCommand(int index)
{
    const QString cmd = frameDiffCommand(index);
    if (cmd.isEmpty())
        return;
    if (auto* clip = QGuiApplication::clipboard()) {
        clip->setText(cmd);
        emit actionFinished(true, QStringLiteral("Diff command copied."));
    }
}

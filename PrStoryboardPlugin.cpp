#include "PrStoryboardPlugin.h"
#include "StoryboardEngine.h"

#include <QtQml>

QString PrStoryboardPlugin::id() const
{
    return QStringLiteral("com.gitease.pr-storyboard");
}

QString PrStoryboardPlugin::name() const
{
    return QStringLiteral("PR Storyboard");
}

QString PrStoryboardPlugin::version() const
{
    return QStringLiteral("0.1.0");
}

void PrStoryboardPlugin::initialize(IPluginContext* ctx)
{
    m_ctx = ctx;
    qmlRegisterType<StoryboardEngine>("GitEasePrStoryboard", 1, 0, "StoryboardEngine");
    ctx->registerPage(this);
    ctx->registerContextMenu(this);
}

void PrStoryboardPlugin::shutdown()
{
    m_ctx = nullptr;
}

QString PrStoryboardPlugin::pageId() const
{
    return QStringLiteral("pr-storyboard");
}

QString PrStoryboardPlugin::pageTitle() const
{
    return QStringLiteral("Storyboard");
}

QString PrStoryboardPlugin::pageIcon() const
{
    return QStringLiteral("\uf008"); // fa-film
}

QUrl PrStoryboardPlugin::pageQmlUrl() const
{
    return QUrl(QStringLiteral("qrc:/com.gitease.pr-storyboard/qml/StoryboardPage.qml"));
}

int PrStoryboardPlugin::pageOrder() const
{
    return 60;
}

void PrStoryboardPlugin::repositoryChanged(const QString& repoPath)
{
    if (m_repoPath == repoPath)
        return;
    m_repoPath = repoPath;
    emit activeRepositoryChanged(repoPath);
}

QList<IContextMenuPlugin::MenuTarget> PrStoryboardPlugin::targets() const
{
    return { MenuTarget::Branch };
}

QList<IContextMenuPlugin::MenuItem> PrStoryboardPlugin::menuItems(MenuTarget target,
                                                                  const QVariantMap& /*ctx*/) const
{
    if (target != MenuTarget::Branch)
        return {};
    return {
        { QStringLiteral("open-storyboard"),
          QStringLiteral("Open Storyboard"),
          QStringLiteral("\uf008"),
          false,
          40 },
    };
}

void PrStoryboardPlugin::executeAction(const QString& itemId,
                                       MenuTarget target,
                                       const QVariantMap& ctx)
{
    if (itemId != QStringLiteral("open-storyboard") || target != MenuTarget::Branch)
        return;
    const QString branch = ctx.value(QStringLiteral("branch")).toString();
    if (m_ctx) {
        m_ctx->setSetting(id(), QStringLiteral("pendingBranch"), branch);
        m_ctx->publish(QStringLiteral("pr-storyboard.open"),
                       QVariantMap{ { QStringLiteral("branch"), branch },
                                    { QStringLiteral("repoPath"),
                                      ctx.value(QStringLiteral("repoPath")).toString() } });
        m_ctx->notify(QStringLiteral("Open Storyboard from the navigation rail to review “%1”.")
                          .arg(branch.isEmpty() ? QStringLiteral("this branch") : branch),
                      QStringLiteral("info"));
    }
    emit openStoryboardRequested(branch);
}

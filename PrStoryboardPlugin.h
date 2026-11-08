#pragma once

#include <QObject>
#include "IPagePlugin.h"
#include "IRepositoryAwarePlugin.h"
#include "IContextMenuPlugin.h"

class PrStoryboardPlugin : public QObject,
                           public IPagePlugin,
                           public IRepositoryAwarePlugin,
                           public IContextMenuPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.gitease.IPagePlugin/1.0" FILE "plugin.json")
    Q_INTERFACES(IPlugin IPagePlugin IRepositoryAwarePlugin IContextMenuPlugin)

public:
    QString id()      const override;
    QString name()    const override;
    QString version() const override;

    void initialize(IPluginContext* ctx) override;
    void shutdown() override;

    QString pageId()     const override;
    QString pageTitle()  const override;
    QString pageIcon()   const override;
    QUrl    pageQmlUrl() const override;
    int     pageOrder()  const override;

    void repositoryChanged(const QString& repoPath) override;

    QList<MenuTarget> targets() const override;
    QList<MenuItem> menuItems(MenuTarget target, const QVariantMap& ctx) const override;
    void executeAction(const QString& itemId, MenuTarget target, const QVariantMap& ctx) override;

signals:
    void activeRepositoryChanged(const QString& repoPath);
    void openStoryboardRequested(const QString& branch);

private:
    IPluginContext* m_ctx = nullptr;
    QString m_repoPath;
};

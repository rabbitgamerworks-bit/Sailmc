#pragma once

#include <QAbstractListModel>

#include "news/NewsChecker.h"

/*
 * News feed shown on the home page (MojoLauncher shows news next to the play button too).
 */
class NewsModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(QString error READ error NOTIFY loadingChanged)
    Q_PROPERTY(int count READ rowCount NOTIFY loadingChanged)
public:
    enum Roles {
        TitleRole = Qt::UserRole + 1,
        SummaryRole,
        ContentRole,
        LinkRole,
        AuthorRole,
        DateRole
    };

    explicit NewsModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    bool loading() const { return m_loading; }
    QString error() const { return m_error; }

    Q_INVOKABLE void reload();

signals:
    void loadingChanged();

private:
    NewsChecker *m_checker = nullptr;
    QList<NewsEntryPtr> m_entries;
    bool m_loading = false;
    QString m_error;
};

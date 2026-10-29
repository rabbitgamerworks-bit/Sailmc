#include "NewsModel.h"

#include <QRegExp>
#include <QTextDocument>

#include "Application.h"
#include "BuildConfig.h"

NewsModel::NewsModel(QObject *parent) : QAbstractListModel(parent)
{
    m_checker = new NewsChecker(APPLICATION->network(), BuildConfig.NEWS_RSS_URL);
    m_checker->setParent(this);
    connect(m_checker, &NewsChecker::newsLoaded, this, [this]() {
        beginResetModel();
        m_entries = m_checker->getNewsEntries();
        endResetModel();
        m_loading = false;
        m_error.clear();
        emit loadingChanged();
    });
    connect(m_checker, &NewsChecker::newsLoadingFailed, this, [this](QString error) {
        m_loading = false;
        m_error = error;
        emit loadingChanged();
    });
}

void NewsModel::reload()
{
    m_loading = true;
    emit loadingChanged();
    m_checker->reloadNews();
}

int NewsModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_entries.size();
}

QHash<int, QByteArray> NewsModel::roleNames() const
{
    return {
        {TitleRole, "title"},
        {SummaryRole, "summary"},
        {ContentRole, "content"},
        {LinkRole, "link"},
        {AuthorRole, "author"},
        {DateRole, "date"},
    };
}

QVariant NewsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_entries.size()) {
        return QVariant();
    }
    const auto &entry = m_entries.at(index.row());
    switch (role) {
    case Qt::DisplayRole:
    case TitleRole:
        return entry->title;
    case SummaryRole: {
        QTextDocument doc;
        doc.setHtml(entry->content);
        return doc.toPlainText().simplified().left(280);
    }
    case ContentRole:
        return entry->content;
    case LinkRole:
        return entry->link;
    case AuthorRole:
        return entry->author;
    case DateRole:
        return entry->pubDate;
    default:
        return QVariant();
    }
}

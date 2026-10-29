#pragma once

#include <QIdentityProxyModel>

#include "launch/LogModel.h"

/*
 * Gives the launcher's game log model role names usable from QML.
 */
class LogProxyModel : public QIdentityProxyModel
{
    Q_OBJECT
public:
    using QIdentityProxyModel::QIdentityProxyModel;

    QHash<int, QByteArray> roleNames() const override
    {
        return {
            {Qt::DisplayRole, "line"},
            {LogModel::LevelRole, "level"},
        };
    }

    QVariant data(const QModelIndex &index, int role) const override
    {
        if (role == LogModel::LevelRole) {
            // MessageLevel::Enum as a plain int for QML
            return QIdentityProxyModel::data(index, role).toInt();
        }
        return QIdentityProxyModel::data(index, role);
    }
};

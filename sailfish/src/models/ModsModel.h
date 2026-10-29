#pragma once

#include <QAbstractListModel>
#include <memory>

class ModFolderModel;

/*
 * Mods (or resource packs, ...) of one instance, backed by the launcher's ModFolderModel.
 */
class ModsModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString instanceId READ instanceId WRITE setInstanceId NOTIFY instanceIdChanged)
    Q_PROPERTY(QString kind READ kind WRITE setKind NOTIFY instanceIdChanged)
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(QString folder READ folder NOTIFY instanceIdChanged)
public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        VersionRole,
        EnabledRole,
        FileNameRole,
        DescriptionRole
    };

    explicit ModsModel(QObject *parent = nullptr);
    ~ModsModel();

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString instanceId() const { return m_instanceId; }
    void setInstanceId(const QString &id);
    // "mods", "resourcepacks" or "coremods"
    QString kind() const { return m_kind; }
    void setKind(const QString &kind);
    QString folder() const;

    Q_INVOKABLE void setEnabled(int row, bool enabled);
    Q_INVOKABLE void remove(int row);
    Q_INVOKABLE void refresh();

signals:
    void instanceIdChanged();
    void countChanged();

private:
    void attach();

    QString m_instanceId;
    QString m_kind = "mods";
    std::shared_ptr<ModFolderModel> m_source;
};

#pragma once

#include <QObject>
#include <QPointer>
#include <QTimer>
#include <QVariant>
#include <functional>

#include "BaseInstance.h"
#include "QObjectPtr.h"
#include "tasks/Task.h"
#include "minecraft/auth/MinecraftAccount.h"
#include "minecraft/auth/AuthSession.h"
#include "minecraft/auth/AccountTask.h"
#include "launch/LaunchTask.h"

class Task;
class LogProxyModel;
class JavaRuntimeManager;

/*
 * The bridge between the Silica UI and the MultiMC launcher core.
 * It replaces MainWindow/LaunchController/AccountsDialog of the QtWidgets frontend:
 * every place where those showed a modal dialog, this object emits a signal or exposes state.
 */
class SailLauncher : public QObject
{
    Q_OBJECT
    // selection
    Q_PROPERTY(QString selectedInstanceId READ selectedInstanceId WRITE setSelectedInstanceId NOTIFY selectedChanged)
    Q_PROPERTY(QString selectedName READ selectedName NOTIFY selectedChanged)
    Q_PROPERTY(QString selectedIconKey READ selectedIconKey NOTIFY selectedChanged)
    Q_PROPERTY(QString selectedGameVersion READ selectedGameVersion NOTIFY selectedChanged)
    Q_PROPERTY(QString selectedLoader READ selectedLoader NOTIFY selectedChanged)
    // background work (instance creation, downloads, launch preparation)
    Q_PROPERTY(bool busy READ busy NOTIFY taskChanged)
    Q_PROPERTY(QString taskTitle READ taskTitle NOTIFY taskChanged)
    Q_PROPERTY(QString taskStatus READ taskStatus NOTIFY taskChanged)
    Q_PROPERTY(qreal taskProgress READ taskProgress NOTIFY taskChanged)
    // the game
    Q_PROPERTY(bool gameRunning READ gameRunning NOTIFY gameChanged)
    Q_PROPERTY(QString runningInstanceId READ runningInstanceId NOTIFY gameChanged)
    Q_PROPERTY(QObject *log READ log NOTIFY logChanged)
    // Microsoft login (device code flow)
    Q_PROPERTY(bool loginActive READ loginActive NOTIFY loginChanged)
    Q_PROPERTY(QString loginCode READ loginCode NOTIFY loginChanged)
    Q_PROPERTY(QString loginUrl READ loginUrl NOTIFY loginChanged)
    Q_PROPERTY(QString loginStatus READ loginStatus NOTIFY loginChanged)
    Q_PROPERTY(int loginSecondsLeft READ loginSecondsLeft NOTIFY loginTick)
    // misc
    Q_PROPERTY(QString dataPath READ dataPath CONSTANT)
    Q_PROPERTY(QString version READ version CONSTANT)
    Q_PROPERTY(QString lastError READ lastError NOTIFY errorOccurred)

public:
    explicit SailLauncher(JavaRuntimeManager *java, QObject *parent = nullptr);
    ~SailLauncher();

    QString selectedInstanceId() const { return m_selectedId; }
    void setSelectedInstanceId(const QString &id);
    QString selectedName() const;
    QString selectedIconKey() const;
    QString selectedGameVersion() const;
    QString selectedLoader() const;

    bool busy() const { return m_task != nullptr; }
    QString taskTitle() const { return m_taskTitle; }
    QString taskStatus() const { return m_taskStatus; }
    qreal taskProgress() const { return m_taskProgress; }

    bool gameRunning() const;
    QString runningInstanceId() const { return m_runningId; }
    QObject *log() const;

    bool loginActive() const { return m_loginTask != nullptr; }
    QString loginCode() const { return m_loginCode; }
    QString loginUrl() const { return m_loginUrl; }
    QString loginStatus() const { return m_loginStatus; }
    int loginSecondsLeft() const { return m_loginSecondsLeft; }

    QString dataPath() const;
    QString version() const;
    QString lastError() const { return m_lastError; }

    // instances
    Q_INVOKABLE void createInstance(const QString &name, const QString &minecraftVersion,
                                    const QString &loaderUid, const QString &loaderVersion);
    Q_INVOKABLE void updateInstance(const QString &id);
    Q_INVOKABLE bool renameInstance(const QString &id, const QString &name);
    Q_INVOKABLE void deleteInstance(const QString &id);
    Q_INVOKABLE QString instanceFolder(const QString &id) const;
    Q_INVOKABLE QVariantMap instanceInfo(const QString &id) const;
    Q_INVOKABLE void setInstanceIcon(const QString &id, const QString &iconKey);
    Q_INVOKABLE QStringList iconKeys() const;

    // launching
    Q_INVOKABLE void launch(const QString &id, bool offline = false, const QString &offlineName = QString(),
                            const QString &server = QString());
    Q_INVOKABLE void kill();
    Q_INVOKABLE void cancelTask();
    Q_INVOKABLE QString logText() const;

    // accounts
    Q_INVOKABLE void startLogin();
    Q_INVOKABLE void cancelLogin();

    // settings: global and per instance (instance values are only used when the override group is on)
    Q_INVOKABLE QVariant setting(const QString &key) const;
    Q_INVOKABLE void setSetting(const QString &key, const QVariant &value);
    Q_INVOKABLE QVariant instanceSetting(const QString &id, const QString &key) const;
    Q_INVOKABLE void setInstanceSetting(const QString &id, const QString &key, const QVariant &value);
    Q_INVOKABLE void applyProxySettings();
    Q_INVOKABLE int systemMemoryMiB() const;

signals:
    void selectedChanged();
    void taskChanged();
    void gameChanged();
    void logChanged();
    void loginChanged();
    void loginTick();
    void loginSucceeded(const QString &profileName);
    void instanceCreated(const QString &id);
    void taskSucceeded(const QString &title);
    void errorOccurred(const QString &message);
    // there is no usable account - the UI should offer the account page
    void accountRequired(const QString &message);
    // the account does not own the game; the UI can ask and call launch(id, true, name) for the demo/offline mode
    void offlineNameRequired(const QString &instanceId, const QString &suggestedName);
    void javaRequired(int majorVersion);

private:
    void runTask(Task *task, const QString &title, std::function<void()> onSuccess = nullptr);
    void finishTask();
    void fail(const QString &message);
    void proceedLogin(InstancePtr instance, MinecraftAccountPtr account, bool offline, const QString &offlineName,
                      const QString &server);
    void startGame(InstancePtr instance, AuthSessionPtr session, const QString &server);
    bool ensureJava(InstancePtr instance);
    void stopLogin();

    JavaRuntimeManager *m_java;
    QString m_selectedId;

    QPointer<Task> m_task;
    std::unique_ptr<Task> m_ownedTask;
    Task::Ptr m_sharedTask;
    QString m_taskTitle;
    QString m_taskStatus;
    qreal m_taskProgress = -1;

    shared_qobject_ptr<LaunchTask> m_launchTask;
    QString m_runningId;
    LogProxyModel *m_log = nullptr;

    MinecraftAccountPtr m_loginAccount;
    shared_qobject_ptr<AccountTask> m_loginTask;
    QString m_loginCode;
    QString m_loginUrl;
    QString m_loginStatus;
    int m_loginSecondsLeft = 0;
    QTimer m_loginTimer;

    QString m_lastError;
};

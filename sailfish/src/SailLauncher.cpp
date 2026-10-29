#include "SailLauncher.h"

#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <quazip.h>
#include <quazipfile.h>
#include <QRegExp>
#include <QUrlQuery>
#include "FileSystem.h"

#include "Application.h"
#include "BuildConfig.h"
#include "InstanceCreationTask.h"
#include "InstanceImportTask.h"
#include "InstanceList.h"
#include "icons/IconList.h"
#include "launch/LaunchTask.h"
#include "launch/steps/TextPrint.h"
#include "meta/Index.h"
#include "meta/Version.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "minecraft/auth/AccountList.h"
#include "minecraft/auth/AccountTask.h"
#include "minecraft/launch/QuickPlayTarget.h"
#include "settings/SettingsObject.h"

#include "JavaRuntimeManager.h"
#include "RendererConfig.h"
#include "YggdrasilTasks.h"
#include "models/InstanceModel.h"
#include "models/LogProxyModel.h"

SailLauncher::SailLauncher(JavaRuntimeManager *java, QObject *parent) : QObject(parent), m_java(java)
{
    m_selectedId = APPLICATION->settings()->get("SelectedInstance").toString();
    if (!APPLICATION->instances()->getInstanceById(m_selectedId) && APPLICATION->instances()->count() > 0) {
        m_selectedId = APPLICATION->instances()->at(0)->id();
    }
    connect(APPLICATION->instances().get(), &InstanceList::instancesChanged, this, &SailLauncher::selectedChanged);
    connect(APPLICATION->instances().get(), &QAbstractItemModel::dataChanged, this, &SailLauncher::selectedChanged);

    m_log = new LogProxyModel(this);

    m_loginTimer.setInterval(1000);
    connect(&m_loginTimer, &QTimer::timeout, this, [this]() {
        if (m_loginSecondsLeft > 0) {
            m_loginSecondsLeft--;
            emit loginTick();
        }
        if (m_loginSecondsLeft == 0) {
            m_loginTimer.stop();
        }
    });
}

SailLauncher::~SailLauncher()
{
}

// ---------------------------------------------------------------- selection

void SailLauncher::setSelectedInstanceId(const QString &id)
{
    if (id == m_selectedId) {
        return;
    }
    m_selectedId = id;
    APPLICATION->settings()->set("SelectedInstance", id);
    emit selectedChanged();
}

QString SailLauncher::selectedName() const
{
    auto inst = APPLICATION->instances()->getInstanceById(m_selectedId);
    return inst ? inst->name() : QString();
}

QString SailLauncher::selectedIconKey() const
{
    auto inst = APPLICATION->instances()->getInstanceById(m_selectedId);
    return inst ? inst->iconKey() : QString();
}

QString SailLauncher::selectedGameVersion() const
{
    return InstanceModel::gameVersionOf(APPLICATION->instances()->getInstanceById(m_selectedId));
}

QString SailLauncher::selectedLoader() const
{
    return InstanceModel::loaderOf(APPLICATION->instances()->getInstanceById(m_selectedId));
}

QString SailLauncher::dataPath() const
{
    return APPLICATION->dataPath();
}

QString SailLauncher::version() const
{
    return BuildConfig.printableVersionString();
}

QObject *SailLauncher::log() const
{
    return m_log;
}

bool SailLauncher::gameRunning() const
{
    return !m_runningId.isEmpty();
}

// ---------------------------------------------------------------- tasks

void SailLauncher::fail(const QString &message)
{
    qWarning() << "SailMC error:" << message;
    m_lastError = message;
    emit errorOccurred(message);
}

void SailLauncher::runTask(Task *task, const QString &title, std::function<void()> onSuccess)
{
    m_task = task;
    m_taskTitle = title;
    m_taskStatus = task->getStatus();
    m_taskProgress = -1;
    connect(task, &Task::status, this, [this](QString status) {
        m_taskStatus = status;
        emit taskChanged();
    });
    connect(task, &Task::progress, this, [this](qint64 current, qint64 total) {
        m_taskProgress = total > 0 ? qreal(current) / total : -1;
        emit taskChanged();
    });
    connect(task, &Task::succeeded, this, [this, title, onSuccess]() {
        if (onSuccess) {
            onSuccess();
        }
        emit taskSucceeded(title);
    });
    connect(task, &Task::failed, this, [this, title](QString reason) {
        fail(tr("%1 failed: %2").arg(title, reason));
    });
    connect(task, &Task::finished, this, &SailLauncher::finishTask, Qt::QueuedConnection);
    emit taskChanged();
    task->start();
}

void SailLauncher::finishTask()
{
    m_task.clear();
    m_ownedTask.reset();
    m_sharedTask.reset();
    m_taskTitle.clear();
    m_taskStatus.clear();
    m_taskProgress = -1;
    emit taskChanged();
}

void SailLauncher::cancelTask()
{
    if (m_task && m_task->canAbort()) {
        m_task->abort();
    }
}

// ---------------------------------------------------------------- instances

void SailLauncher::createInstance(const QString &name, const QString &minecraftVersion,
                                  const QString &loaderUid, const QString &loaderVersion)
{
    if (busy()) {
        fail(tr("Please wait for the current operation to finish"));
        return;
    }
    auto version = APPLICATION->metadataIndex()->get("net.minecraft", minecraftVersion);
    if (!version) {
        fail(tr("Unknown Minecraft version %1").arg(minecraftVersion));
        return;
    }
    auto creation = new InstanceCreationTask(version);
    creation->setName(name.trimmed().isEmpty() ? minecraftVersion : name.trimmed());
    runInstanceTask(creation, tr("Creating instance"), [loaderUid, loaderVersion](InstancePtr instance) {
        auto profile = InstanceModel::loadedProfile(instance);
        if (profile && !loaderUid.isEmpty() && !loaderVersion.isEmpty()) {
            profile->setComponentVersion(loaderUid, loaderVersion, true);
            profile->saveNow();
        }
    });
}

void SailLauncher::importInstance(const QString &path, const QString &name, const QString &minecraftVersion)
{
    if (busy()) {
        fail(tr("Please wait for the current operation to finish"));
        return;
    }
    const QString file = QUrl(path).isLocalFile() ? QUrl(path).toLocalFile() : path;
    if (!QFileInfo(file).isFile()) {
        fail(tr("File not found: %1").arg(file));
        return;
    }
    const QString baseName = QFileInfo(file).completeBaseName();

    if (file.endsWith(".zip", Qt::CaseInsensitive)) {
        auto import = new InstanceImportTask(QUrl::fromLocalFile(file));
        import->setName(name.trimmed().isEmpty() ? baseName : name.trimmed());
        runInstanceTask(import, tr("Importing instance"));
        return;
    }
    if (!file.endsWith(".jar", Qt::CaseInsensitive)) {
        fail(tr("Choose a Minecraft .jar or an exported instance .zip"));
        return;
    }
    // a client jar: a normal instance of that version, with the jar from the device instead of the
    // downloaded one (MultiMC's "Replace Minecraft.jar")
    auto version = APPLICATION->metadataIndex()->get("net.minecraft", minecraftVersion);
    if (minecraftVersion.isEmpty() || !version) {
        fail(tr("Choose the Minecraft version of this jar"));
        return;
    }
    auto creation = new InstanceCreationTask(version);
    creation->setName(name.trimmed().isEmpty() ? minecraftVersion : name.trimmed());
    runInstanceTask(creation, tr("Adding instance"), [file](InstancePtr instance) {
        auto profile = InstanceModel::loadedProfile(instance);
        if (profile) {
            profile->installCustomJar(file);
            // add the dependencies (LWJGL) now, like the first online update of a new instance does
            profile->resolve(Net::Mode::Online);
            profile->saveNow();
        }
    });
}

QString SailLauncher::jarMinecraftVersion(const QString &path) const
{
    const QString file = QUrl(path).isLocalFile() ? QUrl(path).toLocalFile() : path;
    QuaZip zip(file);
    if (!zip.open(QuaZip::mdUnzip) || !zip.setCurrentFile("version.json")) {
        return QString();
    }
    QuaZipFile entry(&zip);
    if (!entry.open(QIODevice::ReadOnly)) {
        return QString();
    }
    return QJsonDocument::fromJson(entry.readAll()).object().value("id").toString();
}

void SailLauncher::runInstanceTask(InstanceTask *task, const QString &title,
                                   std::function<void(InstancePtr)> onCreated)
{
    task->setIcon("default");
    task->setGroup(QString());

    // the staged task reports the new instance id through the instance list
    QStringList before;
    for (int i = 0; i < APPLICATION->instances()->count(); i++) {
        before << APPLICATION->instances()->at(i)->id();
    }

    m_ownedTask.reset(APPLICATION->instances()->wrapInstanceTask(task));
    runTask(m_ownedTask.get(), title, [this, before, onCreated]() {
        QString newId;
        for (int i = 0; i < APPLICATION->instances()->count(); i++) {
            auto id = APPLICATION->instances()->at(i)->id();
            if (!before.contains(id)) {
                newId = id;
            }
        }
        if (newId.isEmpty()) {
            return;
        }
        if (onCreated) {
            onCreated(APPLICATION->instances()->getInstanceById(newId));
        }
        setSelectedInstanceId(newId);
        emit instanceCreated(newId);
    });
}

void SailLauncher::updateInstance(const QString &id)
{
    if (busy()) {
        fail(tr("Please wait for the current operation to finish"));
        return;
    }
    auto instance = APPLICATION->instances()->getInstanceById(id);
    if (!instance) {
        return;
    }
    m_sharedTask = instance->createUpdateTask(Net::Mode::Online);
    if (!m_sharedTask) {
        fail(tr("This instance can't be updated"));
        return;
    }
    runTask(m_sharedTask.get(), tr("Downloading game files"));
}

bool SailLauncher::renameInstance(const QString &id, const QString &name)
{
    auto instance = APPLICATION->instances()->getInstanceById(id);
    if (!instance || name.trimmed().isEmpty()) {
        return false;
    }
    instance->setName(name.trimmed());
    emit selectedChanged();
    return true;
}

void SailLauncher::deleteInstance(const QString &id)
{
    auto instance = APPLICATION->instances()->getInstanceById(id);
    if (!instance || instance->isRunning()) {
        return;
    }
    APPLICATION->instances()->deleteInstance(id);
    if (id == m_selectedId) {
        setSelectedInstanceId(APPLICATION->instances()->count() > 0 ? APPLICATION->instances()->at(0)->id() : QString());
    }
}

QString SailLauncher::instanceFolder(const QString &id) const
{
    auto instance = APPLICATION->instances()->getInstanceById(id);
    return instance ? instance->gameRoot() : QString();
}

QVariantMap SailLauncher::instanceInfo(const QString &id) const
{
    QVariantMap info;
    auto instance = APPLICATION->instances()->getInstanceById(id);
    if (!instance) {
        return info;
    }
    info["id"] = instance->id();
    info["name"] = instance->name();
    info["iconKey"] = instance->iconKey();
    info["gameVersion"] = InstanceModel::gameVersionOf(instance);
    info["loader"] = InstanceModel::loaderOf(instance);
    info["playTime"] = InstanceModel::formatPlayTime(instance->totalTimePlayed());
    info["lastLaunch"] = instance->lastLaunch() > 0 ? QDateTime::fromMSecsSinceEpoch(instance->lastLaunch()) : QVariant();
    info["folder"] = instance->gameRoot();
    info["running"] = instance->isRunning();
    info["notes"] = instance->notes();
    info["recommendedJava"] = JavaRuntimeManager::recommendedMajorFor(InstanceModel::gameVersionOf(instance));
    return info;
}

void SailLauncher::setInstanceIcon(const QString &id, const QString &iconKey)
{
    auto instance = APPLICATION->instances()->getInstanceById(id);
    if (instance) {
        instance->setIconKey(iconKey);
        emit selectedChanged();
    }
}

QStringList SailLauncher::iconKeys() const
{
    QStringList keys;
    auto icons = APPLICATION->icons();
    for (int i = 0; i < icons->rowCount(QModelIndex()); i++) {
        keys << icons->data(icons->index(i), Qt::UserRole).toString();
    }
    return keys;
}

// ---------------------------------------------------------------- launching

bool SailLauncher::ensureJava(InstancePtr instance)
{
    auto settings = instance->settings();
    QString javaPath = settings->get("JavaPath").toString();
    if (!javaPath.isEmpty() && QFile::exists(javaPath)) {
        return true;
    }
    // pick an installed runtime that fits the game version
    const int major = JavaRuntimeManager::recommendedMajorFor(InstanceModel::gameVersionOf(instance));
    m_java->refresh();
    const QString best = m_java->bestFor(major);
    if (best.isEmpty()) {
        emit javaRequired(major);
        fail(tr("Java %1 is needed to play this version. Install it in Settings → Java runtimes.").arg(major));
        return false;
    }
    if (settings->get("OverrideJavaLocation").toBool() || settings->get("OverrideJava").toBool()) {
        settings->set("JavaPath", best);
    } else {
        APPLICATION->settings()->set("JavaPath", best);
    }
    return true;
}

void SailLauncher::launch(const QString &id, bool offline, const QString &offlineName, const QString &server)
{
    if (busy() || m_launchTask) {
        fail(tr("Please wait for the current operation to finish"));
        return;
    }
    auto instance = APPLICATION->instances()->getInstanceById(id);
    if (!instance) {
        fail(tr("Select an instance first"));
        return;
    }
    if (!instance->canLaunch()) {
        fail(tr("This instance can't be launched right now"));
        return;
    }
    auto account = APPLICATION->accounts()->defaultAccount();
    for (int i = 0; !account && i < APPLICATION->accounts()->count(); i++) {
        const auto &entry = APPLICATION->accounts()->at(i);
        if (entry.isAccount) {
            account = entry.account;
        }
    }
    if (!account) {
        emit accountRequired(tr("Add a Microsoft account that owns Minecraft to play"));
        return;
    }
    // only set for Yggdrasil accounts (authlib-injector)
    qunsetenv("SAILMC_AUTH_JVM_ARGS");
    proceedLogin(instance, account, offline, offlineName, server);
}

void SailLauncher::proceedLogin(InstancePtr instance, MinecraftAccountPtr account, bool offline,
                                const QString &offlineName, const QString &server, bool refreshed)
{
    auto session = std::make_shared<AuthSession>();
    session->wants_online = !offline;
    account->fillSession(session);

    if (account->isYggdrasil()) {
        // same rule as offline accounts, checked again in case the Microsoft account was removed
        if (!APPLICATION->accounts()->anyAccountIsValid()) {
            emit accountRequired(tr("Accounts of other servers need a Microsoft account that owns Minecraft "
                                    "in the account list"));
            return;
        }
        auto task = new YggdrasilLaunchTask(account, FS::PathCombine(APPLICATION->dataPath(), "authlib-injector"));
        m_ownedTask.reset(task);
        runTask(task, tr("Signing in"), [this, task, instance, account, session, server]() {
            qputenv("SAILMC_AUTH_JVM_ARGS", task->jvmArguments().join('\n').toLocal8Bit());
            qDebug() << "authlib-injector:" << task->jvmArguments().value(0);
            session->wants_online = true;
            account->fillSession(session); // the token was refreshed
            startGame(instance, session, server);
        });
        return;
    }

    if (account->isOffline()) {
        // offline account (ported from Prism Launcher): no servers to talk to
        session->wants_online = false;
        session->MakeOffline(account->profileName());
        startGame(instance, session, server);
        return;
    }

    // asks for the player name (once per launch), returns false while waiting for it
    auto makeOffline = [&](const QString &reason) {
        QString name = offlineName;
        if (name.isEmpty()) {
            const QString last = APPLICATION->settings()->get("LastOfflinePlayerName").toString();
            emit offlineNameRequired(instance->id(), last.isEmpty() ? session->player_name : last, reason);
            return false;
        }
        APPLICATION->settings()->set("LastOfflinePlayerName", name);
        session->MakeOffline(name);
        return true;
    };

    switch (account->accountState()) {
    case AccountState::Offline:
        session->wants_online = false;
        // fallthrough
    case AccountState::Online: {
        if (!session->wants_online && !makeOffline(account->lastError())) {
            return;
        }
        if (account->ownsMinecraft()) {
            if (!account->hasProfile()) {
                emit accountRequired(tr("The account doesn't have a Minecraft profile yet. Create one on minecraft.net first."));
                return;
            }
            startGame(instance, session, server);
        } else {
            // same as the desktop launcher: accounts without the game get the demo
            session->MakeDemo();
            startGame(instance, session, server);
        }
        return;
    }
    case AccountState::Errored:
    case AccountState::Unchecked:
        // Signing in already failed during this launch (or the player picked offline mode after that):
        // Microsoft answered, but Xbox Live or the Minecraft services did not. Like MojoLauncher
        // (performAuthPing -> toLocal), fall back to offline mode - only for an account that has
        // already shown that it owns the game.
        if (refreshed || offline) {
            if (account->ownsMinecraft() && account->hasProfile()) {
                session->wants_online = false;
                if (makeOffline(account->lastError())) {
                    startGame(instance, session, server);
                }
                return;
            }
            fail(tr("Could not sign in: %1").arg(account->lastError()));
            return;
        }
        account->refresh();
        // fallthrough
    case AccountState::Working: {
        auto task = account->currentTask();
        if (!task) {
            fail(tr("Could not refresh the account"));
            return;
        }
        m_taskTitle = tr("Signing in");
        m_taskStatus = task->getStatus();
        m_taskProgress = -1;
        m_task = task.get();
        emit taskChanged();
        connect(task.get(), &Task::finished, this, [=]() {
            finishTask();
            if (account->accountState() == AccountState::Working) {
                fail(tr("Signing in did not finish"));
                return;
            }
            proceedLogin(instance, account, offline, offlineName, server, true);
        }, Qt::QueuedConnection);
        // refresh() only creates the task (the desktop UI starts it in a progress dialog); a background
        // refresh from the account list may already be running it
        if (!task->isRunning()) {
            task->start();
        }
        return;
    }
    case AccountState::Expired:
        emit accountRequired(tr("The account has expired and needs to be signed in again."));
        return;
    case AccountState::Gone:
        fail(tr("The account no longer exists on the servers."));
        return;
    case AccountState::MustMigrate:
        fail(tr("The account must be migrated to a Microsoft account."));
        return;
    }
}

void SailLauncher::startGame(InstancePtr instance, AuthSessionPtr session, const QString &server)
{
    if (!instance->reloadSettings()) {
        fail(tr("Couldn't load the instance profile."));
        return;
    }
    if (!ensureJava(instance)) {
        return;
    }
    RendererConfig::applyForLaunch();

    QuickPlayTargetPtr quickPlay;
    if (!server.isEmpty()) {
        quickPlay = std::make_shared<QuickPlayTarget>(QuickPlayTarget::parseMultiplayer(server));
    }
    m_launchTask = instance->createLaunchTask(session, quickPlay);
    if (!m_launchTask) {
        fail(tr("Couldn't instantiate a launcher."));
        return;
    }
    m_launchTask->prependStep(new TextPrint(m_launchTask.get(),
        QString("%1 %2 on Sailfish OS, renderer: %3\n\n")
            .arg(BuildConfig.LAUNCHER_DISPLAYNAME, BuildConfig.printableVersionString(),
                 APPLICATION->settings()->get("SailRenderer").toString()),
        MessageLevel::Launcher));

    m_log->setSourceModel(m_launchTask->getLogModel().get());
    emit logChanged();

    m_runningId = instance->id();
    emit gameChanged();

    const QString instanceId = instance->id();
    connect(m_launchTask.get(), &LaunchTask::readyForLaunch, this, [this]() {
        m_launchTask->proceed();
    });
    // the update/download step wants a progress display
    connect(m_launchTask.get(), &LaunchTask::requestProgress, this, [this](Task *task) {
        m_launchTask->proceed();
        m_task = task;
        m_taskTitle = tr("Preparing the game");
        m_taskStatus = task->getStatus();
        m_taskProgress = -1;
        connect(task, &Task::status, this, [this](QString status) {
            m_taskStatus = status;
            emit taskChanged();
        });
        connect(task, &Task::progress, this, [this](qint64 current, qint64 total) {
            m_taskProgress = total > 0 ? qreal(current) / total : -1;
            emit taskChanged();
        });
        connect(task, &Task::finished, this, &SailLauncher::finishTask, Qt::QueuedConnection);
        emit taskChanged();
    });
    auto done = [this]() {
        m_launchTask.reset();
        m_runningId.clear();
        emit gameChanged();
        emit selectedChanged();
    };
    connect(m_launchTask.get(), &LaunchTask::succeeded, this, done);
    connect(m_launchTask.get(), &LaunchTask::failed, this, [this, done](QString reason) {
        fail(tr("The game stopped: %1").arg(reason));
        done();
    });
    m_launchTask->start();
}

QString SailLauncher::logText() const
{
    auto model = qobject_cast<LogModel *>(m_log->sourceModel());
    return model ? model->toPlainText() : QString();
}

void SailLauncher::kill()
{
    if (m_launchTask && m_launchTask->canAbort()) {
        m_launchTask->abort();
    }
}

// ---------------------------------------------------------------- Microsoft login

void SailLauncher::startLogin()
{
    stopLogin();
    m_loginAccount = MinecraftAccount::createBlankMSA();
    m_loginTask = m_loginAccount->loginMSA();
    m_loginStatus = tr("Contacting Microsoft…");
    connect(m_loginTask.get(), &Task::status, this, [this](QString status) {
        m_loginStatus = status;
        emit loginChanged();
    });
    connect(m_loginTask.get(), &AccountTask::showVerificationUriAndCode, this,
            [this](const QUrl &uri, const QString &code, int expiresIn) {
        QUrl url(uri);
        QUrlQuery query;
        query.addQueryItem("otc", code);
        url.setQuery(query);
        m_loginUrl = url.toString();
        m_loginCode = code;
        m_loginSecondsLeft = expiresIn;
        m_loginTimer.start();
        emit loginChanged();
        emit loginTick();
    });
    connect(m_loginTask.get(), &AccountTask::hideVerificationUriAndCode, this, [this]() {
        m_loginCode.clear();
        m_loginUrl.clear();
        m_loginTimer.stop();
        emit loginChanged();
    });
    connect(m_loginTask.get(), &Task::succeeded, this, [this]() {
        auto account = m_loginAccount;
        APPLICATION->accounts()->addAccount(account);
        if (!APPLICATION->accounts()->defaultAccount()) {
            APPLICATION->accounts()->setDefaultAccount(account);
        }
        const QString name = account->hasProfile() ? account->profileName() : account->gamerTag();
        stopLogin();
        emit loginSucceeded(name);
    });
    connect(m_loginTask.get(), &Task::failed, this, [this](QString reason) {
        // a network error in the first step leaves the account Offline: there is no connection
        const bool noConnection = m_loginAccount && m_loginAccount->accountState() == AccountState::Offline;
        QString message;
        if (!noConnection) {
            message = tr("Signing in failed: %1").arg(reason);
        } else if (APPLICATION->accounts()->anyAccountIsValid()) {
            message = tr("No internet connection. Signing in needs a connection - "
                         "your account that owns Minecraft can still play offline.");
        } else {
            message = tr("No internet connection. Signing in needs a connection the first time, "
                         "after that the game can be played offline.");
        }
        stopLogin();
        // keep the whole message on the sign in page too, the notice shows only one line
        m_loginStatus = message;
        emit loginChanged();
        fail(message);
    });
    emit loginChanged();
    m_loginTask->start();
}

void SailLauncher::stopLogin()
{
    m_loginTimer.stop();
    if (m_loginTask) {
        disconnect(m_loginTask.get(), nullptr, this, nullptr);
    }
    m_loginTask.reset();
    m_loginAccount.reset();
    m_loginCode.clear();
    m_loginUrl.clear();
    m_loginStatus.clear();
    m_loginSecondsLeft = 0;
    emit loginChanged();
}

QString SailLauncher::addOfflineAccount(const QString &name)
{
    // Same rule as Prism Launcher: offline accounts need an account that owns the game
    if (!APPLICATION->accounts()->anyAccountIsValid()) {
        return tr("You must add a Microsoft account that owns Minecraft before you can add an offline account.");
    }
    const QString username = name.trimmed();
    if (!QRegExp("[A-Za-z0-9_]{3,16}").exactMatch(username)) {
        return tr("The name must have 3 to 16 characters: letters, digits and _");
    }
    auto account = MinecraftAccount::createOffline(username);
    APPLICATION->accounts()->addAccount(account);
    if (!APPLICATION->accounts()->defaultAccount()) {
        APPLICATION->accounts()->setDefaultAccount(account);
    }
    return QString();
}

void SailLauncher::addYggdrasilAccount(const QString &server, const QString &username, const QString &password)
{
    // same rule as offline accounts (Prism Launcher): only next to an account that owns the game
    if (!APPLICATION->accounts()->anyAccountIsValid()) {
        fail(tr("You must add a Microsoft account that owns Minecraft before you can add accounts of other servers."));
        return;
    }
    if (busy()) {
        fail(tr("Please wait for the current operation to finish"));
        return;
    }
    auto task = new YggdrasilLoginTask(server, username, password);
    m_ownedTask.reset(task);
    runTask(task, tr("Adding the account"), [this, task]() {
        auto account = MinecraftAccount::createYggdrasil(task->apiRoot(), task->clientToken(), task->accessToken(),
                                                         task->profileId(), task->profileName());
        APPLICATION->accounts()->addAccount(account);
        if (!APPLICATION->accounts()->defaultAccount()) {
            APPLICATION->accounts()->setDefaultAccount(account);
        }
        emit loginSucceeded(task->profileName());
    });
}

bool SailLauncher::canAddOfflineAccount() const
{
    return APPLICATION->accounts()->anyAccountIsValid();
}

void SailLauncher::cancelLogin()
{
    if (m_loginTask && m_loginTask->canAbort()) {
        m_loginTask->abort();
    }
    stopLogin();
}

// ---------------------------------------------------------------- settings

QVariant SailLauncher::setting(const QString &key) const
{
    return APPLICATION->settings()->get(key);
}

void SailLauncher::setSetting(const QString &key, const QVariant &value)
{
    APPLICATION->settings()->set(key, value);
}

QVariant SailLauncher::instanceSetting(const QString &id, const QString &key) const
{
    auto instance = APPLICATION->instances()->getInstanceById(id);
    return instance ? instance->settings()->get(key) : QVariant();
}

void SailLauncher::setInstanceSetting(const QString &id, const QString &key, const QVariant &value)
{
    auto instance = APPLICATION->instances()->getInstanceById(id);
    if (instance) {
        instance->settings()->set(key, value);
    }
}

void SailLauncher::applyProxySettings()
{
    auto s = APPLICATION->settings();
    APPLICATION->updateProxySettings(s->get("ProxyType").toString(), s->get("ProxyAddr").toString(),
                                     s->get("ProxyPort").toInt(), s->get("ProxyUser").toString(),
                                     s->get("ProxyPass").toString());
}

int SailLauncher::systemMemoryMiB() const
{
    QFile meminfo("/proc/meminfo");
    if (meminfo.open(QIODevice::ReadOnly)) {
        const QString first = QString::fromLatin1(meminfo.readLine());
        const QStringList parts = first.split(' ', QString::SkipEmptyParts);
        if (parts.size() >= 2) {
            return int(parts.at(1).toLongLong() / 1024);
        }
    }
    return 2048;
}

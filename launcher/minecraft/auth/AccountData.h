#pragma once
#include <QString>
#include <QByteArray>
#include <QVector>
#include <katabasis/Bits.h>
#include <QJsonObject>

struct Skin {
    QString id;
    QString url;
    QString variant;

    QByteArray data;
};

struct Cape {
    QString id;
    QString url;
    QString alias;

    QByteArray data;
};

struct MinecraftEntitlement {
    bool ownsMinecraft = false;
    bool canPlayMinecraft = false;
    Katabasis::Validity validity = Katabasis::Validity::None;
};

struct MinecraftProfile {
    QString id;
    QString name;
    Skin skin;
    QString currentCape;
    QMap<QString, Cape> capes;
    Katabasis::Validity validity = Katabasis::Validity::None;
};

enum class AccountState {
    Unchecked,
    Offline,
    Working,
    Online,
    Errored,
    Expired,
    Gone,
    MustMigrate
};

// Offline accounts are ported from Prism Launcher: they can only be added when an account that
// owns Minecraft is present (see AccountList::anyAccountIsValid), and never count as owning the game.
// Yggdrasil accounts (a server of your own used through authlib-injector, e.g. Drasl) follow the
// same rule.
enum class AccountType {
    MSA,
    Offline,
    Yggdrasil
};

struct AccountData {
    QJsonObject saveState() const;
    bool resumeStateFromV3(QJsonObject data);

    //! gamertag for MSA
    QString gamerTag() const;

    //! Yggdrasil access token, as passed to the game.
    QString accessToken() const;

    QString profileId() const;
    QString profileName() const;

    QString xid() const;

    QString lastError() const;

    AccountType type = AccountType::MSA;

    //! Yggdrasil accounts: API root of the authentication server and the client token used with it
    QString yggdrasilServer;
    QString yggdrasilClientToken;

    Katabasis::Token msaToken;
    Katabasis::Token userToken;
    Katabasis::Token xboxApiToken;
    Katabasis::Token mojangservicesToken;

    Katabasis::Token yggdrasilToken;
    MinecraftProfile minecraftProfile;
    MinecraftEntitlement minecraftEntitlement;
    Katabasis::Validity validity_ = Katabasis::Validity::None;

    // runtime only information (not saved with the account)
    QString internalId;
    QString errorString;
    AccountState accountState = AccountState::Unchecked;
};

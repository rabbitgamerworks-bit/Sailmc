// SailMC qmake build: the CMake build generates this file from buildconfig/BuildConfig.cpp.in.
// Keep the values in sync with the Launcher_* variables in the top level CMakeLists.txt.
#include "BuildConfig.h"
#include <QObject>

const Config BuildConfig;

Config::Config()
{
    // Name and copyright
    LAUNCHER_NAME = "sailmc";
    LAUNCHER_DISPLAYNAME = "SailMC";
    LAUNCHER_COPYRIGHT = "MultiMC Contributors";
    LAUNCHER_DOMAIN = "multimc.org";
    LAUNCHER_CONFIGFILE = "sailmc.cfg";
    LAUNCHER_GIT = "https://github.com/MultiMC/Launcher";

    USER_AGENT = "SailMC/1.0 (MultiMC/5.0)";
    USER_AGENT_UNCACHED = USER_AGENT + " (Uncached)";

    // Version information
    VERSION_MAJOR = 0;
    VERSION_MINOR = 7;
    VERSION_HOTFIX = 0;
    VERSION_BUILD = -1;

    BUILD_PLATFORM = "";
    UPDATER_BASE = "";
    NOTIFICATION_URL = "";
    TRANSLATIONS_BASE_URL = "https://files.multimc.org/translations/";
    FULL_VERSION_STR = "0.7.-1";

    GIT_COMMIT = "";
    GIT_REFSPEC = "";
    if(GIT_REFSPEC.startsWith("refs/heads/") && !UPDATER_BASE.isEmpty() && !BUILD_PLATFORM.isEmpty() && VERSION_BUILD >= 0)
    {
        VERSION_CHANNEL = GIT_REFSPEC;
        VERSION_CHANNEL.remove("refs/heads/");
        UPDATER_ENABLED = true;
    }
    else
    {
        VERSION_CHANNEL = QObject::tr("custom");
    }

    VERSION_STR = "";
    NEWS_URL = "https://multimc.org/posts.html";
    NEWS_RSS_URL = "https://multimc.org/rss.xml";
    PASTE_EE_KEY = "utLvciUouSURFzfjPxLBf5W4ISsUX4pwBDF7N1AfZ";
    MSA_CLIENT_ID = "499546d9-bbfe-4b9b-a086-eb3d75afb78f";
    IMGUR_CLIENT_ID = "5b97b0713fba4a3";
    META_URL = "https://meta.multimc.org/v1/";

    BUG_TRACKER_URL = "https://github.com/MultiMC/Launcher/issues";
    DISCORD_URL = "https://discord.gg/multimc";
    SUBREDDIT_URL = "https://www.reddit.com/r/MultiMC/";
    PATREON_URL = "https://www.patreon.com/multimc";
}

QString Config::printableVersionString() const
{
    QString vstr = QString("%1.%2.%3").arg(QString::number(VERSION_MAJOR), QString::number(VERSION_MINOR), QString::number(VERSION_HOTFIX));

    // If the build is not a main release, append the channel
    if(VERSION_CHANNEL != "develop")
    {
        vstr += "-" + VERSION_CHANNEL;
    }

    // if a build number is set, also add it to the end
    if(VERSION_BUILD >= 0)
    {
        vstr += "-" + QString::number(VERSION_BUILD);
    }
    return vstr;
}

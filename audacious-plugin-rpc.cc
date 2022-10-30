#include <iostream>
#include <string.h>

#include "HTTPRequest.hpp"
#include "json.hpp"
#include "encode.h"

#include <libaudcore/drct.h>
#include <libaudcore/i18n.h>
#include <libaudcore/plugin.h>
#include <libaudcore/hook.h>
#include <libaudcore/audstrings.h>
#include <libaudcore/tuple.h>
#include <libaudcore/preferences.h>
#include <libaudcore/runtime.h>

#include <discord_rpc.h>

#define EXPORT __attribute__((visibility("default")))
#define APPLICATION_ID "1036306255507095572"

static const char *SETTING_EXTRA_TEXT = "extra_text";
using json = nlohmann::json;

class RPCPlugin : public GeneralPlugin
{

public:
    static const char about[];
    static const PreferencesWidget widgets[];
    static const PluginPreferences prefs;

    static constexpr PluginInfo info = {
        N_("Discord RPC"),
        "audacious-plugin-rpc",
        about,
        &prefs};

    constexpr RPCPlugin() : GeneralPlugin(info, false) {}

    bool init();
    void cleanup();
};

EXPORT RPCPlugin aud_plugin_instance;

DiscordEventHandlers handlers;
DiscordRichPresence presence;
std::string fullTitle;
std::string playingStatus;

void init_discord()
{
    memset(&handlers, 0, sizeof(handlers));
    Discord_Initialize(APPLICATION_ID, &handlers, 1, NULL);
}

void update_presence()
{
    Discord_UpdatePresence(&presence);
}

void init_presence()
{
    memset(&presence, 0, sizeof(presence));
    presence.state = "Initialized";
    presence.details = "Waiting...";
    presence.largeImageKey = "logo";
    presence.smallImageKey = "stop";
    update_presence();
}

void cleanup_discord()
{
    Discord_ClearPresence();
    Discord_Shutdown();
}

void title_changed()
{
    try
    {
        const char *albumCoverURL;

        if (!aud_drct_get_ready())
        {
            return;
        }

        if (aud_drct_get_playing())
        {
            bool paused = aud_drct_get_paused();
            Tuple tuple = aud_drct_get_tuple();
            std::string artist(tuple.get_str(Tuple::Artist));
            std::string album(tuple.get_str(Tuple::Album));
            std::string title(tuple.get_str(Tuple::Title));

            std::string requestURL("http://ws.audioscrobbler.com/2.0/?method=album.getinfo&api_key=29c8a554e57d377f721cf665d14f6b5f&artist=" + url_encode(artist) + "&album=" + url_encode(album) + "&format=json");

            http::Request request{requestURL};
            const auto response = request.send("GET");
            std::string responseData(response.body.begin(), response.body.end());
            std::cout << requestURL + "\n";

            json responseJson = json::parse(responseData);

            albumCoverURL = std::string(responseJson["album"]["image"][3]["#text"]).c_str();

            if (artist.length() > 0)
            {
                fullTitle = (std::string(artist) + " - " + title).substr(0, 127);
            }
            else
            {
                fullTitle = title.substr(0, 127);
            }

            playingStatus = paused ? "Paused" : "Listening";

            presence.details = fullTitle.c_str();
            presence.smallImageKey = paused ? "pause" : "play";
        }
        else
        {
            playingStatus = "Stopped";
            presence.state = "Stopped";
            presence.smallImageKey = "stop";
        }

        std::string extraText(aud_get_str("audacious-plugin-rpc", SETTING_EXTRA_TEXT));
        playingStatus = (playingStatus + " " + extraText).substr(0, 127);
        // presence.largeImageKey = albumCoverURL;
        std::cout << std::string(albumCoverURL) + "\n";

        presence.state = playingStatus.c_str();
        update_presence();
    }
    catch (const std::exception &exc)
    {
        presence.largeImageKey = "logo";
    }
}

void update_title_presence(void *, void *)
{
    title_changed();
}

void open_github()
{
    system("xdg-open https://github.com/darktohka/audacious-plugin-rpc");
}

bool RPCPlugin::init()
{
    init_discord();
    init_presence();
    hook_associate("playback ready", update_title_presence, nullptr);
    hook_associate("playback end", update_title_presence, nullptr);
    hook_associate("playback stop", update_title_presence, nullptr);
    hook_associate("playback pause", update_title_presence, nullptr);
    hook_associate("playback unpause", update_title_presence, nullptr);
    hook_associate("title change", update_title_presence, nullptr);
    return true;
}

void RPCPlugin::cleanup()
{
    hook_dissociate("playback ready", update_title_presence);
    hook_dissociate("playback end", update_title_presence);
    hook_dissociate("playback stop", update_title_presence);
    hook_dissociate("playback pause", update_title_presence);
    hook_dissociate("playback unpause", update_title_presence);
    hook_dissociate("title change", update_title_presence);
    cleanup_discord();
}

const char RPCPlugin::about[] = N_("Discord RPC music status plugin\n\nWritten by: Derzsi Daniel <daniel@tohka.us> \n\n Edited by: crackheadakira, Sayykii, Tibix");

const PreferencesWidget RPCPlugin::widgets[] =
    {
        WidgetEntry(
            N_("Extra status text:"),
            WidgetString("audacious-plugin-rpc", SETTING_EXTRA_TEXT, title_changed)),
        WidgetButton(
            N_("Fork on GitHub"),
            {open_github})};

const PluginPreferences RPCPlugin::prefs = {{widgets}};
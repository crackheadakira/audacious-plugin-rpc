#include <iostream>
#include <string.h>

#include "libs/HTTPRequest.hpp"
#include "libs/encode.h"
#include "libs/json.hpp"

#include <libaudcore/audstrings.h>
#include <libaudcore/drct.h>
#include <libaudcore/hook.h>
#include <libaudcore/i18n.h>
#include <libaudcore/plugin.h>
#include <libaudcore/preferences.h>
#include <libaudcore/runtime.h>
#include <libaudcore/tuple.h>

#include <discord_rpc.h>

#define EXPORT __attribute__((visibility("default")))
#define APPLICATION_ID "1036306255507095572"
using json = nlohmann::json;

class RPCPlugin : public GeneralPlugin {
public:
    static const char about[];
    static const PreferencesWidget widgets[];
    static const PluginPreferences prefs;

    static constexpr PluginInfo info = { N_("Discord RPC"), "audacious-plugin-rpc", about, &prefs };

    constexpr RPCPlugin()
        : GeneralPlugin(info, false)
    {
    }

    bool init();
    void cleanup();
};

EXPORT RPCPlugin aud_plugin_instance;
static const char* fetch_album = "TRUE";
static const char* lastfm_api_key = "";

DiscordEventHandlers handlers;
DiscordRichPresence presence;
std::string fullTitle;
std::string playingStatus;
std::string lastfm_key;

void init_discord()
{
    memset(&handlers, 0, sizeof(handlers));
    Discord_Initialize(APPLICATION_ID, &handlers, 1, NULL);
}

void update_presence() { Discord_UpdatePresence(&presence); }

void init_presence()
{
    memset(&presence, 0, sizeof(presence));
    presence.state = "Initialized";
    presence.details = "Waiting...";
    presence.largeImageKey = "logo";
    presence.largeImageText = "Audacious";
    update_presence();
}

void cleanup_discord()
{
    Discord_ClearPresence();
    Discord_Shutdown();
}

void title_changed()
{
    try {
        std::string imgUrl;
        if (!aud_drct_get_ready()) {
            return;
        }

        if (aud_drct_get_playing()) {
            bool paused = aud_drct_get_paused();
            Tuple tuple = aud_drct_get_tuple();
            std::string artist(tuple.get_str(Tuple::Artist));
            std::string album(tuple.get_str(Tuple::Album));
            std::string title(tuple.get_str(Tuple::Title));
            playingStatus = "on " + album;
            presence.largeImageText = tuple.get_str(Tuple::Album);

            lastfm_key = aud_get_str("lastfm_api_key", lastfm_api_key);

            // Logic for getting timestamp that can be sent to Discord
            int songLength = aud_drct_get_length() / 1000;
            int currentSongSpot = aud_drct_get_time() / 1000;
            presence.endTimestamp = paused ? 0 : time(NULL) + (songLength - currentSongSpot);

            // Make request to last.FM to get album info
            if (lastfm_key.length() > 0) {
                if (aud_get_bool("lastfm_album", fetch_album)) {
                    std::string requestURL(
                        "http://ws.audioscrobbler.com/2.0/?method=album.getinfo&api_key=" + lastfm_key
                        + "&artist=" + url_encode(artist) + "&album=" + url_encode(album) + "&format=json");
                    http::Request request { requestURL };
                    const auto response = request.send("GET");
                    std::string responseData(response.body.begin(), response.body.end());

                    // Parse JSON data and get usable image URL
                    json responseJson = json::parse(responseData);

                    auto test = responseJson["album"]["image"][3]["#text"];
                    for (auto it = test.cbegin(); it != test.cend(); ++it) {
                        imgUrl += *it;
                    }
                }
            }

            if (artist.length() > 0) {
                fullTitle = (std::string(artist) + " - " + title).substr(0, 127);
            } else {
                fullTitle = title.substr(0, 127);
            }

            presence.details = fullTitle.c_str();
            presence.smallImageKey = paused ? "pause" : "play";
            presence.smallImageText = paused ? "Paused" : "Playing";
        } else {
            presence.state = "Stopped";
            presence.smallImageKey = "stop";
        }

        presence.largeImageKey
            = aud_get_bool("lastfm_album", fetch_album) && lastfm_key.length() > 0 ? imgUrl.c_str() : "logo";
        presence.state = playingStatus.c_str();
        update_presence();
    } catch (const std::exception& exc) {
        presence.largeImageKey = "logo";
    }
}

void update_title_presence(void*, void*) { title_changed(); }

void open_github() { system("xdg-open https://github.com/crackheadakira/audacious-plugin-rpc"); }

bool RPCPlugin::init()
{
    init_discord();
    init_presence();
    hook_associate("playback ready", update_title_presence, nullptr);
    hook_associate("playback end", update_title_presence, nullptr);
    hook_associate("playback stop", update_title_presence, nullptr);
    hook_associate("playback pause", update_title_presence, nullptr);
    hook_associate("playback unpause", update_title_presence, nullptr);
    hook_associate("playback seek", update_title_presence, nullptr);
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
    hook_dissociate("playback seek", update_title_presence);
    hook_dissociate("title change", update_title_presence);
    cleanup_discord();
}

const char RPCPlugin::about[] = N_("Discord RPC Plugin \n Written by: Derzsi Daniel <daniel@tohka.us> \n Modified by: "
                                   "crackheadakira, Sayykii, Levev and Tibix");

const PreferencesWidget RPCPlugin::widgets[] = { WidgetCheck(N_("Fetch album from Last.FM and display it on Discord"),
                                                     WidgetBool("lastfm_album", fetch_album, title_changed)),
    WidgetEntry(N_("Add your Last.FM Api key:"), WidgetString("lastfm_api_key", lastfm_api_key, title_changed)),
    WidgetButton(N_("Fork on GitHub"), { open_github }) };

const PluginPreferences RPCPlugin::prefs = { { widgets } };
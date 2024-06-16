#include "ChatLink.h"

#include <codecvt>
#include <regex>

#include "Bancho.h"
#include "Lobby.h"
#include "MainMenu.h"
#include "NotificationOverlay.h"
#include "Osu.h"
#include "RoomScreen.h"
#include "SongBrowser/SongBrowser.h"
#include "TooltipOverlay.h"

ChatLink::ChatLink(float xPos, float yPos, float xSize, float ySize, UString link, UString label)
    : CBaseUILabel(xPos, yPos, xSize, ySize, link, label) {
    m_link = link;
    setDrawFrame(false);
    setDrawBackground(true);
    setBackgroundColor(0xff2e3784);
}

void ChatLink::mouse_update(bool *propagate_clicks) {
    CBaseUILabel::mouse_update(propagate_clicks);

    if(isMouseInside()) {
        osu->getTooltipOverlay()->begin();
        osu->getTooltipOverlay()->addLine(UString::format("link: %s", m_link.toUtf8()));
        osu->getTooltipOverlay()->end();

        setBackgroundColor(0xff3d48ac);
    } else {
        setBackgroundColor(0xff2e3784);
    }
}

void ChatLink::onMouseUpInside() {
    if(m_link.startsWith("osump://")) {
        if(osu->m_room->isVisible()) {
            osu->getNotificationOverlay()->addNotification("You are already in a multiplayer room.");
            return;
        }

        // If the password has a space in it, parsing will break, but there's no way around it...
        // osu!stable also considers anything after a space to be part of the lobby title :(
        std::regex password_regex("osump://(\\d+)/(\\S*)");
        std::string invite_str = m_link.toUtf8();
        std::smatch match;
        std::regex_search(invite_str, match, password_regex);
        u32 invite_id = strtoul(match.str(1).c_str(), NULL, 10);
        UString password = match.str(2).c_str();
        osu->m_lobby->joinRoom(invite_id, password);
        return;
    }

    // This lazy escaping is only good for endpoint URLs, not anything more serious
    UString escaped_endpoint;
    for(int i = 0; i < bancho.endpoint.length(); i++) {
        if(bancho.endpoint[i] == L'.') {
            escaped_endpoint.append("\\.");
        } else {
            escaped_endpoint.append(bancho.endpoint[i]);
        }
    }

    // https:\/\/(akatsuki.gg|osu.ppy.sh)\/b(eatmapsets\/\d+#(osu)?)?\/(\d+)
    UString map_pattern = "https://(osu\\.";
    map_pattern.append(escaped_endpoint);
    map_pattern.append("|osu.ppy.sh)/b(eatmapsets/(\\d+)#(osu)?)?/(\\d+)");
    std::wregex map_regex(map_pattern.wc_str());

    std::wstring link_str = m_link.wc_str();
    std::wsmatch match;
    if(std::regex_search(link_str, match, map_regex)) {
        i32 map_id = std::stoi(std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(match.str(5)));
        i32 set_id = 0;
        if(match[3].matched) {
            // Set ID doesn't match if the URL only contains the map ID
            set_id = std::stoi(std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(match.str(3)));
        }

        if(osu->getSongBrowser()->isVisible()) {
            osu->getSongBrowser()->map_autodl = map_id;
            osu->getSongBrowser()->set_autodl = set_id;
        } else if(osu->getMainMenu()->isVisible()) {
            osu->toggleSongBrowser();
            osu->getSongBrowser()->map_autodl = map_id;
            osu->getSongBrowser()->set_autodl = set_id;
        } else {
            env->openURLInDefaultBrowser(m_link);
        }

        return;
    }

    env->openURLInDefaultBrowser(m_link);
}

#include "ChatLink.h"

#include <codecvt>
#include <regex>

#include "Bancho.h"
#include "MainMenu.h"
#include "Osu.h"
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
    // TODO: Handle lobby invite links, on click join the lobby (even if passworded)

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

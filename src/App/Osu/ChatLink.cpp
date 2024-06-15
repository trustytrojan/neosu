#include "ChatLink.h"

#include "Osu.h"
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
    // XXX: Handle map links, on click auto-download, add to song browser and select
    // XXX: Handle lobby invite links, on click join the lobby (even if passworded)
    env->openURLInDefaultBrowser(m_link);
}

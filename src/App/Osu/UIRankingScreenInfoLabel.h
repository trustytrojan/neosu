#pragma once
#include "CBaseUIElement.h"

class McFont;

class Beatmap;
class DatabaseBeatmap;

class UIRankingScreenInfoLabel : public CBaseUIElement {
   public:
    UIRankingScreenInfoLabel(float xPos, float yPos, float xSize, float ySize, UString name);

    void draw(Graphics *g);

    void setFromBeatmap(Beatmap *beatmap, DatabaseBeatmap *diff2);

    void setArtist(std::string artist) { m_sArtist = artist; }
    void setTitle(std::string title) { m_sTitle = title; }
    void setDiff(std::string diff) { m_sDiff = diff; }
    void setMapper(std::string mapper) { m_sMapper = mapper; }
    void setPlayer(std::string player) { m_sPlayer = player; }
    void setDate(std::string date) { m_sDate = date; }

    float getMinimumWidth();
    float getMinimumHeight();

   private:
    UString buildPlayerString();

    McFont *m_font;

    int m_iMargin;
    float m_fSubTitleScale;

    std::string m_sArtist;
    std::string m_sTitle;
    std::string m_sDiff;
    std::string m_sMapper;
    std::string m_sPlayer;
    std::string m_sDate;
};

#pragma once
#include "CBaseUIButton.h"

class McFont;

class Beatmap;
class DatabaseBeatmap;

class InfoLabel : public CBaseUIButton {
   public:
    InfoLabel(float xPos, float yPos, float xSize, float ySize, UString name);

    void draw(Graphics *g);
    virtual void mouse_update(bool *propagate_clicks);

    void setFromBeatmap(Beatmap *beatmap, DatabaseBeatmap *diff2);
    void setFromMissingBeatmap(long beatmapId);

    void setArtist(std::string artist) { m_sArtist = artist; }
    void setTitle(std::string title) { m_sTitle = title; }
    void setDiff(std::string diff) { m_sDiff = diff; }
    void setMapper(std::string mapper) { m_sMapper = mapper; }

    void setLengthMS(unsigned long lengthMS) { m_iLengthMS = lengthMS; }
    void setBPM(int minBPM, int maxBPM, int mostCommonBPM) {
        m_iMinBPM = minBPM;
        m_iMaxBPM = maxBPM;
        m_iMostCommonBPM = mostCommonBPM;
    }
    void setNumObjects(int numObjects) { m_iNumObjects = numObjects; }

    void setCS(float CS) { m_fCS = CS; }
    void setAR(float AR) { m_fAR = AR; }
    void setOD(float OD) { m_fOD = OD; }
    void setHP(float HP) { m_fHP = HP; }
    void setStars(float stars) { m_fStars = stars; }

    void setLocalOffset(long localOffset) { m_iLocalOffset = localOffset; }
    void setOnlineOffset(long onlineOffset) { m_iOnlineOffset = onlineOffset; }

    float getMinimumWidth();
    float getMinimumHeight();

    long getBeatmapID() const { return m_iBeatmapId; }

   private:
    UString buildSongInfoString();
    UString buildDiffInfoString();
    UString buildOffsetInfoString();

    ConVar *m_osu_debug_ref;
    ConVar *m_osu_songbrowser_dynamic_star_recalc_ref;

    McFont *m_font;

    int m_iMargin;
    float m_fTitleScale;
    float m_fSubTitleScale;
    float m_fSongInfoScale;
    float m_fDiffInfoScale;
    float m_fOffsetInfoScale;

    std::string m_sArtist;
    std::string m_sTitle;
    std::string m_sDiff;
    std::string m_sMapper;

    unsigned long m_iLengthMS;
    int m_iMinBPM;
    int m_iMaxBPM;
    int m_iMostCommonBPM;
    int m_iNumObjects;

    float m_fCS;
    float m_fAR;
    float m_fOD;
    float m_fHP;
    float m_fStars;

    long m_iLocalOffset;
    long m_iOnlineOffset;

    // custom
    long m_iBeatmapId;
};

#pragma once
#include "CBaseUIButton.h"
#include "Database.h"
#include "score.h"

class SkinImage;

class UIAvatar;
class UIContextMenu;

class ScoreButton : public CBaseUIButton {
   public:
    static SkinImage *getGradeImage(FinishedScore::Grade grade);
    static UString getModsStringForDisplay(Replay::Mods mods);

    enum class STYLE { SCORE_BROWSER, TOP_RANKS };

    ScoreButton(UIContextMenu *contextMenu, float xPos, float yPos, float xSize, float ySize,
                STYLE style = STYLE::SCORE_BROWSER);
    virtual ~ScoreButton();

    void draw(Graphics *g);
    void mouse_update(bool *propagate_clicks);

    void highlight();
    void resetHighlight();

    void setScore(const FinishedScore &score, DatabaseBeatmap *diff2, int index = 1, UString titleString = "",
                  float weight = 1.0f);
    void setIndex(int index) { this->iScoreIndexNumber = index; }

    inline FinishedScore getScore() const { return this->score; }
    inline u64 getScoreUnixTimestamp() const { return this->score.unixTimestamp; }
    inline unsigned long long getScoreScore() const { return this->score.score; }

    inline UString getDateTime() const { return this->sScoreDateTime; }
    inline int getIndex() const { return this->iScoreIndexNumber; }

    bool is_friend = false;
    UIAvatar *avatar = NULL;
    MD5Hash map_hash;

   private:
    static UString recentScoreIconString;

    void updateElapsedTimeString();

    virtual void onClicked();

    virtual void onMouseInside();
    virtual void onMouseOutside();

    virtual void onFocusStolen();

    void onRightMouseUpInside();
    void onContextMenu(UString text, int id = -1);
    void onUseModsClicked();
    void onDeleteScoreClicked();
    void onDeleteScoreConfirmed(UString text, int id);

    bool isContextMenuVisible();

    UIContextMenu *contextMenu;
    STYLE style;
    float fIndexNumberAnim;
    bool bIsPulseAnim;

    bool bRightClick;
    bool bRightClickCheck;

    // score data
    FinishedScore score;

    int iScoreIndexNumber;
    u64 iScoreUnixTimestamp;

    FinishedScore::Grade scoreGrade;

    // STYLE::SCORE_BROWSER
    UString sScoreTime;
    UString sScoreUsername;
    UString sScoreScore;
    UString sScoreScorePP;
    UString sScoreAccuracy;
    UString sScoreAccuracyFC;
    UString sScoreMods;
    UString sCustom;

    // STYLE::TOP_RANKS
    UString sScoreTitle;
    UString sScoreScorePPWeightedPP;
    UString sScoreScorePPWeightedWeight;
    UString sScoreWeight;

    std::vector<UString> tooltipLines;
    UString sScoreDateTime;
};

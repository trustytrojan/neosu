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

    ScoreButton(UIContextMenu *contextMenu, float xPos, float yPos, float xSize, float ySize);
    ~ScoreButton() override;

    void draw() override;
    void mouse_update(bool *propagate_clicks) override;

    void highlight();
    void resetHighlight();

    void setScore(const FinishedScore &score, DatabaseBeatmap *diff2, int index = 1, const UString& titleString = "",
                  float weight = 1.0f);
    void setIndex(int index) { this->iScoreIndexNumber = index; }

    [[nodiscard]] inline FinishedScore getScore() const { return this->score; }
    [[nodiscard]] inline u64 getScoreUnixTimestamp() const { return this->score.unixTimestamp; }
    [[nodiscard]] inline unsigned long long getScoreScore() const { return this->score.score; }

    [[nodiscard]] inline UString getDateTime() const { return this->sScoreDateTime; }
    [[nodiscard]] inline int getIndex() const { return this->iScoreIndexNumber; }

    bool is_friend = false;
    UIAvatar *avatar = NULL;
    MD5Hash map_hash;

   private:
    static UString recentScoreIconString;

    void updateElapsedTimeString();

    void onClicked(bool left = true, bool right = false) override;

    void onMouseInside() override;
    void onMouseOutside() override;

    void onFocusStolen() override;

    void onRightMouseUpInside();
    void onContextMenu(const UString& text, int id = -1);
    void onUseModsClicked();
    void onDeleteScoreClicked();
    void onDeleteScoreConfirmed(const UString& text, int id);

    bool isContextMenuVisible();

    UIContextMenu *contextMenu;
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

    std::vector<UString> tooltipLines;
    UString sScoreDateTime;
};

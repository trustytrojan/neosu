#pragma once
#include <utility>

#include "CBaseUIButton.h"

class DatabaseBeatmap;
class SongBrowser;
class SongButton;
class UIContextMenu;

class CBaseUIScrollView;

class Button : public CBaseUIButton {
   public:
    Button(SongBrowser *songBrowser, CBaseUIScrollView *view, UIContextMenu *contextMenu, float xPos, float yPos,
           float xSize, float ySize, UString name);
    ~Button() override;
    void deleteAnimations();

    void draw() override;
    void mouse_update(bool *propagate_clicks) override;

    virtual void updateLayoutEx();

    Button *setVisible(bool visible) override;

    void select(bool fireCallbacks = true, bool autoSelectBottomMostChild = true, bool wasParentSelected = true);
    void deselect();

    void resetAnimations();

    void setTargetRelPosY(float targetRelPosY);
    void setChildren(std::vector<SongButton *> children) { this->children = std::move(children); }
    void setOffsetPercent(float offsetPercent) { this->fOffsetPercent = offsetPercent; }
    void setHideIfSelected(bool hideIfSelected) { this->bHideIfSelected = hideIfSelected; }
    void setIsSearchMatch(bool isSearchMatch) { this->bIsSearchMatch = isSearchMatch; }

    [[nodiscard]] Vector2 getActualOffset() const;
    [[nodiscard]] inline Vector2 getActualSize() const { return this->vSize - 2 * this->getActualOffset(); }
    [[nodiscard]] inline Vector2 getActualPos() const { return this->vPos + this->getActualOffset(); }
    inline std::vector<SongButton *> &getChildren() { return this->children; }

    [[nodiscard]] virtual DatabaseBeatmap *getDatabaseBeatmap() const { return NULL; }
    [[nodiscard]] virtual Color getActiveBackgroundColor() const;
    [[nodiscard]] virtual Color getInactiveBackgroundColor() const;

    [[nodiscard]] inline bool isSelected() const { return this->bSelected; }
    [[nodiscard]] inline bool isHiddenIfSelected() const { return this->bHideIfSelected; }
    [[nodiscard]] inline bool isSearchMatch() const { return this->bIsSearchMatch.load(); }

   protected:
    void drawMenuButtonBackground();

    virtual void onSelected(bool  /*wasSelected*/, bool  /*autoSelectBottomMostChild*/, bool  /*wasParentSelected*/) { ; }
    virtual void onRightMouseUpInside() { ; }

    CBaseUIScrollView *view;
    SongBrowser *songBrowser;
    UIContextMenu *contextMenu;

    McFont *font;
    McFont *fontBold;

    bool bSelected;

    std::vector<SongButton *> children;

   private:
    static int marginPixelsX;
    static int marginPixelsY;
    static float lastHoverSoundTime;

    enum class MOVE_AWAY_STATE : uint8_t { MOVE_CENTER, MOVE_UP, MOVE_DOWN };

    void onClicked() override;
    void onMouseInside() override;
    void onMouseOutside() override;

    void setMoveAwayState(MOVE_AWAY_STATE moveAwayState, bool animate = true);

    bool bRightClick;
    bool bRightClickCheck;

    float fTargetRelPosY;
    float fScale;
    float fOffsetPercent;
    float fHoverOffsetAnimation;
    float fHoverMoveAwayAnimation;
    float fCenterOffsetAnimation;
    float fCenterOffsetVelocityAnimation;

    std::atomic<bool> bIsSearchMatch;

    bool bHideIfSelected;

    MOVE_AWAY_STATE moveAwayState;
};

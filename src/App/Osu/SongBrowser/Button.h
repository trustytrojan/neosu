#pragma once
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
    virtual ~Button();
    void deleteAnimations();

    virtual void draw(Graphics *g);
    virtual void mouse_update(bool *propagate_clicks);

    virtual void updateLayoutEx();

    Button *setVisible(bool visible);

    void select(bool fireCallbacks = true, bool autoSelectBottomMostChild = true, bool wasParentSelected = true);
    void deselect();

    void resetAnimations();

    void setTargetRelPosY(float targetRelPosY);
    void setChildren(std::vector<SongButton *> children) { m_children = children; }
    void setOffsetPercent(float offsetPercent) { m_fOffsetPercent = offsetPercent; }
    void setHideIfSelected(bool hideIfSelected) { m_bHideIfSelected = hideIfSelected; }
    void setIsSearchMatch(bool isSearchMatch) { m_bIsSearchMatch = isSearchMatch; }

    Vector2 getActualOffset() const;
    inline Vector2 getActualSize() const { return m_vSize - 2 * getActualOffset(); }
    inline Vector2 getActualPos() const { return m_vPos + getActualOffset(); }
    inline std::vector<SongButton *> &getChildren() { return m_children; }
    inline int getSortHack() const { return m_iSortHack; }

    virtual DatabaseBeatmap *getDatabaseBeatmap() const { return NULL; }
    virtual Color getActiveBackgroundColor() const;
    virtual Color getInactiveBackgroundColor() const;

    inline bool isSelected() const { return m_bSelected; }
    inline bool isHiddenIfSelected() const { return m_bHideIfSelected; }
    inline bool isSearchMatch() const { return m_bIsSearchMatch.load(); }

   protected:
    void drawMenuButtonBackground(Graphics *g);

    virtual void onSelected(bool wasSelected, bool autoSelectBottomMostChild, bool wasParentSelected) { ; }
    virtual void onRightMouseUpInside() { ; }

    CBaseUIScrollView *m_view;
    SongBrowser *m_songBrowser;
    UIContextMenu *m_contextMenu;

    McFont *m_font;
    McFont *m_fontBold;

    bool m_bSelected;

    std::vector<SongButton *> m_children;

   private:
    static int marginPixelsX;
    static int marginPixelsY;
    static float lastHoverSoundTime;
    static int sortHackCounter;

    enum class MOVE_AWAY_STATE { MOVE_CENTER, MOVE_UP, MOVE_DOWN };

    virtual void onClicked();
    virtual void onMouseInside();
    virtual void onMouseOutside();

    void setMoveAwayState(MOVE_AWAY_STATE moveAwayState, bool animate = true);

    bool m_bRightClick;
    bool m_bRightClickCheck;

    float m_fTargetRelPosY;
    float m_fScale;
    float m_fOffsetPercent;
    float m_fHoverOffsetAnimation;
    float m_fHoverMoveAwayAnimation;
    float m_fCenterOffsetAnimation;
    float m_fCenterOffsetVelocityAnimation;

    int m_iSortHack;
    std::atomic<bool> m_bIsSearchMatch;

    bool m_bHideIfSelected;

    MOVE_AWAY_STATE m_moveAwayState;
};

#pragma once
#include "CBaseUIButton.h"
#include "CBaseUIScrollView.h"
#include "CBaseUITextbox.h"

class CBaseUIContainer;

class UIContextMenuButton;
class UIContextMenuTextbox;

class UIContextMenu : public CBaseUIScrollView {
   public:
    static void clampToBottomScreenEdge(UIContextMenu *menu);
    static void clampToRightScreenEdge(UIContextMenu *menu);

   public:
    UIContextMenu(float xPos = 0, float yPos = 0, float xSize = 0, float ySize = 0, UString name = "",
                  CBaseUIScrollView *parent = NULL);

    virtual void draw(Graphics *g);
    virtual void mouse_update(bool *propagate_clicks);

    virtual void onKeyUp(KeyboardEvent &e);
    virtual void onKeyDown(KeyboardEvent &e);
    virtual void onChar(KeyboardEvent &e);

    typedef fastdelegate::FastDelegate2<UString, int> ButtonClickCallback;
    void setClickCallback(ButtonClickCallback clickCallback) { this->clickCallback = clickCallback; }

    void begin(int minWidth = 0, bool bigStyle = false);
    UIContextMenuButton *addButton(UString text, int id = -1);
    UIContextMenuTextbox *addTextbox(UString text, int id = -1);

    void end(bool invertAnimation, bool clampUnderflowAndOverflowAndEnableScrollingIfNecessary);

    void setVisible2(bool visible2);

    virtual bool isVisible() { return this->bVisible && this->bVisible2; }

   private:
    virtual void onResized();
    virtual void onMoved();
    virtual void onMouseDownOutside();

    void onClick(CBaseUIButton *button);
    void onHitEnter(UIContextMenuTextbox *textbox);

    CBaseUIScrollView *parent;

    UIContextMenuTextbox *containedTextbox;

    ButtonClickCallback clickCallback;

    int iYCounter;
    int iWidthCounter;

    bool bVisible2;
    float fAnimation;
    bool bInvertAnimation;

    bool bBigStyle;
    bool bClampUnderflowAndOverflowAndEnableScrollingIfNecessary;

    std::vector<CBaseUIElement *> selfDeletionCrashWorkaroundScheduledElementDeleteHack;
};

class UIContextMenuButton : public CBaseUIButton {
   public:
    UIContextMenuButton(float xPos, float yPos, float xSize, float ySize, UString name, UString text, int id);
    virtual ~UIContextMenuButton() { ; }

    virtual void mouse_update(bool *propagate_clicks);

    virtual void onMouseInside();
    virtual void onMouseDownInside();

    inline int getID() const { return this->iID; }

    void setTooltipText(UString text);

   private:
    int iID;

    std::vector<UString> tooltipTextLines;
};

class UIContextMenuTextbox : public CBaseUITextbox {
   public:
    UIContextMenuTextbox(float xPos, float yPos, float xSize, float ySize, UString name, int id);
    virtual ~UIContextMenuTextbox() { ; }

    inline int getID() const { return this->iID; }

   private:
    int iID;
};

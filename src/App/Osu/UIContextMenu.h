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

    void draw() override;
    void mouse_update(bool *propagate_clicks) override;

    void onKeyUp(KeyboardEvent &e) override;
    void onKeyDown(KeyboardEvent &e) override;
    void onChar(KeyboardEvent &e) override;

    typedef fastdelegate::FastDelegate2<UString, int> ButtonClickCallback;
    void setClickCallback(ButtonClickCallback clickCallback) { this->clickCallback = clickCallback; }

    void begin(int minWidth = 0, bool bigStyle = false);
    UIContextMenuButton *addButton(UString text, int id = -1);
    UIContextMenuTextbox *addTextbox(UString text, int id = -1);

    void end(bool invertAnimation, bool clampUnderflowAndOverflowAndEnableScrollingIfNecessary);

    CBaseUIElement *setVisible(bool visible) override {
        // HACHACK: this->bVisible is always true, since we want to be able to put a context menu in a scrollview.
        //          When scrolling, scrollviews call setVisible(false) to clip items, and that breaks the menu.
        (void)visible;
        return this;
    }
    void setVisible2(bool visible2);

    bool isVisible() override { return this->bVisible2; }

   private:
    void onMouseDownOutside() override;

    void onClick(CBaseUIButton *button);
    void onHitEnter(UIContextMenuTextbox *textbox);

    CBaseUIScrollView *parent = NULL;
    UIContextMenuTextbox *containedTextbox = NULL;
    ButtonClickCallback clickCallback = NULL;

    i32 iYCounter = 0;
    i32 iWidthCounter = 0;

    bool bVisible2 = false;
    f32 fAnimation = 0.f;
    bool bInvertAnimation = false;

    bool bBigStyle;
    bool bClampUnderflowAndOverflowAndEnableScrollingIfNecessary;

    std::vector<CBaseUIElement *> selfDeletionCrashWorkaroundScheduledElementDeleteHack;
};

class UIContextMenuButton : public CBaseUIButton {
   public:
    UIContextMenuButton(float xPos, float yPos, float xSize, float ySize, UString name, UString text, int id);
    ~UIContextMenuButton() override { ; }

    void mouse_update(bool *propagate_clicks) override;

    void onMouseInside() override;
    void onMouseDownInside() override;

    [[nodiscard]] inline int getID() const { return this->iID; }

    void setTooltipText(UString text);

   private:
    int iID;

    std::vector<UString> tooltipTextLines;
};

class UIContextMenuTextbox : public CBaseUITextbox {
   public:
    UIContextMenuTextbox(float xPos, float yPos, float xSize, float ySize, UString name, int id);
    ~UIContextMenuTextbox() override { ; }

    [[nodiscard]] inline int getID() const { return this->iID; }

   private:
    int iID;
};

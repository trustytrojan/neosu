#pragma once
#include "CBaseUIElement.h"

class McFont;

class CBaseUITextbox : public CBaseUIElement {
   public:
    CBaseUITextbox(float xPos = 0.0f, float yPos = 0.0f, float xSize = 0.0f, float ySize = 0.0f, UString name = "");
    virtual ~CBaseUITextbox() { ; }

    virtual void draw(Graphics *g);
    virtual void mouse_update(bool *propagate_clicks);
    virtual void onFocusStolen();

    virtual void onChar(KeyboardEvent &e);
    virtual void onKeyDown(KeyboardEvent &e);

    UString getVisibleText();
    inline const UString getText() const { return this->sText; }
    inline UString &getTextRef() { return this->sText; }  // DEPRECATED
    inline McFont *getFont() const { return this->font; }

    CBaseUITextbox *setDrawFrame(bool drawFrame) {
        this->bDrawFrame = drawFrame;
        return this;
    }
    CBaseUITextbox *setDrawBackground(bool drawBackground) {
        this->bDrawBackground = drawBackground;
        return this;
    }

    CBaseUITextbox *setBackgroundColor(Color backgroundColor) {
        this->backgroundColor = backgroundColor;
        return this;
    }
    CBaseUITextbox *setTextColor(Color textColor) {
        this->textColor = textColor;
        return this;
    }
    CBaseUITextbox *setCaretColor(Color caretColor) {
        this->caretColor = caretColor;
        return this;
    }
    CBaseUITextbox *setFrameColor(Color frameColor) {
        this->frameColor = frameColor;
        return this;
    }
    CBaseUITextbox *setFrameBrightColor(Color frameBrightColor) {
        this->frameBrightColor = frameBrightColor;
        return this;
    }
    CBaseUITextbox *setFrameDarkColor(Color frameDarkColor) {
        this->frameDarkColor = frameDarkColor;
        return this;
    }

    CBaseUITextbox *setFont(McFont *font);
    CBaseUITextbox *setTextAddX(float textAddX) {
        this->iTextAddX = textAddX;
        return this;
    }
    CBaseUITextbox *setCaretWidth(int caretWidth) {
        this->iCaretWidth = caretWidth;
        return this;
    }
    CBaseUITextbox *setTextJustification(int textJustification) {
        this->iTextJustification = textJustification;
        this->setText(this->sText);
        return this;
    }

    virtual CBaseUITextbox *setText(UString text);

    void setCursorPosRight();

    bool hitEnter();
    bool hasSelectedText() const;
    void clear();
    void focus(bool move_caret = true);

    bool is_password = false;

    UString sText;
    int iCaretPosition;
    void tickCaret();
    void updateTextPos();

   protected:
    virtual void drawText(Graphics *g);

    // events
    virtual void onMouseDownInside();
    virtual void onMouseDownOutside();
    virtual void onMouseUpInside();
    virtual void onMouseUpOutside();
    virtual void onResized();

    void handleCaretKeyboardMove();
    void handleCaretKeyboardDelete();
    void updateCaretX();

    void handleDeleteSelectedText();
    void insertTextFromClipboard();
    void deselectText();
    UString getSelectedText();

    McFont *font;

    Color textColor;
    Color frameColor;
    Color frameBrightColor;
    Color frameDarkColor;
    Color caretColor;
    Color backgroundColor;

    bool bContextMouse;
    bool bBlockMouse;
    bool bCatchMouse;
    bool bDrawFrame;
    bool bDrawBackground;
    bool bLine;

    int iTextAddX;
    int iTextAddY;
    float fTextScrollAddX;
    int iCaretX;
    int iCaretWidth;
    int iTextJustification;

    float fLinetime;
    float fTextWidth;

    bool bHitenter;

    bool bSelectCheck;
    int iSelectStart;
    int iSelectEnd;
    int iSelectX;
};

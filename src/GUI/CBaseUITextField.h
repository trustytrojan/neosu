#pragma once
#include "CBaseUIScrollView.h"

class CBaseUITextField : public CBaseUIScrollView {
   public:
    CBaseUITextField(float xPos = 0, float yPos = 0, float xSize = 0, float ySize = 0, UString name = "",
                     UString text = "");
    ~CBaseUITextField() override { ; }

    void draw() override;

    CBaseUITextField *setFont(McFont *font) {
        this->textObject->setFont(font);
        return this;
    }

    CBaseUITextField *append(UString text);

    void onResized() override;

   protected:
    //********************************************************//
    //	The object which is added to this scrollview wrapper  //
    //********************************************************//

    class TextObject : public CBaseUIElement {
       public:
        TextObject(float xPos, float yPos, float width, float height, UString text);

        void draw() override;

        CBaseUIElement *setText(UString text);
        CBaseUIElement *setFont(McFont *font) {
            this->font = font;
            this->updateStringMetrics();
            return this;
        }

        CBaseUIElement *setTextColor(Color textColor) {
            this->textColor = textColor;
            return this;
        }
        CBaseUIElement *setParentSize(Vector2 parentSize) {
            this->vParentSize = parentSize;
            this->onResized();
            return this;
        }

        [[nodiscard]] inline Color getTextColor() const { return this->textColor; }

        [[nodiscard]] inline UString getText() const { return this->sText; }
        [[nodiscard]] inline McFont *getFont() const { return this->font; }

        void onResized() override;

       private:
        void updateStringMetrics();

        Vector2 vParentSize;

        UString sText;
        Color textColor;
        McFont *font;
        float fStringHeight;
    };

    TextObject *textObject;
};

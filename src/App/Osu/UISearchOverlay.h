#pragma once
#include <utility>

#include "CBaseUIElement.h"

class UISearchOverlay : public CBaseUIElement {
   public:
    UISearchOverlay(float xPos, float yPos, float xSize, float ySize, UString name);

    void draw() override;

    void setDrawNumResults(bool drawNumResults) { this->bDrawNumResults = drawNumResults; }
    void setOffsetRight(int offsetRight) { this->iOffsetRight = offsetRight; }

    void setSearchString(UString searchString, UString hardcodedSearchString = "") {
        this->sSearchString = std::move(searchString);
        this->sHardcodedSearchString = std::move(hardcodedSearchString);
    }
    void setNumFoundResults(int numFoundResults) { this->iNumFoundResults = numFoundResults; }

    void setSearching(bool searching) { this->bSearching = searching; }

   private:
    McFont *font;

    int iOffsetRight;
    bool bDrawNumResults;

    UString sSearchString;
    UString sHardcodedSearchString;
    int iNumFoundResults;

    bool bSearching;
};

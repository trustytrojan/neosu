#pragma once
#include "CBaseUIElement.h"

class UISearchOverlay : public CBaseUIElement {
   public:
    UISearchOverlay(float xPos, float yPos, float xSize, float ySize, UString name);

    virtual void draw(Graphics *g);

    void setDrawNumResults(bool drawNumResults) { m_bDrawNumResults = drawNumResults; }
    void setOffsetRight(int offsetRight) { m_iOffsetRight = offsetRight; }

    void setSearchString(UString searchString, UString hardcodedSearchString = "") {
        m_sSearchString = searchString;
        m_sHardcodedSearchString = hardcodedSearchString;
    }
    void setNumFoundResults(int numFoundResults) { m_iNumFoundResults = numFoundResults; }

    void setSearching(bool searching) { m_bSearching = searching; }

   private:
    McFont *m_font;

    int m_iOffsetRight;
    bool m_bDrawNumResults;

    UString m_sSearchString;
    UString m_sHardcodedSearchString;
    int m_iNumFoundResults;

    bool m_bSearching;
};

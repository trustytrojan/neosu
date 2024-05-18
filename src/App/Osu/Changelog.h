#pragma once
#include "ScreenBackable.h"

class CBaseUIContainer;
class CBaseUIScrollView;
class CBaseUIImage;
class CBaseUILabel;

class Changelog : public ScreenBackable {
   public:
    Changelog();
    virtual ~Changelog();

    virtual void mouse_update(bool *propagate_clicks);

    virtual CBaseUIContainer *setVisible(bool visible);

   private:
    virtual void updateLayout();
    virtual void onBack();

    void onChangeClicked(CBaseUIButton *button);

    CBaseUIScrollView *m_scrollView;

    struct CHANGELOG {
        UString title;
        std::vector<UString> changes;
    };

    struct CHANGELOG_UI {
        CBaseUILabel *title;
        std::vector<CBaseUIButton *> changes;
    };

    std::vector<CHANGELOG_UI> m_changelogs;
};

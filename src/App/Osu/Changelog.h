#pragma once
#include "ScreenBackable.h"

class CBaseUIContainer;
class CBaseUIScrollView;
class CBaseUIImage;
class CBaseUILabel;

class Changelog : public ScreenBackable {
   public:
    Changelog();
    ~Changelog() override;

    void mouse_update(bool *propagate_clicks) override;

    CBaseUIContainer *setVisible(bool visible) override;

   private:
    void updateLayout() override;
    void onBack() override;

    void onChangeClicked(CBaseUIButton *button);

    CBaseUIScrollView *scrollView;

    struct CHANGELOG {
        UString title;
        std::vector<UString> changes;
    };

    struct CHANGELOG_UI {
        CBaseUILabel *title;
        std::vector<CBaseUIButton *> changes;
    };

    std::vector<CHANGELOG_UI> changelogs;
};

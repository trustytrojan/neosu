#pragma once
#include "CBaseUIWindow.h"

class CBaseUITextbox;
class CBaseUIScrollView;
class CBaseUILabel;
class CBaseUITextField;
class McFont;

class Console : public CBaseUIWindow {
   public:
    static void processCommand(UString command);
    static void execConfigFile(std::string filename);

   public:
    Console();
    ~Console() override;

    void mouse_update(bool *propagate_clicks) override;

    void log(UString text, Color textColor = 0xffffffff);
    void clear();

    // events
    void onResized() override;

    static std::vector<UString> g_commandQueue;

   private:
    CBaseUIScrollView *log_view;
    CBaseUITextbox *textbox;

    McFont *logFont;
};

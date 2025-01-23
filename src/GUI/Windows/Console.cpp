#include "Console.h"

#include <mutex>

#include "CBaseUIButton.h"
#include "CBaseUIContainer.h"
#include "CBaseUILabel.h"
#include "CBaseUIScrollView.h"
#include "CBaseUITextField.h"
#include "CBaseUITextbox.h"
#include "ConVar.h"
#include "Engine.h"
#include "ResourceManager.h"

#define CONSOLE_BORDER 6

std::vector<UString> Console::g_commandQueue;
std::mutex g_consoleLogMutex;

Console::Console() : CBaseUIWindow(350, 100, 620, 550, "Console") {
    // convar bindings
    cmd_clear.setCallback(fastdelegate::MakeDelegate(this, &Console::clear));

    // resources
    this->logFont = engine->getResourceManager()->getFont("FONT_CONSOLE");
    McFont *textboxFont = engine->getResourceManager()->loadFont("tahoma.ttf", "FONT_CONSOLE_TEXTBOX", 9.0f, false);
    McFont *titleFont = engine->getResourceManager()->loadFont("tahoma.ttf", "FONT_CONSOLE_TITLE", 10.0f, false);

    // colors
    // Color frameColor = 0xff9a9a9a;
    Color brightColor = 0xffb7b7b7;
    Color darkColor = 0xff343434;
    Color backgroundColor = 0xff555555;
    Color windowBackgroundColor = 0xff7b7b7b;

    this->setTitleFont(titleFont);
    this->setDrawTitleBarLine(false);
    this->setDrawFrame(false);
    this->setRoundedRectangle(true);

    this->setBackgroundColor(windowBackgroundColor);

    // setFrameColor(frameColor);
    this->setFrameDarkColor(0xff9a9a9a);
    this->setFrameBrightColor(0xff8a8a8a);

    this->getCloseButton()->setBackgroundColor(0xffbababa);
    this->getCloseButton()->setDrawBackground(false);

    int textboxHeight = 20;

    // log scrollview
    this->log_view = new CBaseUIScrollView(
        CONSOLE_BORDER, 0, this->vSize.x - 2 * CONSOLE_BORDER,
        this->vSize.y - this->getTitleBarHeight() - 2 * CONSOLE_BORDER - textboxHeight - 1, "consolelog");
    this->log_view->setHorizontalScrolling(false);
    this->log_view->setVerticalScrolling(true);
    this->log_view->setBackgroundColor(backgroundColor);
    this->log_view->setFrameDarkColor(darkColor);
    this->log_view->setFrameBrightColor(brightColor);
    this->getContainer()->addBaseUIElement(this->log_view);

    // textbox
    this->textbox =
        new CBaseUITextbox(CONSOLE_BORDER, this->vSize.y - this->getTitleBarHeight() - textboxHeight - CONSOLE_BORDER,
                           this->vSize.x - 2 * CONSOLE_BORDER, textboxHeight, "consoletextbox");
    this->textbox->setText("");
    this->textbox->setFont(textboxFont);
    this->textbox->setBackgroundColor(backgroundColor);
    this->textbox->setFrameDarkColor(darkColor);
    this->textbox->setFrameBrightColor(brightColor);
    this->textbox->setCaretWidth(1);
    this->getContainer()->addBaseUIElement(this->textbox);

    // notify engine, exec autoexec
    engine->setConsole(this);
    Console::execConfigFile("autoexec.cfg");
}

Console::~Console() {}

void Console::processCommand(UString command) {
    if(command.length() < 1) return;

    // remove empty space at beginning if it exists
    if(command.find(" ", 0, 1) != -1) command.erase(0, 1);

    // handle multiple commands separated by semicolons
    if(command.find(";") != -1 && command.find("echo") == -1) {
        const std::vector<UString> commands = command.split(";");
        for(size_t i = 0; i < commands.size(); i++) {
            processCommand(commands[i]);
        }

        return;
    }

    // separate convar name and value
    const std::vector<UString> tokens = command.split(" ");
    UString commandName;
    UString commandValue;
    for(size_t i = 0; i < tokens.size(); i++) {
        if(i == 0)
            commandName = tokens[i];
        else {
            commandValue.append(tokens[i]);
            if(i < (int)(tokens.size() - 1)) commandValue.append(" ");
        }
    }

    // get convar
    ConVar *var = convar->getConVarByName(commandName, false);
    if(var == NULL) {
        debugLog("Unknown command: %s\n", commandName.toUtf8());
        return;
    }

    // set new value (this handles all callbacks internally)
    if(commandValue.length() > 0)
        var->setValue(commandValue);
    else {
        var->exec();
        var->execArgs("");
    }

    // log
    if(cv_console_logging.getBool()) {
        UString logMessage;

        bool doLog = false;
        if(commandValue.length() < 1) {
            doLog = var->hasValue();  // assume ConCommands never have helpstrings

            logMessage = commandName;

            if(var->hasValue()) {
                logMessage.append(UString::format(" = %s ( def. \"%s\" , ", var->getString().toUtf8(),
                                                  var->getDefaultString().toUtf8()));
                logMessage.append(ConVar::typeToString(var->getType()));
                logMessage.append(", ");
                logMessage.append(ConVarHandler::flagsToString(var->getFlags()));
                logMessage.append(" )");
            }

            if(var->getHelpstring().length() > 0) {
                logMessage.append(" - ");
                logMessage.append(var->getHelpstring());
            }
        } else if(var->hasValue()) {
            doLog = true;

            logMessage = commandName;
            logMessage.append(" : ");
            logMessage.append(var->getString());
        }

        if(logMessage.length() > 0 && doLog) debugLog("%s\n", logMessage.toUtf8());
    }
}

void Console::execConfigFile(std::string filename) {
    // handle extension
    filename.insert(0, MCENGINE_DATA_DIR "cfg/");
    if(filename.find(".cfg", (filename.length() - 4), filename.length()) == -1) filename.append(".cfg");

    // open it
    std::ifstream inFile(filename.c_str());
    if(!inFile.good()) {
        debugLog("Console::execConfigFile() error, file \"%s\" not found!\n", filename.c_str());
        return;
    }

    // go through every line
    std::string line;
    std::vector<UString> cmds;
    while(std::getline(inFile, line)) {
        if(line.size() > 0) {
            // handle comments
            UString cmd = UString(line.c_str());
            const int commentIndex = cmd.find("//", 0, cmd.length());
            if(commentIndex != -1) cmd.erase(commentIndex, cmd.length() - commentIndex);

            // add command
            cmds.push_back(cmd);
        }
    }

    // process the collected commands
    for(size_t i = 0; i < cmds.size(); i++) {
        processCommand(cmds[i]);
    }
}

void Console::mouse_update(bool *propagate_clicks) {
    if(!this->bVisible) return;
    CBaseUIWindow::mouse_update(propagate_clicks);

    // TODO: this needs proper callbacks in the textbox class
    if(this->textbox->hitEnter()) {
        processCommand(this->textbox->getText());
        this->textbox->clear();
    }
}

void Console::log(UString text, Color textColor) {
    std::lock_guard<std::mutex> lk(g_consoleLogMutex);

    if(text.length() < 1) return;

    // delete illegal characters
    int newline = text.find("\n", 0);
    while(newline != -1) {
        text.erase(newline, 1);
        newline = text.find("\n", 0);
    }

    // get index
    const int index = this->log_view->getContainer()->getElements().size();
    int height = 13;

    // create new label, add it
    CBaseUILabel *logEntry = new CBaseUILabel(3, height * index - 1, 150, height, text, text);
    logEntry->setDrawFrame(false);
    logEntry->setDrawBackground(false);
    logEntry->setTextColor(textColor);
    logEntry->setFont(this->logFont);
    logEntry->setSizeToContent(1, 4);
    this->log_view->getContainer()->addBaseUIElement(logEntry);

    // update scrollsize, scroll to bottom, clear textbox
    this->log_view->setScrollSizeToContent();
    this->log_view->scrollToBottom();
}

void Console::clear() { this->log_view->clear(); }

void Console::onResized() {
    CBaseUIWindow::onResized();

    this->log_view->setSize(
        this->vSize.x - 2 * CONSOLE_BORDER,
        this->vSize.y - this->getTitleBarHeight() - 2 * CONSOLE_BORDER - this->textbox->getSize().y - 1);
    this->textbox->setSize(this->vSize.x - 2 * CONSOLE_BORDER, this->textbox->getSize().y);
    this->textbox->setRelPosY(this->log_view->getRelPos().y + this->log_view->getSize().y + CONSOLE_BORDER + 1);

    this->log_view->scrollToY(this->log_view->getRelPosY());
}

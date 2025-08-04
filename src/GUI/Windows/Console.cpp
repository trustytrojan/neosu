#include "Console.h"

#include <mutex>

#include "SString.h"
#include "CBaseUIButton.h"
#include "CBaseUIContainer.h"
#include "CBaseUILabel.h"
#include "CBaseUIScrollView.h"
#include "CBaseUITextbox.h"
#include "ConVar.h"
#include "Engine.h"
#include "File.h"
#include "ResourceManager.h"

#define CONSOLE_BORDER 6

std::vector<std::string> Console::g_commandQueue;
std::mutex g_consoleLogMutex;

Console::Console() : CBaseUIWindow(350, 100, 620, 550, "Console") {
    // convar bindings
    cv::cmd::clear.setCallback(SA::MakeDelegate<&Console::freeElements>(this));

    // resources
    this->logFont = resourceManager->getFont("FONT_CONSOLE");
    McFont *textboxFont = resourceManager->loadFont("tahoma.ttf", "FONT_CONSOLE_TEXTBOX", 9.0f, false);
    McFont *titleFont = resourceManager->loadFont("tahoma.ttf", "FONT_CONSOLE_TITLE", 10.0f, false);

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

void Console::processCommand(std::string command, bool fromFile) {
    if(command.length() < 1) return;

    // remove whitespace from beginning/end of string
    SString::trim(&command);

    // handle multiple commands separated by semicolons
    if(command.find(';') != std::string::npos && command.find("echo") == std::string::npos) {
        const std::vector<std::string> commands = UString{command}.split<std::string>(";");
        for(const auto &command : commands) {
            processCommand(command);
        }

        return;
    }

    // separate convar name and value
    const std::vector<std::string> tokens = UString{command}.split<std::string>(" ");
    std::string commandName;
    std::string commandValue;
    for(size_t i = 0; i < tokens.size(); i++) {
        if(i == 0)
            commandName = tokens[i];
        else {
            commandValue.append(tokens[i]);
            if(i < (tokens.size() - 1)) commandValue.append(" ");
        }
    }

    // get convar
    ConVar *var = convar->getConVarByName(commandName, false);
    if(var == NULL || var->isFlagSet(FCVAR_NOEXEC) || (fromFile && var->isFlagSet(FCVAR_NOLOAD))) {
#ifdef _DEBUG
        if(var) {
            debugLog("not executing {}, flags: {}\n", var->getName(), ConVarHandler::flagsToString(var->getFlags()));
        }
#endif
        debugLog("Unknown command: {:s}\n", commandName);
        return;
    }

    // set new value (this handles all callbacks internally)
    // except for help, don't set a value for that, just run the callback
    if(commandValue.length() > 0) {
        if(commandName == "help") {
            var->execArgs(UString{commandValue});
        } else {
            var->setValue(UString{commandValue});
        }
    } else {
        var->exec();
        var->execArgs("");
        var->execFloat(var->getFloat());
    }

    // log
    if(cv::console_logging.getBool() && !var->isFlagSet(FCVAR_HIDDEN)) {
        std::string logMessage;

        bool doLog = false;
        if(commandValue.length() < 1) {
            doLog = var->hasValue();  // assume ConCommands never have helpstrings

            logMessage = commandName;

            if(var->hasValue()) {
                logMessage.append(fmt::format(" = {:s} ( def. \"{:s}\" , ", var->getString(), var->getDefaultString()));
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

        if(logMessage.length() > 0 && doLog) debugLog("{:s}\n", logMessage);
    }
}

void Console::execConfigFile(std::string filename) {
    // handle extension
    filename.insert(0, MCENGINE_DATA_DIR "cfg" PREF_PATHSEP "");
    if(filename.find(".cfg", (filename.length() - 4), filename.length()) == std::string::npos) filename.append(".cfg");

    File configFile(filename, File::TYPE::READ);
    if(!configFile.canRead()) {
        debugLog("error, file \"{:s}\" not found!\n", filename);
        return;
    }

    // collect commands first
    std::vector<UString> cmds;
    while(true) {
        UString line{configFile.readLine()};

        // if canRead() is false after readLine(), we hit EOF
        if(!configFile.canRead()) break;

        // only process non-empty lines
        if(!line.isEmpty()) {
            // handle comments - find "//" and remove everything after
            const int commentIndex = line.find("//");
            if(commentIndex != -1) line.erase(commentIndex, line.length() - commentIndex);

            // add command (original adds all processed lines, even if they become empty after comment removal)
            cmds.push_back(line);
        }
    }

    // process the collected commands
    for(const auto &cmd : cmds) processCommand(cmd.toUtf8(), true);
}

void Console::mouse_update(bool *propagate_clicks) {
    if(!this->bVisible) return;
    CBaseUIWindow::mouse_update(propagate_clicks);

    // TODO: this needs proper callbacks in the textbox class
    if(this->textbox->hitEnter()) {
        processCommand(this->textbox->getText().toUtf8());
        this->textbox->clear();
    }
}

void Console::log(UString text, Color textColor) {
    std::scoped_lock lk(g_consoleLogMutex);

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

void Console::freeElements() { this->log_view->freeElements(); }

void Console::onResized() {
    CBaseUIWindow::onResized();

    this->log_view->setSize(
        this->vSize.x - 2 * CONSOLE_BORDER,
        this->vSize.y - this->getTitleBarHeight() - 2 * CONSOLE_BORDER - this->textbox->getSize().y - 1);
    this->textbox->setSize(this->vSize.x - 2 * CONSOLE_BORDER, this->textbox->getSize().y);
    this->textbox->setRelPosY(this->log_view->getRelPos().y + this->log_view->getSize().y + CONSOLE_BORDER + 1);

    this->log_view->scrollToY(this->log_view->getRelPosY());
}

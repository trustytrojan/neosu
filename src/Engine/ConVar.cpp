#include "ConVar.h"

#include "Bancho.h"
#include "BanchoUsers.h"
#include "Beatmap.h"
#include "CBaseUILabel.h"
#include "Console.h"
#include "Database.h"
#include "Engine.h"
#include "Keyboard.h"
#include "ModSelector.h"
#include "Osu.h"
#include "Profiler.h"
#include "RichPresence.h"
#include "SongBrowser/LoudnessCalcThread.h"
#include "SoundEngine.h"
#include "SpectatorScreen.h"

bool ConVar::isUnlocked() const {
    if(this->isFlagSet(FCVAR_PRIVATE)) return true;
    if(!bancho.is_online()) return true;

    if(bancho.is_in_a_multi_room()) {
        return this->isFlagSet(FCVAR_BANCHO_COMPATIBLE);
    } else {
        return true;
    }
}

bool ConVar::getBool() const { return this->getFloat() > 0; }
int ConVar::getInt() { return (int)this->getFloat(); }
float ConVar::getFloat() const { return this->isUnlocked() ? this->fValue.load() : this->fDefaultValue.load(); }
const UString &ConVar::getString() { return this->isUnlocked() ? this->sValue : this->sDefaultValue; }

static std::vector<ConVar *> &_getGlobalConVarArray() {
    static std::vector<ConVar *> g_vConVars;  // (singleton)
    return g_vConVars;
}

static std::unordered_map<std::string, ConVar *> &_getGlobalConVarMap() {
    static std::unordered_map<std::string, ConVar *> g_vConVarMap;  // (singleton)
    return g_vConVarMap;
}

static void _addConVar(ConVar *c) {
    if(_getGlobalConVarArray().size() < 1) _getGlobalConVarArray().reserve(1024);

    auto cname = c->getName();
    std::string cname_str(cname.toUtf8(), cname.lengthUtf8());

    if(_getGlobalConVarMap().find(cname_str) == _getGlobalConVarMap().end()) {
        _getGlobalConVarArray().push_back(c);
        _getGlobalConVarMap()[cname_str] = c;
    } else {
        printf("FATAL: Duplicate ConVar name (\"%s\")\n", c->getName().toUtf8());
        std::exit(100);
    }
}

static ConVar *_getConVar(const UString &name) {
    const auto result = _getGlobalConVarMap().find(std::string(name.toUtf8(), name.lengthUtf8()));
    if(result != _getGlobalConVarMap().end())
        return result->second;
    else
        return NULL;
}

std::string ConVar::getFancyDefaultValue() {
    switch(this->getType()) {
        case ConVar::CONVAR_TYPE::CONVAR_TYPE_BOOL:
            return this->fDefaultDefaultValue == 0 ? "false" : "true";
        case ConVar::CONVAR_TYPE::CONVAR_TYPE_INT:
            return std::to_string((int)this->fDefaultDefaultValue);
        case ConVar::CONVAR_TYPE::CONVAR_TYPE_FLOAT:
            return std::to_string(this->fDefaultDefaultValue);
        case ConVar::CONVAR_TYPE::CONVAR_TYPE_STRING: {
            std::string out = "\"";
            out.append(this->sDefaultDefaultValue.toUtf8());
            out.append("\"");
            return out;
        }
    }

    return "unreachable";
}

UString ConVar::typeToString(CONVAR_TYPE type) {
    switch(type) {
        case ConVar::CONVAR_TYPE::CONVAR_TYPE_BOOL:
            return "bool";
        case ConVar::CONVAR_TYPE::CONVAR_TYPE_INT:
            return "int";
        case ConVar::CONVAR_TYPE::CONVAR_TYPE_FLOAT:
            return "float";
        case ConVar::CONVAR_TYPE::CONVAR_TYPE_STRING:
            return "string";
    }

    return "";
}

void ConVar::init(int flags) {
    this->callbackfunc = NULL;
    this->callbackfuncargs = NULL;
    this->changecallback = NULL;

    this->fValue = 0.0f;
    this->fDefaultDefaultValue = 0.0f;
    this->fDefaultValue = 0.0f;
    this->sDefaultDefaultValue = "";
    this->sDefaultValue = "";

    this->bHasValue = false;
    this->type = CONVAR_TYPE::CONVAR_TYPE_FLOAT;
    this->iDefaultFlags = flags;
    this->iFlags = flags;
}

void ConVar::init(UString &name, int flags) {
    this->init(flags);

    this->sName = name;
    this->type = CONVAR_TYPE::CONVAR_TYPE_STRING;
}

void ConVar::init(UString &name, int flags, ConVarCallback callback) {
    this->init(flags);

    this->sName = name;
    this->callbackfunc = callback;
    this->type = CONVAR_TYPE::CONVAR_TYPE_STRING;
}

void ConVar::init(UString &name, int flags, UString helpString, ConVarCallback callback) {
    this->init(name, flags, callback);

    this->sHelpString = helpString;
}

void ConVar::init(UString &name, int flags, ConVarCallbackArgs callbackARGS) {
    this->init(flags);

    this->sName = name;
    this->callbackfuncargs = callbackARGS;
    this->type = CONVAR_TYPE::CONVAR_TYPE_STRING;
}

void ConVar::init(UString &name, int flags, UString helpString, ConVarCallbackArgs callbackARGS) {
    this->init(name, flags, callbackARGS);

    this->sHelpString = helpString;
}

void ConVar::init(UString &name, float defaultValue, int flags, UString helpString, ConVarChangeCallback callback) {
    this->init(flags);

    this->type = CONVAR_TYPE::CONVAR_TYPE_FLOAT;
    this->sName = name;
    this->fDefaultValue = defaultValue;
    this->fDefaultDefaultValue = defaultValue;
    this->sDefaultValue = UString::format("%g", defaultValue);
    this->sDefaultDefaultValue = this->sDefaultValue;
    this->setValue(defaultValue);
    this->sHelpString = helpString;
    this->changecallback = callback;
}

void ConVar::init(UString &name, UString defaultValue, int flags, UString helpString, ConVarChangeCallback callback) {
    this->init(flags);

    this->type = CONVAR_TYPE::CONVAR_TYPE_STRING;
    this->sName = name;
    this->sDefaultValue = defaultValue;
    this->sDefaultDefaultValue = defaultValue;
    this->setValue(defaultValue);
    this->sHelpString = helpString;
    this->changecallback = callback;
}

ConVar::ConVar(UString name) {
    this->init(name, FCVAR_BANCHO_COMPATIBLE);
    _addConVar(this);
}

ConVar::ConVar(UString name, int flags, ConVarCallback callback) {
    this->init(name, flags, callback);
    _addConVar(this);
}

ConVar::ConVar(UString name, int flags, const char *helpString, ConVarCallback callback) {
    this->init(name, flags, helpString, callback);
    _addConVar(this);
}

ConVar::ConVar(UString name, int flags, ConVarCallbackArgs callbackARGS) {
    this->init(name, flags, callbackARGS);
    _addConVar(this);
}

ConVar::ConVar(UString name, int flags, const char *helpString, ConVarCallbackArgs callbackARGS) {
    this->init(name, flags, helpString, callbackARGS);
    _addConVar(this);
}

ConVar::ConVar(UString name, float fDefaultValue, int flags) {
    this->init(name, fDefaultValue, flags, UString(""), NULL);
    _addConVar(this);
}

ConVar::ConVar(UString name, float fDefaultValue, int flags, ConVarChangeCallback callback) {
    this->init(name, fDefaultValue, flags, UString(""), callback);
    _addConVar(this);
}

ConVar::ConVar(UString name, float fDefaultValue, int flags, const char *helpString) {
    this->init(name, fDefaultValue, flags, UString(helpString), NULL);
    _addConVar(this);
}

ConVar::ConVar(UString name, float fDefaultValue, int flags, const char *helpString, ConVarChangeCallback callback) {
    this->init(name, fDefaultValue, flags, UString(helpString), callback);
    _addConVar(this);
}

ConVar::ConVar(UString name, int iDefaultValue, int flags) {
    this->init(name, (float)iDefaultValue, flags, "", NULL);
    this->type = CONVAR_TYPE::CONVAR_TYPE_INT;
    _addConVar(this);
}

ConVar::ConVar(UString name, int iDefaultValue, int flags, ConVarChangeCallback callback) {
    this->init(name, (float)iDefaultValue, flags, "", callback);
    this->type = CONVAR_TYPE::CONVAR_TYPE_INT;
    _addConVar(this);
}

ConVar::ConVar(UString name, int iDefaultValue, int flags, const char *helpString) {
    this->init(name, (float)iDefaultValue, flags, UString(helpString), NULL);
    this->type = CONVAR_TYPE::CONVAR_TYPE_INT;
    _addConVar(this);
}

ConVar::ConVar(UString name, int iDefaultValue, int flags, const char *helpString, ConVarChangeCallback callback) {
    this->init(name, (float)iDefaultValue, flags, UString(helpString), callback);
    this->type = CONVAR_TYPE::CONVAR_TYPE_INT;
    _addConVar(this);
}

ConVar::ConVar(UString name, bool bDefaultValue, int flags) {
    this->init(name, bDefaultValue ? 1.0f : 0.0f, flags, "", NULL);
    this->type = CONVAR_TYPE::CONVAR_TYPE_BOOL;
    _addConVar(this);
}

ConVar::ConVar(UString name, bool bDefaultValue, int flags, ConVarChangeCallback callback) {
    this->init(name, bDefaultValue ? 1.0f : 0.0f, flags, "", callback);
    this->type = CONVAR_TYPE::CONVAR_TYPE_BOOL;
    _addConVar(this);
}

ConVar::ConVar(UString name, bool bDefaultValue, int flags, const char *helpString) {
    this->init(name, bDefaultValue ? 1.0f : 0.0f, flags, UString(helpString), NULL);
    this->type = CONVAR_TYPE::CONVAR_TYPE_BOOL;
    _addConVar(this);
}

ConVar::ConVar(UString name, bool bDefaultValue, int flags, const char *helpString, ConVarChangeCallback callback) {
    this->init(name, bDefaultValue ? 1.0f : 0.0f, flags, UString(helpString), callback);
    this->type = CONVAR_TYPE::CONVAR_TYPE_BOOL;
    _addConVar(this);
}

ConVar::ConVar(UString name, const char *sDefaultValue, int flags) {
    this->init(name, UString(sDefaultValue), flags, UString(""), NULL);
    _addConVar(this);
}

ConVar::ConVar(UString name, const char *sDefaultValue, int flags, const char *helpString) {
    this->init(name, UString(sDefaultValue), flags, UString(helpString), NULL);
    _addConVar(this);
}

ConVar::ConVar(UString name, const char *sDefaultValue, int flags, ConVarChangeCallback callback) {
    this->init(name, UString(sDefaultValue), flags, UString(""), callback);
    _addConVar(this);
}

ConVar::ConVar(UString name, const char *sDefaultValue, int flags, const char *helpString,
               ConVarChangeCallback callback) {
    this->init(name, UString(sDefaultValue), flags, UString(helpString), callback);
    _addConVar(this);
}

void ConVar::exec() {
    if(!this->isUnlocked()) return;

    if(osu != NULL) {
        auto is_vanilla = convar->isVanilla();

        auto beatmap = osu->getSelectedBeatmap();
        if(beatmap != NULL) {
            beatmap->vanilla &= is_vanilla;
        }

        auto mod_selector = osu->modSelector;
        if(mod_selector && mod_selector->nonVanillaWarning) {
            mod_selector->nonVanillaWarning->setVisible(!is_vanilla && bancho.submit_scores());
        }
    }

    if(this->callbackfunc != NULL) this->callbackfunc();
}

void ConVar::execArgs(UString args) {
    if(!this->isUnlocked()) return;

    if(this->callbackfuncargs != NULL) this->callbackfuncargs(args);
}

// Reset default values to the actual defaults (before neosu.json overrides)
void ConVar::resetDefaults() {
    this->iFlags = this->iDefaultFlags;
    this->fDefaultValue = this->fDefaultDefaultValue;
    this->sDefaultValue = this->sDefaultDefaultValue;
}

void ConVar::setDefaultFloat(float defaultValue) {
    if(this->isFlagSet(FCVAR_PRIVATE)) return;
    this->fDefaultValue = defaultValue;
    this->sDefaultValue = UString::format("%g", defaultValue);
}

void ConVar::setDefaultString(UString defaultValue) {
    if(this->isFlagSet(FCVAR_PRIVATE)) return;
    this->sDefaultValue = defaultValue;
}

void ConVar::setValue(float value, bool fire_callbacks) {
    if(!this->isUnlocked()) return;

    if(osu && this->isFlagSet(FCVAR_GAMEPLAY)) {
        if(bancho.is_playing_a_multi_map()) {
            debugLog("Can't edit %s while in a multiplayer match.\n", this->sName.toUtf8());
            return;
        } else {
            if(osu->isInPlayMode()) {
                debugLog("%s affects gameplay: won't submit score.\n", this->sName.toUtf8());
            }
            osu->getScore()->setCheated();
        }

        osu->getScore()->mods = Replay::Mods::from_cvars();
    }

    // TODO: make this less unsafe in multithreaded environments (for float convars at least)

    // backup previous value
    const float oldValue = this->fValue.load();

    // then set the new value
    const UString newStringValue = UString::format("%g", value);
    {
        this->fValue = value;
        this->sValue = newStringValue;
        this->bHasValue = true;
    }

    // handle callbacks
    if(fire_callbacks) {
        // possible void callback
        this->exec();

        // possible change callback
        if(this->changecallback != NULL) this->changecallback(UString::format("%g", oldValue), newStringValue);

        // possible arg callback
        this->execArgs(newStringValue);
    }
}

void ConVar::setValue(UString sValue, bool fire_callbacks) {
    if(!this->isUnlocked()) return;

    if(osu && this->isFlagSet(FCVAR_GAMEPLAY)) {
        if(bancho.is_playing_a_multi_map()) {
            debugLog("Can't edit %s while in a multiplayer match.\n", this->sName.toUtf8());
            return;
        } else {
            debugLog("%s affects gameplay: won't submit score.\n", this->sName.toUtf8());
            osu->getScore()->setCheated();
        }

        osu->getScore()->mods = Replay::Mods::from_cvars();
    }

    // backup previous value
    const UString oldValue = this->sValue;

    // then set the new value
    {
        this->sValue = sValue;
        this->bHasValue = true;

        if(sValue.length() > 0) this->fValue = sValue.toFloat();
    }

    // handle callbacks
    if(fire_callbacks) {
        // possible void callback
        this->exec();

        // possible change callback
        if(this->changecallback != NULL) this->changecallback(oldValue, sValue);

        // possible arg callback
        this->execArgs(sValue);
    }
}

void ConVar::setCallback(NativeConVarCallback callback) { this->callbackfunc = callback; }

void ConVar::setCallback(NativeConVarCallbackArgs callback) { this->callbackfuncargs = callback; }

void ConVar::setCallback(NativeConVarChangeCallback callback) { this->changecallback = callback; }

void ConVar::setHelpString(UString helpString) { this->sHelpString = helpString; }

//********************************//
//  ConVarHandler Implementation  //
//********************************//

ConVar _emptyDummyConVar(
    "emptyDummyConVar", 42.0f, FCVAR_BANCHO_COMPATIBLE,
    "this placeholder convar is returned by convar->getConVarByName() if no matching convar is found");

ConVarHandler *convar = new ConVarHandler();

ConVarHandler::ConVarHandler() { convar = this; }

ConVarHandler::~ConVarHandler() { convar = NULL; }

const std::vector<ConVar *> &ConVarHandler::getConVarArray() const { return _getGlobalConVarArray(); }

int ConVarHandler::getNumConVars() const { return _getGlobalConVarArray().size(); }

ConVar *ConVarHandler::getConVarByName(UString name, bool warnIfNotFound) const {
    ConVar *found = _getConVar(name);
    if(found != NULL) return found;

    if(warnIfNotFound) {
        UString errormsg = UString("ENGINE: ConVar \"");
        errormsg.append(name);
        errormsg.append("\" does not exist...\n");
        debugLog(errormsg.toUtf8());
        engine->showMessageWarning("Engine Error", errormsg);
    }

    if(!warnIfNotFound)
        return NULL;
    else
        return &_emptyDummyConVar;
}

std::vector<ConVar *> ConVarHandler::getConVarByLetter(UString letters) const {
    std::unordered_set<std::string> matchingConVarNames;
    std::vector<ConVar *> matchingConVars;
    {
        if(letters.length() < 1) return matchingConVars;

        const std::vector<ConVar *> &convars = this->getConVarArray();

        // first try matching exactly
        for(size_t i = 0; i < convars.size(); i++) {
            if(convars[i]->isFlagSet(FCVAR_HIDDEN)) continue;

            if(convars[i]->getName().find(letters, 0, letters.length()) == 0) {
                if(letters.length() > 1)
                    matchingConVarNames.insert(
                        std::string(convars[i]->getName().toUtf8(), convars[i]->getName().lengthUtf8()));

                matchingConVars.push_back(convars[i]);
            }
        }

        // then try matching substrings
        if(letters.length() > 1) {
            for(size_t i = 0; i < convars.size(); i++) {
                if(convars[i]->isFlagSet(FCVAR_HIDDEN)) continue;

                if(convars[i]->getName().find(letters) != -1) {
                    std::string stdName(convars[i]->getName().toUtf8(), convars[i]->getName().lengthUtf8());
                    if(matchingConVarNames.find(stdName) == matchingConVarNames.end()) {
                        matchingConVarNames.insert(stdName);
                        matchingConVars.push_back(convars[i]);
                    }
                }
            }
        }

        // (results should be displayed in vector order)
    }
    return matchingConVars;
}

UString ConVarHandler::flagsToString(int flags) {
    UString string;
    {
        if(flags == 0) {
            string.append("no flags");
        } else {
            if(flags & FCVAR_HIDDEN) string.append(string.length() > 0 ? " hidden" : "hidden");
            if(flags & FCVAR_BANCHO_SUBMITTABLE)
                string.append(string.length() > 0 ? " bancho_submittable" : "bancho_submittable");
            if(flags & FCVAR_BANCHO_COMPATIBLE)
                string.append(string.length() > 0 ? " bancho_compatible" : "bancho_compatible");
        }
    }
    return string;
}

bool ConVarHandler::isVanilla() {
    for(auto cv : _getGlobalConVarArray()) {
        if(cv->isFlagSet(FCVAR_BANCHO_SUBMITTABLE)) continue;
        if(cv->getString() != cv->getDefaultString()) {
            return false;
        }
    }

    // Also check for non-vanilla mod combinations here while we're at it
    if(osu != NULL) {
        // We don't want to submit target scores, even though it's allowed in multiplayer
        if(osu->getModTarget()) return false;

        if(osu->getModNightmare()) return false;
        if(osu->getModEZ() && osu->getModHR()) return false;
        f32 speed = cv_speed_override.getFloat();
        if(speed != -1.f && speed != 0.75 && speed != 1.0 && speed != 1.5) return false;
    }

    return true;
}

//*****************************//
//	ConVarHandler ConCommands  //
//*****************************//

static void _find(UString args) {
    if(args.length() < 1) {
        debugLog("Usage:  find <string>");
        return;
    }

    const std::vector<ConVar *> &convars = convar->getConVarArray();

    std::vector<ConVar *> matchingConVars;
    for(size_t i = 0; i < convars.size(); i++) {
        if(convars[i]->isFlagSet(FCVAR_HIDDEN)) continue;

        const UString name = convars[i]->getName();
        if(name.find(args, 0, name.length()) != -1) matchingConVars.push_back(convars[i]);
    }

    if(matchingConVars.size() > 0) {
        struct CONVAR_SORT_COMPARATOR {
            bool operator()(const ConVar *var1, const ConVar *var2) { return (var1->getName() < var2->getName()); }
        };
        std::sort(matchingConVars.begin(), matchingConVars.end(), CONVAR_SORT_COMPARATOR());
    }

    if(matchingConVars.size() < 1) {
        UString thelog = "No commands found containing \"";
        thelog.append(args);
        thelog.append("\".\n");
        debugLog("%s", thelog.toUtf8());
        return;
    }

    debugLog("----------------------------------------------\n");
    {
        UString thelog = "[ find : ";
        thelog.append(args);
        thelog.append(" ]\n");
        debugLog("%s", thelog.toUtf8());

        for(size_t i = 0; i < matchingConVars.size(); i++) {
            UString tstring = matchingConVars[i]->getName();
            tstring.append("\n");
            debugLog("%s", tstring.toUtf8());
        }
    }
    debugLog("----------------------------------------------\n");
}

static void _help(UString args) {
    args = args.trim();

    if(args.length() < 1) {
        debugLog("Usage:  help <cvarname>\nTo get a list of all available commands, type \"listcommands\".\n");
        return;
    }

    const std::vector<ConVar *> matches = convar->getConVarByLetter(args);

    if(matches.size() < 1) {
        UString thelog = "ConVar \"";
        thelog.append(args);
        thelog.append("\" does not exist.\n");
        debugLog("%s", thelog.toUtf8());
        return;
    }

    // use closest match
    size_t index = 0;
    for(size_t i = 0; i < matches.size(); i++) {
        if(matches[i]->getName() == args) {
            index = i;
            break;
        }
    }
    ConVar *match = matches[index];

    if(match->getHelpstring().length() < 1) {
        UString thelog = "ConVar \"";
        thelog.append(match->getName());
        thelog.append("\" does not have a helpstring.\n");
        debugLog("%s", thelog.toUtf8());
        return;
    }

    UString thelog = match->getName();
    {
        if(match->hasValue()) {
            auto &cv_str = match->getString();
            auto &default_str = match->getDefaultString();
            thelog.append(UString::format(" = %s ( def. \"%s\" , ", cv_str.toUtf8(), default_str.toUtf8()));
            thelog.append(ConVar::typeToString(match->getType()));
            thelog.append(", ");
            thelog.append(ConVarHandler::flagsToString(match->getFlags()));
            thelog.append(" )");
        }

        thelog.append(" - ");
        thelog.append(match->getHelpstring());
    }
    debugLog("%s", thelog.toUtf8());
}

static void _listcommands(void) {
    debugLog("----------------------------------------------\n");
    {
        std::vector<ConVar *> convars = convar->getConVarArray();
        struct CONVAR_SORT_COMPARATOR {
            bool operator()(ConVar const *var1, ConVar const *var2) { return (var1->getName() < var2->getName()); }
        };
        std::sort(convars.begin(), convars.end(), CONVAR_SORT_COMPARATOR());

        for(size_t i = 0; i < convars.size(); i++) {
            if(convars[i]->isFlagSet(FCVAR_HIDDEN)) continue;

            ConVar *var = convars[i];

            UString tstring = var->getName();
            {
                if(var->hasValue()) {
                    auto var_str = var->getString();
                    auto default_str = var->getDefaultString();
                    tstring.append(UString::format(" = %s ( def. \"%s\" , ", var_str.toUtf8(), default_str.toUtf8()));
                    tstring.append(ConVar::typeToString(var->getType()));
                    tstring.append(", ");
                    tstring.append(ConVarHandler::flagsToString(var->getFlags()));
                    tstring.append(" )");
                }

                if(var->getHelpstring().length() > 0) {
                    tstring.append(" - ");
                    tstring.append(var->getHelpstring());
                }

                tstring.append("\n");
            }
            debugLog("%s", tstring.toUtf8());
        }
    }
    debugLog("----------------------------------------------\n");
}

static void _dumpcommands(void) {
    std::vector<ConVar *> convars = convar->getConVarArray();
    struct CONVAR_SORT_COMPARATOR {
        bool operator()(ConVar const *var1, ConVar const *var2) { return (var1->getName() < var2->getName()); }
    };
    std::sort(convars.begin(), convars.end(), CONVAR_SORT_COMPARATOR());

    FILE *file = fopen("commands.htm", "w");
    if(file == NULL) {
        debugLog("Failed to open commands.htm for writing\n");
        return;
    }

    for(size_t i = 0; i < convars.size(); i++) {
        ConVar *var = convars[i];
        if(!var->hasValue()) continue;
        if(var->isFlagSet(FCVAR_HIDDEN)) continue;
        if(var->isFlagSet(FCVAR_PRIVATE)) continue;

        std::string cmd = "<h4>";
        cmd.append(var->getName().toUtf8());
        cmd.append("</h4>\n");
        cmd.append(var->getHelpstring().toUtf8());
        cmd.append("<pre>{");
        cmd.append("\n    \"default\": ");
        cmd.append(var->getFancyDefaultValue());
        cmd.append("\n    \"bancho_submittable\": ");
        cmd.append(var->isFlagSet(FCVAR_BANCHO_SUBMITTABLE) ? "true" : "false");
        cmd.append("\n    \"bancho_compatible\": ");
        cmd.append(var->isFlagSet(FCVAR_BANCHO_COMPATIBLE) ? "true" : "false");
        cmd.append("\n}</pre>\n");

        fwrite(cmd.c_str(), cmd.size(), 1, file);
    }

    fflush(file);
    fclose(file);

    debugLog("Commands dumped to commands.htm\n");
}

void _exec(UString args) { Console::execConfigFile(args.toUtf8()); }

void _echo(UString args) {
    if(args.length() > 0) {
        args.append("\n");
        debugLog("%s", args.toUtf8());
    }
}

void _volume(UString oldValue, UString newValue) {
    (void)oldValue;
    soundEngine->setVolume(newValue.toFloat());
}

void _RESTART_SOUND_ENGINE_ON_CHANGE(UString oldValue, UString newValue) {
    const int oldValueMS = std::round(oldValue.toFloat() * 1000.0f);
    const int newValueMS = std::round(newValue.toFloat() * 1000.0f);

    if(oldValueMS != newValueMS) soundEngine->restart();
}

void _vprof(UString oldValue, UString newValue) {
    const bool enable = (newValue.toFloat() > 0.0f);

    if(enable != g_profCurrentProfile.isEnabled()) {
        if(enable)
            g_profCurrentProfile.start();
        else
            g_profCurrentProfile.stop();
    }
}

void _osuOptionsSliderQualityWrapper(UString oldValue, UString newValue) {
    float value = std::lerp(1.0f, 2.5f, 1.0f - newValue.toFloat());
    cv_slider_curve_points_separation.setValue(value);
};

void spectate_by_username(UString username) {
    auto user = find_user(username);
    if(user == NULL) {
        debugLog("Couldn't find user \"%s\"!", username.toUtf8());
        return;
    }

    debugLog("Spectating %s (user %d)...\n", username.toUtf8(), user->user_id);
    start_spectating(user->user_id);
}

void _vsync(UString oldValue, UString newValue) {
    if(newValue.length() < 1)
        debugLog("Usage: 'vsync 1' to turn vsync on, 'vsync 0' to turn vsync off\n");
    else {
        bool vsync = newValue.toFloat() > 0.0f;
        g->setVSync(vsync);
    }
}

void _fullscreen_windowed_borderless(UString oldValue, UString newValue) {
    env->setFullscreenWindowedBorderless(newValue.toFloat() > 0.0f);
}

void _monitor(UString oldValue, UString newValue) { env->setMonitor(newValue.toInt()); }

void _osu_songbrowser_search_hardcoded_filter(UString oldValue, UString newValue) {
    if(newValue.length() == 1 && newValue.isWhitespaceOnly()) cv_songbrowser_search_hardcoded_filter.setValue("");
}

void loudness_cb(UString oldValue, UString newValue) {
    // Restart loudness calc.
    loct_abort();
    if(db && cv_normalize_loudness.getBool()) {
        loct_calc(db->loudness_to_calc);
    }
}

void _save(void) { db->save(); }

#undef CONVARDEFS_H
#define DEFINE_CONVARS
#include "ConVarDefs.h"

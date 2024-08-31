#include "ConVar.h"

#include "Bancho.h"
#include "BanchoUsers.h"
#include "Beatmap.h"
#include "CBaseUILabel.h"
#include "Console.h"
#include "Engine.h"
#include "Keyboard.h"
#include "ModSelector.h"
#include "Osu.h"
#include "Profiler.h"
#include "RichPresence.h"
#include "SoundEngine.h"
#include "SpectatorScreen.h"

bool ConVar::isUnlocked() const {
    if(isFlagSet(FCVAR_PRIVATE)) return true;
    if(!bancho.is_online()) return true;

    if(bancho.is_in_a_multi_room()) {
        return isFlagSet(FCVAR_UNLOCK_MULTIPLAYER);
    } else {
        return isFlagSet(FCVAR_UNLOCK_SINGLEPLAYER);
    }
}

bool ConVar::getBool() const { return getFloat() > 0; }
int ConVar::getInt() { return (int)getFloat(); }
float ConVar::getFloat() const { return isUnlocked() ? m_fValue.load() : m_fDefaultValue.load(); }
const UString &ConVar::getString() { return isUnlocked() ? m_sValue : m_sDefaultValue; }

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
    switch(getType()) {
        case ConVar::CONVAR_TYPE::CONVAR_TYPE_BOOL:
            return m_fDefaultDefaultValue == 0 ? "false" : "true";
        case ConVar::CONVAR_TYPE::CONVAR_TYPE_INT:
            return std::to_string((int)m_fDefaultDefaultValue);
        case ConVar::CONVAR_TYPE::CONVAR_TYPE_FLOAT:
            return std::to_string(m_fDefaultDefaultValue);
        case ConVar::CONVAR_TYPE::CONVAR_TYPE_STRING: {
            std::string out = "\"";
            out.append(m_sDefaultDefaultValue.toUtf8());
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
    m_callbackfunc = NULL;
    m_callbackfuncargs = NULL;
    m_changecallback = NULL;

    m_fValue = 0.0f;
    m_fDefaultDefaultValue = 0.0f;
    m_fDefaultValue = 0.0f;
    m_sDefaultDefaultValue = "";
    m_sDefaultValue = "";

    m_bHasValue = false;
    m_type = CONVAR_TYPE::CONVAR_TYPE_FLOAT;
    m_iDefaultFlags = flags;
    m_iFlags = flags;
}

void ConVar::init(UString &name, int flags) {
    init(flags);

    m_sName = name;
    m_type = CONVAR_TYPE::CONVAR_TYPE_STRING;
}

void ConVar::init(UString &name, int flags, ConVarCallback callback) {
    init(flags);

    m_sName = name;
    m_callbackfunc = callback;
    m_type = CONVAR_TYPE::CONVAR_TYPE_STRING;
}

void ConVar::init(UString &name, int flags, UString helpString, ConVarCallback callback) {
    init(name, flags, callback);

    m_sHelpString = helpString;
}

void ConVar::init(UString &name, int flags, ConVarCallbackArgs callbackARGS) {
    init(flags);

    m_sName = name;
    m_callbackfuncargs = callbackARGS;
    m_type = CONVAR_TYPE::CONVAR_TYPE_STRING;
}

void ConVar::init(UString &name, int flags, UString helpString, ConVarCallbackArgs callbackARGS) {
    init(name, flags, callbackARGS);

    m_sHelpString = helpString;
}

void ConVar::init(UString &name, float defaultValue, int flags, UString helpString, ConVarChangeCallback callback) {
    init(flags);

    m_type = CONVAR_TYPE::CONVAR_TYPE_FLOAT;
    m_sName = name;
    m_fDefaultValue = defaultValue;
    m_fDefaultDefaultValue = defaultValue;
    m_sDefaultValue = UString::format("%g", defaultValue);
    m_sDefaultDefaultValue = m_sDefaultValue;
    setValue(defaultValue);
    m_sHelpString = helpString;
    m_changecallback = callback;
}

void ConVar::init(UString &name, UString defaultValue, int flags, UString helpString, ConVarChangeCallback callback) {
    init(flags);

    m_type = CONVAR_TYPE::CONVAR_TYPE_STRING;
    m_sName = name;
    m_sDefaultValue = defaultValue;
    m_sDefaultDefaultValue = defaultValue;
    setValue(defaultValue);
    m_sHelpString = helpString;
    m_changecallback = callback;
}

ConVar::ConVar(UString name) {
    init(name, FCVAR_DEFAULT);
    _addConVar(this);
}

ConVar::ConVar(UString name, int flags, ConVarCallback callback) {
    init(name, flags, callback);
    _addConVar(this);
}

ConVar::ConVar(UString name, int flags, const char *helpString, ConVarCallback callback) {
    init(name, flags, helpString, callback);
    _addConVar(this);
}

ConVar::ConVar(UString name, int flags, ConVarCallbackArgs callbackARGS) {
    init(name, flags, callbackARGS);
    _addConVar(this);
}

ConVar::ConVar(UString name, int flags, const char *helpString, ConVarCallbackArgs callbackARGS) {
    init(name, flags, helpString, callbackARGS);
    _addConVar(this);
}

ConVar::ConVar(UString name, float fDefaultValue, int flags) {
    init(name, fDefaultValue, flags, UString(""), NULL);
    _addConVar(this);
}

ConVar::ConVar(UString name, float fDefaultValue, int flags, ConVarChangeCallback callback) {
    init(name, fDefaultValue, flags, UString(""), callback);
    _addConVar(this);
}

ConVar::ConVar(UString name, float fDefaultValue, int flags, const char *helpString) {
    init(name, fDefaultValue, flags, UString(helpString), NULL);
    _addConVar(this);
}

ConVar::ConVar(UString name, float fDefaultValue, int flags, const char *helpString, ConVarChangeCallback callback) {
    init(name, fDefaultValue, flags, UString(helpString), callback);
    _addConVar(this);
}

ConVar::ConVar(UString name, int iDefaultValue, int flags) {
    init(name, (float)iDefaultValue, flags, "", NULL);
    m_type = CONVAR_TYPE::CONVAR_TYPE_INT;
    _addConVar(this);
}

ConVar::ConVar(UString name, int iDefaultValue, int flags, ConVarChangeCallback callback) {
    init(name, (float)iDefaultValue, flags, "", callback);
    m_type = CONVAR_TYPE::CONVAR_TYPE_INT;
    _addConVar(this);
}

ConVar::ConVar(UString name, int iDefaultValue, int flags, const char *helpString) {
    init(name, (float)iDefaultValue, flags, UString(helpString), NULL);
    m_type = CONVAR_TYPE::CONVAR_TYPE_INT;
    _addConVar(this);
}

ConVar::ConVar(UString name, int iDefaultValue, int flags, const char *helpString, ConVarChangeCallback callback) {
    init(name, (float)iDefaultValue, flags, UString(helpString), callback);
    m_type = CONVAR_TYPE::CONVAR_TYPE_INT;
    _addConVar(this);
}

ConVar::ConVar(UString name, bool bDefaultValue, int flags) {
    init(name, bDefaultValue ? 1.0f : 0.0f, flags, "", NULL);
    m_type = CONVAR_TYPE::CONVAR_TYPE_BOOL;
    _addConVar(this);
}

ConVar::ConVar(UString name, bool bDefaultValue, int flags, ConVarChangeCallback callback) {
    init(name, bDefaultValue ? 1.0f : 0.0f, flags, "", callback);
    m_type = CONVAR_TYPE::CONVAR_TYPE_BOOL;
    _addConVar(this);
}

ConVar::ConVar(UString name, bool bDefaultValue, int flags, const char *helpString) {
    init(name, bDefaultValue ? 1.0f : 0.0f, flags, UString(helpString), NULL);
    m_type = CONVAR_TYPE::CONVAR_TYPE_BOOL;
    _addConVar(this);
}

ConVar::ConVar(UString name, bool bDefaultValue, int flags, const char *helpString, ConVarChangeCallback callback) {
    init(name, bDefaultValue ? 1.0f : 0.0f, flags, UString(helpString), callback);
    m_type = CONVAR_TYPE::CONVAR_TYPE_BOOL;
    _addConVar(this);
}

ConVar::ConVar(UString name, const char *sDefaultValue, int flags) {
    init(name, UString(sDefaultValue), flags, UString(""), NULL);
    _addConVar(this);
}

ConVar::ConVar(UString name, const char *sDefaultValue, int flags, const char *helpString) {
    init(name, UString(sDefaultValue), flags, UString(helpString), NULL);
    _addConVar(this);
}

ConVar::ConVar(UString name, const char *sDefaultValue, int flags, ConVarChangeCallback callback) {
    init(name, UString(sDefaultValue), flags, UString(""), callback);
    _addConVar(this);
}

ConVar::ConVar(UString name, const char *sDefaultValue, int flags, const char *helpString,
               ConVarChangeCallback callback) {
    init(name, UString(sDefaultValue), flags, UString(helpString), callback);
    _addConVar(this);
}

void ConVar::exec() {
    if(!isUnlocked()) return;

    if(osu != NULL) {
        auto is_vanilla = convar->isVanilla();

        auto beatmap = osu->getSelectedBeatmap();
        if(beatmap != NULL) {
            beatmap->vanilla &= is_vanilla;
        }

        auto mod_selector = osu->m_modSelector;
        if(mod_selector && mod_selector->m_nonVanillaWarning) {
            mod_selector->m_nonVanillaWarning->setVisible(!is_vanilla && bancho.submit_scores());
        }
    }

    if(m_callbackfunc != NULL) m_callbackfunc();
}

void ConVar::execArgs(UString args) {
    if(!isUnlocked()) return;

    if(m_callbackfuncargs != NULL) m_callbackfuncargs(args);
}

// Reset default values to the actual defaults (before neosu.json overrides)
void ConVar::resetDefaults() {
    m_iFlags = m_iDefaultFlags;
    m_fDefaultValue = m_fDefaultDefaultValue;
    m_sDefaultValue = m_sDefaultDefaultValue;
}

void ConVar::setDefaultFloat(float defaultValue) {
    if(isFlagSet(FCVAR_PRIVATE)) return;
    m_fDefaultValue = defaultValue;
    m_sDefaultValue = UString::format("%g", defaultValue);
}

void ConVar::setDefaultString(UString defaultValue) {
    if(isFlagSet(FCVAR_PRIVATE)) return;
    m_sDefaultValue = defaultValue;
}

void ConVar::setValue(float value) {
    if(!isUnlocked()) return;

    // TODO: make this less unsafe in multithreaded environments (for float convars at least)

    // backup previous value
    const float oldValue = m_fValue.load();

    // then set the new value
    const UString newStringValue = UString::format("%g", value);
    {
        m_fValue = value;
        m_sValue = newStringValue;
        m_bHasValue = true;
    }

    // handle callbacks
    {
        // possible void callback
        exec();

        // possible change callback
        if(m_changecallback != NULL) m_changecallback(UString::format("%g", oldValue), newStringValue);

        // possible arg callback
        execArgs(newStringValue);
    }
}

void ConVar::setValue(UString sValue) {
    if(!isUnlocked()) return;

    // backup previous value
    const UString oldValue = m_sValue;

    // then set the new value
    {
        m_sValue = sValue;
        m_bHasValue = true;

        if(sValue.length() > 0) m_fValue = sValue.toFloat();
    }

    // handle callbacks
    {
        // possible void callback
        exec();

        // possible change callback
        if(m_changecallback != NULL) m_changecallback(oldValue, sValue);

        // possible arg callback
        execArgs(sValue);
    }
}

void ConVar::setCallback(NativeConVarCallback callback) { m_callbackfunc = callback; }

void ConVar::setCallback(NativeConVarCallbackArgs callback) { m_callbackfuncargs = callback; }

void ConVar::setCallback(NativeConVarChangeCallback callback) { m_changecallback = callback; }

void ConVar::setHelpString(UString helpString) { m_sHelpString = helpString; }

//********************************//
//  ConVarHandler Implementation  //
//********************************//

ConVar _emptyDummyConVar(
    "emptyDummyConVar", 42.0f, FCVAR_DEFAULT,
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

        const std::vector<ConVar *> &convars = getConVarArray();

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
            if(flags & FCVAR_UNLOCK_SINGLEPLAYER) string.append(string.length() > 0 ? " unlock_solo" : "unlock_solo");
            if(flags & FCVAR_UNLOCK_MULTIPLAYER) string.append(string.length() > 0 ? " unlock_multi" : "unlock_multi");
            if(flags & FCVAR_ALWAYS_SUBMIT) string.append(string.length() > 0 ? " always_submit" : "always_submit");
        }
    }
    return string;
}

bool ConVarHandler::isVanilla() {
    for(auto cv : _getGlobalConVarArray()) {
        if(cv->isFlagSet(FCVAR_ALWAYS_SUBMIT)) continue;
        if(cv->getString() != cv->getDefaultString()) {
            return false;
        }
    }

    // Also check for non-vanilla mod combinations here while we're at it
    if(osu != NULL) {
        if(osu->getModTarget()) return false;
        if(osu->getModNightmare()) return false;
        if(osu->getModEZ() && osu->getModHR()) return false;
        if((osu->getModDT() || osu->getModNC()) && (osu->getModHT() || osu->getModDC())) return false;
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
        cmd.append(var->getName());
        cmd.append("</h4>\n");
        cmd.append(var->getHelpstring().toUtf8());
        cmd.append("<pre>{");
        cmd.append("\n    \"default\": ");
        cmd.append(var->getFancyDefaultValue());
        cmd.append("\n    \"unlock_singleplayer\": ");
        cmd.append(var->isFlagSet(FCVAR_UNLOCK_SINGLEPLAYER) ? "true" : "false");
        cmd.append("\n    \"unlock_multiplayer\": ");
        cmd.append(var->isFlagSet(FCVAR_UNLOCK_MULTIPLAYER) ? "true" : "false");
        cmd.append("\n    \"always_submit\": ");
        cmd.append(var->isFlagSet(FCVAR_ALWAYS_SUBMIT) ? "true" : "false");
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
    engine->getSound()->setVolume(newValue.toFloat());
}

void _RESTART_SOUND_ENGINE_ON_CHANGE(UString oldValue, UString newValue) {
    const int oldValueMS = std::round(oldValue.toFloat() * 1000.0f);
    const int newValueMS = std::round(newValue.toFloat() * 1000.0f);

    if(oldValueMS != newValueMS) engine->getSound()->restart();
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
    float value = lerp<float>(1.0f, 2.5f, 1.0f - newValue.toFloat());
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
        engine->getGraphics()->setVSync(vsync);
    }
}

void _mat_wireframe(UString oldValue, UString newValue) {
    engine->getGraphics()->setWireframe(newValue.toFloat() > 0.0f);
}
void _fullscreen_windowed_borderless(UString oldValue, UString newValue) {
    env->setFullscreenWindowedBorderless(newValue.toFloat() > 0.0f);
}

void _monitor(UString oldValue, UString newValue) { env->setMonitor(newValue.toInt()); }

void _host_timescale_(UString oldValue, UString newValue) {
    if(newValue.toFloat() < 0.01f) {
        debugLog(0xffff4444, UString::format("Value must be >= 0.01!\n").toUtf8());
        cv_host_timescale.setValue(1.0f);
    }
}

void _osu_songbrowser_search_hardcoded_filter(UString oldValue, UString newValue) {
    if(newValue.length() == 1 && newValue.isWhitespaceOnly()) cv_songbrowser_search_hardcoded_filter.setValue("");
}

ConVar cmd_borderless("borderless", FCVAR_DEFAULT, _borderless);
ConVar cmd_center("center", FCVAR_DEFAULT, _center);
ConVar cmd_clear("clear");
ConVar cmd_dpiinfo("dpiinfo", FCVAR_DEFAULT, _dpiinfo);
ConVar cmd_dumpcommands("dumpcommands", FCVAR_HIDDEN, _dumpcommands);
ConVar cmd_echo("echo", FCVAR_DEFAULT, _echo);
ConVar cmd_errortest("errortest", FCVAR_DEFAULT, _errortest);
ConVar cmd_exec("exec", FCVAR_DEFAULT, _exec);
ConVar cmd_exit("exit", FCVAR_DEFAULT, _exit);
ConVar cmd_find("find", FCVAR_DEFAULT, _find);
ConVar cmd_focus("focus", FCVAR_DEFAULT, _focus);
ConVar cmd_fullscreen("fullscreen", FCVAR_DEFAULT, _fullscreen);
ConVar cmd_help("help", FCVAR_DEFAULT, _help);
ConVar cmd_listcommands("listcommands", FCVAR_DEFAULT, _listcommands);
ConVar cmd_maximize("maximize", FCVAR_DEFAULT, _maximize);
ConVar cmd_minimize("minimize", FCVAR_DEFAULT, _minimize);
ConVar cmd_printsize("printsize", FCVAR_DEFAULT, _printsize);
ConVar cmd_resizable_toggle("resizable_toggle", FCVAR_DEFAULT, _toggleresizable);
ConVar cmd_restart("restart", FCVAR_DEFAULT, _restart);
ConVar cmd_showconsolebox("showconsolebox");
ConVar cmd_shutdown("shutdown", FCVAR_DEFAULT, _exit);
ConVar cmd_spectate("spectate", FCVAR_HIDDEN, spectate_by_username);
ConVar cmd_windowed("windowed", FCVAR_DEFAULT, _windowed);

ConVar cv_BOSS_KEY("osu_key_boss", (int)KEY_INSERT, FCVAR_DEFAULT);
ConVar cv_DECREASE_LOCAL_OFFSET("osu_key_decrease_local_offset", (int)KEY_SUBTRACT, FCVAR_DEFAULT);
ConVar cv_DECREASE_VOLUME("osu_key_decrease_volume", (int)KEY_DOWN, FCVAR_DEFAULT);
ConVar cv_DISABLE_MOUSE_BUTTONS("osu_key_disable_mouse_buttons", (int)KEY_F10, FCVAR_DEFAULT);
ConVar cv_FPOSU_ZOOM("osu_key_fposu_zoom", 0, FCVAR_DEFAULT);
ConVar cv_GAME_PAUSE("osu_key_game_pause", (int)KEY_ESCAPE, FCVAR_DEFAULT);
ConVar cv_INCREASE_LOCAL_OFFSET("osu_key_increase_local_offset", (int)KEY_ADD, FCVAR_DEFAULT);
ConVar cv_INCREASE_VOLUME("osu_key_increase_volume", (int)KEY_UP, FCVAR_DEFAULT);
ConVar cv_INSTANT_REPLAY("osu_key_instant_replay", (int)KEY_F2, FCVAR_DEFAULT);
ConVar cv_LEFT_CLICK("osu_key_left_click", (int)KEY_Z, FCVAR_DEFAULT);
ConVar cv_LEFT_CLICK_2("osu_key_left_click_2", 0, FCVAR_DEFAULT);
ConVar cv_MOD_AUTO("osu_key_mod_auto", (int)KEY_V, FCVAR_DEFAULT);
ConVar cv_MOD_AUTOPILOT("osu_key_mod_autopilot", (int)KEY_X, FCVAR_DEFAULT);
ConVar cv_MOD_DOUBLETIME("osu_key_mod_doubletime", (int)KEY_D, FCVAR_DEFAULT);
ConVar cv_MOD_EASY("osu_key_mod_easy", (int)KEY_Q, FCVAR_DEFAULT);
ConVar cv_MOD_FLASHLIGHT("osu_key_mod_flashlight", (int)KEY_G, FCVAR_DEFAULT);
ConVar cv_MOD_HALFTIME("osu_key_mod_halftime", (int)KEY_E, FCVAR_DEFAULT);
ConVar cv_MOD_HARDROCK("osu_key_mod_hardrock", (int)KEY_A, FCVAR_DEFAULT);
ConVar cv_MOD_HIDDEN("osu_key_mod_hidden", (int)KEY_F, FCVAR_DEFAULT);
ConVar cv_MOD_NOFAIL("osu_key_mod_nofail", (int)KEY_W, FCVAR_DEFAULT);
ConVar cv_MOD_RELAX("osu_key_mod_relax", (int)KEY_Z, FCVAR_DEFAULT);
ConVar cv_MOD_SCOREV2("osu_key_mod_scorev2", (int)KEY_B, FCVAR_DEFAULT);
ConVar cv_MOD_SPUNOUT("osu_key_mod_spunout", (int)KEY_C, FCVAR_DEFAULT);
ConVar cv_MOD_SUDDENDEATH("osu_key_mod_suddendeath", (int)KEY_S, FCVAR_DEFAULT);
ConVar cv_OPEN_SKIN_SELECT_MENU("key_open_skin_select_menu", 0, FCVAR_DEFAULT);
ConVar cv_QUICK_LOAD("osu_key_quick_load", (int)KEY_F7, FCVAR_DEFAULT);
ConVar cv_QUICK_RETRY("osu_key_quick_retry", (int)KEY_BACKSPACE, FCVAR_DEFAULT);
ConVar cv_QUICK_SAVE("osu_key_quick_save", (int)KEY_F6, FCVAR_DEFAULT);
ConVar cv_RANDOM_BEATMAP("osu_key_random_beatmap", (int)KEY_F2, FCVAR_DEFAULT);
ConVar cv_RIGHT_CLICK("osu_key_right_click", (int)KEY_X, FCVAR_DEFAULT);
ConVar cv_RIGHT_CLICK_2("osu_key_right_click_2", 0, FCVAR_DEFAULT);
ConVar cv_SAVE_SCREENSHOT("osu_key_save_screenshot", (int)KEY_F12, FCVAR_DEFAULT);
ConVar cv_SEEK_TIME("osu_key_seek_time", (int)KEY_SHIFT, FCVAR_DEFAULT);
ConVar cv_SEEK_TIME_BACKWARD("osu_key_seek_time_backward", (int)KEY_LEFT, FCVAR_DEFAULT);
ConVar cv_SEEK_TIME_FORWARD("osu_key_seek_time_forward", (int)KEY_RIGHT, FCVAR_DEFAULT);
ConVar cv_SKIP_CUTSCENE("osu_key_skip_cutscene", (int)KEY_SPACE, FCVAR_DEFAULT);
ConVar cv_TOGGLE_CHAT("osu_key_toggle_chat", (int)KEY_F8, FCVAR_DEFAULT);
ConVar cv_TOGGLE_MAP_BACKGROUND("key_toggle_map_background", 0, FCVAR_DEFAULT);
ConVar cv_TOGGLE_MODSELECT("osu_key_toggle_modselect", (int)KEY_F1, FCVAR_DEFAULT);
ConVar cv_TOGGLE_SCOREBOARD("osu_key_toggle_scoreboard", (int)KEY_TAB, FCVAR_DEFAULT);
ConVar cv_alt_f4_quits_even_while_playing("osu_alt_f4_quits_even_while_playing", true, FCVAR_DEFAULT);
ConVar cv_always_render_cursor_trail("always_render_cursor_trail", true, FCVAR_DEFAULT,
                                     "always render the cursor trail, even when not moving the cursor");
ConVar cv_animation_speed_override("osu_animation_speed_override", -1.0f, FCVAR_LOCKED);
ConVar cv_approach_circle_alpha_multiplier("osu_approach_circle_alpha_multiplier", 0.9f, FCVAR_DEFAULT);
ConVar cv_approach_scale_multiplier("osu_approach_scale_multiplier", 3.0f, FCVAR_DEFAULT);
ConVar cv_approachtime_max("osu_approachtime_max", 450, FCVAR_LOCKED);
ConVar cv_approachtime_mid("osu_approachtime_mid", 1200, FCVAR_LOCKED);
ConVar cv_approachtime_min("osu_approachtime_min", 1800, FCVAR_LOCKED);
ConVar cv_ar_override("osu_ar_override", -1.0f, FCVAR_UNLOCKED,
                      "use this to override between AR 0 and AR 12.5+. active if value is more than or equal to 0.");
ConVar cv_ar_override_lock("osu_ar_override_lock", false, FCVAR_UNLOCKED,
                           "always force constant AR even through speed changes");
ConVar cv_ar_overridenegative("osu_ar_overridenegative", 0.0f, FCVAR_UNLOCKED,
                              "use this to override below AR 0. active if value is less than 0, disabled otherwise. "
                              "this override always overrides the other override.");
ConVar cv_asio_buffer_size("asio_buffer_size", -1, FCVAR_DEFAULT | FCVAR_PRIVATE,
                           "buffer size in samples (usually 44100 samples per second)");
ConVar cv_auto_and_relax_block_user_input("osu_auto_and_relax_block_user_input", true, FCVAR_DEFAULT);
ConVar cv_auto_cursordance("osu_auto_cursordance", false, FCVAR_DEFAULT);
ConVar cv_auto_snapping_strength("osu_auto_snapping_strength", 1.0f, FCVAR_DEFAULT,
                                 "How many iterations of quadratic interpolation to use, more = snappier, 0 = linear");
ConVar cv_auto_update("auto_update", true, FCVAR_DEFAULT);
ConVar cv_automatic_cursor_size("osu_automatic_cursor_size", false, FCVAR_DEFAULT);
ConVar cv_autopilot_lenience("osu_autopilot_lenience", 0.75f, FCVAR_DEFAULT);
ConVar cv_autopilot_snapping_strength(
    "osu_autopilot_snapping_strength", 2.0f, FCVAR_DEFAULT,
    "How many iterations of quadratic interpolation to use, more = snappier, 0 = linear");
ConVar cv_avoid_flashes("avoid_flashes", false, FCVAR_DEFAULT,
                        "disable flashing elements (like FL dimming on sliders)");
ConVar cv_background_brightness("osu_background_brightness", 0.0f, FCVAR_DEFAULT,
                                "0 to 1, if this is larger than 0 then it will replace the entire beatmap background "
                                "image with a solid color (see osu_background_color_r/g/b)");
ConVar cv_background_color_b("osu_background_color_b", 255.0f, FCVAR_DEFAULT,
                             "0 to 255, only relevant if osu_background_brightness is larger than 0");
ConVar cv_background_color_g("osu_background_color_g", 255.0f, FCVAR_DEFAULT,
                             "0 to 255, only relevant if osu_background_brightness is larger than 0");
ConVar cv_background_color_r("osu_background_color_r", 255.0f, FCVAR_DEFAULT,
                             "0 to 255, only relevant if osu_background_brightness is larger than 0");
ConVar cv_background_dim("osu_background_dim", 0.9f, FCVAR_DEFAULT);
ConVar cv_background_dont_fade_during_breaks("osu_background_dont_fade_during_breaks", false, FCVAR_DEFAULT);
ConVar cv_background_fade_after_load("osu_background_fade_after_load", true, FCVAR_DEFAULT);
ConVar cv_background_fade_in_duration("osu_background_fade_in_duration", 0.85f, FCVAR_DEFAULT);
ConVar cv_background_fade_min_duration("osu_background_fade_min_duration", 1.4f, FCVAR_DEFAULT,
                                       "Only fade if the break is longer than this (in seconds)");
ConVar cv_background_fade_out_duration("osu_background_fade_out_duration", 0.25f, FCVAR_DEFAULT);
ConVar cv_background_image_cache_size("osu_background_image_cache_size", 32, FCVAR_DEFAULT,
                                      "how many images can stay loaded in parallel");
ConVar cv_background_image_eviction_delay_frames(
    "osu_background_image_eviction_delay_frames", 0, FCVAR_DEFAULT,
    "how many frames to keep stale background images in the cache before deleting them (if seconds && frames)");
ConVar cv_background_image_eviction_delay_seconds(
    "osu_background_image_eviction_delay_seconds", 0.05f, FCVAR_DEFAULT,
    "how many seconds to keep stale background images in the cache before deleting them (if seconds && frames)");
ConVar cv_background_image_loading_delay(
    "osu_background_image_loading_delay", 0.1f, FCVAR_DEFAULT,
    "how many seconds to wait until loading background images for visible beatmaps starts");
ConVar cv_beatmap_max_num_hitobjects(
    "osu_beatmap_max_num_hitobjects", 40000, FCVAR_DEFAULT,
    "maximum number of total allowed hitobjects per beatmap (prevent crashing on deliberate game-breaking beatmaps)");
ConVar cv_beatmap_max_num_slider_scoringtimes("osu_beatmap_max_num_slider_scoringtimes", 32768, FCVAR_DEFAULT,
                                              "maximum number of slider score increase events allowed per slider "
                                              "(prevent crashing on deliberate game-breaking beatmaps)");

// If catboy.best doesn't work for you, here are some alternatives:
// - https://api.osu.direct/d/
// - https://api.nerinyan.moe/d/
// - https://osu.gatari.pw/d/
// - https://osu.sayobot.cn/osu.php?s=
ConVar cv_beatmap_mirror("beatmap_mirror", "https://catboy.best/d/", FCVAR_DEFAULT,
                         "mirror from which beatmapsets will be downloaded");

ConVar cv_beatmap_preview_mods_live(
    "osu_beatmap_preview_mods_live", false, FCVAR_DEFAULT,
    "whether to immediately apply all currently selected mods while browsing beatmaps (e.g. speed/pitch)");
ConVar cv_beatmap_preview_music_loop("osu_beatmap_preview_music_loop", true, FCVAR_DEFAULT);
ConVar cv_beatmap_version("osu_beatmap_version", 128, FCVAR_DEFAULT,
                          "maximum supported .osu file version, above this will simply not load (this was 14 but got "
                          "bumped to 128 due to lazer backports)");
ConVar cv_bug_flicker_log("osu_bug_flicker_log", false, FCVAR_DEFAULT);
ConVar cv_circle_color_saturation("osu_circle_color_saturation", 1.0f, FCVAR_DEFAULT);
ConVar cv_circle_fade_out_scale("osu_circle_fade_out_scale", 0.4f, FCVAR_LOCKED);
ConVar cv_circle_number_rainbow("osu_circle_number_rainbow", false, FCVAR_DEFAULT);
ConVar cv_circle_rainbow("osu_circle_rainbow", false, FCVAR_DEFAULT);
ConVar cv_circle_shake_duration("osu_circle_shake_duration", 0.120f, FCVAR_DEFAULT);
ConVar cv_circle_shake_strength("osu_circle_shake_strength", 8.0f, FCVAR_DEFAULT);
ConVar cv_collections_custom_enabled("osu_collections_custom_enabled", true, FCVAR_DEFAULT,
                                     "load custom collections.db");
ConVar cv_collections_custom_version("osu_collections_custom_version", 20220110, FCVAR_DEFAULT,
                                     "maximum supported custom collections.db version");
ConVar cv_collections_legacy_enabled("osu_collections_legacy_enabled", true, FCVAR_DEFAULT,
                                     "load osu!'s collection.db");
ConVar cv_collections_save_immediately("osu_collections_save_immediately", true, FCVAR_DEFAULT,
                                       "write collections.db as soon as anything is changed");
ConVar cv_combo_anim1_duration("osu_combo_anim1_duration", 0.15f, FCVAR_DEFAULT);
ConVar cv_combo_anim1_size("osu_combo_anim1_size", 0.15f, FCVAR_DEFAULT);
ConVar cv_combo_anim2_duration("osu_combo_anim2_duration", 0.4f, FCVAR_DEFAULT);
ConVar cv_combo_anim2_size("osu_combo_anim2_size", 0.5f, FCVAR_DEFAULT);
ConVar cv_combobreak_sound_combo("osu_combobreak_sound_combo", 20, FCVAR_DEFAULT,
                                 "Only play the combobreak sound if the combo is higher than this");
ConVar cv_compensate_music_speed(
    "osu_compensate_music_speed", true, FCVAR_DEFAULT,
    "compensates speeds slower than 1x a little bit, by adding an offset depending on the slowness");
ConVar cv_confine_cursor_fullscreen("osu_confine_cursor_fullscreen", true, FCVAR_DEFAULT);
ConVar cv_confine_cursor_windowed("osu_confine_cursor_windowed", false, FCVAR_DEFAULT);
ConVar cv_console_logging("console_logging", true, FCVAR_DEFAULT);
ConVar cv_console_overlay("console_overlay", false, FCVAR_DEFAULT,
                          "should the log overlay always be visible (or only if the console is out)");
ConVar cv_console_overlay_lines("console_overlay_lines", 6, FCVAR_DEFAULT, "max number of lines of text");
ConVar cv_console_overlay_scale("console_overlay_scale", 1.0f, FCVAR_DEFAULT, "log text size multiplier");
ConVar cv_consolebox_animspeed("consolebox_animspeed", 12.0f, FCVAR_DEFAULT);
ConVar cv_consolebox_draw_helptext("consolebox_draw_helptext", true, FCVAR_DEFAULT,
                                   "whether convar suggestions also draw their helptext");
ConVar cv_consolebox_draw_preview("consolebox_draw_preview", true, FCVAR_DEFAULT,
                                  "whether the textbox shows the topmost suggestion while typing");
ConVar cv_cs_cap_sanity("osu_cs_cap_sanity", true, FCVAR_LOCKED);
ConVar cv_cs_override("osu_cs_override", -1.0f, FCVAR_UNLOCKED,
                      "use this to override between CS 0 and CS 12.1429. active if value is more than or equal to 0.");
ConVar cv_cs_overridenegative("osu_cs_overridenegative", 0.0f, FCVAR_UNLOCKED,
                              "use this to override below CS 0. active if value is less than 0, disabled otherwise. "
                              "this override always overrides the other override.");
ConVar cv_cursor_alpha("osu_cursor_alpha", 1.0f, FCVAR_DEFAULT);
ConVar cv_cursor_expand_duration("osu_cursor_expand_duration", 0.1f, FCVAR_DEFAULT);
ConVar cv_cursor_expand_scale_multiplier("osu_cursor_expand_scale_multiplier", 1.3f, FCVAR_DEFAULT);
ConVar cv_cursor_ripple_additive("osu_cursor_ripple_additive", true, FCVAR_DEFAULT, "use additive blending");
ConVar cv_cursor_ripple_alpha("osu_cursor_ripple_alpha", 1.0f, FCVAR_DEFAULT);
ConVar cv_cursor_ripple_anim_end_scale("osu_cursor_ripple_anim_end_scale", 0.5f, FCVAR_DEFAULT, "end size multiplier");
ConVar cv_cursor_ripple_anim_start_fadeout_delay(
    "osu_cursor_ripple_anim_start_fadeout_delay", 0.0f, FCVAR_DEFAULT,
    "delay in seconds after which to start fading out (limited by osu_cursor_ripple_duration of course)");
ConVar cv_cursor_ripple_anim_start_scale("osu_cursor_ripple_anim_start_scale", 0.05f, FCVAR_DEFAULT,
                                         "start size multiplier");
ConVar cv_cursor_ripple_duration("osu_cursor_ripple_duration", 0.7f, FCVAR_DEFAULT,
                                 "time in seconds each cursor ripple is visible");
ConVar cv_cursor_ripple_tint_b("osu_cursor_ripple_tint_b", 255, FCVAR_DEFAULT, "from 0 to 255");
ConVar cv_cursor_ripple_tint_g("osu_cursor_ripple_tint_g", 255, FCVAR_DEFAULT, "from 0 to 255");
ConVar cv_cursor_ripple_tint_r("osu_cursor_ripple_tint_r", 255, FCVAR_DEFAULT, "from 0 to 255");
ConVar cv_cursor_scale("osu_cursor_scale", 1.0f, FCVAR_DEFAULT);
ConVar cv_cursor_trail_alpha("osu_cursor_trail_alpha", 1.0f, FCVAR_DEFAULT);
ConVar cv_cursor_trail_expand(
    "osu_cursor_trail_expand", true, FCVAR_DEFAULT,
    "if \"CursorExpand: 1\" in your skin.ini, whether the trail should then also expand or not");
ConVar cv_cursor_trail_length("osu_cursor_trail_length", 0.17f, FCVAR_DEFAULT,
                              "how long unsmooth cursortrails should be, in seconds");
ConVar cv_cursor_trail_max_size("osu_cursor_trail_max_size", 2048, FCVAR_DEFAULT,
                                "maximum number of rendered trail images, array size limit");
ConVar cv_cursor_trail_scale("osu_cursor_trail_scale", 1.0f, FCVAR_DEFAULT);
ConVar cv_cursor_trail_smooth_div(
    "osu_cursor_trail_smooth_div", 4.0f, FCVAR_DEFAULT,
    "divide the cursortrail.png image size by this much, for determining the distance to the next trail image");
ConVar cv_cursor_trail_smooth_force("osu_cursor_trail_smooth_force", false, FCVAR_DEFAULT);
ConVar cv_cursor_trail_smooth_length("osu_cursor_trail_smooth_length", 0.5f, FCVAR_DEFAULT,
                                     "how long smooth cursortrails should be, in seconds");
ConVar cv_cursor_trail_spacing(
    "cursor_trail_spacing", 15.f, FCVAR_DEFAULT,
    "how big the gap between consecutive unsmooth cursortrail images should be, in milliseconds");
ConVar cv_database_enabled("osu_database_enabled", true, FCVAR_DEFAULT);
ConVar cv_database_ignore_version("osu_database_ignore_version", true, FCVAR_DEFAULT,
                                  "ignore upper version limit and force load the db file (may crash)");
ConVar cv_database_ignore_version_warnings("osu_database_ignore_version_warnings", false, FCVAR_DEFAULT);
ConVar cv_database_version("osu_database_version", OSU_VERSION_DATEONLY, FCVAR_DEFAULT,
                           "maximum supported osu!.db version, above this will use fallback loader");
ConVar cv_debug("osu_debug", true, FCVAR_DEFAULT);
ConVar cv_debug_anim("debug_anim", false, FCVAR_DEFAULT);
ConVar cv_debug_background_star_calc(
    "osu_debug_background_star_calc", false, FCVAR_DEFAULT,
    "prints the name of the beatmap about to get its stars calculated (cmd/terminal window only, no in-game log!)");
ConVar cv_debug_box_shadows("debug_box_shadows", false, FCVAR_DEFAULT);
ConVar cv_debug_corporeal("debug_ghost", FCVAR_DEFAULT, false, _debugCorporeal);
ConVar cv_debug_draw_timingpoints("osu_debug_draw_timingpoints", false, FCVAR_LOCKED);
ConVar cv_debug_engine("debug_engine", false, FCVAR_DEFAULT);
ConVar cv_debug_env("debug_env", false, FCVAR_DEFAULT);
ConVar cv_debug_file("debug_file", false, FCVAR_DEFAULT);
ConVar cv_debug_hiterrorbar_misaims("osu_debug_hiterrorbar_misaims", false, FCVAR_DEFAULT);
ConVar cv_debug_mouse("debug_mouse", false, FCVAR_LOCKED);
ConVar cv_debug_pp("osu_debug_pp", false, FCVAR_DEFAULT);
ConVar cv_debug_rm("debug_rm", false, FCVAR_DEFAULT);
ConVar cv_debug_rt("debug_rt", false, FCVAR_LOCKED, "draws all rendertargets with a translucent green background");
ConVar cv_debug_shaders("debug_shaders", false, FCVAR_DEFAULT);
ConVar cv_debug_vprof("debug_vprof", false, FCVAR_DEFAULT);
ConVar cv_disable_mousebuttons("osu_disable_mousebuttons", false, FCVAR_DEFAULT);
ConVar cv_disable_mousewheel("osu_disable_mousewheel", false, FCVAR_DEFAULT);
ConVar cv_drain_kill("osu_drain_kill", true, FCVAR_DEFAULT, "whether to kill the player upon failing");
ConVar cv_drain_kill_notification_duration(
    "osu_drain_kill_notification_duration", 1.0f, FCVAR_DEFAULT,
    "how long to display the \"You have failed, but you can keep playing!\" notification (0 = disabled)");
ConVar cv_drain_vr_100("osu_drain_vr_100", -0.10f, FCVAR_DEFAULT);
ConVar cv_drain_vr_300("osu_drain_vr_300", 0.035f, FCVAR_DEFAULT);
ConVar cv_drain_vr_50("osu_drain_vr_50", -0.125f, FCVAR_DEFAULT);
ConVar cv_drain_vr_miss("osu_drain_vr_miss", -0.15f, FCVAR_DEFAULT);
ConVar cv_drain_vr_multiplier("osu_drain_vr_multiplier", 1.0f, FCVAR_DEFAULT);
ConVar cv_drain_vr_sliderbreak("osu_drain_vr_sliderbreak", -0.10f, FCVAR_DEFAULT);
ConVar cv_draw_accuracy("osu_draw_accuracy", true, FCVAR_DEFAULT);
ConVar cv_draw_approach_circles("osu_draw_approach_circles", true, FCVAR_DEFAULT);
ConVar cv_draw_beatmap_background_image("osu_draw_beatmap_background_image", true, FCVAR_DEFAULT);
ConVar cv_draw_circles("osu_draw_circles", true, FCVAR_DEFAULT);
ConVar cv_draw_combo("osu_draw_combo", true, FCVAR_DEFAULT);
ConVar cv_draw_continue("osu_draw_continue", true, FCVAR_DEFAULT);
ConVar cv_draw_cursor_ripples("osu_draw_cursor_ripples", false, FCVAR_DEFAULT);
ConVar cv_draw_cursor_trail("osu_draw_cursor_trail", true, FCVAR_DEFAULT);
ConVar cv_draw_followpoints("osu_draw_followpoints", true, FCVAR_DEFAULT);
ConVar cv_draw_fps("osu_draw_fps", true, FCVAR_DEFAULT);
ConVar cv_draw_hiterrorbar("osu_draw_hiterrorbar", true, FCVAR_DEFAULT);
ConVar cv_draw_hiterrorbar_bottom("osu_draw_hiterrorbar_bottom", true, FCVAR_DEFAULT);
ConVar cv_draw_hiterrorbar_left("osu_draw_hiterrorbar_left", false, FCVAR_DEFAULT);
ConVar cv_draw_hiterrorbar_right("osu_draw_hiterrorbar_right", false, FCVAR_DEFAULT);
ConVar cv_draw_hiterrorbar_top("osu_draw_hiterrorbar_top", false, FCVAR_DEFAULT);
ConVar cv_draw_hiterrorbar_ur("osu_draw_hiterrorbar_ur", true, FCVAR_DEFAULT);
ConVar cv_draw_hitobjects("osu_draw_hitobjects", true, FCVAR_DEFAULT);
ConVar cv_draw_hud("osu_draw_hud", true, FCVAR_DEFAULT);
ConVar cv_draw_inputoverlay("osu_draw_inputoverlay", true, FCVAR_DEFAULT);
ConVar cv_draw_menu_background("osu_draw_menu_background", false, FCVAR_DEFAULT);
ConVar cv_draw_numbers("osu_draw_numbers", true, FCVAR_DEFAULT);
ConVar cv_draw_playfield_border("osu_draw_playfield_border", true, FCVAR_DEFAULT);
ConVar cv_draw_progressbar("osu_draw_progressbar", true, FCVAR_DEFAULT);
ConVar cv_draw_rankingscreen_background_image("osu_draw_rankingscreen_background_image", true, FCVAR_DEFAULT);
ConVar cv_draw_reverse_order("osu_draw_reverse_order", false, FCVAR_DEFAULT);
ConVar cv_draw_score("osu_draw_score", true, FCVAR_DEFAULT);
ConVar cv_draw_scorebar("osu_draw_scorebar", true, FCVAR_DEFAULT);
ConVar cv_draw_scorebarbg("osu_draw_scorebarbg", true, FCVAR_DEFAULT);
ConVar cv_draw_scoreboard("osu_draw_scoreboard", true, FCVAR_DEFAULT);
ConVar cv_draw_scoreboard_mp("osu_draw_scoreboard_mp", true, FCVAR_DEFAULT);
ConVar cv_draw_scrubbing_timeline("osu_draw_scrubbing_timeline", true, FCVAR_DEFAULT);
ConVar cv_draw_scrubbing_timeline_breaks("osu_draw_scrubbing_timeline_breaks", true, FCVAR_DEFAULT);
ConVar cv_draw_scrubbing_timeline_strain_graph("osu_draw_scrubbing_timeline_strain_graph", false, FCVAR_DEFAULT);
ConVar cv_draw_songbrowser_background_image("osu_draw_songbrowser_background_image", true, FCVAR_DEFAULT);
ConVar cv_draw_songbrowser_menu_background_image("osu_draw_songbrowser_menu_background_image", true, FCVAR_DEFAULT);
ConVar cv_draw_songbrowser_strain_graph("osu_draw_songbrowser_strain_graph", false, FCVAR_DEFAULT);
ConVar cv_draw_songbrowser_thumbnails("osu_draw_songbrowser_thumbnails", true, FCVAR_DEFAULT);
ConVar cv_draw_spectator_list("draw_spectator_list", true, FCVAR_DEFAULT);
ConVar cv_draw_statistics_ar("osu_draw_statistics_ar", false, FCVAR_DEFAULT);
ConVar cv_draw_statistics_bpm("osu_draw_statistics_bpm", false, FCVAR_DEFAULT);
ConVar cv_draw_statistics_cs("osu_draw_statistics_cs", false, FCVAR_DEFAULT);
ConVar cv_draw_statistics_hitdelta("osu_draw_statistics_hitdelta", false, FCVAR_DEFAULT);
ConVar cv_draw_statistics_hitwindow300("osu_draw_statistics_hitwindow300", false, FCVAR_DEFAULT);
ConVar cv_draw_statistics_hp("osu_draw_statistics_hp", false, FCVAR_DEFAULT);
ConVar cv_draw_statistics_livestars("osu_draw_statistics_livestars", false, FCVAR_DEFAULT);
ConVar cv_draw_statistics_maxpossiblecombo("osu_draw_statistics_maxpossiblecombo", false, FCVAR_DEFAULT);
ConVar cv_draw_statistics_misses("osu_draw_statistics_misses", false, FCVAR_DEFAULT);
ConVar cv_draw_statistics_nd("osu_draw_statistics_nd", false, FCVAR_DEFAULT);
ConVar cv_draw_statistics_nps("osu_draw_statistics_nps", false, FCVAR_DEFAULT);
ConVar cv_draw_statistics_od("osu_draw_statistics_od", false, FCVAR_DEFAULT);
ConVar cv_draw_statistics_perfectpp("osu_draw_statistics_perfectpp", false, FCVAR_DEFAULT);
ConVar cv_draw_statistics_pp("osu_draw_statistics_pp", false, FCVAR_DEFAULT);
ConVar cv_draw_statistics_sliderbreaks("osu_draw_statistics_sliderbreaks", false, FCVAR_DEFAULT);
ConVar cv_draw_statistics_totalstars("osu_draw_statistics_totalstars", false, FCVAR_DEFAULT);
ConVar cv_draw_statistics_ur("osu_draw_statistics_ur", false, FCVAR_DEFAULT);
ConVar cv_draw_target_heatmap("osu_draw_target_heatmap", true, FCVAR_DEFAULT);
ConVar cv_early_note_time(
    "osu_early_note_time", 1500.0f, FCVAR_DEFAULT,
    "Timeframe in ms at the beginning of a beatmap which triggers a starting delay for easier reading");
ConVar cv_end_delay_time("osu_end_delay_time", 750.0f, FCVAR_DEFAULT,
                         "Duration in ms which is added at the end of a beatmap after the last hitobject is finished "
                         "but before the ranking screen is automatically shown");
ConVar cv_end_skip(
    "osu_end_skip", true, FCVAR_DEFAULT,
    "whether the beatmap jumps to the ranking screen as soon as the last hitobject plus lenience has passed");
ConVar cv_end_skip_time("osu_end_skip_time", 400.0f, FCVAR_DEFAULT,
                        "Duration in ms which is added to the endTime of the last hitobject, after which pausing the "
                        "game will immediately jump to the ranking screen");
ConVar cv_epilepsy("epilepsy", false, FCVAR_DEFAULT);
ConVar cv_fail_time("osu_fail_time", 2.25f, FCVAR_DEFAULT,
                    "Timeframe in s for the slowdown effect after failing, before the pause menu is shown");
ConVar cv_file_size_max("file_size_max", 1024, FCVAR_DEFAULT,
                        "maximum filesize sanity limit in MB, all files bigger than this are not allowed to load");
ConVar cv_flashlight_always_hard("flashlight_always_hard", false, FCVAR_DEFAULT,
                                 "always use 200+ combo flashlight radius");
ConVar cv_flashlight_follow_delay("flashlight_follow_delay", 0.120f, FCVAR_LOCKED);
ConVar cv_flashlight_radius("flashlight_radius", 100.f, FCVAR_LOCKED);

#ifdef _WIN32
ConVar cv_osu_folder("osu_folder", "C:/Program Files (x86)/osu!/", FCVAR_DEFAULT);
#elif defined __APPLE__
ConVar cv_osu_folder("osu_folder", "/osu!/", FCVAR_DEFAULT);
#else
ConVar cv_osu_folder("osu_folder", "", FCVAR_DEFAULT);
#endif

ConVar cv_osu_folder_sub_skins("osu_folder_sub_skins", "Skins/", FCVAR_DEFAULT);
ConVar cv_osu_folder_sub_songs("osu_folder_sub_songs", "Songs/", FCVAR_DEFAULT);
ConVar cv_followpoints_anim("osu_followpoints_anim", false, FCVAR_DEFAULT,
                            "scale + move animation while fading in followpoints (osu only does this when its "
                            "internal default skin is being used)");
ConVar cv_followpoints_approachtime("osu_followpoints_approachtime", 800.0f, FCVAR_DEFAULT);
ConVar cv_followpoints_clamp("osu_followpoints_clamp", false, FCVAR_DEFAULT,
                             "clamp followpoint approach time to current circle approach time (instead of using the "
                             "hardcoded default 800 ms raw)");
ConVar cv_followpoints_connect_combos("osu_followpoints_connect_combos", false, FCVAR_DEFAULT,
                                      "connect followpoints even if a new combo has started");
ConVar cv_followpoints_connect_spinners("osu_followpoints_connect_spinners", false, FCVAR_DEFAULT,
                                        "connect followpoints even through spinners");
ConVar cv_followpoints_prevfadetime("osu_followpoints_prevfadetime", 400.0f, FCVAR_DEFAULT);
ConVar cv_followpoints_scale_multiplier("osu_followpoints_scale_multiplier", 1.0f, FCVAR_DEFAULT);
ConVar cv_followpoints_separation_multiplier("osu_followpoints_separation_multiplier", 1.0f, FCVAR_DEFAULT);
ConVar cv_force_legacy_slider_renderer("osu_force_legacy_slider_renderer", false, FCVAR_DEFAULT,
                                       "on some older machines, this may be faster than vertexbuffers");
ConVar cv_fposu_absolute_mode("fposu_absolute_mode", false, FCVAR_DEFAULT);
ConVar cv_fposu_cube("fposu_cube", true, FCVAR_DEFAULT);
ConVar cv_fposu_cube_size("fposu_cube_size", 500.0f, FCVAR_DEFAULT);
ConVar cv_fposu_cube_tint_b("fposu_cube_tint_b", 255, FCVAR_DEFAULT, "from 0 to 255");
ConVar cv_fposu_cube_tint_g("fposu_cube_tint_g", 255, FCVAR_DEFAULT, "from 0 to 255");
ConVar cv_fposu_cube_tint_r("fposu_cube_tint_r", 255, FCVAR_DEFAULT, "from 0 to 255");
ConVar cv_fposu_curved("fposu_curved", true, FCVAR_DEFAULT);
ConVar cv_fposu_distance("fposu_distance", 0.5f, FCVAR_DEFAULT);
ConVar cv_fposu_draw_cursor_trail("fposu_draw_cursor_trail", true, FCVAR_DEFAULT);
ConVar cv_fposu_draw_scorebarbg_on_top("fposu_draw_scorebarbg_on_top", false, FCVAR_DEFAULT);
ConVar cv_fposu_fov("fposu_fov", 103.0f, FCVAR_DEFAULT);
ConVar cv_fposu_invert_horizontal("fposu_invert_horizontal", false, FCVAR_DEFAULT);
ConVar cv_fposu_invert_vertical("fposu_invert_vertical", false, FCVAR_DEFAULT);
ConVar cv_fposu_mod_strafing("fposu_mod_strafing", false, FCVAR_UNLOCKED);
ConVar cv_fposu_mod_strafing_frequency_x("fposu_mod_strafing_frequency_x", 0.1f, FCVAR_DEFAULT);
ConVar cv_fposu_mod_strafing_frequency_y("fposu_mod_strafing_frequency_y", 0.2f, FCVAR_DEFAULT);
ConVar cv_fposu_mod_strafing_frequency_z("fposu_mod_strafing_frequency_z", 0.15f, FCVAR_DEFAULT);
ConVar cv_fposu_mod_strafing_strength_x("fposu_mod_strafing_strength_x", 0.3f, FCVAR_DEFAULT);
ConVar cv_fposu_mod_strafing_strength_y("fposu_mod_strafing_strength_y", 0.1f, FCVAR_DEFAULT);
ConVar cv_fposu_mod_strafing_strength_z("fposu_mod_strafing_strength_z", 0.15f, FCVAR_DEFAULT);
ConVar cv_fposu_mouse_cm_360("fposu_mouse_cm_360", 30.0f, FCVAR_DEFAULT);
ConVar cv_fposu_mouse_dpi("fposu_mouse_dpi", 400, FCVAR_DEFAULT);
ConVar cv_fposu_noclip("fposu_noclip", false, FCVAR_DEFAULT);
ConVar cv_fposu_noclipaccelerate("fposu_noclipaccelerate", 20.0f, FCVAR_DEFAULT);
ConVar cv_fposu_noclipfriction("fposu_noclipfriction", 10.0f, FCVAR_DEFAULT);
ConVar cv_fposu_noclipspeed("fposu_noclipspeed", 2.0f, FCVAR_DEFAULT);
ConVar cv_fposu_playfield_position_x("fposu_playfield_position_x", 0.0f, FCVAR_DEFAULT);
ConVar cv_fposu_playfield_position_y("fposu_playfield_position_y", 0.0f, FCVAR_DEFAULT);
ConVar cv_fposu_playfield_position_z("fposu_playfield_position_z", 0.0f, FCVAR_DEFAULT);
ConVar cv_fposu_playfield_rotation_x("fposu_playfield_rotation_x", 0.0f, FCVAR_DEFAULT);
ConVar cv_fposu_playfield_rotation_y("fposu_playfield_rotation_y", 0.0f, FCVAR_DEFAULT);
ConVar cv_fposu_playfield_rotation_z("fposu_playfield_rotation_z", 0.0f, FCVAR_DEFAULT);
ConVar cv_fposu_skybox("fposu_skybox", true, FCVAR_DEFAULT);
ConVar cv_fposu_transparent_playfield("fposu_transparent_playfield", false, FCVAR_DEFAULT,
                                      "only works if background dim is 100% and background brightness is 0%");
ConVar cv_fposu_vertical_fov("fposu_vertical_fov", false, FCVAR_DEFAULT);
ConVar cv_fposu_zoom_anim_duration("fposu_zoom_anim_duration", 0.065f, FCVAR_DEFAULT,
                                   "time in seconds for the zoom/unzoom animation");
ConVar cv_fposu_zoom_fov("fposu_zoom_fov", 45.0f, FCVAR_DEFAULT);
ConVar cv_fposu_zoom_sensitivity_ratio("fposu_zoom_sensitivity_ratio", 1.0f, FCVAR_DEFAULT,
                                       "replicates zoom_sensitivity_ratio behavior on css/csgo/tf2/etc.");
ConVar cv_fposu_zoom_toggle("fposu_zoom_toggle", false, FCVAR_DEFAULT, "whether the zoom key acts as a toggle");
ConVar cv_fps_max("fps_max", 60.0f, FCVAR_DEFAULT, "framerate limiter, foreground");
ConVar cv_fps_max_background("fps_max_background", 30.0f, FCVAR_DEFAULT, "framerate limiter, background");
ConVar cv_fps_max_background_interleaved("fps_max_background_interleaved", 1, FCVAR_DEFAULT,
                                         "experimental, update normally but only draw every n-th frame");
ConVar cv_fps_max_yield("fps_max_yield", false, FCVAR_DEFAULT,
                        "always release rest of timeslice once per frame (call scheduler via sleep(0))");
ConVar cv_fps_unlimited("fps_unlimited", false, FCVAR_DEFAULT);
ConVar cv_fps_unlimited_yield(
    "fps_unlimited_yield", false, FCVAR_DEFAULT,
    "always release rest of timeslice once per frame (call scheduler via sleep(0)), even if unlimited fps are enabled");
ConVar cv_fullscreen_windowed_borderless("fullscreen_windowed_borderless", false, FCVAR_DEFAULT,
                                         _fullscreen_windowed_borderless);
ConVar cv_gamemode("osu_gamemode", "std", FCVAR_DEFAULT);
ConVar cv_hiterrorbar_misaims("osu_hiterrorbar_misaims", true, FCVAR_DEFAULT);
ConVar cv_hiterrorbar_misses("osu_hiterrorbar_misses", true, FCVAR_DEFAULT);
ConVar cv_hitobject_fade_in_time("osu_hitobject_fade_in_time", 400, FCVAR_LOCKED, "in milliseconds (!)");
ConVar cv_hitobject_fade_out_time("osu_hitobject_fade_out_time", 0.293f, FCVAR_LOCKED, "in seconds (!)");
ConVar cv_hitobject_fade_out_time_speed_multiplier_min(
    "osu_hitobject_fade_out_time_speed_multiplier_min", 0.5f, FCVAR_LOCKED,
    "The minimum multiplication factor allowed for the speed multiplier influencing the fadeout duration");
ConVar cv_hitobject_hittable_dim(
    "osu_hitobject_hittable_dim", true, FCVAR_DEFAULT,
    "whether to dim objects not yet within the miss-range (when they can't even be missed yet)");
ConVar cv_hitobject_hittable_dim_duration("osu_hitobject_hittable_dim_duration", 100, FCVAR_DEFAULT,
                                          "in milliseconds (!)");
ConVar cv_hitobject_hittable_dim_start_percent(
    "osu_hitobject_hittable_dim_start_percent", 0.7647f, FCVAR_DEFAULT,
    "dimmed objects start at this brightness value before becoming fullbright (only RGB, this does not affect "
    "alpha/transparency)");
ConVar cv_hitresult_animated(
    "osu_hitresult_animated", true, FCVAR_DEFAULT,
    "whether to animate hitresult scales (depending on particle<SCORE>.png, either scale wobble or smooth scale)");
ConVar cv_hitresult_delta_colorize("osu_hitresult_delta_colorize", false, FCVAR_DEFAULT,
                                   "whether to colorize hitresults depending on how early/late the hit (delta) was");
ConVar cv_hitresult_delta_colorize_early_b("osu_hitresult_delta_colorize_early_b", 0, FCVAR_DEFAULT, "from 0 to 255");
ConVar cv_hitresult_delta_colorize_early_g("osu_hitresult_delta_colorize_early_g", 0, FCVAR_DEFAULT, "from 0 to 255");
ConVar cv_hitresult_delta_colorize_early_r("osu_hitresult_delta_colorize_early_r", 255, FCVAR_DEFAULT, "from 0 to 255");
ConVar cv_hitresult_delta_colorize_interpolate("osu_hitresult_delta_colorize_interpolate", true, FCVAR_DEFAULT,
                                               "whether colorized hitresults should smoothly interpolate between "
                                               "early/late colors depending on the hit delta amount");
ConVar cv_hitresult_delta_colorize_late_b("osu_hitresult_delta_colorize_late_b", 255, FCVAR_DEFAULT, "from 0 to 255");
ConVar cv_hitresult_delta_colorize_late_g("osu_hitresult_delta_colorize_late_g", 0, FCVAR_DEFAULT, "from 0 to 255");
ConVar cv_hitresult_delta_colorize_late_r("osu_hitresult_delta_colorize_late_r", 0, FCVAR_DEFAULT, "from 0 to 255");
ConVar cv_hitresult_delta_colorize_multiplier(
    "osu_hitresult_delta_colorize_multiplier", 2.0f, FCVAR_DEFAULT,
    "early/late colors are multiplied by this (assuming interpolation is enabled, increasing this will make early/late "
    "colors appear fully earlier)");
ConVar cv_hitresult_draw_300s("osu_hitresult_draw_300s", false, FCVAR_DEFAULT);
ConVar cv_hitresult_duration(
    "osu_hitresult_duration", 1.100f, FCVAR_DEFAULT,
    "max duration of the entire hitresult in seconds (this limits all other values, except for animated skins!)");
ConVar cv_hitresult_duration_max("osu_hitresult_duration_max", 5.0f, FCVAR_DEFAULT,
                                 "absolute hard limit in seconds, even for animated skins");
ConVar cv_hitresult_fadein_duration("osu_hitresult_fadein_duration", 0.120f, FCVAR_DEFAULT);
ConVar cv_hitresult_fadeout_duration("osu_hitresult_fadeout_duration", 0.600f, FCVAR_DEFAULT);
ConVar cv_hitresult_fadeout_start_time("osu_hitresult_fadeout_start_time", 0.500f, FCVAR_DEFAULT);
ConVar cv_hitresult_miss_fadein_scale("osu_hitresult_miss_fadein_scale", 2.0f, FCVAR_DEFAULT);
ConVar cv_hitresult_scale("osu_hitresult_scale", 1.0f, FCVAR_DEFAULT);
ConVar cv_host_timescale("host_timescale", 1.0f, FCVAR_LOCKED,
                         "Scale by which the engine measures elapsed time, affects engine->getTime()",
                         _host_timescale_);
ConVar cv_hp_override("osu_hp_override", -1.0f, FCVAR_UNLOCKED);
ConVar cv_hud_accuracy_scale("osu_hud_accuracy_scale", 1.0f, FCVAR_DEFAULT);
ConVar cv_hud_combo_scale("osu_hud_combo_scale", 1.0f, FCVAR_DEFAULT);
ConVar cv_hud_fps_smoothing("osu_hud_fps_smoothing", true, FCVAR_DEFAULT);
ConVar cv_hud_hiterrorbar_alpha("osu_hud_hiterrorbar_alpha", 1.0f, FCVAR_DEFAULT,
                                "opacity multiplier for entire hiterrorbar");
ConVar cv_hud_hiterrorbar_bar_alpha("osu_hud_hiterrorbar_bar_alpha", 1.0f, FCVAR_DEFAULT,
                                    "opacity multiplier for background color bar");
ConVar cv_hud_hiterrorbar_bar_height_scale("osu_hud_hiterrorbar_bar_height_scale", 3.4f, FCVAR_DEFAULT);
ConVar cv_hud_hiterrorbar_bar_width_scale("osu_hud_hiterrorbar_bar_width_scale", 0.6f, FCVAR_DEFAULT);
ConVar cv_hud_hiterrorbar_centerline_alpha("osu_hud_hiterrorbar_centerline_alpha", 1.0f, FCVAR_DEFAULT,
                                           "opacity multiplier for center line");
ConVar cv_hud_hiterrorbar_centerline_b("osu_hud_hiterrorbar_centerline_b", 255, FCVAR_DEFAULT);
ConVar cv_hud_hiterrorbar_centerline_g("osu_hud_hiterrorbar_centerline_g", 255, FCVAR_DEFAULT);
ConVar cv_hud_hiterrorbar_centerline_r("osu_hud_hiterrorbar_centerline_r", 255, FCVAR_DEFAULT);
ConVar cv_hud_hiterrorbar_entry_100_b("osu_hud_hiterrorbar_entry_100_b", 19, FCVAR_DEFAULT);
ConVar cv_hud_hiterrorbar_entry_100_g("osu_hud_hiterrorbar_entry_100_g", 227, FCVAR_DEFAULT);
ConVar cv_hud_hiterrorbar_entry_100_r("osu_hud_hiterrorbar_entry_100_r", 87, FCVAR_DEFAULT);
ConVar cv_hud_hiterrorbar_entry_300_b("osu_hud_hiterrorbar_entry_300_b", 231, FCVAR_DEFAULT);
ConVar cv_hud_hiterrorbar_entry_300_g("osu_hud_hiterrorbar_entry_300_g", 188, FCVAR_DEFAULT);
ConVar cv_hud_hiterrorbar_entry_300_r("osu_hud_hiterrorbar_entry_300_r", 50, FCVAR_DEFAULT);
ConVar cv_hud_hiterrorbar_entry_50_b("osu_hud_hiterrorbar_entry_50_b", 70, FCVAR_DEFAULT);
ConVar cv_hud_hiterrorbar_entry_50_g("osu_hud_hiterrorbar_entry_50_g", 174, FCVAR_DEFAULT);
ConVar cv_hud_hiterrorbar_entry_50_r("osu_hud_hiterrorbar_entry_50_r", 218, FCVAR_DEFAULT);
ConVar cv_hud_hiterrorbar_entry_additive("osu_hud_hiterrorbar_entry_additive", true, FCVAR_DEFAULT,
                                         "whether to use additive blending for all hit error entries/lines");
ConVar cv_hud_hiterrorbar_entry_alpha("osu_hud_hiterrorbar_entry_alpha", 0.75f, FCVAR_DEFAULT,
                                      "opacity multiplier for all hit error entries/lines");
ConVar cv_hud_hiterrorbar_entry_hit_fade_time("osu_hud_hiterrorbar_entry_hit_fade_time", 6.0f, FCVAR_DEFAULT,
                                              "fade duration of 50/100/300 hit entries/lines in seconds");
ConVar cv_hud_hiterrorbar_entry_miss_b("osu_hud_hiterrorbar_entry_miss_b", 0, FCVAR_DEFAULT);
ConVar cv_hud_hiterrorbar_entry_miss_fade_time("osu_hud_hiterrorbar_entry_miss_fade_time", 4.0f, FCVAR_DEFAULT,
                                               "fade duration of miss entries/lines in seconds");
ConVar cv_hud_hiterrorbar_entry_miss_g("osu_hud_hiterrorbar_entry_miss_g", 0, FCVAR_DEFAULT);
ConVar cv_hud_hiterrorbar_entry_miss_r("osu_hud_hiterrorbar_entry_miss_r", 205, FCVAR_DEFAULT);
ConVar cv_hud_hiterrorbar_height_percent("osu_hud_hiterrorbar_height_percent", 0.007f, FCVAR_DEFAULT);
ConVar cv_hud_hiterrorbar_hide_during_spinner("osu_hud_hiterrorbar_hide_during_spinner", true, FCVAR_DEFAULT);
ConVar cv_hud_hiterrorbar_max_entries("osu_hud_hiterrorbar_max_entries", 32, FCVAR_DEFAULT,
                                      "maximum number of entries/lines");
ConVar cv_hud_hiterrorbar_offset_bottom_percent("osu_hud_hiterrorbar_offset_bottom_percent", 0.0f, FCVAR_DEFAULT);
ConVar cv_hud_hiterrorbar_offset_left_percent("osu_hud_hiterrorbar_offset_left_percent", 0.0f, FCVAR_DEFAULT);
ConVar cv_hud_hiterrorbar_offset_percent("osu_hud_hiterrorbar_offset_percent", 0.0f, FCVAR_DEFAULT);
ConVar cv_hud_hiterrorbar_offset_right_percent("osu_hud_hiterrorbar_offset_right_percent", 0.0f, FCVAR_DEFAULT);
ConVar cv_hud_hiterrorbar_offset_top_percent("osu_hud_hiterrorbar_offset_top_percent", 0.0f, FCVAR_DEFAULT);
ConVar cv_hud_hiterrorbar_scale("osu_hud_hiterrorbar_scale", 1.0f, FCVAR_DEFAULT);
ConVar cv_hud_hiterrorbar_showmisswindow("osu_hud_hiterrorbar_showmisswindow", false, FCVAR_DEFAULT);
ConVar cv_hud_hiterrorbar_ur_alpha("osu_hud_hiterrorbar_ur_alpha", 0.5f, FCVAR_DEFAULT,
                                   "opacity multiplier for unstable rate text above hiterrorbar");
ConVar cv_hud_hiterrorbar_ur_offset_x_percent("osu_hud_hiterrorbar_ur_offset_x_percent", 0.0f, FCVAR_DEFAULT);
ConVar cv_hud_hiterrorbar_ur_offset_y_percent("osu_hud_hiterrorbar_ur_offset_y_percent", 0.0f, FCVAR_DEFAULT);
ConVar cv_hud_hiterrorbar_ur_scale("osu_hud_hiterrorbar_ur_scale", 1.0f, FCVAR_DEFAULT);
ConVar cv_hud_hiterrorbar_width_percent("osu_hud_hiterrorbar_width_percent", 0.15f, FCVAR_DEFAULT);
ConVar cv_hud_hiterrorbar_width_percent_with_misswindow("osu_hud_hiterrorbar_width_percent_with_misswindow", 0.4f,
                                                        FCVAR_DEFAULT);
ConVar cv_hud_inputoverlay_anim_color_duration("osu_hud_inputoverlay_anim_color_duration", 0.1f, FCVAR_DEFAULT);
ConVar cv_hud_inputoverlay_anim_scale_duration("osu_hud_inputoverlay_anim_scale_duration", 0.16f, FCVAR_DEFAULT);
ConVar cv_hud_inputoverlay_anim_scale_multiplier("osu_hud_inputoverlay_anim_scale_multiplier", 0.8f, FCVAR_DEFAULT);
ConVar cv_hud_inputoverlay_offset_x("osu_hud_inputoverlay_offset_x", 0.0f, FCVAR_DEFAULT);
ConVar cv_hud_inputoverlay_offset_y("osu_hud_inputoverlay_offset_y", 0.0f, FCVAR_DEFAULT);
ConVar cv_hud_inputoverlay_scale("osu_hud_inputoverlay_scale", 1.0f, FCVAR_DEFAULT);
ConVar cv_hud_playfield_border_size("osu_hud_playfield_border_size", 5.0f, FCVAR_DEFAULT);
ConVar cv_hud_progressbar_scale("osu_hud_progressbar_scale", 1.0f, FCVAR_DEFAULT);
ConVar cv_hud_scale("osu_hud_scale", 1.0f, FCVAR_DEFAULT);
ConVar cv_hud_score_scale("osu_hud_score_scale", 1.0f, FCVAR_DEFAULT);
ConVar cv_hud_scorebar_hide_anim_duration("osu_hud_scorebar_hide_anim_duration", 0.5f, FCVAR_DEFAULT);
ConVar cv_hud_scorebar_hide_during_breaks("osu_hud_scorebar_hide_during_breaks", true, FCVAR_DEFAULT);
ConVar cv_hud_scorebar_scale("osu_hud_scorebar_scale", 1.0f, FCVAR_DEFAULT);
ConVar cv_hud_scoreboard_offset_y_percent("osu_hud_scoreboard_offset_y_percent", 0.11f, FCVAR_DEFAULT);
ConVar cv_hud_scoreboard_scale("osu_hud_scoreboard_scale", 1.0f, FCVAR_DEFAULT);
ConVar cv_hud_scoreboard_use_menubuttonbackground("osu_hud_scoreboard_use_menubuttonbackground", true, FCVAR_DEFAULT);
ConVar cv_hud_scrubbing_timeline_hover_tooltip_offset_multiplier(
    "osu_hud_scrubbing_timeline_hover_tooltip_offset_multiplier", 1.0f, FCVAR_DEFAULT);
ConVar cv_hud_scrubbing_timeline_strains_aim_color_b("osu_hud_scrubbing_timeline_strains_aim_color_b", 0,
                                                     FCVAR_DEFAULT);
ConVar cv_hud_scrubbing_timeline_strains_aim_color_g("osu_hud_scrubbing_timeline_strains_aim_color_g", 255,
                                                     FCVAR_DEFAULT);
ConVar cv_hud_scrubbing_timeline_strains_aim_color_r("osu_hud_scrubbing_timeline_strains_aim_color_r", 0,
                                                     FCVAR_DEFAULT);
ConVar cv_hud_scrubbing_timeline_strains_alpha("osu_hud_scrubbing_timeline_strains_alpha", 0.4f, FCVAR_DEFAULT);
ConVar cv_hud_scrubbing_timeline_strains_height("osu_hud_scrubbing_timeline_strains_height", 200.0f, FCVAR_DEFAULT);
ConVar cv_hud_scrubbing_timeline_strains_speed_color_b("osu_hud_scrubbing_timeline_strains_speed_color_b", 0,
                                                       FCVAR_DEFAULT);
ConVar cv_hud_scrubbing_timeline_strains_speed_color_g("osu_hud_scrubbing_timeline_strains_speed_color_g", 0,
                                                       FCVAR_DEFAULT);
ConVar cv_hud_scrubbing_timeline_strains_speed_color_r("osu_hud_scrubbing_timeline_strains_speed_color_r", 255,
                                                       FCVAR_DEFAULT);
ConVar cv_hud_shift_tab_toggles_everything("osu_hud_shift_tab_toggles_everything", true, FCVAR_DEFAULT);
ConVar cv_hud_statistics_ar_offset_x("osu_hud_statistics_ar_offset_x", 0.0f, FCVAR_DEFAULT);
ConVar cv_hud_statistics_ar_offset_y("osu_hud_statistics_ar_offset_y", 0.0f, FCVAR_DEFAULT);
ConVar cv_hud_statistics_bpm_offset_x("osu_hud_statistics_bpm_offset_x", 0.0f, FCVAR_DEFAULT);
ConVar cv_hud_statistics_bpm_offset_y("osu_hud_statistics_bpm_offset_y", 0.0f, FCVAR_DEFAULT);
ConVar cv_hud_statistics_cs_offset_x("osu_hud_statistics_cs_offset_x", 0.0f, FCVAR_DEFAULT);
ConVar cv_hud_statistics_cs_offset_y("osu_hud_statistics_cs_offset_y", 0.0f, FCVAR_DEFAULT);
ConVar cv_hud_statistics_hitdelta_chunksize("osu_hud_statistics_hitdelta_chunksize", 30, FCVAR_DEFAULT,
                                            "how many recent hit deltas to average (-1 = all)");
ConVar cv_hud_statistics_hitdelta_offset_x("osu_hud_statistics_hitdelta_offset_x", 0.0f, FCVAR_DEFAULT);
ConVar cv_hud_statistics_hitdelta_offset_y("osu_hud_statistics_hitdelta_offset_y", 0.0f, FCVAR_DEFAULT);
ConVar cv_hud_statistics_hitwindow300_offset_x("osu_hud_statistics_hitwindow300_offset_x", 0.0f, FCVAR_DEFAULT);
ConVar cv_hud_statistics_hitwindow300_offset_y("osu_hud_statistics_hitwindow300_offset_y", 0.0f, FCVAR_DEFAULT);
ConVar cv_hud_statistics_hp_offset_x("osu_hud_statistics_hp_offset_x", 0.0f, FCVAR_DEFAULT);
ConVar cv_hud_statistics_hp_offset_y("osu_hud_statistics_hp_offset_y", 0.0f, FCVAR_DEFAULT);
ConVar cv_hud_statistics_livestars_offset_x("osu_hud_statistics_livestars_offset_x", 0.0f, FCVAR_DEFAULT);
ConVar cv_hud_statistics_livestars_offset_y("osu_hud_statistics_livestars_offset_y", 0.0f, FCVAR_DEFAULT);
ConVar cv_hud_statistics_maxpossiblecombo_offset_x("osu_hud_statistics_maxpossiblecombo_offset_x", 0.0f, FCVAR_DEFAULT);
ConVar cv_hud_statistics_maxpossiblecombo_offset_y("osu_hud_statistics_maxpossiblecombo_offset_y", 0.0f, FCVAR_DEFAULT);
ConVar cv_hud_statistics_misses_offset_x("osu_hud_statistics_misses_offset_x", 0.0f, FCVAR_DEFAULT);
ConVar cv_hud_statistics_misses_offset_y("osu_hud_statistics_misses_offset_y", 0.0f, FCVAR_DEFAULT);
ConVar cv_hud_statistics_nd_offset_x("osu_hud_statistics_nd_offset_x", 0.0f, FCVAR_DEFAULT);
ConVar cv_hud_statistics_nd_offset_y("osu_hud_statistics_nd_offset_y", 0.0f, FCVAR_DEFAULT);
ConVar cv_hud_statistics_nps_offset_x("osu_hud_statistics_nps_offset_x", 0.0f, FCVAR_DEFAULT);
ConVar cv_hud_statistics_nps_offset_y("osu_hud_statistics_nps_offset_y", 0.0f, FCVAR_DEFAULT);
ConVar cv_hud_statistics_od_offset_x("osu_hud_statistics_od_offset_x", 0.0f, FCVAR_DEFAULT);
ConVar cv_hud_statistics_od_offset_y("osu_hud_statistics_od_offset_y", 0.0f, FCVAR_DEFAULT);
ConVar cv_hud_statistics_offset_x("osu_hud_statistics_offset_x", 5.0f, FCVAR_DEFAULT);
ConVar cv_hud_statistics_offset_y("osu_hud_statistics_offset_y", 50.0f, FCVAR_DEFAULT);
ConVar cv_hud_statistics_perfectpp_offset_x("osu_hud_statistics_perfectpp_offset_x", 0.0f, FCVAR_DEFAULT);
ConVar cv_hud_statistics_perfectpp_offset_y("osu_hud_statistics_perfectpp_offset_y", 0.0f, FCVAR_DEFAULT);
ConVar cv_hud_statistics_pp_decimal_places("osu_hud_statistics_pp_decimal_places", 0, FCVAR_DEFAULT,
                                           "number of decimal places for the live pp counter (min = 0, max = 2)");
ConVar cv_hud_statistics_pp_offset_x("osu_hud_statistics_pp_offset_x", 0.0f, FCVAR_DEFAULT);
ConVar cv_hud_statistics_pp_offset_y("osu_hud_statistics_pp_offset_y", 0.0f, FCVAR_DEFAULT);
ConVar cv_hud_statistics_scale("osu_hud_statistics_scale", 1.0f, FCVAR_DEFAULT);
ConVar cv_hud_statistics_sliderbreaks_offset_x("osu_hud_statistics_sliderbreaks_offset_x", 0.0f, FCVAR_DEFAULT);
ConVar cv_hud_statistics_sliderbreaks_offset_y("osu_hud_statistics_sliderbreaks_offset_y", 0.0f, FCVAR_DEFAULT);
ConVar cv_hud_statistics_spacing_scale("osu_hud_statistics_spacing_scale", 1.1f, FCVAR_DEFAULT);
ConVar cv_hud_statistics_totalstars_offset_x("osu_hud_statistics_totalstars_offset_x", 0.0f, FCVAR_DEFAULT);
ConVar cv_hud_statistics_totalstars_offset_y("osu_hud_statistics_totalstars_offset_y", 0.0f, FCVAR_DEFAULT);
ConVar cv_hud_statistics_ur_offset_x("osu_hud_statistics_ur_offset_x", 0.0f, FCVAR_DEFAULT);
ConVar cv_hud_statistics_ur_offset_y("osu_hud_statistics_ur_offset_y", 0.0f, FCVAR_DEFAULT);
ConVar cv_hud_volume_duration("osu_hud_volume_duration", 1.0f, FCVAR_DEFAULT);
ConVar cv_hud_volume_size_multiplier("osu_hud_volume_size_multiplier", 1.5f, FCVAR_DEFAULT);
ConVar cv_ignore_beatmap_combo_colors("osu_ignore_beatmap_combo_colors", false, FCVAR_DEFAULT);
ConVar cv_ignore_beatmap_combo_numbers("osu_ignore_beatmap_combo_numbers", false, FCVAR_DEFAULT,
                                       "may be used in conjunction with osu_number_max");
ConVar cv_ignore_beatmap_sample_volume("osu_ignore_beatmap_sample_volume", false, FCVAR_DEFAULT);
ConVar cv_instafade("instafade", false, FCVAR_DEFAULT, "don't draw hitcircle fadeout animations");
ConVar cv_instafade_sliders("instafade_sliders", false, FCVAR_DEFAULT, "don't draw slider fadeout animations");
ConVar cv_instant_replay_duration("instant_replay_duration", 15.f, FCVAR_DEFAULT,
                                  "instant replay (F2) duration, in seconds");
ConVar cv_interpolate_music_pos(
    "osu_interpolate_music_pos", true, FCVAR_DEFAULT,
    "Interpolate song position with engine time if the audio library reports the same position more than once");
ConVar cv_letterboxing("osu_letterboxing", true, FCVAR_DEFAULT);
ConVar cv_letterboxing_offset_x("osu_letterboxing_offset_x", 0.0f, FCVAR_DEFAULT);
ConVar cv_letterboxing_offset_y("osu_letterboxing_offset_y", 0.0f, FCVAR_DEFAULT);
ConVar cv_load_beatmap_background_images("osu_load_beatmap_background_images", true, FCVAR_DEFAULT);
ConVar cv_main_menu_alpha("osu_main_menu_alpha", 0.8f, FCVAR_DEFAULT);
ConVar cv_main_menu_banner_always_text("osu_main_menu_banner_always_text", "", FCVAR_DEFAULT);
ConVar cv_main_menu_banner_ifupdatedfromoldversion_le3300_text(
    "osu_main_menu_banner_ifupdatedfromoldversion_le3300_text", "", FCVAR_DEFAULT);
ConVar cv_main_menu_banner_ifupdatedfromoldversion_le3303_text(
    "osu_main_menu_banner_ifupdatedfromoldversion_le3303_text", "", FCVAR_DEFAULT);
ConVar cv_main_menu_banner_ifupdatedfromoldversion_text("osu_main_menu_banner_ifupdatedfromoldversion_text", "",
                                                        FCVAR_DEFAULT);
ConVar cv_main_menu_friend("osu_main_menu_friend", true, FCVAR_DEFAULT);
ConVar cv_main_menu_startup_anim_duration("osu_main_menu_startup_anim_duration", 0.25f, FCVAR_DEFAULT);
ConVar cv_main_menu_use_server_logo("main_menu_use_server_logo", true, FCVAR_DEFAULT);
ConVar cv_mat_wireframe_("mat_wireframe", false, FCVAR_LOCKED, _mat_wireframe);
ConVar cv_minimize_on_focus_lost_if_borderless_windowed_fullscreen(
    "minimize_on_focus_lost_if_borderless_windowed_fullscreen", false, FCVAR_DEFAULT);
ConVar cv_minimize_on_focus_lost_if_fullscreen("minimize_on_focus_lost_if_fullscreen", true, FCVAR_DEFAULT);
ConVar cv_mod_anti_flashlight("mod_anti_flashlight", false, FCVAR_DEFAULT);
ConVar cv_mod_approach_different("osu_mod_approach_different", false, FCVAR_UNLOCKED,
                                 "replicates osu!lazer's \"Approach Different\" mod");
ConVar cv_mod_approach_different_initial_size(
    "osu_mod_approach_different_initial_size", 4.0f, FCVAR_DEFAULT,
    "initial size of the approach circles, relative to hit circles (as a multiplier)");
ConVar cv_mod_approach_different_style(
    "osu_mod_approach_different_style", 1, FCVAR_DEFAULT,
    "0 = linear, 1 = gravity, 2 = InOut1, 3 = InOut2, 4 = Accelerate1, 5 = Accelerate2, 6 = Accelerate3, 7 = "
    "Decelerate1, 8 = Decelerate2, 9 = Decelerate3");
ConVar cv_mod_artimewarp("osu_mod_artimewarp", false, FCVAR_UNLOCKED);
ConVar cv_mod_artimewarp_multiplier("osu_mod_artimewarp_multiplier", 0.5f, FCVAR_DEFAULT);
ConVar cv_mod_arwobble("osu_mod_arwobble", false, FCVAR_UNLOCKED);
ConVar cv_mod_arwobble_interval("osu_mod_arwobble_interval", 7.0f, FCVAR_LOCKED);
ConVar cv_mod_arwobble_strength("osu_mod_arwobble_strength", 1.0f, FCVAR_LOCKED);
ConVar cv_mod_endless("osu_mod_endless", false, FCVAR_LOCKED);
ConVar cv_mod_fadingcursor("osu_mod_fadingcursor", false, FCVAR_UNLOCKED);
ConVar cv_mod_fadingcursor_combo("osu_mod_fadingcursor_combo", 50.0f, FCVAR_DEFAULT);
ConVar cv_mod_fposu("osu_mod_fposu", false, FCVAR_DEFAULT);
ConVar cv_mod_fposu_sound_panning("osu_mod_fposu_sound_panning", false, FCVAR_DEFAULT, "see osu_sound_panning");
ConVar cv_mod_fps("osu_mod_fps", false, FCVAR_UNLOCKED);
ConVar cv_mod_fps_sound_panning("osu_mod_fps_sound_panning", false, FCVAR_DEFAULT, "see osu_sound_panning");
ConVar cv_mod_fullalternate("osu_mod_fullalternate", false, FCVAR_DEFAULT);
ConVar cv_mod_halfwindow("osu_mod_halfwindow", false, FCVAR_UNLOCKED);
ConVar cv_mod_halfwindow_allow_300s("osu_mod_halfwindow_allow_300s", true, FCVAR_DEFAULT,
                                    "should positive hit deltas be allowed within 300 range");
ConVar cv_mod_hd_circle_fadein_end_percent(
    "osu_mod_hd_circle_fadein_end_percent", 0.6f, FCVAR_LOCKED,
    "hiddenFadeInEndTime = circleTime - approachTime * osu_mod_hd_circle_fadein_end_percent");
ConVar cv_mod_hd_circle_fadein_start_percent(
    "osu_mod_hd_circle_fadein_start_percent", 1.0f, FCVAR_LOCKED,
    "hiddenFadeInStartTime = circleTime - approachTime * osu_mod_hd_circle_fadein_start_percent");
ConVar cv_mod_hd_circle_fadeout_end_percent(
    "osu_mod_hd_circle_fadeout_end_percent", 0.3f, FCVAR_LOCKED,
    "hiddenFadeOutEndTime = circleTime - approachTime * osu_mod_hd_circle_fadeout_end_percent");
ConVar cv_mod_hd_circle_fadeout_start_percent(
    "osu_mod_hd_circle_fadeout_start_percent", 0.6f, FCVAR_LOCKED,
    "hiddenFadeOutStartTime = circleTime - approachTime * osu_mod_hd_circle_fadeout_start_percent");
ConVar cv_mod_hd_slider_fade_percent("osu_mod_hd_slider_fade_percent", 1.0f, FCVAR_LOCKED);
ConVar cv_mod_hd_slider_fast_fade("osu_mod_hd_slider_fast_fade", false, FCVAR_DEFAULT);
ConVar cv_mod_jigsaw1("osu_mod_jigsaw1", false, FCVAR_UNLOCKED);
ConVar cv_mod_jigsaw2("osu_mod_jigsaw2", false, FCVAR_UNLOCKED);
ConVar cv_mod_jigsaw_followcircle_radius_factor("osu_mod_jigsaw_followcircle_radius_factor", 0.0f, FCVAR_DEFAULT);
ConVar cv_mod_mafham("osu_mod_mafham", false, FCVAR_UNLOCKED);
ConVar cv_mod_mafham_ignore_hittable_dim("osu_mod_mafham_ignore_hittable_dim", true, FCVAR_DEFAULT,
                                         "having hittable dim enabled makes it possible to \"read\" the beatmap by "
                                         "looking at the un-dim animations (thus making it a lot easier)");
ConVar cv_mod_mafham_render_chunksize("osu_mod_mafham_render_chunksize", 15, FCVAR_DEFAULT,
                                      "render this many hitobjects per frame chunk into the scene buffer (spreads "
                                      "rendering across many frames to minimize lag)");
ConVar cv_mod_mafham_render_livesize(
    "osu_mod_mafham_render_livesize", 25, FCVAR_DEFAULT,
    "render this many hitobjects without any scene buffering, higher = more lag but more up-to-date scene");
ConVar cv_mod_millhioref("osu_mod_millhioref", false, FCVAR_UNLOCKED);
ConVar cv_mod_millhioref_multiplier("osu_mod_millhioref_multiplier", 2.0f, FCVAR_LOCKED);
ConVar cv_mod_ming3012("osu_mod_ming3012", false, FCVAR_UNLOCKED);
ConVar cv_mod_minimize("osu_mod_minimize", false, FCVAR_UNLOCKED);
ConVar cv_mod_minimize_multiplier("osu_mod_minimize_multiplier", 0.5f, FCVAR_LOCKED);
ConVar cv_mod_no100s("osu_mod_no100s", false, FCVAR_UNLOCKED);
ConVar cv_mod_no50s("osu_mod_no50s", false, FCVAR_UNLOCKED);
ConVar cv_mod_reverse_sliders("osu_mod_reverse_sliders", false, FCVAR_UNLOCKED);
ConVar cv_mod_shirone("osu_mod_shirone", false, FCVAR_UNLOCKED);
ConVar cv_mod_shirone_combo("osu_mod_shirone_combo", 20.0f, FCVAR_DEFAULT);
ConVar cv_mod_strict_tracking("osu_mod_strict_tracking", false, FCVAR_UNLOCKED);
ConVar cv_mod_strict_tracking_remove_slider_ticks("osu_mod_strict_tracking_remove_slider_ticks", false, FCVAR_LOCKED,
                                                  "whether the strict tracking mod should remove slider ticks or not, "
                                                  "this changed after its initial implementation in lazer");
ConVar cv_mod_suddendeath_restart("osu_mod_suddendeath_restart", false, FCVAR_UNLOCK_SINGLEPLAYER | FCVAR_ALWAYS_SUBMIT,
                                  "osu! has this set to false (i.e. you fail after missing). if set to true, then "
                                  "behave like SS/PF, instantly restarting the map");
ConVar cv_mod_target_100_percent("osu_mod_target_100_percent", 0.7f, FCVAR_LOCKED);
ConVar cv_mod_target_300_percent("osu_mod_target_300_percent", 0.5f, FCVAR_LOCKED);
ConVar cv_mod_target_50_percent("osu_mod_target_50_percent", 0.95f, FCVAR_LOCKED);
ConVar cv_mod_timewarp("osu_mod_timewarp", false, FCVAR_UNLOCKED);
ConVar cv_mod_timewarp_multiplier("osu_mod_timewarp_multiplier", 1.5f, FCVAR_DEFAULT);
ConVar cv_mod_touchdevice("osu_mod_touchdevice", false, FCVAR_DEFAULT, "used for force applying touch pp nerf always");
ConVar cv_mod_wobble("osu_mod_wobble", false, FCVAR_UNLOCKED);
ConVar cv_mod_wobble2("osu_mod_wobble2", false, FCVAR_UNLOCKED);
ConVar cv_mod_wobble_frequency("osu_mod_wobble_frequency", 1.0f, FCVAR_DEFAULT);
ConVar cv_mod_wobble_rotation_speed("osu_mod_wobble_rotation_speed", 1.0f, FCVAR_DEFAULT);
ConVar cv_mod_wobble_strength("osu_mod_wobble_strength", 25.0f, FCVAR_DEFAULT);
ConVar cv_monitor("monitor", 0, FCVAR_DEFAULT, "monitor/display device to switch to, 0 = primary monitor", _monitor);
ConVar cv_mouse_fakelag("mouse_fakelag", 0.000f, FCVAR_DEFAULT,
                        "delay all mouse movement by this many seconds (e.g. 0.1 = 100 ms delay)");
ConVar cv_mouse_raw_input("mouse_raw_input", false, FCVAR_DEFAULT);
ConVar cv_mouse_raw_input_absolute_to_window("mouse_raw_input_absolute_to_window", false, FCVAR_DEFAULT);
ConVar cv_mouse_sensitivity("mouse_sensitivity", 1.0f, FCVAR_DEFAULT);
ConVar cv_mp_autologin("mp_autologin", false, FCVAR_DEFAULT);
ConVar cv_mp_password("mp_password", "", FCVAR_DEFAULT | FCVAR_HIDDEN);
ConVar cv_mp_server("mp_server", "akatsuki.gg", FCVAR_DEFAULT);
ConVar cv_name("name", "Guest", FCVAR_DEFAULT);
ConVar cv_nightcore_enjoyer("nightcore_enjoyer", false, FCVAR_DEFAULT,
                            "automatically select nightcore when speed modifying");
ConVar cv_normalize_loudness("normalize_loudness", false, FCVAR_DEFAULT, "normalize loudness across songs");
ConVar cv_notelock_stable_tolerance2b("osu_notelock_stable_tolerance2b", 3, FCVAR_LOCKED,
                                      "time tolerance in milliseconds to allow hitting simultaneous objects close "
                                      "together (e.g. circle at end of slider)");
ConVar cv_notelock_type("osu_notelock_type", 2, FCVAR_LOCKED,
                        "which notelock algorithm to use (0 = None, 1 = neosu, 2 = osu!stable, 3 = osu!lazer 2020)");
ConVar cv_notification("osu_notification");
ConVar cv_notification_color_b("osu_notification_color_b", 255, FCVAR_DEFAULT);
ConVar cv_notification_color_g("osu_notification_color_g", 255, FCVAR_DEFAULT);
ConVar cv_notification_color_r("osu_notification_color_r", 255, FCVAR_DEFAULT);
ConVar cv_notification_duration("osu_notification_duration", 1.25f, FCVAR_DEFAULT);
ConVar cv_number_max("osu_number_max", 0, FCVAR_DEFAULT,
                     "0 = disabled, 1/2/3/4/etc. limits visual circle numbers to this number");
ConVar cv_number_scale_multiplier("osu_number_scale_multiplier", 1.0f, FCVAR_DEFAULT);
ConVar cv_od_override("osu_od_override", -1.0f, FCVAR_UNLOCKED);
ConVar cv_od_override_lock("osu_od_override_lock", false, FCVAR_UNLOCKED,
                           "always force constant OD even through speed changes");
ConVar cv_old_beatmap_offset(
    "osu_old_beatmap_offset", 24.0f, FCVAR_DEFAULT,
    "offset in ms which is added to beatmap versions < 5 (default value is hardcoded 24 ms in stable)");
ConVar cv_options_high_quality_sliders("osu_options_high_quality_sliders", false, FCVAR_DEFAULT);
ConVar cv_options_save_on_back("osu_options_save_on_back", true, FCVAR_DEFAULT);
ConVar cv_options_slider_preview_use_legacy_renderer(
    "osu_options_slider_preview_use_legacy_renderer", false, FCVAR_DEFAULT,
    "apparently newer AMD drivers with old gpus are crashing here with the legacy renderer? was just me being lazy "
    "anyway, so now there is a vao render path as it should be");
ConVar cv_options_slider_quality("osu_options_slider_quality", 0.0f, FCVAR_DEFAULT, _osuOptionsSliderQualityWrapper);
ConVar cv_pause_anim_duration("osu_pause_anim_duration", 0.15f, FCVAR_DEFAULT);
ConVar cv_pause_dim_alpha("osu_pause_dim_alpha", 0.58f, FCVAR_DEFAULT);
ConVar cv_pause_dim_background("osu_pause_dim_background", true, FCVAR_DEFAULT);
ConVar cv_pause_on_focus_loss("osu_pause_on_focus_loss", true, FCVAR_DEFAULT);
ConVar cv_play_hitsound_on_click_while_playing("osu_play_hitsound_on_click_while_playing", false, FCVAR_DEFAULT);
ConVar cv_playfield_border_bottom_percent("osu_playfield_border_bottom_percent", 0.0834f, FCVAR_DEFAULT);
ConVar cv_playfield_border_top_percent("osu_playfield_border_top_percent", 0.117f, FCVAR_DEFAULT);
ConVar cv_playfield_mirror_horizontal("osu_playfield_mirror_horizontal", false, FCVAR_DEFAULT);
ConVar cv_playfield_mirror_vertical("osu_playfield_mirror_vertical", false, FCVAR_DEFAULT);
ConVar cv_playfield_rotation("osu_playfield_rotation", 0.0f, FCVAR_LOCKED,
                             "rotates the entire playfield by this many degrees");
ConVar cv_pp_live_timeout(
    "osu_pp_live_timeout", 1.0f, FCVAR_DEFAULT,
    "show message that we're still calculating stars after this many seconds, on the first start of the beatmap");
ConVar cv_pvs("osu_pvs", true, FCVAR_DEFAULT,
              "optimizes all loops over all hitobjects by clamping the range to the Potentially Visible Set");
ConVar cv_quick_retry_delay("osu_quick_retry_delay", 0.27f, FCVAR_DEFAULT);
ConVar cv_quick_retry_time(
    "osu_quick_retry_time", 2000.0f, FCVAR_DEFAULT,
    "Timeframe in ms subtracted from the first hitobject when quick retrying (not regular retry)");
ConVar cv_r_3dscene_zf("r_3dscene_zf", 5000.0f, FCVAR_LOCKED);
ConVar cv_r_3dscene_zn("r_3dscene_zn", 5.0f, FCVAR_LOCKED);
ConVar cv_r_debug_disable_3dscene("r_debug_disable_3dscene", false, FCVAR_LOCKED);
ConVar cv_r_debug_disable_cliprect("r_debug_disable_cliprect", false, FCVAR_LOCKED);
ConVar cv_r_debug_drawimage("r_debug_drawimage", false, FCVAR_LOCKED);
ConVar cv_r_debug_drawstring_unbind("r_debug_drawstring_unbind", false, FCVAR_DEFAULT);
ConVar cv_r_debug_flush_drawstring("r_debug_flush_drawstring", false, FCVAR_DEFAULT);
ConVar cv_r_drawstring_max_string_length("r_drawstring_max_string_length", 65536, FCVAR_LOCKED,
                                         "maximum number of characters per call, sanity/memory buffer limit");
ConVar cv_r_globaloffset_x("r_globaloffset_x", 0.0f, FCVAR_LOCKED);
ConVar cv_r_globaloffset_y("r_globaloffset_y", 0.0f, FCVAR_LOCKED);
ConVar cv_r_image_unbind_after_drawimage("r_image_unbind_after_drawimage", true, FCVAR_DEFAULT);
ConVar cv_r_opengl_legacy_vao_use_vertex_array(
    "r_opengl_legacy_vao_use_vertex_array", false, FCVAR_LOCKED,
    "dramatically reduces per-vao draw calls, but completely breaks legacy ffp draw calls (vertices work, but "
    "texcoords/normals/etc. are NOT in gl_MultiTexCoord0 -> requiring a shader with attributes)");
ConVar cv_rankingscreen_pp("osu_rankingscreen_pp", true, FCVAR_DEFAULT);
ConVar cv_rankingscreen_topbar_height_percent("osu_rankingscreen_topbar_height_percent", 0.785f, FCVAR_DEFAULT);
ConVar cv_relax_offset(
    "osu_relax_offset", -12, FCVAR_DEFAULT,
    "osu!relax always hits -12 ms too early, so set this to -12 (note the negative) if you want it to be the same");
ConVar cv_resolution("osu_resolution", "1280x720", FCVAR_DEFAULT);
ConVar cv_resolution_enabled("osu_resolution_enabled", false, FCVAR_DEFAULT);
ConVar cv_resolution_keep_aspect_ratio("osu_resolution_keep_aspect_ratio", false, FCVAR_DEFAULT);
ConVar cv_restart_sound_engine_before_playing(
    "restart_sound_engine_before_playing", false, FCVAR_DEFAULT,
    "jank fix for users who experience sound issues after playing for a while");
ConVar cv_rich_presence("osu_rich_presence", true, FCVAR_DEFAULT, RichPresence::onRichPresenceChange);
ConVar cv_rich_presence_discord_show_totalpp("osu_rich_presence_discord_show_totalpp", true, FCVAR_DEFAULT);
ConVar cv_rich_presence_dynamic_windowtitle(
    "osu_rich_presence_dynamic_windowtitle", true, FCVAR_DEFAULT,
    "should the window title show the currently playing beatmap Artist - Title and [Difficulty] name");
ConVar cv_rich_presence_show_recentplaystats("osu_rich_presence_show_recentplaystats", true, FCVAR_DEFAULT);
ConVar cv_rm_debug_async_delay("rm_debug_async_delay", 0.0f, FCVAR_LOCKED);
ConVar cv_rm_interrupt_on_destroy("rm_interrupt_on_destroy", true, FCVAR_LOCKED);
ConVar cv_rm_numthreads(
    "rm_numthreads", 3, FCVAR_DEFAULT,
    "how many parallel resource loader threads are spawned once on startup (!), and subsequently used during runtime");
ConVar cv_rm_warnings("rm_warnings", false, FCVAR_DEFAULT);
ConVar cv_scoreboard_animations("scoreboard_animations", true, FCVAR_DEFAULT, "animate in-game scoreboard");
ConVar cv_scores_bonus_pp("osu_scores_bonus_pp", true, FCVAR_DEFAULT,
                          "whether to add bonus pp to total (real) pp or not");
ConVar cv_scores_enabled("osu_scores_enabled", true, FCVAR_DEFAULT);
ConVar cv_scores_save_immediately("osu_scores_save_immediately", true, FCVAR_DEFAULT,
                                  "write scores.db as soon as a new score is added");
ConVar cv_scores_sort_by_pp("osu_scores_sort_by_pp", true, FCVAR_DEFAULT,
                            "display pp in score browser instead of score");
ConVar cv_scrubbing_smooth("osu_scrubbing_smooth", true, FCVAR_DEFAULT);
ConVar cv_sdl_joystick0_deadzone("sdl_joystick0_deadzone", 0.3f, FCVAR_DEFAULT);
ConVar cv_sdl_joystick_mouse_sensitivity("sdl_joystick_mouse_sensitivity", 1.0f, FCVAR_DEFAULT);
ConVar cv_sdl_joystick_zl_threshold("sdl_joystick_zl_threshold", -0.5f, FCVAR_DEFAULT);
ConVar cv_sdl_joystick_zr_threshold("sdl_joystick_zr_threshold", -0.5f, FCVAR_DEFAULT);
ConVar cv_seek_delta("osu_seek_delta", 5, FCVAR_DEFAULT,
                     "how many seconds to skip backward/forward when quick seeking");
ConVar cv_show_approach_circle_on_first_hidden_object("osu_show_approach_circle_on_first_hidden_object", true,
                                                      FCVAR_DEFAULT);
ConVar cv_skin("osu_skin", "default", FCVAR_DEFAULT);
ConVar cv_skin_animation_force("osu_skin_animation_force", false, FCVAR_DEFAULT);
ConVar cv_skin_animation_fps_override("osu_skin_animation_fps_override", -1.0f, FCVAR_DEFAULT);
ConVar cv_skin_async("osu_skin_async", true, FCVAR_DEFAULT, "load in background without blocking");
ConVar cv_skin_color_index_add("osu_skin_color_index_add", 0, FCVAR_DEFAULT);
ConVar cv_skin_force_hitsound_sample_set("osu_skin_force_hitsound_sample_set", 0, FCVAR_DEFAULT,
                                         "force a specific hitsound sample set to always be used regardless of what "
                                         "the beatmap says. 0 = disabled, 1 = normal, 2 = soft, 3 = drum.");
ConVar cv_skin_hd("osu_skin_hd", true, FCVAR_DEFAULT, "load and use @2x versions of skin images, if available");
ConVar cv_skin_mipmaps(
    "osu_skin_mipmaps", false, FCVAR_DEFAULT,
    "generate mipmaps for every skin image (only useful on lower game resolutions, requires more vram)");
ConVar cv_skin_random("osu_skin_random", false, FCVAR_DEFAULT,
                      "select random skin from list on every skin load/reload");
ConVar cv_skin_random_elements("osu_skin_random_elements", false, FCVAR_DEFAULT,
                               "sElECt RanDOM sKIn eLemENTs FRoM ranDom SkINs");
ConVar cv_skin_reload("osu_skin_reload");
ConVar cv_skin_use_skin_hitsounds(
    "osu_skin_use_skin_hitsounds", true, FCVAR_DEFAULT,
    "If enabled: Use skin's sound samples. If disabled: Use default skin's sound samples. For hitsounds only.");
ConVar cv_skip_breaks_enabled("osu_skip_breaks_enabled", true, FCVAR_DEFAULT,
                              "enables/disables skip button for breaks in the middle of beatmaps");
ConVar cv_skip_intro_enabled("osu_skip_intro_enabled", true, FCVAR_DEFAULT,
                             "enables/disables skip button for intro until first hitobject");
ConVar cv_skip_time("osu_skip_time", 5000.0f, FCVAR_LOCKED,
                    "Timeframe in ms within a beatmap which allows skipping if it doesn't contain any hitobjects");
ConVar cv_slider_alpha_multiplier("osu_slider_alpha_multiplier", 1.0f, FCVAR_DEFAULT);
ConVar cv_slider_ball_tint_combo_color("osu_slider_ball_tint_combo_color", true, FCVAR_DEFAULT);
ConVar cv_slider_body_alpha_multiplier("osu_slider_body_alpha_multiplier", 1.0f, FCVAR_DEFAULT);
ConVar cv_slider_body_color_saturation("osu_slider_body_color_saturation", 1.0f, FCVAR_DEFAULT);
ConVar cv_slider_body_fade_out_time_multiplier("osu_slider_body_fade_out_time_multiplier", 1.0f, FCVAR_DEFAULT,
                                               "multiplies osu_hitobject_fade_out_time");
ConVar cv_slider_body_lazer_fadeout_style("osu_slider_body_lazer_fadeout_style", true, FCVAR_DEFAULT,
                                          "if snaking out sliders are enabled (aka shrinking sliders), smoothly fade "
                                          "out the last remaining part of the body (instead of vanishing instantly)");
ConVar cv_slider_body_smoothsnake(
    "osu_slider_body_smoothsnake", true, FCVAR_DEFAULT,
    "draw 1 extra interpolated circle mesh at the start & end of every slider for extra smooth snaking/shrinking");
ConVar cv_slider_body_unit_circle_subdivisions("osu_slider_body_unit_circle_subdivisions", 42, FCVAR_DEFAULT);
ConVar cv_slider_border_feather("osu_slider_border_feather", 0.0f, FCVAR_DEFAULT);
ConVar cv_slider_border_size_multiplier("osu_slider_border_size_multiplier", 1.0f, FCVAR_DEFAULT);
ConVar cv_slider_border_tint_combo_color("osu_slider_border_tint_combo_color", false, FCVAR_DEFAULT);
ConVar cv_slider_break_epilepsy("osu_slider_break_epilepsy", false, FCVAR_DEFAULT);
ConVar cv_slider_curve_max_length("osu_slider_curve_max_length", 65536 / 2, FCVAR_LOCKED,
                                  "maximum slider length in osu!pixels (i.e. pixelLength). also used to clamp all "
                                  "(control-)point coordinates to sane values.");
ConVar cv_slider_curve_max_points("osu_slider_curve_max_points", 9999.0f, FCVAR_LOCKED,
                                  "maximum number of allowed interpolated curve points. quality will be forced to go "
                                  "down if a slider has more steps than this");
ConVar cv_slider_curve_points_separation(
    "osu_slider_curve_points_separation", 2.5f, FCVAR_LOCKED,
    "slider body curve approximation step width in osu!pixels, don't set this lower than around 1.5");
ConVar cv_slider_debug_draw("osu_slider_debug_draw", false, FCVAR_DEFAULT,
                            "draw hitcircle at every curve point and nothing else (no vao, no rt, no shader, nothing) "
                            "(requires enabling legacy slider renderer)");
ConVar cv_slider_debug_draw_square_vao(
    "osu_slider_debug_draw_square_vao", false, FCVAR_DEFAULT,
    "generate square vaos and nothing else (no rt, no shader) (requires disabling legacy slider renderer)");
ConVar cv_slider_debug_wireframe("osu_slider_debug_wireframe", false, FCVAR_DEFAULT, "unused");
ConVar cv_slider_draw_body("osu_slider_draw_body", true, FCVAR_DEFAULT);
ConVar cv_slider_draw_endcircle("osu_slider_draw_endcircle", true, FCVAR_DEFAULT);
ConVar cv_slider_end_inside_check_offset("osu_slider_end_inside_check_offset", 36, FCVAR_LOCKED,
                                         "offset in milliseconds going backwards from the end point, at which \"being "
                                         "inside the slider\" is checked. (osu bullshit behavior)");
ConVar cv_slider_end_miss_breaks_combo("osu_slider_end_miss_breaks_combo", false, FCVAR_DEFAULT,
                                       "should a missed sliderend break combo (aka cause a regular sliderbreak)");
ConVar cv_slider_followcircle_fadein_fade_time("osu_slider_followcircle_fadein_fade_time", 0.06f, FCVAR_DEFAULT);
ConVar cv_slider_followcircle_fadein_scale("osu_slider_followcircle_fadein_scale", 0.5f, FCVAR_DEFAULT);
ConVar cv_slider_followcircle_fadein_scale_time("osu_slider_followcircle_fadein_scale_time", 0.18f, FCVAR_DEFAULT);
ConVar cv_slider_followcircle_fadeout_fade_time("osu_slider_followcircle_fadeout_fade_time", 0.25f, FCVAR_DEFAULT);
ConVar cv_slider_followcircle_fadeout_scale("osu_slider_followcircle_fadeout_scale", 0.8f, FCVAR_DEFAULT);
ConVar cv_slider_followcircle_fadeout_scale_time("osu_slider_followcircle_fadeout_scale_time", 0.25f, FCVAR_DEFAULT);
ConVar cv_slider_followcircle_tick_pulse_scale("osu_slider_followcircle_tick_pulse_scale", 0.1f, FCVAR_DEFAULT);
ConVar cv_slider_followcircle_tick_pulse_time("osu_slider_followcircle_tick_pulse_time", 0.2f, FCVAR_DEFAULT);
ConVar cv_slider_legacy_use_baked_vao(
    "osu_slider_legacy_use_baked_vao", false, FCVAR_DEFAULT,
    "use baked cone mesh instead of raw mesh for legacy slider renderer (disabled by default because usually slower on "
    "very old gpus even though it should not be)");
ConVar cv_slider_max_repeats("osu_slider_max_repeats", 9000, FCVAR_LOCKED,
                             "maximum number of repeats allowed per slider (clamp range)");
ConVar cv_slider_max_ticks("osu_slider_max_ticks", 2048, FCVAR_LOCKED,
                           "maximum number of ticks allowed per slider (clamp range)");
ConVar cv_slider_osu_next_style("osu_slider_osu_next_style", false, FCVAR_DEFAULT);
ConVar cv_slider_rainbow("osu_slider_rainbow", false, FCVAR_DEFAULT);
ConVar cv_slider_reverse_arrow_alpha_multiplier("osu_slider_reverse_arrow_alpha_multiplier", 1.0f, FCVAR_DEFAULT);
ConVar cv_slider_reverse_arrow_animated("osu_slider_reverse_arrow_animated", true, FCVAR_DEFAULT,
                                        "pulse animation on reverse arrows");
ConVar cv_slider_reverse_arrow_black_threshold(
    "osu_slider_reverse_arrow_black_threshold", 1.0f, FCVAR_DEFAULT,
    "Blacken reverse arrows if the average color brightness percentage is above this value");
ConVar cv_slider_reverse_arrow_fadein_duration("osu_slider_reverse_arrow_fadein_duration", 150, FCVAR_DEFAULT,
                                               "duration in ms of the reverse arrow fadein animation after it starts");
ConVar cv_slider_shrink("osu_slider_shrink", false, FCVAR_DEFAULT);
ConVar cv_slider_sliderhead_fadeout("osu_slider_sliderhead_fadeout", true, FCVAR_DEFAULT);
ConVar cv_slider_snake_duration_multiplier("osu_slider_snake_duration_multiplier", 1.0f, FCVAR_DEFAULT,
                                           "the default snaking duration is multiplied with this (max sensible value "
                                           "is 3, anything above that will take longer than the approachtime)");
ConVar cv_slider_use_gradient_image("osu_slider_use_gradient_image", false, FCVAR_DEFAULT);
ConVar cv_snaking_sliders("osu_snaking_sliders", true, FCVAR_DEFAULT);
ConVar cv_snd_async_buffer("snd_async_buffer", 65536, FCVAR_DEFAULT | FCVAR_PRIVATE,
                           "BASS_CONFIG_ASYNCFILE_BUFFER length in bytes. Set to 0 to disable.");
ConVar cv_snd_change_check_interval("snd_change_check_interval", 0.0f, FCVAR_DEFAULT | FCVAR_PRIVATE,
                                    "check for output device changes every this many seconds. 0 = disabled (default)");
ConVar cv_snd_dev_buffer("snd_dev_buffer", 30, FCVAR_DEFAULT | FCVAR_PRIVATE,
                         "BASS_CONFIG_DEV_BUFFER length in milliseconds");
ConVar cv_snd_dev_period("snd_dev_period", 10, FCVAR_DEFAULT | FCVAR_PRIVATE,
                         "BASS_CONFIG_DEV_PERIOD length in milliseconds, or if negative then in samples");
ConVar cv_snd_freq("snd_freq", 44100, FCVAR_DEFAULT | FCVAR_PRIVATE, "output sampling rate in Hz");
ConVar cv_snd_output_device("snd_output_device", "Default", FCVAR_DEFAULT | FCVAR_PRIVATE);
ConVar cv_snd_play_interp_duration(
    "snd_play_interp_duration", 0.75f, FCVAR_DEFAULT,
    "smooth over freshly started channel position jitter with engine time over this duration in seconds");
ConVar cv_snd_play_interp_ratio("snd_play_interp_ratio", 0.50f, FCVAR_DEFAULT,
                                "percentage of snd_play_interp_duration to use 100% engine time over audio time (some "
                                "devices report 0 for very long)");
ConVar cv_snd_ready_delay("snd_ready_delay", 0.0f, FCVAR_DEFAULT | FCVAR_PRIVATE,
                          "after a sound engine restart, wait this many seconds before marking it as ready");
ConVar cv_snd_restart("snd_restart");
ConVar cv_snd_restrict_play_frame(
    "snd_restrict_play_frame", true, FCVAR_DEFAULT | FCVAR_PRIVATE,
    "only allow one new channel per frame for overlayable sounds (prevents lag and earrape)");
ConVar cv_snd_updateperiod("snd_updateperiod", 10, FCVAR_DEFAULT | FCVAR_PRIVATE,
                           "BASS_CONFIG_UPDATEPERIOD length in milliseconds");
ConVar cv_snd_wav_file_min_size(
    "snd_wav_file_min_size", 51, FCVAR_DEFAULT,
    "minimum file size in bytes for WAV files to be considered valid (everything below will "
    "fail to load), this is a workaround for BASS crashes");
ConVar cv_songbrowser_background_fade_in_duration("osu_songbrowser_background_fade_in_duration", 0.1f, FCVAR_DEFAULT);
ConVar cv_songbrowser_background_star_calculation("osu_songbrowser_background_star_calculation", true, FCVAR_DEFAULT,
                                                  "precalculate stars for all loaded beatmaps while in songbrowser");
ConVar cv_songbrowser_bottombar_percent("osu_songbrowser_bottombar_percent", 0.116f, FCVAR_DEFAULT);
ConVar cv_songbrowser_button_active_color_a("osu_songbrowser_button_active_color_a", 220 + 10, FCVAR_DEFAULT);
ConVar cv_songbrowser_button_active_color_b("osu_songbrowser_button_active_color_b", 255, FCVAR_DEFAULT);
ConVar cv_songbrowser_button_active_color_g("osu_songbrowser_button_active_color_g", 255, FCVAR_DEFAULT);
ConVar cv_songbrowser_button_active_color_r("osu_songbrowser_button_active_color_r", 255, FCVAR_DEFAULT);
ConVar cv_songbrowser_button_collection_active_color_a("osu_songbrowser_button_collection_active_color_a", 255,
                                                       FCVAR_DEFAULT);
ConVar cv_songbrowser_button_collection_active_color_b("osu_songbrowser_button_collection_active_color_b", 44,
                                                       FCVAR_DEFAULT);
ConVar cv_songbrowser_button_collection_active_color_g("osu_songbrowser_button_collection_active_color_g", 240,
                                                       FCVAR_DEFAULT);
ConVar cv_songbrowser_button_collection_active_color_r("osu_songbrowser_button_collection_active_color_r", 163,
                                                       FCVAR_DEFAULT);
ConVar cv_songbrowser_button_collection_inactive_color_a("osu_songbrowser_button_collection_inactive_color_a", 255,
                                                         FCVAR_DEFAULT);
ConVar cv_songbrowser_button_collection_inactive_color_b("osu_songbrowser_button_collection_inactive_color_b", 143,
                                                         FCVAR_DEFAULT);
ConVar cv_songbrowser_button_collection_inactive_color_g("osu_songbrowser_button_collection_inactive_color_g", 50,
                                                         FCVAR_DEFAULT);
ConVar cv_songbrowser_button_collection_inactive_color_r("osu_songbrowser_button_collection_inactive_color_r", 35,
                                                         FCVAR_DEFAULT);
ConVar cv_songbrowser_button_difficulty_inactive_color_a("osu_songbrowser_button_difficulty_inactive_color_a", 255,
                                                         FCVAR_DEFAULT);
ConVar cv_songbrowser_button_difficulty_inactive_color_b("osu_songbrowser_button_difficulty_inactive_color_b", 236,
                                                         FCVAR_DEFAULT);
ConVar cv_songbrowser_button_difficulty_inactive_color_g("osu_songbrowser_button_difficulty_inactive_color_g", 150,
                                                         FCVAR_DEFAULT);
ConVar cv_songbrowser_button_difficulty_inactive_color_r("osu_songbrowser_button_difficulty_inactive_color_r", 0,
                                                         FCVAR_DEFAULT);
ConVar cv_songbrowser_button_inactive_color_a("osu_songbrowser_button_inactive_color_a", 240, FCVAR_DEFAULT);
ConVar cv_songbrowser_button_inactive_color_b("osu_songbrowser_button_inactive_color_b", 153, FCVAR_DEFAULT);
ConVar cv_songbrowser_button_inactive_color_g("osu_songbrowser_button_inactive_color_g", 73, FCVAR_DEFAULT);
ConVar cv_songbrowser_button_inactive_color_r("osu_songbrowser_button_inactive_color_r", 235, FCVAR_DEFAULT);
ConVar cv_songbrowser_scorebrowser_enabled("osu_songbrowser_scorebrowser_enabled", true, FCVAR_DEFAULT);
ConVar cv_songbrowser_scores_sortingtype("osu_songbrowser_scores_sortingtype", "Sort By Score", FCVAR_DEFAULT);
ConVar cv_songbrowser_search_delay("osu_songbrowser_search_delay", 0.2f, FCVAR_DEFAULT,
                                   "delay until search update when entering text");
ConVar cv_songbrowser_search_hardcoded_filter("osu_songbrowser_search_hardcoded_filter", "", FCVAR_DEFAULT,
                                              "allows forcing the specified search filter to be active all the time",
                                              _osu_songbrowser_search_hardcoded_filter);
ConVar cv_songbrowser_sortingtype("osu_songbrowser_sortingtype", "By Date Added", FCVAR_DEFAULT);
ConVar cv_songbrowser_thumbnail_delay("osu_songbrowser_thumbnail_delay", 0.1f, FCVAR_DEFAULT);
ConVar cv_songbrowser_thumbnail_fade_in_duration("osu_songbrowser_thumbnail_fade_in_duration", 0.1f, FCVAR_DEFAULT);
ConVar cv_songbrowser_topbar_left_percent("osu_songbrowser_topbar_left_percent", 0.93f, FCVAR_DEFAULT);
ConVar cv_songbrowser_topbar_left_width_percent("osu_songbrowser_topbar_left_width_percent", 0.265f, FCVAR_DEFAULT);
ConVar cv_songbrowser_topbar_middle_width_percent("osu_songbrowser_topbar_middle_width_percent", 0.15f, FCVAR_DEFAULT);
ConVar cv_songbrowser_topbar_right_height_percent("osu_songbrowser_topbar_right_height_percent", 0.5f, FCVAR_DEFAULT);
ConVar cv_songbrowser_topbar_right_percent("osu_songbrowser_topbar_right_percent", 0.378f, FCVAR_DEFAULT);
ConVar cv_sort_skins_by_name("sort_skins_by_name", true, FCVAR_DEFAULT, "set to false to use old behavior");
ConVar cv_sound_panning("osu_sound_panning", true, FCVAR_DEFAULT,
                        "positional hitsound audio depending on the playfield position");
ConVar cv_sound_panning_multiplier("osu_sound_panning_multiplier", 1.0f, FCVAR_DEFAULT,
                                   "the final panning value is multiplied with this, e.g. if you want to reduce or "
                                   "increase the effect strength by a percentage");
ConVar cv_speed_override("osu_speed_override", -1.0f, FCVAR_UNLOCKED);
ConVar cv_spinner_fade_out_time_multiplier("osu_spinner_fade_out_time_multiplier", 0.7f, FCVAR_LOCKED);
ConVar cv_spinner_use_ar_fadein(
    "osu_spinner_use_ar_fadein", false, FCVAR_DEFAULT,
    "whether spinners should fade in with AR (same as circles), or with hardcoded 400 ms fadein time (osu!default)");
ConVar cv_stars_slider_curve_points_separation(
    "osu_stars_slider_curve_points_separation", 20.0f, FCVAR_DEFAULT,
    "massively reduce curve accuracy for star calculations to save memory/performance");
ConVar cv_stars_stacking("osu_stars_stacking", true, FCVAR_DEFAULT,
                         "respect hitobject stacking before calculating stars/pp");
ConVar cv_start_first_main_menu_song_at_preview_point("start_first_main_menu_song_at_preview_point", false,
                                                      FCVAR_DEFAULT);
ConVar cv_submit_after_pause("submit_after_pause", true, FCVAR_DEFAULT);
ConVar cv_submit_scores("submit_scores", false, FCVAR_DEFAULT);
ConVar cv_tablet_sensitivity_ignore("tablet_sensitivity_ignore", false, FCVAR_DEFAULT);
ConVar cv_timingpoints_force("osu_timingpoints_force", true, FCVAR_DEFAULT,
                             "Forces the correct sample type and volume to be used, by getting the active timingpoint "
                             "through iteration EVERY TIME a hitsound is played (performance!)");
ConVar cv_timingpoints_offset("osu_timingpoints_offset", 5.0f, FCVAR_DEFAULT,
                              "Offset in ms which is added before determining the active timingpoint for the sample "
                              "type and sample volume (hitsounds) of the current frame");
ConVar cv_tooltip_anim_duration("osu_tooltip_anim_duration", 0.4f, FCVAR_DEFAULT);
ConVar cv_ui_scale("osu_ui_scale", 1.0f, FCVAR_DEFAULT, "multiplier");
ConVar cv_ui_scale_to_dpi("osu_ui_scale_to_dpi", true, FCVAR_DEFAULT,
                          "whether the game should scale its UI based on the DPI reported by your operating system");
ConVar cv_ui_scale_to_dpi_minimum_height(
    "osu_ui_scale_to_dpi_minimum_height", 1300, FCVAR_DEFAULT,
    "any in-game resolutions below this will have osu_ui_scale_to_dpi force disabled");
ConVar cv_ui_scale_to_dpi_minimum_width(
    "osu_ui_scale_to_dpi_minimum_width", 2200, FCVAR_DEFAULT,
    "any in-game resolutions below this will have osu_ui_scale_to_dpi force disabled");
ConVar cv_ui_scrollview_kinetic_approach_time("ui_scrollview_kinetic_approach_time", 0.075f, FCVAR_DEFAULT,
                                              "approach target afterscroll delta over this duration");
ConVar cv_ui_scrollview_kinetic_energy_multiplier("ui_scrollview_kinetic_energy_multiplier", 24.0f, FCVAR_DEFAULT,
                                                  "afterscroll delta multiplier");
ConVar cv_ui_scrollview_mousewheel_multiplier("ui_scrollview_mousewheel_multiplier", 3.5f, FCVAR_DEFAULT);
ConVar cv_ui_scrollview_mousewheel_overscrollbounce("ui_scrollview_mousewheel_overscrollbounce", true, FCVAR_DEFAULT);
ConVar cv_ui_scrollview_resistance("ui_scrollview_resistance", 5.0f, FCVAR_DEFAULT,
                                   "how many pixels you have to pull before you start scrolling");
ConVar cv_ui_scrollview_scrollbarwidth("ui_scrollview_scrollbarwidth", 15.0f, FCVAR_DEFAULT);
ConVar cv_ui_textbox_caret_blink_time("ui_textbox_caret_blink_time", 0.5f, FCVAR_DEFAULT);
ConVar cv_ui_textbox_text_offset_x("ui_textbox_text_offset_x", 3, FCVAR_DEFAULT);
ConVar cv_ui_window_animspeed("ui_window_animspeed", 0.29f, FCVAR_DEFAULT);
ConVar cv_ui_window_shadow_radius("ui_window_shadow_radius", 13.0f, FCVAR_DEFAULT);
ConVar cv_universal_offset("osu_universal_offset", 0.0f, FCVAR_DEFAULT);
ConVar cv_universal_offset_hardcoded("osu_universal_offset_hardcoded", 0.0f, FCVAR_DEFAULT | FCVAR_PRIVATE);
ConVar cv_use_https("use_https", true, FCVAR_DEFAULT);
ConVar cv_use_ppv3("use_ppv3", false, FCVAR_DEFAULT, "use ppv3 instead of ppv2 (experimental)");
ConVar cv_user_draw_accuracy("osu_user_draw_accuracy", true, FCVAR_DEFAULT);
ConVar cv_user_draw_level("osu_user_draw_level", true, FCVAR_DEFAULT);
ConVar cv_user_draw_level_bar("osu_user_draw_level_bar", true, FCVAR_DEFAULT);
ConVar cv_user_draw_pp("osu_user_draw_pp", true, FCVAR_DEFAULT);
ConVar cv_user_include_relax_and_autopilot_for_stats("osu_user_include_relax_and_autopilot_for_stats", false,
                                                     FCVAR_DEFAULT);
ConVar cv_version("osu_version", 36.00f, FCVAR_DEFAULT | FCVAR_HIDDEN);
ConVar cv_volume("volume", 1.0f, FCVAR_DEFAULT | FCVAR_PRIVATE, _volume);
ConVar cv_volume_change_interval("osu_volume_change_interval", 0.05f, FCVAR_DEFAULT);
ConVar cv_volume_effects("osu_volume_effects", 1.0f, FCVAR_DEFAULT);
ConVar cv_volume_master("osu_volume_master", 1.0f, FCVAR_DEFAULT);
ConVar cv_volume_master_inactive("osu_volume_master_inactive", 0.25f, FCVAR_DEFAULT);
ConVar cv_volume_music("osu_volume_music", 0.4f, FCVAR_DEFAULT);
ConVar cv_vprof("vprof", false, FCVAR_DEFAULT, "enables/disables the visual profiler", _vprof);
ConVar cv_vprof_display_mode("vprof_display_mode", 0, FCVAR_DEFAULT,
                             "which info blade to show on the top right (gpu/engine/app/etc. info), use CTRL + TAB to "
                             "cycle through, 0 = disabled");
ConVar cv_vprof_graph("vprof_graph", true, FCVAR_DEFAULT, "whether to draw the graph when the overlay is enabled");
ConVar cv_vprof_graph_alpha("vprof_graph_alpha", 1.0f, FCVAR_DEFAULT, "line opacity");
ConVar cv_vprof_graph_draw_overhead("vprof_graph_draw_overhead", false, FCVAR_DEFAULT,
                                    "whether to draw the profiling overhead time in white (usually negligible)");
ConVar cv_vprof_graph_height("vprof_graph_height", 250.0f, FCVAR_DEFAULT);
ConVar cv_vprof_graph_margin("vprof_graph_margin", 40.0f, FCVAR_DEFAULT);
ConVar cv_vprof_graph_range_max("vprof_graph_range_max", 16.666666f, FCVAR_DEFAULT,
                                "max value of the y-axis in milliseconds");
ConVar cv_vprof_graph_width("vprof_graph_width", 800.0f, FCVAR_DEFAULT);
ConVar cv_vprof_spike("vprof_spike", 0, FCVAR_DEFAULT,
                      "measure and display largest spike details (1 = small info, 2 = extended info)");
ConVar cv_vs_browser_animspeed("vs_browser_animspeed", 0.15f, FCVAR_DEFAULT);
ConVar cv_vs_percent("vs_percent", 0.0f, FCVAR_DEFAULT);
ConVar cv_vs_repeat("vs_repeat", false, FCVAR_DEFAULT);
ConVar cv_vs_shuffle("vs_shuffle", false, FCVAR_DEFAULT);
ConVar cv_vs_volume("vs_volume", 1.0f, FCVAR_DEFAULT);
ConVar cv_vsync("vsync", false, FCVAR_DEFAULT, _vsync);
ConVar cv_win_disable_windows_key(
    "win_disable_windows_key", false, FCVAR_DEFAULT,
    "if compiled on Windows, set to 0/1 to disable/enable all windows keys via low level keyboard hook");
ConVar cv_win_disable_windows_key_while_playing("osu_win_disable_windows_key_while_playing", true, FCVAR_DEFAULT);
ConVar cv_win_ink_workaround("win_ink_workaround", false, FCVAR_DEFAULT);
ConVar cv_win_mouse_raw_input_buffer("win_mouse_raw_input_buffer", false, FCVAR_DEFAULT,
                                     "use GetRawInputBuffer() to reduce wndproc event queue overflow stalls on insane "
                                     "mouse usb polling rates above 1000 Hz");
ConVar cv_win_processpriority("win_processpriority", 1, FCVAR_DEFAULT,
                              "if compiled on Windows, sets the main process priority (0 = normal, 1 = high)");
ConVar cv_win_realtimestylus("win_realtimestylus", false, FCVAR_DEFAULT,
                             "if compiled on Windows, enables native RealTimeStylus support for tablet clicks");
ConVar cv_win_snd_wasapi_buffer_size(
    "win_snd_wasapi_buffer_size", 0.011f, FCVAR_DEFAULT | FCVAR_PRIVATE,
    "buffer size/length in seconds (e.g. 0.011 = 11 ms), directly responsible for audio delay and crackling");
ConVar cv_win_snd_wasapi_exclusive("win_snd_wasapi_exclusive", true, FCVAR_DEFAULT | FCVAR_PRIVATE);
ConVar cv_win_snd_wasapi_period_size(
    "win_snd_wasapi_period_size", 0.0f, FCVAR_DEFAULT | FCVAR_PRIVATE,
    "interval between OutputWasapiProc calls in seconds (e.g. 0.016 = 16 ms) (0 = use default)");

//================ Copyright (c) 2011, PG, All rights reserved. =================//
//
// Purpose:		console variables
//
// $NoKeywords: $convar
//===============================================================================//

#include "ConVar.h"

#include "Bancho.h"
#include "Beatmap.h"
#include "CBaseUILabel.h"
#include "Engine.h"
#include "ModSelector.h"
#include "Osu.h"

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

    if(bancho.osu != nullptr) {
        auto is_vanilla = convar->isVanilla();

        auto beatmap = bancho.osu->getSelectedBeatmap();
        if(beatmap != nullptr) {
            beatmap->vanilla &= is_vanilla;
        }

        auto mod_selector = bancho.osu->m_modSelector;
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
    if(bancho.osu != nullptr) {
        if(bancho.osu->getModTarget()) return false;
        if(bancho.osu->getModNightmare()) return false;
        if(bancho.osu->getModEZ() && bancho.osu->getModHR()) return false;
        if((bancho.osu->getModDT() || bancho.osu->getModNC()) && (bancho.osu->getModHT() || bancho.osu->getModDC()))
            return false;
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

ConVar _find_("find", FCVAR_DEFAULT, _find);
ConVar _help_("help", FCVAR_DEFAULT, _help);
ConVar _listcommands_("listcommands", FCVAR_DEFAULT, _listcommands);
ConVar _dumpcommands_("dumpcommands", FCVAR_HIDDEN, _dumpcommands);

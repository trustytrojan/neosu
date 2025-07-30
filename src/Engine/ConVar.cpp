#include "ConVar.h"

#include "Bancho.h"
#include "BanchoUsers.h"
#include "Beatmap.h"
#include "CBaseUILabel.h"
#include "Console.h"
#include "Database.h"
#include "Engine.h"
#include "ModSelector.h"
#include "Osu.h"
#include "Profiler.h"
#include "RichPresence.h"
#include "SongBrowser/LoudnessCalcThread.h"
#include "SoundEngine.h"
#include "SpectatorScreen.h"

#include <algorithm>
#include <unordered_set>

static std::vector<ConVar *> &_getGlobalConVarArray() {
    static std::vector<ConVar *> g_vConVars;  // (singleton)
    return g_vConVars;
}

static std::unordered_map<std::string, ConVar *> &_getGlobalConVarMap() {
    static std::unordered_map<std::string, ConVar *> g_vConVarMap;  // (singleton)
    return g_vConVarMap;
}

void ConVar::addConVar(ConVar *c) {
    if(_getGlobalConVarArray().size() < 1) _getGlobalConVarArray().reserve(1024);

    const std::string &cname_str = c->getName();

    if(_getGlobalConVarMap().find(cname_str) == _getGlobalConVarMap().end()) {
        _getGlobalConVarArray().push_back(c);
        _getGlobalConVarMap()[cname_str] = c;
    } else {
        printf("FATAL: Duplicate ConVar name (\"%s\")\n", cname_str.c_str());
        std::exit(100);
    }
}

static ConVar *_getConVar(const ConVarString &name) {
    const auto result = _getGlobalConVarMap().find(name);
    if(result != _getGlobalConVarMap().end())
        return result->second;
    else
        return NULL;
}

ConVarString ConVar::getFancyDefaultValue() {
    switch(this->getType()) {
        case ConVar::CONVAR_TYPE::CONVAR_TYPE_BOOL:
            return this->fDefaultDefaultValue == 0 ? "false" : "true";
        case ConVar::CONVAR_TYPE::CONVAR_TYPE_INT:
            return std::to_string((int)this->fDefaultDefaultValue);
        case ConVar::CONVAR_TYPE::CONVAR_TYPE_FLOAT:
            return std::to_string(this->fDefaultDefaultValue);
        case ConVar::CONVAR_TYPE::CONVAR_TYPE_STRING: {
            ConVarString out = "\"";
            out.append(this->sDefaultDefaultValue);
            out.append("\"");
            return out;
        }
    }

    return "unreachable";
}

ConVarString ConVar::typeToString(CONVAR_TYPE type) {
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

void ConVar::exec() {
    if(!this->isUnlocked()) return;

    if(osu != nullptr) {
        auto is_vanilla = convar->isVanilla();

        auto beatmap = osu->getSelectedBeatmap();
        if(beatmap != NULL) {
            beatmap->vanilla &= is_vanilla;
        }

        auto mod_selector = osu->modSelector;
        if(mod_selector && mod_selector->nonVanillaWarning) {
            mod_selector->nonVanillaWarning->setVisible(!is_vanilla && bancho->can_submit_scores());
        }
    }

    if(auto *cb = std::get_if<NativeConVarCallback>(&this->callback)) (*cb)();
}

void ConVar::execArgs(const UString &args) {
    if(!this->isUnlocked()) return;

    if(auto *cb = std::get_if<NativeConVarCallbackArgs>(&this->callback)) (*cb)(args);
}

void ConVar::execFloat(float args) {
    if(!this->isUnlocked()) return;

    if(auto *cb = std::get_if<NativeConVarCallbackFloat>(&this->callback)) (*cb)(args);
}

// Reset default values to the actual defaults (before neosu.json overrides)
void ConVar::resetDefaults() {
    std::atomic_thread_fence(std::memory_order_seq_cst);
    if(this->fDefaultValue != this->fDefaultDefaultValue) this->fDefaultValue = this->fDefaultDefaultValue;
    if(this->sDefaultValue != this->sDefaultDefaultValue) this->sDefaultValue = this->sDefaultDefaultValue;
    if(this->iFlags != this->iDefaultFlags) this->iFlags = this->iDefaultFlags;
}

void ConVar::setFlags(uint8_t new_flags) {
    std::atomic_thread_fence(std::memory_order_seq_cst);
    this->iFlags = new_flags;
}

void ConVar::setDefaultString(const UString &defaultValue) {
    if(this->isFlagSet(FCVAR_PRIVATE)) return;

    this->setDefaultStringInt(defaultValue.utf8View());
}

void ConVar::setDefaultFloat(float defaultValue) {
    if(this->isFlagSet(FCVAR_PRIVATE)) return;

    this->setDefaultFloatInt(defaultValue);
}

void ConVar::setDefaultFloatInt(float defaultValue) {
    this->fDefaultValue = defaultValue;
    this->sDefaultValue = fmt::format("{:g}", defaultValue);
}

void ConVar::setDefaultStringInt(const std::string_view &defaultValue) {
    this->sDefaultValue = defaultValue;
    // also try to parse default float from the default string
    const float f = std::strtof(this->sDefaultValue.c_str(), nullptr);
    if(f != 0.0f) {
        this->fDefaultValue = f;
    }
}

bool ConVar::isUnlocked() const {
    if(this->isFlagSet(FCVAR_PRIVATE)) return true;
    if(bancho == nullptr || !bancho->is_online()) return true;

    if(bancho->is_in_a_multi_room()) {
        return this->isFlagSet(FCVAR_BANCHO_COMPATIBLE);
    } else {
        return true;
    }
}

bool ConVar::gameplayCompatCheck() const {
    if(osu && this->isFlagSet(FCVAR_GAMEPLAY)) {
        if(bancho->is_playing_a_multi_map()) {
            debugLog("Can't edit %s while in a multiplayer match.\n", this->sName);
            return false;
        } else {
            if(osu->isInPlayMode()) {
                debugLog("%s affects gameplay: won't submit score.\n", this->sName);
            }
            osu->getScore()->setCheated();
        }
        osu->getScore()->mods = Replay::Mods::from_cvars();
    }

    return true;
}

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

ConVar *ConVarHandler::getConVarByName(const ConVarString &name, bool warnIfNotFound) const {
    ConVar *found = _getConVar(name);
    if(found != NULL && !found->isFlagSet(FCVAR_INTERNAL)) return found;

    if(warnIfNotFound) {
        ConVarString errormsg = ConVarString("ENGINE: ConVar \"");
        errormsg.append(name);
        errormsg.append("\" does not exist...\n");
        Engine::logRaw("{:s}", errormsg.c_str());
        engine->showMessageWarning("Engine Error", errormsg.c_str());
    }

    if(!warnIfNotFound)
        return NULL;
    else
        return &_emptyDummyConVar;
}

std::vector<ConVar *> ConVarHandler::getConVarByLetter(const ConVarString &letters) const {
    std::unordered_set<std::string> matchingConVarNames;
    std::vector<ConVar *> matchingConVars;
    {
        if(letters.length() < 1) return matchingConVars;

        const std::vector<ConVar *> &convars = this->getConVarArray();

        // first try matching exactly
        for(auto convar : convars) {
            if(convar->isFlagSet(FCVAR_HIDDEN)) continue;

            if(convar->getName().find(letters) != std::string::npos) {
                if(letters.length() > 1) matchingConVarNames.insert(convar->getName());

                matchingConVars.push_back(convar);
            }
        }

        // then try matching substrings
        if(letters.length() > 1) {
            for(auto convar : convars) {
                if(convar->isFlagSet(FCVAR_HIDDEN)) continue;

                if(convar->getName().find(letters) != std::string::npos) {
                    const ConVarString &stdName = convar->getName();
                    if(matchingConVarNames.find(stdName) == matchingConVarNames.end()) {
                        matchingConVarNames.insert(stdName);
                        matchingConVars.push_back(convar);
                    }
                }
            }
        }

        // (results should be displayed in vector order)
    }
    return matchingConVars;
}

ConVarString ConVarHandler::flagsToString(uint8_t flags) {
    ConVarString string;
    {
        if(flags == 0) {
            string.append("no flags");
        } else {
            if(flags & FCVAR_HIDDEN) string.append(string.length() > 0 ? " hidden" : "hidden");
            if(flags & FCVAR_BANCHO_SUBMITTABLE)
                string.append(string.length() > 0 ? " bancho_submittable" : "bancho_submittable");
            if(flags & FCVAR_BANCHO_COMPATIBLE)
                string.append(string.length() > 0 ? " bancho_compatible" : "bancho_compatible");
#ifdef _DEBUG
            bool internal = flags & FCVAR_INTERNAL;
            if(internal) {
                string.append(string.length() > 0 ? " internal" : "internal");
            } else {
                if(flags & FCVAR_NOEXEC) string.append(string.length() > 0 ? " noexec" : "noexec");
                if(flags & FCVAR_NOSAVE) string.append(string.length() > 0 ? " nosave" : "nosave");
                if(flags & FCVAR_NOLOAD) string.append(string.length() > 0 ? " noload" : "noload");
            }
#endif
        }
    }
    return string;
}

bool ConVarHandler::isVanilla() {
    for(const auto &cv : _getGlobalConVarArray()) {
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
        f32 speed = cv::speed_override.getFloat();
        if(speed != -1.f && speed != 0.75 && speed != 1.0 && speed != 1.5) return false;
    }

    return true;
}

//*****************************//
//	ConVarHandler ConCommands  //
//*****************************//

static void _find(const UString &args) {
    if(args.length() < 1) {
        Engine::logRaw("Usage:  find <string>\n");
        return;
    }

    const std::vector<ConVar *> &convars = convar->getConVarArray();

    std::vector<ConVar *> matchingConVars;
    for(auto convar : convars) {
        if(convar->isFlagSet(FCVAR_HIDDEN)) continue;

        const ConVarString &name = convar->getName();
        if(name.find(args.toUtf8()) != std::string::npos) matchingConVars.push_back(convar);
    }

    if(matchingConVars.size() > 0) {
        std::ranges::sort(matchingConVars, {}, [](const ConVar *v) { return v->getName(); });
    }

    if(matchingConVars.size() < 1) {
        Engine::logRaw("No commands found containing {:s}.\n", args);
        return;
    }

    Engine::logRaw("----------------------------------------------\n");
    {
        UString thelog = "[ find : ";
        thelog.append(args);
        thelog.append(" ]\n");
        Engine::logRaw("{:s}", thelog.toUtf8());

        for(auto &matchingConVar : matchingConVars) {
            Engine::logRaw("{:s}\n", matchingConVar->getName());
        }
    }
    Engine::logRaw("----------------------------------------------\n");
}

static void _help(const UString &args) {
    std::string trimmedArgs{args.trim().utf8View()};

    if(trimmedArgs.length() < 1) {
        Engine::logRaw("Usage:  help <cvarname>\nTo get a list of all available commands, type \"listcommands\".\n");
        return;
    }

    const std::vector<ConVar *> matches = convar->getConVarByLetter(trimmedArgs);

    if(matches.size() < 1) {
        Engine::logRaw("ConVar {:s} does not exist.\n", trimmedArgs);
        return;
    }

    // use closest match
    size_t index = 0;
    for(size_t i = 0; i < matches.size(); i++) {
        if(matches[i]->getName() == trimmedArgs) {
            index = i;
            break;
        }
    }
    ConVar *match = matches[index];

    if(match->getHelpstring().length() < 1) {
        Engine::logRaw("ConVar {:s} does not have a helpstring.\n", match->getName());
        return;
    }

    ConVarString thelog{match->getName()};
    {
        if(match->hasValue()) {
            const auto &cv_str = match->getString();
            const auto &default_str = match->getDefaultString();
            thelog.append(fmt::format(" = {:s} ( def. \"{:s}\" , ", cv_str.c_str(), default_str.c_str()));
            thelog.append(ConVar::typeToString(match->getType()));
            thelog.append(", ");
            thelog.append(ConVarHandler::flagsToString(match->getFlags()).c_str());
            thelog.append(" )");
        }

        thelog.append(" - ");
        thelog.append(match->getHelpstring().c_str());
    }
    Engine::logRaw("{:s}\n", thelog);
}

static void _listcommands(void) {
    Engine::logRaw("----------------------------------------------\n");
    {
        std::vector<ConVar *> convars = convar->getConVarArray();
        std::ranges::sort(convars, {}, [](const ConVar *v) { return v->getName(); });

        for(auto &convar : convars) {
            if(convar->isFlagSet(FCVAR_HIDDEN)) continue;

            ConVar *var = convar;

            ConVarString tstring{var->getName()};
            {
                if(var->hasValue()) {
                    const auto &var_str = var->getString();
                    const auto &default_str = var->getDefaultString();
                    tstring.append(fmt::format(" = {:s} ( def. \"{:s}\" , ", var_str.c_str(), default_str.c_str()));
                    tstring.append(ConVar::typeToString(var->getType()));
                    tstring.append(", ");
                    tstring.append(ConVarHandler::flagsToString(var->getFlags()).c_str());
                    tstring.append(" )");
                }

                if(var->getHelpstring().length() > 0) {
                    tstring.append(" - ");
                    tstring.append(var->getHelpstring().c_str());
                }

                tstring.append("\n");
            }
            Engine::logRaw("{:s}", tstring);
        }
    }
    Engine::logRaw("----------------------------------------------\n");
}

static void _dumpcommands(void) {
    std::vector<ConVar *> convars = convar->getConVarArray();
    std::ranges::sort(convars, {}, [](const ConVar *v) { return v->getName(); });

    FILE *file = fopen("commands.htm", "w");
    if(file == NULL) {
        Engine::logRaw("Failed to open commands.htm for writing\n");
        return;
    }

    for(auto var : convars) {
        if(!var->hasValue()) continue;
        if(var->isFlagSet(FCVAR_HIDDEN)) continue;
        if(var->isFlagSet(FCVAR_PRIVATE)) continue;

        std::string cmd = "<h4>";
        cmd.append(var->getName());
        cmd.append("</h4>\n");
        cmd.append(var->getHelpstring());
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

    Engine::logRaw("Commands dumped to commands.htm\n");
}

void _exec(const UString &args) { Console::execConfigFile(args.toUtf8()); }

void _echo(const UString &args) {
    if(args.length() > 0) {
        Engine::logRaw("{:s}\n", args.toUtf8());
    }
}

void _volume(const UString & /*oldValue*/, const UString &newValue) {
    if(!soundEngine) return;
    soundEngine->setVolume(newValue.toFloat());
}

void _vprof(float newValue) {
    const bool enable = !!static_cast<int>(newValue);

    if(enable != g_profCurrentProfile.isEnabled()) {
        if(enable)
            g_profCurrentProfile.start();
        else
            g_profCurrentProfile.stop();
    }
}

void _osuOptionsSliderQualityWrapper(float newValue) {
    float value = std::lerp(1.0f, 2.5f, 1.0f - newValue);
    cv::slider_curve_points_separation.setValue(value);
};

void spectate_by_username(const UString &username) {
    auto user = BANCHO::User::find_user(username);
    if(user == NULL) {
        debugLog("Couldn't find user \"%s\"!", username.toUtf8());
        return;
    }

    debugLog("Spectating %s (user %d)...\n", username.toUtf8(), user->user_id);
    start_spectating(user->user_id);
}

void _monitor(const UString & /*oldValue*/, const UString &newValue) { env->setMonitor(newValue.toInt()); }

void _osu_songbrowser_search_hardcoded_filter(const UString & /*oldValue*/, const UString &newValue) {
    if(newValue.length() == 1 && newValue.isWhitespaceOnly()) cv::songbrowser_search_hardcoded_filter.setValue("");
}

void loudness_cb(const UString & /*oldValue*/, const UString & /*newValue*/) {
    // Restart loudness calc.
    VolNormalization::abort();
    if(db && cv::normalize_loudness.getBool()) {
        VolNormalization::start_calc(db->loudness_to_calc);
    }
}

void _save(void) { db->save(); }

#undef CONVARDEFS_H
#define DEFINE_CONVARS

#include "ConVarDefs.h"

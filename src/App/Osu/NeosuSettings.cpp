#include "NeosuSettings.h"

#include "ConVar.h"
#include "Engine.h"
#include "cJSON.h"

void process_neosu_settings(Packet packet) {
    if(packet.size == 0) {
        debugLog("Failed to get neosu.json! Server probably doesn't support it.\n");
        return;
    }

    cJSON *json = cJSON_ParseWithLength((char *)packet.memory, packet.size);
    if(json == NULL) {
        debugLog("Failed to parse neosu.json! (received %d bytes)\n", packet.size);
        return;
    }

    int nb_overrides = 0;
    int nb_errors = 0;

    cJSON *object = NULL;
    cJSON_ArrayForEach(object, json) {
        cJSON *default_value = NULL;
        cJSON *unlock_singleplayer = NULL;
        cJSON *unlock_multiplayer = NULL;
        cJSON *always_submit = NULL;

        if(object->string == NULL) continue;

        auto cvar = convar->getConVarByName(object->string);
        if(cvar->getName() == UString("emptyDummyConVar")) {
            debugLog("Unknown convar \"%s\", cannot override.\n", object->string);
            continue;
        }

        cvar->resetDefaults();
        int flags = cvar->getFlags();
        if(flags & (FCVAR_HIDDEN | FCVAR_PRIVATE)) {
            debugLog("Server tried to override private convar \"%s\"!\n", object->string);
            goto cvar_err;
        }

        default_value = cJSON_GetObjectItemCaseSensitive(object, "default");
        if(default_value != NULL) {
            if(cvar->getType() == ConVar::CONVAR_TYPE::CONVAR_TYPE_STRING) {
                char *new_default = cJSON_GetStringValue(default_value);
                if(new_default) {
                    cvar->setDefaultString(new_default);
                } else {
                    debugLog("Invalid type for \"%s\" in neosu.json! \"default\" should be a string.\n",
                             object->string);
                    goto cvar_err;
                }
            } else if(cvar->getType() == ConVar::CONVAR_TYPE::CONVAR_TYPE_BOOL) {
                if(!cJSON_IsBool(default_value)) {
                    debugLog("Invalid type for \"%s\" in neosu.json! \"default\" should be a boolean.\n",
                             object->string);
                    goto cvar_err;
                } else {
                    cvar->setDefaultFloat(cJSON_IsTrue(default_value) ? 1.f : 0.f);
                }
            } else {
                if(!cJSON_IsNumber(default_value)) {
                    debugLog("Invalid type for \"%s\" in neosu.json! \"default\" should be a number.\n",
                             object->string);
                    goto cvar_err;
                } else {
                    cvar->setDefaultFloat(cJSON_GetNumberValue(default_value));
                }
            }
        }

        unlock_singleplayer = cJSON_GetObjectItemCaseSensitive(object, "unlock_singleplayer");
        if(unlock_singleplayer) {
            if(!cJSON_IsBool(unlock_singleplayer)) {
                debugLog("Invalid type for \"%s\" in neosu.json! \"unlock_singleplayer\" should be a boolean.\n",
                         object->string);
                goto cvar_err;
            }

            flags &= ~(FCVAR_UNLOCK_SINGLEPLAYER);
            if(cJSON_IsTrue(unlock_singleplayer)) {
                flags |= FCVAR_UNLOCK_SINGLEPLAYER;
            }
        }

        unlock_multiplayer = cJSON_GetObjectItemCaseSensitive(object, "unlock_multiplayer");
        if(unlock_multiplayer) {
            if(!cJSON_IsBool(unlock_multiplayer)) {
                debugLog("Invalid type for \"%s\" in neosu.json! \"unlock_multiplayer\" should be a boolean.\n",
                         object->string);
                goto cvar_err;
            }

            flags &= ~(FCVAR_UNLOCK_MULTIPLAYER);
            if(cJSON_IsTrue(unlock_multiplayer)) {
                flags |= FCVAR_UNLOCK_MULTIPLAYER;
            }
        }

        always_submit = cJSON_GetObjectItemCaseSensitive(object, "always_submit");
        if(always_submit) {
            if(!cJSON_IsBool(always_submit)) {
                debugLog("Invalid type for \"%s\" in neosu.json! \"always_submit\" should be a boolean.\n",
                         object->string);
                goto cvar_err;
            }

            flags &= ~(FCVAR_ALWAYS_SUBMIT);
            if(cJSON_IsTrue(always_submit)) {
                flags |= FCVAR_ALWAYS_SUBMIT;
            }
        }

        cvar->setFlags(flags);
        nb_overrides++;
        continue;

    cvar_err:
        cvar->resetDefaults();
        nb_errors++;
    }

    debugLog("Finished applying neosu.json configuration (%d overrides, %d errors)\n", nb_overrides, nb_errors);
    cJSON_Delete(json);
}

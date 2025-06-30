#ifndef CONVAR_H
#define CONVAR_H

#include <atomic>

#include "FastDelegate.h"
#include "UString.h"

#ifndef DEFINE_CONVARS
#include "ConVarDefs.h"
#endif

enum FCVAR_FLAGS {
    // No flags: cvar is only allowed offline.
    FCVAR_LOCKED = 0,

    // If the cvar is modified, scores will not submit unless this flag is set.
    FCVAR_BANCHO_SUBMITTABLE = (1 << 0),

    // Legacy servers only support a limited number of cvars.
    // Unless this flag is set, the cvar will be forced to its default value
    // when playing in a multiplayer lobby on a legacy server.
    FCVAR_BANCHO_MULTIPLAYABLE = (1 << 1),  // NOTE: flag shouldn't be set without BANCHO_SUBMITTABLE
    FCVAR_BANCHO_COMPATIBLE = FCVAR_BANCHO_SUBMITTABLE | FCVAR_BANCHO_MULTIPLAYABLE,

    // hide cvar from console suggestions
    FCVAR_HIDDEN = (1 << 2),

    // prevent servers from touching this cvar
    FCVAR_PRIVATE = (1 << 3),

    // this cvar affects gameplay
    FCVAR_GAMEPLAY = (1 << 4),
};

class ConVar {
   public:
    enum class CONVAR_TYPE { CONVAR_TYPE_BOOL, CONVAR_TYPE_INT, CONVAR_TYPE_FLOAT, CONVAR_TYPE_STRING };

    // raw callbacks
    typedef void (*ConVarCallback)(void);
    typedef void (*ConVarChangeCallback)(UString oldValue, UString newValue);
    typedef void (*ConVarCallbackArgs)(UString args);

    // delegate callbacks
    typedef fastdelegate::FastDelegate0<> NativeConVarCallback;
    typedef fastdelegate::FastDelegate1<UString> NativeConVarCallbackArgs;
    typedef fastdelegate::FastDelegate2<UString, UString> NativeConVarChangeCallback;

   public:
    static UString typeToString(CONVAR_TYPE type);

   public:
    explicit ConVar(UString name);

    explicit ConVar(UString name, int flags, ConVarCallback callback);
    explicit ConVar(UString name, int flags, const char *helpString, ConVarCallback callback);

    explicit ConVar(UString name, int flags, ConVarCallbackArgs callbackARGS);
    explicit ConVar(UString name, int flags, const char *helpString, ConVarCallbackArgs callbackARGS);

    explicit ConVar(UString name, float defaultValue, int flags);
    explicit ConVar(UString name, float defaultValue, int flags, ConVarChangeCallback callback);
    explicit ConVar(UString name, float defaultValue, int flags, const char *helpString);
    explicit ConVar(UString name, float defaultValue, int flags, const char *helpString, ConVarChangeCallback callback);

    explicit ConVar(UString name, int defaultValue, int flags);
    explicit ConVar(UString name, int defaultValue, int flags, ConVarChangeCallback callback);
    explicit ConVar(UString name, int defaultValue, int flags, const char *helpString);
    explicit ConVar(UString name, int defaultValue, int flags, const char *helpString, ConVarChangeCallback callback);

    explicit ConVar(UString name, bool defaultValue, int flags);
    explicit ConVar(UString name, bool defaultValue, int flags, ConVarChangeCallback callback);
    explicit ConVar(UString name, bool defaultValue, int flags, const char *helpString);
    explicit ConVar(UString name, bool defaultValue, int flags, const char *helpString, ConVarChangeCallback callback);

    explicit ConVar(UString name, const char *defaultValue, int flags);
    explicit ConVar(UString name, const char *defaultValue, int flags, const char *helpString);
    explicit ConVar(UString name, const char *defaultValue, int flags, ConVarChangeCallback callback);
    explicit ConVar(UString name, const char *defaultValue, int flags, const char *helpString,
                    ConVarChangeCallback callback);

    // callbacks
    void exec();
    void execArgs(UString args);

    // set
    void resetDefaults();
    void setDefaultBool(bool defaultValue) { this->setDefaultFloat(defaultValue ? 1.f : 0.f); }
    void setDefaultInt(int defaultValue) { this->setDefaultFloat((float)defaultValue); }
    void setDefaultFloat(float defaultValue);
    void setDefaultString(UString defaultValue);

    void setValue(float value, bool fire_callbacks = true);
    void setValue(UString sValue, bool fire_callbacks = true);

    void setCallback(NativeConVarCallback callback);
    void setCallback(NativeConVarCallbackArgs callback);
    void setCallback(NativeConVarChangeCallback callback);

    void setHelpString(UString helpString);

    // get
    [[nodiscard]] inline float getDefaultFloat() const { return this->fDefaultValue.load(); }
    [[nodiscard]] inline const UString &getDefaultString() const { return this->sDefaultValue; }

    [[nodiscard]] bool isUnlocked() const;
    std::string getFancyDefaultValue();

    [[nodiscard]] bool getBool() const;
    [[nodiscard]] float getFloat() const;
    int getInt();
    const UString &getString();

    [[nodiscard]] inline const UString &getHelpstring() const { return this->sHelpString; }
    [[nodiscard]] inline const UString &getName() const { return this->sName; }
    [[nodiscard]] inline CONVAR_TYPE getType() const { return this->type; }
    [[nodiscard]] inline int getFlags() const { return this->iFlags; }
    inline void setFlags(int new_flags) { this->iFlags = new_flags; }

    [[nodiscard]] inline bool hasValue() const { return this->bHasValue; }
    [[nodiscard]] inline bool hasCallbackArgs() const { return (this->callbackfuncargs || this->changecallback); }
    [[nodiscard]] inline bool isFlagSet(int flag) const { return (bool)(this->iFlags & flag); }

   private:
    void init(int flags);
    void init(UString &name, int flags);
    void init(UString &name, int flags, ConVarCallback callback);
    void init(UString &name, int flags, UString helpString, ConVarCallback callback);
    void init(UString &name, int flags, ConVarCallbackArgs callbackARGS);
    void init(UString &name, int flags, UString helpString, ConVarCallbackArgs callbackARGS);
    void init(UString &name, float defaultValue, int flags, UString helpString, ConVarChangeCallback callback);
    void init(UString &name, UString defaultValue, int flags, UString helpString, ConVarChangeCallback callback);

   private:
    bool bHasValue;
    CONVAR_TYPE type;
    int iDefaultFlags;
    int iFlags;

    UString sName;
    UString sHelpString;

    std::atomic<float> fValue;
    std::atomic<float> fDefaultValue;
    float fDefaultDefaultValue;

    UString sValue;
    UString sDefaultValue;
    UString sDefaultDefaultValue;

    NativeConVarCallback callbackfunc;
    NativeConVarCallbackArgs callbackfuncargs;
    NativeConVarChangeCallback changecallback;
};

//*******************//
//  Searching/Lists  //
//*******************//

class ConVarHandler {
   public:
    static UString flagsToString(int flags);

   public:
    ConVarHandler();
    ~ConVarHandler();

    [[nodiscard]] const std::vector<ConVar *> &getConVarArray() const;
    [[nodiscard]] int getNumConVars() const;

    [[nodiscard]] ConVar *getConVarByName(UString name, bool warnIfNotFound = true) const;
    [[nodiscard]] std::vector<ConVar *> getConVarByLetter(UString letters) const;

    bool isVanilla();
};

extern ConVarHandler *convar;

#endif

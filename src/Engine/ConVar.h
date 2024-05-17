#pragma once
#include <atomic>

#include "FastDelegate.h"
#include "UString.h"

enum FCVAR_FLAGS {
    // not visible in find/listcommands results etc., but is settable/gettable via console/help
    FCVAR_HIDDEN = (1 << 0),

    // if flag is not set, value will be forced to default when online
    FCVAR_UNLOCK_SINGLEPLAYER = (1 << 1),
    FCVAR_UNLOCK_MULTIPLAYER = (1 << 2),
    FCVAR_UNLOCKED = (FCVAR_UNLOCK_SINGLEPLAYER | FCVAR_UNLOCK_MULTIPLAYER),

    // if flag is not set, score will not submit when value is not default
    FCVAR_ALWAYS_SUBMIT = (1 << 3),

    // don't allow server to modify this cvar
    FCVAR_PRIVATE = (1 << 4),

    // legacy definitions
    FCVAR_LOCKED = 0,
    FCVAR_DEFAULT = FCVAR_UNLOCKED | FCVAR_ALWAYS_SUBMIT,
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
    void setDefaultBool(bool defaultValue) { setDefaultFloat(defaultValue ? 1.f : 0.f); }
    void setDefaultInt(int defaultValue) { setDefaultFloat((float)defaultValue); }
    void setDefaultFloat(float defaultValue);
    void setDefaultString(UString defaultValue);

    void setValue(float value);
    void setValue(UString sValue);

    void setCallback(NativeConVarCallback callback);
    void setCallback(NativeConVarCallbackArgs callback);
    void setCallback(NativeConVarChangeCallback callback);

    void setHelpString(UString helpString);

    // get
    inline float getDefaultFloat() const { return m_fDefaultValue.load(); }
    inline const UString &getDefaultString() const { return m_sDefaultValue; }

    bool isUnlocked() const;
    std::string getFancyDefaultValue();

    bool getBool() const;
    float getFloat() const;
    int getInt();
    const UString &getString();

    inline const UString &getHelpstring() const { return m_sHelpString; }
    inline const UString &getName() const { return m_sName; }
    inline CONVAR_TYPE getType() const { return m_type; }
    inline int getFlags() const { return m_iFlags; }
    inline void setFlags(int new_flags) { m_iFlags = new_flags; }

    inline bool hasValue() const { return m_bHasValue; }
    inline bool hasCallbackArgs() const { return (m_callbackfuncargs || m_changecallback); }
    inline bool isFlagSet(int flag) const { return (bool)(m_iFlags & flag); }

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
    bool m_bHasValue;
    CONVAR_TYPE m_type;
    int m_iDefaultFlags;
    int m_iFlags;

    UString m_sName;
    UString m_sHelpString;

    std::atomic<float> m_fValue;
    std::atomic<float> m_fDefaultValue;
    float m_fDefaultDefaultValue;

    UString m_sValue;
    UString m_sDefaultValue;
    UString m_sDefaultDefaultValue;

    NativeConVarCallback m_callbackfunc;
    NativeConVarCallbackArgs m_callbackfuncargs;
    NativeConVarChangeCallback m_changecallback;
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

    const std::vector<ConVar *> &getConVarArray() const;
    int getNumConVars() const;

    ConVar *getConVarByName(UString name, bool warnIfNotFound = true) const;
    std::vector<ConVar *> getConVarByLetter(UString letters) const;

    bool isVanilla();
};

extern ConVarHandler *convar;

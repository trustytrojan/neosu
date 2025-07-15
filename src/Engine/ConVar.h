#ifndef CONVAR_H
#define CONVAR_H

#include <atomic>
#include <mutex>
#include <string>
#include <variant>
#include <type_traits>

#include "Delegate.h"
#include "UString.h"

#ifndef DEFINE_CONVARS
#include "ConVarDefs.h"
#endif

// use a more compact string representation for each ConVar object, instead of UString
using ConVarString = std::string;
using std::string_view_literals::operator""sv;

enum FCVAR_FLAGS : uint8_t {
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
    enum class CONVAR_TYPE : uint8_t { CONVAR_TYPE_BOOL, CONVAR_TYPE_INT, CONVAR_TYPE_FLOAT, CONVAR_TYPE_STRING };

    // callback typedefs using Kryukov delegates
    using NativeConVarCallback = SA::delegate<void()>;
    using NativeConVarCallbackArgs = SA::delegate<void(const UString &)>;
    using NativeConVarChangeCallback = SA::delegate<void(const UString &, const UString &)>;
    using NativeConVarCallbackFloat = SA::delegate<void(float)>;
    using NativeConVarChangeCallbackFloat = SA::delegate<void(float, float)>;

    // polymorphic callback storage
    using ExecutionCallback = std::variant<std::monostate,            // empty
                                           NativeConVarCallback,      // void()
                                           NativeConVarCallbackArgs,  // void(const UString&)
                                           NativeConVarCallbackFloat  // void(float)
                                           >;

    using ChangeCallback = std::variant<std::monostate,                  // empty
                                        NativeConVarChangeCallback,      // void(const UString&, const UString&)
                                        NativeConVarChangeCallbackFloat  // void(float, float)
                                        >;

   private:
    // type detection helper
    template <typename T>
    static constexpr CONVAR_TYPE getTypeFor() {
        if constexpr(std::is_same_v<std::decay_t<T>, bool>)
            return CONVAR_TYPE::CONVAR_TYPE_BOOL;
        else if constexpr(std::is_integral_v<std::decay_t<T>>)
            return CONVAR_TYPE::CONVAR_TYPE_INT;
        else if constexpr(std::is_floating_point_v<std::decay_t<T>>)
            return CONVAR_TYPE::CONVAR_TYPE_FLOAT;
        else
            return CONVAR_TYPE::CONVAR_TYPE_STRING;
    }

    static void addConVar(ConVar *);

   public:
    static ConVarString typeToString(CONVAR_TYPE type);

   public:
    // command-only constructor
    explicit ConVar(const std::string_view &name) {
        this->sName = this->sDefaultValue = this->sDefaultDefaultValue = name;
        this->type = CONVAR_TYPE::CONVAR_TYPE_STRING;
        this->addConVar(this);
    };

    // callback-only constructors (no value)
    template <typename Callback>
    explicit ConVar(const std::string_view &name, uint8_t flags, Callback callback)
        requires std::is_invocable_v<Callback> || std::is_invocable_v<Callback, const UString &> ||
                 std::is_invocable_v<Callback, float>
    {
        this->initCallback(name, flags, ""sv, callback);
        this->addConVar(this);
    }

    template <typename Callback>
    explicit ConVar(const std::string_view &name, uint8_t flags, const std::string_view &helpString, Callback callback)
        requires std::is_invocable_v<Callback> || std::is_invocable_v<Callback, const UString &> ||
                 std::is_invocable_v<Callback, float>
    {
        this->initCallback(name, flags, helpString, callback);
        this->addConVar(this);
    }

    // value constructors handle all types uniformly
    template <typename T>
    explicit ConVar(const std::string_view &name, T defaultValue, uint8_t flags,
                    const std::string_view &helpString = ""sv)
        requires(!std::is_same_v<std::decay_t<T>, const char *>)
    {
        this->initValue(name, defaultValue, flags, helpString, nullptr);
        this->addConVar(this);
    }

    template <typename T, typename Callback>
    explicit ConVar(const std::string_view &name, T defaultValue, uint8_t flags, const std::string_view &helpString,
                    Callback callback)
        requires(!std::is_same_v<std::decay_t<T>, const char *>) &&
                (std::is_invocable_v<Callback> || std::is_invocable_v<Callback, const UString &> ||
                 std::is_invocable_v<Callback, float> ||
                 std::is_invocable_v<Callback, const UString &, const UString &> ||
                 std::is_invocable_v<Callback, float, float>)
    {
        this->initValue(name, defaultValue, flags, helpString, callback);
        this->addConVar(this);
    }

    template <typename T, typename Callback>
    explicit ConVar(const std::string_view &name, T defaultValue, uint8_t flags, Callback callback)
        requires(!std::is_same_v<std::decay_t<T>, const char *>) &&
                (std::is_invocable_v<Callback> || std::is_invocable_v<Callback, const UString &> ||
                 std::is_invocable_v<Callback, float> ||
                 std::is_invocable_v<Callback, const UString &, const UString &> ||
                 std::is_invocable_v<Callback, float, float>)
    {
        this->initValue(name, defaultValue, flags, ""sv, callback);
        this->addConVar(this);
    }

    // const char* specializations for string convars
    explicit ConVar(const std::string_view &name, const std::string_view &defaultValue, uint8_t flags,
                    const std::string_view &helpString = ""sv) {
        this->initValue(name, defaultValue, flags, helpString, nullptr);
        this->addConVar(this);
    }

    template <typename Callback>
    explicit ConVar(const std::string_view &name, const std::string_view &defaultValue, uint8_t flags,
                    const std::string_view &helpString, Callback callback)
        requires(std::is_invocable_v<Callback> || std::is_invocable_v<Callback, const UString &> ||
                 std::is_invocable_v<Callback, float> ||
                 std::is_invocable_v<Callback, const UString &, const UString &> ||
                 std::is_invocable_v<Callback, float, float>)
    {
        this->initValue(name, defaultValue, flags, helpString, callback);
        this->addConVar(this);
    }

    template <typename Callback>
    explicit ConVar(const std::string_view &name, const std::string_view &defaultValue, uint8_t flags,
                    Callback callback)
        requires(std::is_invocable_v<Callback> || std::is_invocable_v<Callback, const UString &> ||
                 std::is_invocable_v<Callback, float> ||
                 std::is_invocable_v<Callback, const UString &, const UString &> ||
                 std::is_invocable_v<Callback, float, float>)
    {
        this->initValue(name, defaultValue, flags, ""sv, callback);
        this->addConVar(this);
    }

    // callbacks
    void exec();
    void execArgs(const UString &args);
    void execFloat(float args);

    // set
    void resetDefaults();
    void setDefaultBool(bool defaultValue) { this->setDefaultFloat(defaultValue ? 1.f : 0.f); }
    void setDefaultInt(int defaultValue) { this->setDefaultFloat((float)defaultValue); }
    void setDefaultFloat(float defaultValue);
    void setDefaultString(const UString &defaultValue);
    inline void setHelpString(const UString &helpString) { this->sHelpString = helpString.utf8View(); };

    template <typename T>
    void setValue(T &&value, const bool &doCallback = true) {
        if(!this->isUnlocked() || !this->gameplayCompatCheck()) return;

        this->setValueInt(std::forward<T>(value), doCallback);
    }

    // generic callback setter that auto-detects callback type
    template <typename Callback>
    void setCallback(Callback &&callback)
        requires(std::is_invocable_v<Callback> || std::is_invocable_v<Callback, const UString &> ||
                 std::is_invocable_v<Callback, float> ||
                 std::is_invocable_v<Callback, const UString &, const UString &> ||
                 std::is_invocable_v<Callback, float, float>)
    {
        if constexpr(std::is_invocable_v<Callback>)
            this->callback = NativeConVarCallback(std::forward<Callback>(callback));
        else if constexpr(std::is_invocable_v<Callback, const UString &>)
            this->callback = NativeConVarCallbackArgs(std::forward<Callback>(callback));
        else if constexpr(std::is_invocable_v<Callback, float>)
            this->callback = NativeConVarCallbackFloat(std::forward<Callback>(callback));
        else if constexpr(std::is_invocable_v<Callback, const UString &, const UString &>)
            this->changeCallback = NativeConVarChangeCallback(std::forward<Callback>(callback));
        else if constexpr(std::is_invocable_v<Callback, float, float>)
            this->changeCallback = NativeConVarChangeCallbackFloat(std::forward<Callback>(callback));
        else
            static_assert(Env::always_false_v<Callback>, "Unsupported callback signature");
    }

    // get
    [[nodiscard]] inline float getDefaultFloat() const { return this->fDefaultValue.load(); }
    [[nodiscard]] inline const ConVarString &getDefaultString() const { return this->sDefaultValue; }

    std::string getFancyDefaultValue();

    template <typename T = int>
    [[nodiscard]] constexpr auto getVal() const {
        return static_cast<T>(this->isUnlocked() ? this->fValue.load() : this->fDefaultValue.load());
    }

    [[nodiscard]] constexpr int getInt() const { return getVal<int>(); }
    [[nodiscard]] constexpr bool getBool() const { return getVal<bool>(); }
    [[nodiscard]] constexpr float getFloat() const { return getVal<float>(); }

    [[nodiscard]]
    constexpr const ConVarString &getString() const {
        return this->isUnlocked() ? this->sValue : this->sDefaultValue;
    }

    [[nodiscard]] inline const ConVarString &getHelpstring() const { return this->sHelpString; }
    [[nodiscard]] inline const ConVarString &getName() const { return this->sName; }
    [[nodiscard]] inline CONVAR_TYPE getType() const { return this->type; }
    [[nodiscard]] inline uint8_t getFlags() const { return this->iFlags; }
    void setFlags(uint8_t new_flags);

    [[nodiscard]] inline bool hasValue() const { return this->bHasValue; }

    [[nodiscard]] inline bool hasCallbackArgs() const {
        return std::holds_alternative<NativeConVarCallbackArgs>(this->callback) ||
               !std::holds_alternative<std::monostate>(this->changeCallback);
    }

    [[nodiscard]] inline bool isFlagSet(uint8_t flag) const { return (bool)(this->iFlags & flag); }

   private:
    [[nodiscard]] bool isUnlocked() const;
    [[nodiscard]] bool gameplayCompatCheck() const;

    // unified init for callback-only convars
    template <typename Callback>
    void initCallback(const std::string_view &name, uint8_t flags, const std::string_view &helpString,
                      Callback callback) {
        this->iFlags = this->iDefaultFlags = flags;
        this->sName = name;
        this->sHelpString = helpString;
        this->bHasValue = false;

        if constexpr(std::is_invocable_v<Callback>) {
            this->callback = NativeConVarCallback(callback);
            this->type = CONVAR_TYPE::CONVAR_TYPE_STRING;
        } else if constexpr(std::is_invocable_v<Callback, const UString &>) {
            this->callback = NativeConVarCallbackArgs(callback);
            this->type = CONVAR_TYPE::CONVAR_TYPE_STRING;
        } else if constexpr(std::is_invocable_v<Callback, float>) {
            this->callback = NativeConVarCallbackFloat(callback);
            this->type = CONVAR_TYPE::CONVAR_TYPE_INT;
        }
    }

    // unified init for value convars
    template <typename T, typename Callback>
    void initValue(const std::string_view &name, const T &defaultValue, uint8_t flags,
                   const std::string_view &helpString, Callback callback) {
        this->iFlags = this->iDefaultFlags = flags;
        this->sName = name;
        this->sHelpString = helpString;
        this->type = getTypeFor<T>();

        // set default value
        if constexpr(std::is_same_v<std::decay_t<T>, std::string_view> || std::is_same_v<std::decay_t<T>, const char *>)
            setDefaultStringInt(defaultValue);
        else
            setDefaultFloatInt(static_cast<float>(defaultValue));

        // set "default default" value
        this->fDefaultDefaultValue = this->fDefaultValue;
        this->sDefaultDefaultValue = this->sDefaultValue;

        // set initial value (without triggering callbacks)
        if constexpr(std::is_same_v<std::decay_t<T>, std::string_view> || std::is_same_v<std::decay_t<T>, const char *>)
            setValueInt(defaultValue);
        else
            setValueInt(static_cast<float>(defaultValue));

        // set callback if provided
        if constexpr(!std::is_same_v<Callback, std::nullptr_t>) {
            if constexpr(std::is_invocable_v<Callback>)
                this->callback = NativeConVarCallback(callback);
            else if constexpr(std::is_invocable_v<Callback, const UString &>)
                this->callback = NativeConVarCallbackArgs(callback);
            else if constexpr(std::is_invocable_v<Callback, float>)
                this->callback = NativeConVarCallbackFloat(callback);
            else if constexpr(std::is_invocable_v<Callback, const UString &, const UString &>)
                this->changeCallback = NativeConVarChangeCallback(callback);
            else if constexpr(std::is_invocable_v<Callback, float, float>)
                this->changeCallback = NativeConVarChangeCallbackFloat(callback);
        }
    }

    void setDefaultFloatInt(float defaultValue);
    void setDefaultStringInt(const std::string_view &defaultValue);

    template <typename T>
    void setValueInt(T &&value,
                     const bool &doCallback = true)  // no flag checking, setValue (user-accessible) already does that
    {
        this->bHasValue = true;

        // determine float and string representations depending on whether setValue("string") or setValue(float) was
        // called
        const auto [newFloat, newUString] = [&]() {
            if constexpr(std::is_convertible_v<std::decay_t<T>, float> && !std::is_same_v<std::decay_t<T>, UString>) {
                const float f = std::forward<T>(value);
                return std::make_pair(f, UString::fmt("{:g}", f));
            } else if constexpr(std::is_same_v<std::decay_t<T>, std::string_view>) {
                const float f = !value.empty() ? std::strtof(std::string{value}.c_str(), nullptr) : 0.0f;
                return std::make_pair(f, UString{value});
            } else {
                const UString s = std::forward<T>(value);
                const float f = (s.length() > 0) ? s.toFloat() : 0.0f;
                return std::make_pair(f, s);
            }
        }();

        // backup previous values
        const float oldFloat = this->fValue.load();
        const UString oldUString{this->sValue};

        // set new values
        this->fValue = newFloat;
        this->sValue = newUString.utf8View();

        if(likely(doCallback)) {
            // handle possible execution callbacks
            if(!std::holds_alternative<std::monostate>(this->callback)) {
                std::visit(
                    [&](auto &&callback) {
                        using CallbackType = std::decay_t<decltype(callback)>;
                        if constexpr(std::is_same_v<CallbackType, NativeConVarCallback>)
                            callback();
                        else if constexpr(std::is_same_v<CallbackType, NativeConVarCallbackArgs>)
                            callback(newUString);
                        else if constexpr(std::is_same_v<CallbackType, NativeConVarCallbackFloat>)
                            callback(newFloat);
                    },
                    this->callback);
            }

            // handle possible change callbacks
            if(!std::holds_alternative<std::monostate>(this->changeCallback)) {
                std::visit(
                    [&](auto &&callback) {
                        using CallbackType = std::decay_t<decltype(callback)>;
                        if constexpr(std::is_same_v<CallbackType, NativeConVarChangeCallback>)
                            callback(oldUString, newUString);
                        else if constexpr(std::is_same_v<CallbackType, NativeConVarChangeCallbackFloat>)
                            callback(oldFloat, newFloat);
                    },
                    this->changeCallback);
            }
        }
    }

   private:
    bool bHasValue{false};
    CONVAR_TYPE type{CONVAR_TYPE::CONVAR_TYPE_FLOAT};
    uint8_t iDefaultFlags{FCVAR_BANCHO_COMPATIBLE};
    uint8_t iFlags{FCVAR_BANCHO_COMPATIBLE};

    std::atomic<float> fValue{0.0f};
    std::atomic<float> fDefaultValue{0.0f};
    float fDefaultDefaultValue{0.0f};

    ConVarString sName;
    ConVarString sHelpString;

    ConVarString sValue;
    ConVarString sDefaultValue{};
    ConVarString sDefaultDefaultValue{};

    // callback storage (allow having 1 "change" callback and 1 single value (or void) callback)
    ExecutionCallback callback{std::monostate()};
    ChangeCallback changeCallback{std::monostate()};

    mutable std::mutex flagLock;
};

//*******************//
//  Searching/Lists  //
//*******************//

class ConVarHandler {
   public:
    static ConVarString flagsToString(uint8_t flags);

   public:
    ConVarHandler();
    ~ConVarHandler();

    [[nodiscard]] const std::vector<ConVar *> &getConVarArray() const;
    [[nodiscard]] int getNumConVars() const;

    [[nodiscard]] ConVar *getConVarByName(const ConVarString &name, bool warnIfNotFound = true) const;
    [[nodiscard]] std::vector<ConVar *> getConVarByLetter(const ConVarString &letters) const;

    bool isVanilla();
};

extern ConVarHandler *convar;

#endif

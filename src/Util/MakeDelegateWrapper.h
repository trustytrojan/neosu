#pragma once
#include "Delegate.h"

#include <cstdint>
#include <type_traits>
#include <utility>

namespace SA {

// Helper to extract function signature from member function pointer type
template <typename T>
struct member_function_traits;

template <typename R, typename C, typename... Args>
struct member_function_traits<R (C::*)(Args...)> {
    using return_type = R;
    using class_type = C;
    using signature = R(Args...);
    static constexpr bool is_const = false;
};

template <typename R, typename C, typename... Args>
struct member_function_traits<R (C::*)(Args...) const> {
    using return_type = R;
    using class_type = C;
    using signature = R(Args...);
    static constexpr bool is_const = true;
};

template <auto Method, typename Class>
auto MakeDelegate(Class* instance) {
    using traits = member_function_traits<decltype(Method)>;
    using signature = typename traits::signature;
    using class_type = typename traits::class_type;

    return delegate<signature>::template create<class_type, Method>(instance);
}

template <auto Method, typename Class>
auto MakeDelegate(const Class* instance) {
    using traits = member_function_traits<decltype(Method)>;
    using signature = typename traits::signature;
    using class_type = typename traits::class_type;

    return delegate<signature>::template create<class_type, Method>(instance);
}

template <typename Self, typename... CallArgs>
class auto_callback {
   private:
    SA::delegate<void()> void_callback_;
    SA::delegate<void(CallArgs...)> args_callback_;
    SA::delegate<void(Self*)> self_callback_;

    enum class callback_type : uint8_t { none, void_cb, args_cb, self_cb } type_ = callback_type::none;

   public:
    auto_callback() = default;

    template <typename F>
    auto set_void_callback(F&& f) -> auto_callback&
        requires(std::is_invocable_v<F>)
    {
        void_callback_ = std::forward<F>(f);
        type_ = callback_type::void_cb;
        return *this;
    }

    template <typename F>
    auto set_args_callback(F&& f) -> auto_callback&
        requires(std::is_invocable_v<F, CallArgs...>)
    {
        args_callback_ = std::forward<F>(f);
        type_ = callback_type::args_cb;
        return *this;
    }

    template <typename F>
    auto set_self_callback(F&& f) -> auto_callback&
        requires(std::is_invocable_v<F, Self*>)
    {
        self_callback_ = std::forward<F>(f);
        type_ = callback_type::self_cb;
        return *this;
    }

    template <typename F>
    auto_callback& operator=(F&& f) {
        if constexpr(std::is_invocable_v<F>) {
            return set_void_callback(std::forward<F>(f));
        } else if constexpr(std::is_invocable_v<F, CallArgs...>) {
            return set_args_callback(std::forward<F>(f));
        } else if constexpr(std::is_invocable_v<F, Self*>) {
            return set_self_callback(std::forward<F>(f));
        } else {
            static_assert(sizeof(F) == 0, "Callable is not compatible with any supported signature");
        }
    }

    void operator()(Self* self, CallArgs... args) const {
        switch(type_) {
            case callback_type::void_cb:
                if(!void_callback_.isNull()) void_callback_();
                break;
            case callback_type::args_cb:
                if(!args_callback_.isNull()) args_callback_(args...);
                break;
            case callback_type::self_cb:
                if(!self_callback_.isNull()) self_callback_(self);
                break;
            case callback_type::none:
                break;
        }
    }

    explicit operator bool() const { return type_ != callback_type::none; }

    [[nodiscard]] bool isNull() const { return type_ == callback_type::none; }
};

}  // namespace SA

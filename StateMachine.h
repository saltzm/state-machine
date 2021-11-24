
#pragma once

#include <variant>
#include <optional>

namespace detail { 
// Helpers for using variant.
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

template <typename... Ts>
inline constexpr bool isVariant = false;
template <typename... Ts>
inline constexpr bool isVariant<std::variant<Ts...>> = true;
}


/**
 * Wraps a user-defined state machine to allow it to properly receive messages.
 */
template<typename InternalStateMachine>
class StateMachine {
public:
    StateMachine() : _behavior(typename InternalStateMachine::InitialBehavior{}), _self() {}
    
    template <typename M>
    void receive(M&& m) {
        if constexpr (detail::isVariant<typename InternalStateMachine::Message>) {
            static_assert(std::is_constructible<typename InternalStateMachine::Message, M>::value);
        } else {
            static_assert(std::is_same<typename InternalStateMachine::Message, M>::value);
        }
        
        _self.prereceive();
        // Receive a message. This is all gross to allow for variant and non-variant versions of Behavior and Messages. 
        // It could probably be less gross, or I could just not allow non-variant versions of these things since it's perfectly
        // valid to have std::variant of a single type.
        if constexpr (detail::isVariant<typename InternalStateMachine::Behavior>) {
            std::visit(detail::overloaded {
                [this, &m] (auto &&behavior) { 
                    if constexpr (
                        (detail::isVariant<typename std::decay_t<decltype(behavior)>::IgnoredMessage> && 
                         !std::is_constructible<typename std::decay_t<decltype(behavior)>::IgnoredMessage, typename std::decay_t<M>>::value) ||
                        (!detail::isVariant<typename std::decay_t<decltype(behavior)>::IgnoredMessage> && 
                         !std::is_same<typename std::decay_t<decltype(behavior)>::IgnoredMessage, typename std::decay_t<M>>::value)
                    ) {
                        auto newBehavior = behavior.receive(_self, m); 

                        if (newBehavior) {
                            if constexpr (detail::isVariant<typename std::remove_reference<decltype(behavior)>::type::NextBehavior>) {
                                // This extra dispatch is unfortunate
                                std::visit([this] (auto &&newB) {
                                    _behavior = newB;
                                }, *newBehavior);
                            } else {
                                _behavior = *newBehavior;
                            }
                        }
                    }
                }
            }, _behavior);
        } else {
           auto newBehavior = _behavior.receive(_self, m); 
           if (newBehavior) {
                _behavior = *newBehavior;
           }
        }
                
        _self.postreceive();
    }

    typename InternalStateMachine::Behavior _behavior;
    InternalStateMachine _self;
};

# Modern C++ State Machine

This is a tiny C++ API for writing state machines. It consists of a single header with a single class `StateMachine`. Users must define their StateMachine 
according to an interface that's not currently well-documented in the header but which should be clear from the following examples. 

States in the state machine are referred to as "behaviors" and events are referred to as "messages".

I made this for fun so I did everything in the quickest way I could, so there may be issues or missing features. Feel free to file issues or make PRs if 
you'd like.

# Examples

Compiling versions of these can be found [here](https://godbolt.org/z/oKWTTWqrP).

## State machine with a single state that accepts a single message type

```c++
struct Msg1{};

class SingleStateSingleMessageMachine { 
public:
    // Behaviors
    struct Running;
    
    // These using declarations are required. Message and Behavior can both either
    // be single types or std::variants of types.
    using Message = Msg1;
    using Behavior = Running;
    using InitialBehavior = Running;
    
    void prereceive() {
        globalCounter++;
    }
    
    void postreceive() {
    }
    
    struct Running {
        // This is a rough edge - It should be possible to not require the IgnoredMessages typedef 
        // if no messages should be ignored.
        using IgnoredMessage = void;

        std::optional<Behavior> receive(SingleStateSingleMessageMachine& self, const Msg1& m) {
            std::cout << "Running: received msg 1, globalCounter: " << self.globalCounter << std::endl;
            return std::nullopt;
        }
    };
    
    int globalCounter{0};
};

int main() {
    StateMachine<SingleStateSingleMessageMachine> boringStateMachine;
    boringStateMachine.receive(Msg1{});
    boringStateMachine.receive(Msg1{});
    /* Outputs:
    Running: received msg 1, globalCounter: 1
    Running: received msg 1, globalCounter: 2
    */
}
```

## State machine with a multiple states and message types

```c++
struct Msg1{};
struct Msg2{};
struct Msg3{};

class MyStateMachine { 
public:
    // Behaviors
    struct Start; struct Running; struct Stopped;
    
    using Message = std::variant<Msg1, Msg2, Msg3>;
    using Behavior = std::variant<Start, Running, Stopped>;
    using InitialBehavior = Start;
    
    void prereceive() {
        globalCounter++;
    }
    
    void postreceive() {
    }
        
    struct Start {
        // This specifies the messages that might be received but which should essentially
        // be ignored. In other words, it is not an error to receive these messages, but 
        // no behavior or transition should be executed upon receipt.
        using IgnoredMessage = std::variant<Msg2, Msg3>;
        // This specifies the valid next behaviors that can be transitioned to from
        // the start behavior. If any receive method tries to transition to a behavior
        // other than these, there will be a compilation error.
        using NextBehavior = std::variant<Running, Stopped>;
        
        std::optional<NextBehavior> receive(MyStateMachine& self, const Msg1& m) {
            std::cout << "Start: received msg 1, globalCounter: " << self.globalCounter << std::endl;
            // Returning a new behavior constitutes a state transition.
            return Running{};
        }
    };
    
    struct Running {
        using IgnoredMessage = Msg3;
        using NextBehavior = Stopped;

        std::optional<NextBehavior> receive(MyStateMachine& self, const Msg1& m) {
            std::cout << "Running: received msg 1, globalCounter: " << self.globalCounter << ", localCounter: " << localCounter << std::endl;
            ++localCounter;

            if (localCounter >= 2) {
                return Stopped{};
            } else {
                // Returning std::nullopt means that the current behavior is retained.
                return std::nullopt;
            }
        }

        std::optional<NextBehavior> receive(MyStateMachine& self, const Msg2& m) {
            std::cout << "Running: received msg 2, globalCounter: " << self.globalCounter << ", localCounter: " << localCounter << std::endl;
            ++localCounter;

            if (localCounter >= 2) {
                return Stopped{};
            } else {
                return std::nullopt;
            }
            
            return std::nullopt;
        }
        
        int localCounter{0};
    };
    
    struct Stopped {
        using IgnoredMessage = Msg3;
        using NextBehavior = Stopped;

        std::optional<NextBehavior> receive(MyStateMachine& self, const Msg1& m) {
            std::cout << "Stopped: received msg 1, globalCounter: " << self.globalCounter << std::endl;
            return std::nullopt;
        }

        std::optional<NextBehavior> receive(MyStateMachine& self, const Msg2& m) {
            std::cout << "Stopped: received msg 2, globalCounter: " << self.globalCounter << std::endl;
            return std::nullopt;
        }
    };
    
    int globalCounter{0};
};

int main() {
    StateMachine<MyStateMachine> myStateMachine;
    myStateMachine.receive(Msg1{});
    myStateMachine.receive(Msg1{});
    myStateMachine.receive(Msg2{});
    myStateMachine.receive(Msg1{});
    myStateMachine.receive(Msg2{});
    myStateMachine.receive(Msg3{});
    
    /* Outputs:
    Start: received msg 1, globalCounter: 1
    Running: received msg 1, globalCounter: 2, localCounter: 0
    Running: received msg 2, globalCounter: 3, localCounter: 1
    Stopped: received msg 1, globalCounter: 4
    Stopped: received msg 2, globalCounter: 5
    */
}
```

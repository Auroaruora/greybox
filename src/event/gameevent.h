#pragma once

#include <string>

enum class EventType {
    Click,
};

struct GameEvent {
    EventType type = EventType::Click;
    std::string source;
};

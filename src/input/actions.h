#pragma once

enum class Action {
    LookLeft,
    LookRight,
    LookUp,
    LookDown,
    Back,
    Interact,

    DebugMoveForward,
    DebugMoveBack,
    DebugMoveLeft,
    DebugMoveRight,
    DebugMoveUp,
    DebugMoveDown,
    DebugMoveFast,
    DebugViewPan,
    DebugViewZoom,
    DebugViewDrag,
    DebugViewEmulate,
    DebugViewEmulateDrag,
    DebugToggleOverlay
};
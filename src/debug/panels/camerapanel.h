#pragma once

struct DebugContext;
class Screen;

namespace CameraPanel {
void draw(DebugContext &context);
int getActiveViewpoint(const Screen &screen);
}

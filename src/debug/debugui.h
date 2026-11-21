#pragma once

struct DebugContext;
struct GLFWwindow;

namespace DebugUI {
void initialize(GLFWwindow *window);
void shutdown();

void update(DebugContext &context);
void render(DebugContext &context);
}

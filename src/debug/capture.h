#pragma once

#include <string>

class Screen;

namespace Capture {
void request(const Screen &screen, int viewpoint);
void startBatch(Screen &screen);
void update(Screen &screen);
void save();

bool isBatchActive();
int getBatchStep();
int getBatchTotal(const Screen &screen);
const std::string &getLastPath();
}

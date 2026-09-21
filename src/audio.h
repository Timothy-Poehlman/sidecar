#pragma once

#include <vector>

bool recordAudio(std::vector<float> &samples,
                 int sampleRate,
                 int framesPerBuffer);

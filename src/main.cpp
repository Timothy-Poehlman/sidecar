#include <iostream>
#include <vector>

#include "audio.h"

constexpr int SAMPLE_RATE = 16000; // 16 kHz - standard for speech
constexpr int RECORD_SECONDS = 2;
constexpr int FRAMES_PER_BUFFER = 512;
constexpr std::size_t MAX_FRAME_INDEX = SAMPLE_RATE * RECORD_SECONDS;

int main()
{
    std::vector<float> recordedSamples(MAX_FRAME_INDEX);

    if (!recordAudio(recordedSamples, SAMPLE_RATE, FRAMES_PER_BUFFER))
        return 1;

    std::cout << "Recording stopped\n";
    return 0;
}
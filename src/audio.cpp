#include "audio.h"

#include <cstddef>
#include <iostream>

#include <sys/select.h>
#include <unistd.h>

#include <portaudio.h>

#include "audio_block.h"
#include "audio_queue.h"
#include <algorithm>

namespace
{
    void appendBlockToRollingBuffer(const AudioBlock &block,
                                    std::vector<float> &rollingBuffer,
                                    std::size_t &writeIndex,
                                    std::size_t &bufferedFrames)
    {
        for (std::size_t frame = 0; frame < block.frameCount; ++frame)
        {
            rollingBuffer[writeIndex] = block.samples[frame];
            writeIndex = (writeIndex + 1) % rollingBuffer.size();
        }

        bufferedFrames = std::min(
            rollingBuffer.size(), bufferedFrames + block.frameCount);
    }

    void copyLatestWindow(const std::vector<float> &rollingBuffer,
                          std::size_t writeIndex,
                          std::vector<float> &orderedWindow)
    {
        const std::size_t firstPart = rollingBuffer.size() - writeIndex;

        std::copy(rollingBuffer.begin() + writeIndex,
                  rollingBuffer.end(),
                  orderedWindow.begin());
        std::copy(rollingBuffer.begin(),
                  rollingBuffer.begin() + writeIndex,
                  orderedWindow.begin() + firstPart);
    }

    void processCompletedBlocks(AudioQueue &queue,
                                std::vector<float> &rollingBuffer,
                                std::vector<float> &orderedWindow,
                                int sampleRate,
                                std::size_t &writeIndex,
                                std::size_t &bufferedFrames,
                                std::size_t &framesSinceProcessing)
    {
        AudioBlock block{};

        while (queue.tryRead(block))
        {
            appendBlockToRollingBuffer(
                block, rollingBuffer, writeIndex, bufferedFrames);
            framesSinceProcessing += block.frameCount;
        }

        if (bufferedFrames == rollingBuffer.size() &&
            framesSinceProcessing >= static_cast<std::size_t>(sampleRate))
        {
            // The rolling buffer only contains the current latest window. If
            // processing fell behind, older windows have already been
            // overwritten, so process this snapshot once rather than
            // replaying it for every missed interval.
            copyLatestWindow(rollingBuffer, writeIndex, orderedWindow);
            std::cout << "process!\n";
            framesSinceProcessing = 0;
        }
    }

    bool userRequestedStop()
    {
        fd_set inputSet;
        FD_ZERO(&inputSet);
        FD_SET(STDIN_FILENO, &inputSet);

        timeval timeout{};
        timeout.tv_usec = 50000;

        if (select(STDIN_FILENO + 1, &inputSet, nullptr, nullptr, &timeout) <= 0 ||
            !FD_ISSET(STDIN_FILENO, &inputSet))
        {
            return false;
        }

        char input[32]{};
        const ssize_t bytesRead = read(STDIN_FILENO, input, sizeof(input));

        for (ssize_t index = 0; index < bytesRead; ++index)
        {
            if (input[index] == 'x' || input[index] == 'X')
                return true;
        }

        return false;
    }

    int recordCallback(
        const void *inputBuffer,
        void *,
        unsigned long framesPerBuffer,
        const PaStreamCallbackTimeInfo *,
        PaStreamCallbackFlags,
        void *userData)
    {
        auto *queue = static_cast<AudioQueue *>(userData);

        if (inputBuffer == nullptr)
            return paContinue;

        AudioBlock *block = queue->beginWrite();

        if (block == nullptr)
        {
            // Queue is full.
            // Decide whether to drop this block or stop recording.
            return paContinue;
        }

        const auto *input =
            static_cast<const float *>(inputBuffer);

        block->frameCount = framesPerBuffer;

        std::copy_n(
            input,
            framesPerBuffer,
            block->samples.data());

        queue->finishWrite();

        return paContinue;
    }
}

bool recordAudio(std::vector<float> &samples,
                 int sampleRate,
                 int framesPerBuffer)
{
    if (samples.empty() || framesPerBuffer <= 0 ||
        framesPerBuffer > static_cast<int>(FRAMES_PER_BLOCK))
        return false;

    AudioQueue queue{};
    std::vector<float> orderedWindow(samples.size());
    std::size_t writeIndex = 0;
    std::size_t bufferedFrames = 0;
    std::size_t framesSinceProcessing = 0;

    PaError err = Pa_Initialize();
    if (err != paNoError)
    {
        std::cerr << "Could not initialize PortAudio: "
                  << Pa_GetErrorText(err) << "\n";
        return false;
    }

    PaStream *stream{};
    err = Pa_OpenDefaultStream(&stream,
                               1,
                               0,
                               paFloat32,
                               sampleRate,
                               framesPerBuffer,
                               recordCallback,
                               &queue);
    if (err != paNoError)
    {
        std::cerr << "Could not open the audio stream: "
                  << Pa_GetErrorText(err) << "\n";
        Pa_Terminate();
        return false;
    }

    err = Pa_StartStream(stream);
    if (err != paNoError)
    {
        std::cerr << "Could not start the audio stream: "
                  << Pa_GetErrorText(err) << "\n";
        Pa_CloseStream(stream);
        Pa_Terminate();
        return false;
    }

    std::cout << "Recording continuously. Enter x and press Enter to stop.\n";
    bool stoppedByUser = false;

    while (true)
    {
        // PortAudio returns 1 while recording, 0 when the stream finishes,
        // and a negative value when it reports an error.
        const PaError streamStatus = Pa_IsStreamActive(stream);
        if (streamStatus != 1)
        {
            if (streamStatus < 0)
                std::cerr << "Audio stream status error: "
                          << Pa_GetErrorText(streamStatus) << "\n";
            break;
        }

        // The callback records audio; processing happens outside the
        // real-time callback when a complete two-second window is available.
        processCompletedBlocks(queue,
                               samples,
                               orderedWindow,
                               sampleRate,
                               writeIndex,
                               bufferedFrames,
                               framesSinceProcessing);

        if (userRequestedStop())
        {
            stoppedByUser = true;
            Pa_StopStream(stream);
            break;
        }
    }

    Pa_CloseStream(stream);
    Pa_Terminate();
    return stoppedByUser;
}

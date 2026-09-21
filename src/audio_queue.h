#pragma once

#include <array>
#include <atomic>
#include <cstddef>

#include "audio_block.h"

constexpr std::size_t QUEUE_CAPACITY = 128;

class AudioQueue
{
public:
    AudioBlock *beginWrite()
    {
        const std::size_t writePosition = writeIndex_.load(std::memory_order_relaxed);
        AudioSlot &slot = slots_[writePosition % QUEUE_CAPACITY];

        SlotState expected = SlotState::free;
        if (!slot.state.compare_exchange_strong(
                expected, SlotState::writing, std::memory_order_acquire)) // its checking if the slot is free, if its not free then we need to overwrite the oldest
        {
            // The queue is full. Claim its oldest ready block so new audio wins.
            const std::size_t readPosition = readIndex_.load(std::memory_order_acquire); // we are using acquire because we want to make sure that we see the latest value of readIndex_ before we proceed
            AudioSlot &oldest = slots_[readPosition % QUEUE_CAPACITY];
            expected = SlotState::ready;

            if (!oldest.state.compare_exchange_strong(
                    expected, SlotState::writing, std::memory_order_acquire)) // this is the safety net, before we write to the oldest block, we want to make sure no other thread is reading/writing to it. If the oldest block is not ready, we cannot write to it, so we return nullptr.
            {
                return nullptr; // drop instead of waiting, we dont want to be publishing old audio, its just better to drop and get newer.
            }

            readIndex_.store(readPosition + 1, std::memory_order_release); // wait for everything above to be done before we update. advance the next index to be written to
            pendingWritePosition_ = writePosition;
        }
        else
        {
            pendingWritePosition_ = writePosition;
        }

        return &slots_[pendingWritePosition_ % QUEUE_CAPACITY].block;
    }

    void finishWrite()
    {
        slots_[pendingWritePosition_ % QUEUE_CAPACITY].state.store(
            SlotState::ready, std::memory_order_release);
        writeIndex_.store(pendingWritePosition_ + 1, std::memory_order_release);
    }

    bool tryRead(AudioBlock &destination)
    {
        const std::size_t readPosition = readIndex_.load(std::memory_order_relaxed);
        AudioSlot &slot = slots_[readPosition % QUEUE_CAPACITY];

        SlotState expected = SlotState::ready;
        if (!slot.state.compare_exchange_strong(
                expected, SlotState::reading, std::memory_order_acquire))
        {
            return false;
        }

        destination = slot.block;
        slot.state.store(SlotState::free, std::memory_order_release);
        readIndex_.store(readPosition + 1, std::memory_order_release);
        return true;
    }

private:
    enum class SlotState
    {
        free,
        writing,
        ready,
        reading
    };

    struct AudioSlot
    {
        AudioBlock block{};
        std::atomic<SlotState> state{SlotState::free};
    };

    std::array<AudioSlot, QUEUE_CAPACITY> slots_{};
    std::atomic<std::size_t> writeIndex_{0};
    std::atomic<std::size_t> readIndex_{0};
    std::size_t pendingWritePosition_{};
};
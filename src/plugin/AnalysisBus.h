// LUMI//DANCER — lock-free hand-off from the audio thread to the UI.
//
// A seqlock-style atomic snapshot: the single writer (audio thread) bumps a
// version counter around each copy; readers retry until they observe a
// consistent even version. No locks, no allocation, wait-free for the writer.
#pragma once

#include <atomic>
#include <cstdint>
#include <type_traits>

namespace lumi
{
template <typename T>
class AtomicSnapshot
{
    static_assert (std::is_trivially_copyable_v<T>,
                   "AtomicSnapshot requires trivially copyable payloads");

public:
    // Audio thread only. Wait-free.
    void publish (const T& value) noexcept
    {
        const uint32_t start = version.load (std::memory_order_relaxed);
        version.store (start + 1, std::memory_order_release);   // odd = writing
        std::atomic_thread_fence (std::memory_order_release);
        buffer = value;
        std::atomic_thread_fence (std::memory_order_release);
        version.store (start + 2, std::memory_order_release);
    }

    // Any thread. Retries during a concurrent write (writes are microseconds).
    T read() const noexcept
    {
        T out {};
        uint32_t v1 = 0, v2 = 0;
        do
        {
            v1 = version.load (std::memory_order_acquire);
            std::atomic_thread_fence (std::memory_order_acquire);
            out = buffer;
            std::atomic_thread_fence (std::memory_order_acquire);
            v2 = version.load (std::memory_order_acquire);
        } while (v1 != v2 || (v1 & 1u) != 0);
        return out;
    }

private:
    T buffer {};
    std::atomic<uint32_t> version { 0 };
};
} // namespace lumi

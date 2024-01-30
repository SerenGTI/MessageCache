#pragma once

#include <mutex>
#include <new>
#include <coroutine>
#include <messagecache/ring_buffer.hpp>

namespace messagecache {


template<std::size_t SIZE>
class coro_cache : protected ring_buffer<SIZE>
{
public:
    using T = ring_buffer<SIZE>::T;

    /**
     * A slot takes ownership over a sequence of bytes in the cache.
     * While a caller holds its slot, the data will not be erased from the cache.
     * As soon as the slot is destroyed, the cache may reuse the storage.
     */
    class slot : protected ring_buffer<SIZE>::slot
    {
    private:
        friend coro_cache;

    public:
        constexpr slot() noexcept : ring_buffer<SIZE>::slot() {}

        constexpr slot(coro_cache& buffer, T* start, std::size_t size)
            : ring_buffer<SIZE>::slot(buffer, start, size), cache_(buffer)
        {}

        // move only
        constexpr slot(slot&&) noexcept = default;
        constexpr auto operator=(slot&&) noexcept -> slot& = default;

        constexpr slot(const slot&) noexcept = delete;
        constexpr auto operator=(const slot&) noexcept -> slot& = delete;

        // implicit downcast-conversion
        constexpr slot(ring_buffer<SIZE>::slot&& other) noexcept
            : ring_buffer<SIZE>::slot(std::move(other))
        {}

        ~slot() noexcept
        {
            release();
        }

        void release()
        {
            // release the underlying slot
            this->ring_buffer<SIZE>::slot::release();

            // a slot was released, try to dequeue an awaiter
            std::unique_lock<std::mutex> lock(cache_.mtx_);

            // iterate over all awaiters and try to allocate
            alloc_awaiter* current = cache_.first_;

            while(current) {
                if(current->try_alloc()) {
                    // the first slot was able to allocate, remove it from the list of awaiters.
                    cache_.first_ = current->next_;
                    current->handle_.resume();
                    current = cache_.first_;
                }
                else {
                    return; // allocation failed, stop trying
                }
            }
            if(not cache_.first_) {
                cache_.last_ = nullptr;
            }
        }
    private:
        coro_cache& cache_;
    };


    // asynchronously attempt to allocate a new buffer slot with the given size
    auto alloc(std::size_t slot_size) noexcept
    {
        return alloc_awaiter{*this, slot_size};
    }
private:

    struct alloc_awaiter {
        friend class coro_cache;
        friend class coro_cache::slot;

        alloc_awaiter(coro_cache& cache, std::size_t size)
            : cache_(cache), size_(size)
        {}

        auto await_ready() -> bool
        {
            return false;
        }

        auto await_suspend(std::coroutine_handle<> h)
        {
            // store the handle
            handle_ = h;

            std::unique_lock<std::mutex> lock(cache_.mtx_);
            if(cache_.last_) {
                // append *this to the waiter list
                cache_.last_->next_ = this;
                cache_.last_ = this;
            }
            else {
                // *this is the first awaiter.
                assert(cache_.first_ = nullptr);

                // try to allocate a slot
                if(this->try_alloc()) {
                    return false; // allocation successful, do not suspend
                }

                // allocation failed, enqueue this awaiter into the list of awaiters.
                cache_.first_ = this;
                cache_.last_ = this;
            }

            return true; // suspend the coroutine
        }

        auto await_resume() -> slot {
            ret_ = cache_.ring_buffer<SIZE>::try_alloc(size_);

            assert(ret_ && "alloc_awaiter woke up too early.");
            return ret_;
        }
    private:
        auto try_alloc() noexcept -> bool
        {
            // try to allocate a slot
            ret_ = cache_.try_alloc(size_);
            return ret_;
        }

        coro_cache& cache_;
        std::size_t size_;
        slot ret_;

        alloc_awaiter* next_ = nullptr;

        std::coroutine_handle<> handle_ = nullptr;
    };

    alloc_awaiter* first_ = nullptr;
    alloc_awaiter* last_ = nullptr;
    // this mutex protects against concurrent modifications of first and last
    std::mutex mtx_;
};
} // namespace messagecache
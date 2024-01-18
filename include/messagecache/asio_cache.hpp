#pragma once

#if defined(ASIO_STANDALONE)
#include <asio.hpp>
#else
#include <boost/asio.hpp>
#endif

#include <messagecache/ring_buffer.hpp>

namespace messagecache {


template<std::size_t SIZE>
class asio_cache : public ring_buffer<SIZE>
{
public:
    using T = ring_buffer<SIZE>::T;

    /**
     * A slot takes ownership over a sequence of bytes in the cache.
     * While a caller holds its slot, the data will not be erased from the cache.
     * As soon as the slot is destroyed, the cache may reuse the storage.
     */
    class slot : public ring_buffer<SIZE>::slot
    {
    private:
        friend asio_cache;
    public:

        constexpr slot() noexcept
            : ring_buffer<SIZE>::slot()
        {}

        constexpr slot(asio_cache& buffer, T* start, std::size_t size)
            : ring_buffer<SIZE>::slot(buffer, start, size)
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

        /**
         * Returns an asio::mutable_buffer that spans the slot's memory region.
         * @return An asio::mutable_buffer that can be used to fill the memory region
         */
        auto getWriteBuffer() const -> asio::mutable_buffer
        {
            // TODO: synchronize before?
            return {this->begin(), this->end() - this->begin()};
        }

        /**
         * Returns an asio::const_buffer that spans the slot's memory region.
         * Ensures inter-thread synchronization on the buffer's region.
         * @return An asio::const_buffer that can be used to read from the memory region
         */
        auto getConstBuffer() const -> asio::const_buffer
        {
            this->synchronize();
            return getWriteBuffer();
        }
    };


    // asynchronously attempt to allocate a new buffer slot with the given size
    template<typename CompletionToken>
    auto alloc(std::size_t slot_size, CompletionToken&& token)
    {
        return asio::async_initiate<decltype(token), void(asio::error_code, slot)>(
            [this, slot_size](auto&& token) {
                auto executor = asio::get_associated_executor(token);
                auto state = asio::cancellation_state(asio::get_associated_cancellation_slot(token));

                asio::post(executor, [this, slot_size, token = std::forward<decltype(token)>(token), state]() mutable {
                    std::error_code e;
                    if(state.cancelled() != asio::cancellation_type::none) {
                        // function is cancelled
                        e = asio::error::operation_aborted;
                        std::move(token)(e, slot{}); // default-constructed slot points to invalid memory region
                        return;
                    }

                    // try to allocate
                    auto slot = this->try_alloc(slot_size);
                    if(slot) {
                        // allocation successful
                        std::move(token)(e, std::move(slot));
                    }
                    else{
                        // yield
                        alloc(slot_size, std::move(token));
                    }
                });
            },
            token);
    }

};
}
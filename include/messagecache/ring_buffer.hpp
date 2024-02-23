#pragma once

#include <atomic>
#include <cassert>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <memory>
#include <span>

namespace messagecache {

/**
 * Ringbuffer implementation that allocates once at initialization time.
 * Does not reallocate at runtime.
 * Allows the caller to allocate slots of any size in the buffer.
 * Slots represent contiguous memory regions that can be used arbitrarily
 * while the slot object is alive.
 */
template<std::size_t SIZE, typename Allocator = std::allocator<std::byte>>
class ring_buffer : private Allocator
{
    constexpr static std::size_t HEADER_LEN = 4;

public:
    using T = std::byte;

    using value_type = T;
    using allocator_traits = std::allocator_traits<Allocator>;
    using size_type = typename allocator_traits::size_type;

    constexpr ring_buffer()
        : raw_ptr_(allocator_traits::allocate(*this, SIZE + HEADER_LEN)),
          raw_(raw_ptr_, SIZE + HEADER_LEN),
          write_ptr_(raw_.data()),
          free_ptr_(raw_.data())
    {}

    ~ring_buffer() noexcept {
        allocator_traits::deallocate(*this, raw_ptr_, SIZE + HEADER_LEN);
    };

    constexpr ring_buffer(ring_buffer&& other) noexcept
        : raw_ptr_(other.raw_ptr_),
          raw_(raw_ptr_, SIZE + HEADER_LEN),
          write_ptr_(other.write_ptr_.load(std::memory_order_seq_cst)),
          free_ptr_(other.free_ptr_.load(std::memory_order_seq_cst))
    {
        other.raw_ptr_ = nullptr;
        // other.raw_ = decltype(raw_){0, raw_.size()};
    }

    /**
     * A slot takes ownership over a sequence of bytes in the ring buffer.
     * While a caller holds its slot, the data will not be erased from the ring buffer.
     * As soon as the slot is destroyed, the ring buffer may reuse the storage.
     * The caller must ensure synchronization of concurrent access to the slot's
     * memory region.
     */
    class slot
    {
    protected:
        friend ring_buffer;

        /**
         * @param buffer the corresponding buffer
         * @param start  the start pointer of the slot. that is *start is the size of the
         * slot
         * @param size   the size of the slot.
         */
        constexpr slot(ring_buffer& buffer, T* start, std::size_t size) noexcept
            : buf_(std::addressof(buffer)), start_(start), size_(size)
        {}

    public:
        // default-constructed slot points to invalid memory region
        constexpr slot() noexcept : buf_(nullptr), start_(nullptr), size_(0) {}

        // delete copy
        constexpr slot(const slot&) = delete;
        constexpr auto operator=(const slot&) -> slot& = delete;

        // move-only semantics
        constexpr auto operator=(slot&& other) noexcept -> slot&
        {
            if(this != std::addressof(other)) {
                buf_ = other.buf_;
                start_ = other.start_;
                size_ = other.size_;

                other.start_ = nullptr;
                other.size_ = 0;
            }
            return *this;
        }
        constexpr slot(slot&& other) noexcept
            : buf_(other.buf_), start_(other.start_), size_(other.size_)
        {
            other.start_ = nullptr;
            other.size_ = 0;
        }

        // releases the slot's data
        constexpr ~slot() noexcept { release(); }

        // returns true if the slot points to a valid range of memory
        constexpr auto valid() const noexcept -> bool
        {
            return start_ != nullptr and buf_ != nullptr;
        }

        constexpr operator bool() const noexcept { return valid(); }

        // Releases the slot's data.
        // Invalidates all iterators
        void release() noexcept
        {
            if(start_) {
                discard();

                start_ = nullptr;
                size_ = 0;
            }
        }

        constexpr auto begin() const noexcept -> T*
        {
            // returns nullptr if start_ == nullptr,
            // else start_ + HEADER_LEN
            return start_ + (HEADER_LEN * (start_ != nullptr));
        }

        constexpr auto cbegin() const noexcept -> const T* { return begin(); }

        constexpr auto end() const noexcept -> T* { return begin() + size_; }

        constexpr auto cend() const noexcept -> const T* { return cbegin() + size_; }

        /**
         * Returns a read-only std::span that spans the slot's memory region.
         * Ensures inter-thread synchronization on the buffer's region.
         * @return A std::span that can be used to read from the memory region
         */
        constexpr auto asSpan() const -> std::span<const std::byte>
        {
            synchronize();
            return {cbegin(), size_};
        }

        /**
         * Returns a read-only std::span that spans the slot's memory region.
         * Ensures inter-thread synchronization on the buffer's region.
         * @return A std::span that can be used to read from the memory region
         */
        constexpr auto asMutableSpan() -> std::span<std::byte>
        {
            synchronize();
            return {begin(), size_};
        }

        /**
         * Call flush() after modifications of the buffer's memory region.
         * This ensures inter-thread synchronization on the buffer's region
         */
        void flush() noexcept
        {
            std::atomic_thread_fence(std::memory_order_release);
            std::atomic_store_explicit(&flag_, true, std::memory_order_release);
        }

        /**
         * Call synchronize() before accessing buffer's memory region via the
         * begin() or end() iterators. This ensures inter-thread synchronization
         * on the buffer's region
         */
        void synchronize() const noexcept
        {
            std::atomic_load_explicit(&flag_, std::memory_order_acquire);
            std::atomic_thread_fence(std::memory_order_acquire);
        }

        void print() const
        {
            synchronize();
            std::stringstream ss;
            const auto* ptr = reinterpret_cast<const unsigned char*>(start_);
            for(std::size_t i = 0; i < size_ + HEADER_LEN; ++i) {
                ss << std::hex << std::setw(2) << std::setfill('0')
                   << static_cast<int>(ptr[i]) << " ";
            }
            std::cout << ss.str() << std::endl;
        }

    private:
        constexpr void discard()
        {
            // OPTIMIZATION: try to advance the free pointer immediately if it points to
            // the start of the current slot
            auto* old_value = start_;
            if(buf_->free_ptr_.compare_exchange_strong(old_value,
                                                       start_ + size_ + HEADER_LEN,
                                                       std::memory_order_release)) {
                return;
            }

            // mark the slot as OK to free.
            auto* unused_flag = reinterpret_cast<std::uint16_t*>(start_ + 2);
            *unused_flag = static_cast<std::uint16_t>(0b1111'1111'1111'1111);
            buf_->flush();
        }

    private:
        ring_buffer* buf_;

        T* start_;
        std::size_t
            size_; // size of the slot visible to the application, *without* header

        std::atomic_bool flag_;
    };

    // try to allocate a slot of the given size
    auto try_alloc(std::size_t slot_size) noexcept -> slot
    {
        auto* start = getNextWritePointer(slot_size);
        if(start) {
            return slot{*this, start, slot_size};
        }
        return {}; // default-constructed slot points to nullptr memory region
    }

private:
    constexpr static auto lengthAt(T* const loc) noexcept -> std::size_t
    {
        auto* len_field = reinterpret_cast<std::uint16_t*>(loc);
        return *len_field;
    }

    constexpr auto getLengthAndFlag(T* const start) noexcept
        -> std::pair<std::size_t, bool>
    {
        std::size_t length = lengthAt(start);

        auto* unused_flag = reinterpret_cast<std::uint16_t*>(start + 2);
        bool unused = (*unused_flag) > 0;

        return {length, unused};
    }

    constexpr auto updateFreePtr() noexcept
    {
        // synchronize memory to ensure we see updated slot flags
        synchronize();

        auto* const wp = write_ptr_.load(std::memory_order_relaxed);
        auto* const fp = free_ptr_.load(std::memory_order_relaxed);

        // Linear search for next non-free slot
        if(wp < fp) {
            // [==== ==== ==== len,flag ==== ==== ====]
            //      wp         fp
            //      xxxxxxxxxxx                    000
            // .(2).             .........(1).........

            for(T* i = fp; i < raw_.data() + raw_.size() - HEADER_LEN;) {
                // case (1)
                auto [length, unused] = getLengthAndFlag(i);
                if(length == 0) {
                    // There is no slot at the end
                    break;
                }
                if(length > 0 and unused) {
                    i += length + HEADER_LEN;
                } else {
                    free_ptr_.store(i, std::memory_order_relaxed);
                    // spdlog::debug("update wp<fp={}", i-raw_.data());
                    return;
                }
            }
            for(T* i = raw_.data(); i < wp;) {
                // case (2)
                auto [length, unused] = getLengthAndFlag(i);
                if(length > 0 and unused) {
                    i += length + HEADER_LEN;
                } else {
                    free_ptr_.store(i, std::memory_order_relaxed);
                    // spdlog::debug("update wp<fp={}", i-raw_.data());
                    return;
                }
            }
        } else {
            // this is the case for fp < wp.

            // [==== len,flag ==== ==== ==== ==== ====]
            //       fp            wp
            //  xxxxx              xxxxxxxxxxxxxxxxxxx
            for(T* i = fp; i < wp;) {
                auto [length, unused] = getLengthAndFlag(i);
                if(unused) {
                    i += length + HEADER_LEN;
                } else {
                    free_ptr_.store(i, std::memory_order_relaxed);
                    // spdlog::debug("update fp {}->{}", fp-raw_.data(), i-raw_.data()/*,
                    // active_slots_.load(std::memory_order_relaxed)*/);
                    return;
                }
            }
        }

        free_ptr_.store(raw_.data(), std::memory_order_relaxed);
        write_ptr_.store(raw_.data(), std::memory_order_relaxed);
        return;
    }


    /**
     * Returns a pointer to the next slot that has data_size many bytes available
     * @param  data_size Size of the slot to reserve
     * @return           Pointer to the begin of the slot
     */
    auto getNextWritePointer(std::size_t data_size) noexcept -> T*
    {
        std::size_t required_size = data_size + HEADER_LEN;
        if(required_size > raw_.size()) {
            return nullptr; // cannot allocate this many bytes.
        }
        assert(required_size <= raw_.size());

        // TODO: This lock is only necessary for multiple consumers
        // auto _ = mtx_.scopedLock();

        updateFreePtr();

        auto* const wp = write_ptr_.load(std::memory_order_relaxed);
        auto* const fp = free_ptr_.load(std::memory_order_relaxed);

        // spdlog::debug("new slot with fp{}, wp{}", fp - raw_.data(), wp - raw_.data());

        if(wp == fp) {
            // buffer is empty
            free_ptr_.store(raw_.data(), std::memory_order_relaxed);
            write_ptr_.store(raw_.data() + required_size, std::memory_order_relaxed);
            // spdlog::debug("(empty)");
            return setLengthAt(raw_.data(), data_size);
        }
        if(wp < fp) {
            // [==== ==== ==== ==== ==== ==== ====]
            //      wp        fp
            //      xxxxxxxxxx(slot in use)

            std::size_t size_avail_between = fp - wp;
            if(required_size < size_avail_between) {
                // keep the free and write pointer separated while the ring buffer is
                // non-empty
                write_ptr_.store(wp + required_size);
                // spdlog::debug("(2) getting slot wp{}, fp{}", wp-raw_.data(),
                // fp-raw_.data());
                return setLengthAt(wp, data_size);
            }
        } else {
            // [==== ==== ==== ==== ==== ==== ====]
            //        fp            wp
            //  xxxxxx              xxxxxxxxxxxxxx
            //   (2)                    (1)

            std::size_t size_avail_at_end = raw_.data() + raw_.size() - wp;
            if(required_size <= size_avail_at_end) {
                // (1)
                write_ptr_.store(wp + required_size);
                // spdlog::debug("(3) getting slot wp{}, fp{}", wp-raw_.data(),
                // fp-raw_.data());
                return setLengthAt(wp, data_size);
            }

            std::size_t size_avail_at_front = fp - raw_.data();
            if(required_size < size_avail_at_front) {
                // keep the free and write pointer separated while the ring buffer is
                // filled (2) ensure the end is zeroed to avoid buggy freeing near the end
                std::fill(wp, raw_.data() + raw_.size(), static_cast<T>(0));

                write_ptr_.store(raw_.data() + required_size, std::memory_order_relaxed);
                // spdlog::debug("(4) getting slot wp{}, fp{}", wp-raw_.data(),
                // fp-raw_.data());
                return setLengthAt(raw_.data(), data_size);
            }
        }
        return nullptr;
    }

    constexpr auto setLengthAt(T* begin, std::uint16_t size) noexcept -> T*
    {
        auto* len = reinterpret_cast<std::uint16_t*>(begin);
        *len = size;
        auto* flag = reinterpret_cast<std::uint16_t*>(begin + 2);
        *flag = 0;

        flush();
        return begin;
    }

    void flush() noexcept
    {
        std::atomic_thread_fence(std::memory_order_release);
        std::atomic_store_explicit(&flag_, true, std::memory_order_release);
    }
    void synchronize() const noexcept
    {
        std::atomic_load_explicit(&flag_, std::memory_order_acquire);
        std::atomic_thread_fence(std::memory_order_acquire);
    }

private:
    constexpr static std::size_t hardware_destructive_interference_size = 64ul;


    T* raw_ptr_;
    std::span<T, SIZE + HEADER_LEN> raw_;

    alignas(hardware_destructive_interference_size) std::atomic_bool flag_;

    using cursor_type = std::atomic<T*>;
    static_assert(cursor_type::is_always_lock_free);

    // ensure the two pointers are on different cache lines
    alignas(hardware_destructive_interference_size) cursor_type write_ptr_ = nullptr;
    alignas(hardware_destructive_interference_size) cursor_type free_ptr_ = nullptr;

    // do we need padding?..
    // std::array<std::byte, hardware_destructive_interference_size - sizeof(cursor_type)> padding_;
};
} // namespace messagecache

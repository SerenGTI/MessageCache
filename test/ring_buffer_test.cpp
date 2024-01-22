
#include <gtest/gtest.h>
#include <messagecache/ring_buffer.hpp>

TEST(ring_buffer_test, front_alloc) {
    messagecache::ring_buffer<20> buffer;

    auto slot = buffer.try_alloc(10);

    ASSERT_NE(slot.begin(), nullptr);
    ASSERT_EQ(slot.end() - slot.begin(), 10);
}

TEST(ring_buffer_test, two_allocs_full) {
    messagecache::ring_buffer<20> buffer;

    auto slot = buffer.try_alloc(10);

    ASSERT_NE(slot.begin(), nullptr);
    ASSERT_EQ(slot.end() - slot.begin(), 10);

    auto slot2 = buffer.try_alloc(6);

    ASSERT_NE(slot2.begin(), nullptr);
    ASSERT_EQ(slot2.end() - slot2.begin(), 6);
}

TEST(ring_buffer_test, too_large_alloc) {
    messagecache::ring_buffer<20> buffer;

    auto slot = buffer.try_alloc(21);

    ASSERT_EQ(slot.begin(), nullptr);
    ASSERT_EQ(slot.end(), nullptr);
}

TEST(ring_buffer_test, front_alloc_and_memset) {
    messagecache::ring_buffer<20> buffer;

    auto slot = buffer.try_alloc(10);

    ASSERT_NE(slot.begin(), nullptr);
    ASSERT_EQ(slot.end() - slot.begin(), 10);

    memset(slot.begin(), 'a', slot.end() - slot.begin());

    for (std::byte v : slot) {
        ASSERT_EQ(static_cast<const char>(v), 'a');
    }
}

TEST(ring_buffer_test, two_allocs_release_in_order) {
    messagecache::ring_buffer<20> buffer;

    {
    auto slot = buffer.try_alloc(10);

    ASSERT_NE(slot.begin(), nullptr);
    ASSERT_EQ(slot.end() - slot.begin(), 10);
    }

    auto slot2 = buffer.try_alloc(10);

    ASSERT_NE(slot2.begin(), nullptr);
    ASSERT_EQ(slot2.end() - slot2.begin(), 10);
}

TEST(ring_buffer_test, many_allocs_and_frees) {
    messagecache::ring_buffer<2000> buffer;

    std::vector<decltype(buffer)::slot> slots;

    std::byte* last = nullptr;

    while(true) {
        // allocate new slots until the buffer is full
        auto slot = buffer.try_alloc(100);

        if(slot.valid()) {
            EXPECT_TRUE(slot.begin() > last);
            last = slot.begin();

            slots.push_back(std::move(slot));
        }
        else{
            // buffer is full
            break;
        }
    }

    //the buffer is full


    for (auto i = 0; i < 6; ++i) {
        // release some slots at the beginning
        // slots.erase(slots.begin());
        slots[i].release();
    }
    for (auto i = 0; i < 4; ++i) {
        // reallocate some slots
        auto slot = buffer.try_alloc(100);
        slots.push_back(std::move(slot));
    }
    // ringbuffer now looks like this
    // [==== ==== ==== len,flag ==== ==== ====]
    //      wp         fp
    //      xxxxxxxxxxx


    // we now free all slots that are following the fp, and keep the ones at the front
    int i = 0;
    last = nullptr;
    for(auto& slot: slots) {
        ++i;
        if(slot.begin() == nullptr) {
            continue;
        }

        slot.release();

        if(slot.begin() < last) {
            break;
        }
        last = slot.begin();
    }
    // we now released all slots from fp to the right and the first at the front

    // allocate another front to propagate the fp through the right-side boundary
    auto slot = buffer.try_alloc(100);
    slots.push_back(std::move(slot));
}
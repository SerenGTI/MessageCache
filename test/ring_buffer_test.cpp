
#include <gtest/gtest.h>
#include <messagecache/ring_buffer.hpp>


TEST(ring_buffer_test, front_alloc) {
  messagecache::ring_buffer<20> buffer;

  auto slot = buffer.try_alloc(10);

  ASSERT_NE(slot.begin(), nullptr);
  ASSERT_EQ(slot.end() - slot.begin(), 10);
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


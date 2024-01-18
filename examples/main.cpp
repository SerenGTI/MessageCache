
#if defined(ASIO_STANDALONE)
#include <asio.hpp>
#else
#include <boost/asio.hpp>
#endif

#include <iostream>
#include <messagecache/asio_cache.hpp>
#include <messagecache/ring_buffer.hpp>

auto try_alloc_in_ring_buffer() {
  messagecache::ring_buffer<20> buffer;

  auto slot = buffer.try_alloc(10);
  for (std::byte v : slot) {
    std::cout << std::setw(2) << static_cast<char>(v);
  }
  std::cout << std::endl;

  std::memset(slot.begin(), 'a', slot.end() - slot.begin());

  for (std::byte v : slot) {
    std::cout << std::setw(2) << static_cast<char>(v);
  }
  std::cout << std::endl;
}

auto alloc_in_asio_cache() {
  asio::io_context ctx;

  messagecache::asio_cache<12> asiobuf;
  auto fut = asio::co_spawn(
      ctx,
      [&asiobuf]() -> asio::awaitable<decltype(asiobuf)::slot> {
        co_return co_await asiobuf.alloc(12, asio::use_awaitable);
      },
      asio::use_future);
  ctx.run();
  auto slot = fut.get();

  for (std::byte v : slot) {
    std::cout << std::setw(2) << static_cast<char>(v);
  }
  std::cout << std::endl;
}

int main() {

  try_alloc_in_ring_buffer();

  alloc_in_asio_cache();
}
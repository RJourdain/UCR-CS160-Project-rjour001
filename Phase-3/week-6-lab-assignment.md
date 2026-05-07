Warm-up for Phase 3.

Implement a minimal concurrent queue with two operations:

- `Push(int v)`: append `v` to the queue.
- `Pop() -> int`: block until at least one element is available, then return and remove the front element.

Use one `std::mutex` and one `std::condition_variable`.

**Test.** Spawn the consumer thread first; it pops 101 times and logs each value it sees (e.g. `pop 0`). Sleep for ~100 ms in the main thread, then spawn the producer thread, which pushes `0, 1, 2, ..., 100` in order, logs each push (e.g. `push 0`), and **sleeps 1 ms between pushes**. The pop-side output must be exactly `0, 1, 2, ..., 100`. Because the producer is now slower than the consumer, you should observe push and pop log lines interleaved (`push 0 / pop 0 / push 1 / pop 1 / ...`) rather than all pushes followed by all pops. The interleaving is the visible evidence that your `cv.wait` and `cv.notify_one` are actually exercised every iteration.

Phase 3 will extend this skeleton with `Close()`, an `std::optional<T>` return from `Pop`, and multiple concurrent consumers.

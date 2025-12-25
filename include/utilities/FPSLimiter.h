#pragma once

#include <chrono>
#include <thread>

class FPSLimiter {
public:
  FPSLimiter(int target_fps) : target_frame_duration_(1000.0 / target_fps) {
    frame_end_time_ = std::chrono::high_resolution_clock::now();
  }

  void limit() {
    const auto current_time = std::chrono::high_resolution_clock::now();

    // Calculate how long the work (frame) took
    const std::chrono::duration<double, std::milli> work_time =
        current_time - frame_end_time_;

    // Calculate how long we need to sleep
    const std::chrono::duration<double, std::milli> sleep_duration(
        target_frame_duration_.count() - work_time.count());

    if (sleep_duration.count() > 0.0) {
      // Sleep using high-precision duration to avoid truncation
      std::this_thread::sleep_for(sleep_duration);
    }

    // Mark the end of this frame based on the target, not actual time
    // This prevents drift from accumulating
    frame_end_time_ += std::chrono::duration_cast<
        std::chrono::high_resolution_clock::duration>(target_frame_duration_);
  }

private:
  std::chrono::high_resolution_clock::time_point frame_end_time_;
  const std::chrono::duration<double, std::milli> target_frame_duration_;
};
#pragma once

#include <chrono>
#include <iostream>
#include <ostream>

class FPSCounter {
public:
  FPSCounter(
      std::string label = "", std::ostream &out = std::cout,
      std::chrono::duration<float> window = std::chrono::duration<float>(1))
      : label_(label.empty() ? "" : label + " "), out_(out), frame_count_(0),
        start_time_(std::chrono::high_resolution_clock::now()),
        window_(window) {}

  void frame() {
    frame_count_++;
    auto now = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float> elapsed = now - start_time_;
    if (elapsed.count() >= window_.count()) {
      int fps = static_cast<int>(
          std::round(static_cast<float>(frame_count_) / elapsed.count()));
      out_ << label_ << "FPS: " << fps << std::endl;
      frame_count_ = 0;
      start_time_ = now;
    }
  }

private:
  std::string label_;
  std::ostream &out_;
  int frame_count_;
  std::chrono::high_resolution_clock::time_point start_time_;
  std::chrono::duration<float> window_;
};
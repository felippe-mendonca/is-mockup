#ifndef __IS_DRIVER_PIONEER_MOCK_HPP__
#define __IS_DRIVER_PIONEER_MOCK_HPP__

#include <is/logger.hpp>
#include <is/msgs/common.hpp>
#include <is/msgs/geometry.hpp>
#include <is/msgs/robot.hpp>

#include <cmath>
#include <chrono>
#include <memory>
#include <string>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace is {
namespace driver {

using namespace std::chrono;
using namespace is::msg::robot;
using namespace is::msg::common;
using namespace is::msg::geometry;

struct PioneerMock {
  PioneerMock() {
    this->now = high_resolution_clock::now();
  }
  PioneerMock(std::string const&) { 
    this->now = high_resolution_clock::now();
  }
  PioneerMock(std::string const&, int) { 
    this->now = high_resolution_clock::now();
  }
  high_resolution_clock::time_point now; 

  virtual ~PioneerMock() { }
  void connect(std::string const&) { }
  void connect(std::string const&, int) { }
  void set_speed(Speed const& ) { }
  Speed get_speed() { return Speed(); }
  Pose get_pose() { return Pose();  }
  void set_pose(Pose const&) { }
  void set_sample_rate(SamplingRate const&) { }
  SamplingRate get_sample_rate() { return SamplingRate(); }
  void set_delay(Delay const&) { }
  Timestamp get_last_timestamp() { return Timestamp(); }
  void wait() {
    now += milliseconds(100);
    std::this_thread::sleep_until(now);
   }
};

}  // ::driver
}  // ::is

#endif  // __IS_DRIVER_PIONEER_MOCK_HPP__

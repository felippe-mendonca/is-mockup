#ifndef __IS_DRIVER_PTGREY_HPP__
#define __IS_DRIVER_PTGREY_HPP__

#include "ip.hpp"
#include "utils.hpp"
#include <condition_variable>
#include <is/logger.hpp>
#include <is/msgs/camera.hpp>
#include <is/msgs/common.hpp>
#include <mutex>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <string>
#include <thread>

#include <boost/thread/mutex.hpp>

namespace is {
namespace driver {

using namespace std::chrono_literals;
using namespace is::msg::camera;
using namespace is::msg::common;

struct PtGrey {
  Client client;
  Dish dish;
  std::string camera_ip;
  std::string udp_port;
  std::string gw_ip;

  std::mutex mutex;
  bool received;
  std::condition_variable cv;

  Timestamp last_timestamp;
  cv::Mat last_frame;

  enum State { STOPPED, CAPTURING };
  State state;

  std::thread recv_thread;

  PtGrey(std::string const &camera_ip, std::string const &udp_port,
         std::string const &network_ip, std::string const &entity, ip::src const& type)
      : client(camera_ip, "3956"), received(false) {
    this->camera_ip = camera_ip;
    this->udp_port = udp_port;
    this->gw_ip = type == ip::src::NET ? ip::filter::network(network_ip) : network_ip;
    this->last_frame = cv::imread("images/" + entity + ".jpeg");
    is::log::info("Requesting connection configuration");
    this->client.request("req_connection;" + this->gw_ip + ":" +
                         this->udp_port);
    is::log::info("Sending start capture command");

    this->recv_thread = std::thread([this]() {
      {
        std::unique_lock<std::mutex> lk(this->mutex);
        this->dish.bind(this->udp_port);
        this->received = true;
        lk.unlock();
        this->cv.notify_one();
      }

      while (1) {
        try {
          this->dish.recv_frame_for(-1); // timeout = 200ms
          {
            std::unique_lock<std::mutex> lk(this->mutex);
            // this->last_timestamp = Timestamp();
            this->received = true;
          }
          this->cv.notify_one();
        } catch (std::exception const &e) {
          is::log::warn("{}", e.what());
        }
      }
    });

    {
      std::unique_lock<std::mutex> lk(this->mutex);
      this->cv.wait(lk, [this]() { return this->received; });
      this->received = false;
      this->client.request("start;");
      is::log::info("Starting");
      state = CAPTURING;
    }
  }

  ~PtGrey() { this->dish.unbind(); }

  void start_capture() {
    if (state == STOPPED) {
      is::log::info("Starting capture");
      this->client.request("start;");
      // this->dish.bind(this->udp_port);
      is::log::info("Started");
      state = CAPTURING;
    }
  }

  void stop_capture() {
    if (state == CAPTURING) {
      is::log::info("Stopping capture");
      // this->dish.unbind();
      this->client.request("stop;");
      is::log::info("Stopped");
      state = STOPPED;
    }
  }

  void set_sample_rate(SamplingRate) {}

  void set_resolution(Resolution) {}

  void set_image_type(ImageType) {}

  void set_delay(Delay) {}

  void update() {
    if (state == CAPTURING) {
      std::unique_lock<std::mutex> lock(mutex);
      if (this->cv.wait_for(lock, 500ms, [this]() { return this->received; })) {
        this->received = false;
        this->client.request("ack;");
      } else {
        is::log::warn("Frame timeouted, restarting");
        is::log::info("Requesting connection configuration");
        this->client.request("req_connection;" + this->gw_ip + ":" +
                             this->udp_port);
        is::log::info("Sending start capture command");
        this->client.request("start;");
      }
    }
  }

  cv::Mat get_last_frame() { return last_frame; }

  Timestamp get_last_timestamp() { return last_timestamp; }
};

} // ::driver
} // ::is

#endif // __IS_DRIVER_PTGREY_HPP__
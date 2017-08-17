#include "msgs/controller.hpp"
#include "msgs/frame_converter.hpp"
#include <boost/program_options.hpp>
#include <iostream>
#include <is/is.hpp>
#include <is/msgs/camera.hpp>
#include <is/msgs/common.hpp>
#include <is/msgs/geometry.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

using namespace std::chrono_literals;
using namespace std::chrono;
using namespace is::msg;
using namespace is::msg::camera;
using namespace is::msg::robot;
using namespace is::msg::common;
using namespace is::msg::geometry;
using namespace is::msg::controller;
namespace po = boost::program_options;

int main(int argc, char *argv[]) {
  std::string uri;
  std::string robot;
  std::vector<std::string> cameras;
  is::msg::camera::Resolution resolution;
  double fps;
  std::string img_type;
  unsigned int tr;
  unsigned int ts;

  po::options_description description("Allowed options");
  auto &&options = description.add_options();
  options("help,", "show available options");
  options("uri,u",
          po::value<std::string>(&uri)->default_value("amqp://localhost"),
          "broker uri");
  options("cameras,c",
          po::value<std::vector<std::string>>(&cameras)->multitoken(),
          "cameras");
  options("robot,r", po::value<std::string>(&robot), "robot");
  options("height,h",
          po::value<unsigned int>(&resolution.height)->default_value(728),
          "image height");
  options("width,w",
          po::value<unsigned int>(&resolution.width)->default_value(1288),
          "image width");
  options("fps,f", po::value<double>(&fps)->default_value(5.0),
          "frames per second");
  options("type,t", po::value<std::string>(&img_type)->default_value("gray"),
          "image type");
  options("tr", po::value<unsigned int>(&tr)->default_value(10), "run time");
  options("ts", po::value<unsigned int>(&ts)->default_value(10), "stop time");

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, description), vm);
  po::notify(vm);

  if (vm.count("help") || !vm.count("cameras") || !vm.count("robot")) {
    std::cout << description << std::endl;
    return 1;
  }

  VisualServoingConfigure configure;
  configure.cameras = cameras;
  configure.robot = robot;
  if (vm.count("height") && vm.count("width"))
    configure.resolution = resolution;
  if (vm.count("fps")) {
    SamplingRate sample_rate;
    sample_rate.rate = fps;
    configure.sample_rate = sample_rate;
  }
  if (vm.count("type"))
    configure.image_type = ImageType{img_type};

  auto is = is::connect(uri);

  auto send_commands = std::thread([configure,uri,tr,ts]() {
    auto is = is::connect(uri);
    auto client = is::make_client(is);

    while (1) {
      auto id = client.request("visual_servoing.configure", is::msgpack(configure));
      auto msg = client.receive_for(100ms, id, is::policy::discard_others);
      if (msg != nullptr) {
        is::log::info("Configuration sent");
        break;
      }
    }
    
    bool type = false;
    VisualServoingRequest request;
    request.reference = "ptgrey.0";
    while (1) {
      if (type) {
        request.point.x = 590;
        request.point.y = 490;
        type = !type;
        is::log::info("Sending command to stop robot");
      } else {
        request.point.x = 0;
        request.point.y = 0;
        type = !type;
        is::log::info("Sending command to run robot");
      }
      auto req_id =
          client.request("visual_servoing.go_to", is::msgpack(request));
      client.receive_for(10ms, req_id, is::policy::discard_others);

      std::this_thread::sleep_for(std::chrono::seconds(type ? tr : ts));
    }
  });

  std::vector<is::QueueInfo> frames_tags;
  for (auto &camera : cameras) {
    frames_tags.push_back(is.subscribe(camera + ".frame"));
  }

  int n_cameras = cameras.size();
  std::vector<AmqpClient::Envelope::ptr_t> images_message(n_cameras);

  while (1) {
    for (int i = 0; i < n_cameras; ++i) {
      images_message[i] = is.consume(frames_tags[i]);
    }
  }

  send_commands.join();
  return 0;
}

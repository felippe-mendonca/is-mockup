#include <iostream>
#include <boost/program_options.hpp>
#include "../../drivers/ptgrey/ptgrey.hpp"
#include "../camera/camera.hpp"

namespace po = boost::program_options;

int main(int argc, char* argv[]) {
  std::string uri;
  std::string entity;
  std::string camera_ip;
  std::string udp_port;
  std::string network_ip;
  std::string gw_ip;
  std::string format;

  po::options_description description("Allowed options");
  auto&& options = description.add_options();
  options("help,", "show available options");
  options("uri,u", po::value<std::string>(&uri)->default_value("amqp://localhost"), "broker uri");
  options("entity,e", po::value<std::string>(&entity), "entity name");
  options("ip,i", po::value<std::string>(&camera_ip), "camera ip address");
  options("port,p", po::value<std::string>(&udp_port), "camera udp port");
  options("net,n", po::value<std::string>(&network_ip), "network ip address");
  options("ip_gw,g", po::value<std::string>(&gw_ip), "gateway ip address");
  options("format,f", po::value<std::string>(&format)->default_value("jpeg"), "image format [png/jpeg]");

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, description), vm);
  po::notify(vm);

  if (vm.count("help") || !vm.count("entity") || !vm.count("ip") || !vm.count("port") || !(vm.count("net") || vm.count("ip_gw"))) {
    std::cout << description << std::endl;
    return 1;
  }


  if (vm.count("net")) {
    is::driver::PtGrey camera(camera_ip, udp_port, network_ip, entity, ip::src::NET);
    is::gw::Camera<is::driver::PtGrey> gw(entity, uri, format, camera);
  } else {
    is::driver::PtGrey camera(camera_ip, udp_port, gw_ip, entity, ip::src::GW);
    is::gw::Camera<is::driver::PtGrey> gw(entity, uri, format, camera);
  }

  return 0;
}
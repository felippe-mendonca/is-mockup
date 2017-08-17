#include "ip.hpp"
#include "utils.hpp"
#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <is/logger.hpp>

int main(int argc, char **argv) {
  if (argc != 3) {
    std::cout << "\t>> Usage: ./ptgrey <gw-ip> <gw-udp-port>\n";
    exit(1);
  }

  std::string gw_ip(argv[1]);
  std::string udp_port(argv[2]);

  Client client(gw_ip, "3956"); // gvcp tcp port

  is::log::info("Requesting streaming");
  auto eno1_ip = ip::filter::interface("eno1");
  client.send("req_connection;" + eno1_ip + ":" + udp_port);
  is::log::info("Wating reply");
  client.recv();

  is::log::info("Sleep 2 seconds, then send start command");
  std::this_thread::sleep_for(std::chrono::seconds(2));
  client.send("start;");
  client.recv();

  Dish dish;
  dish.bind(udp_port);
  std::this_thread::sleep_for(std::chrono::seconds(10));
  
  int niter = 0;
  try {
    while (1) {
      dish.recv_frame_for(200); // timeout = 200ms
      is::log::info("Frame received");
      client.request("ack;");
      if (++niter == 20) {
        dish.unbind();
        is::log::info("Reply stop command: {}", client.request("stop;"));
        std::this_thread::sleep_for(std::chrono::milliseconds(2500));
      }
    }
  } catch (std::exception const &e) {
    is::log::warn("{}", e.what()); 
  }

  return 0;
}
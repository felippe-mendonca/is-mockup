#ifndef __UTILS_HPP__
#define __UTILS_HPP__

#include <cassert>
#include <chrono>
#include <cstring>
#include <iostream>
#include <iostream>
#include <string>
#include <string>
#include <thread>
#include <time.h>
#include <vector>
#include <zmq.h>
#include "logger.hpp"

using namespace std;
using namespace std::chrono;

struct Radio {
  void *context;
  void *radio;
  string uri;
  zmq_msg_t msg;
  bool connected;

  Radio() : connected(false) {}

  Radio(std::string const &hostname) : connected(false) {
    this->connect(hostname);
  }

  ~Radio() {
    this->disconnect();
  }

  void connect(std::string const &hostname) {
    this->uri = "udp://" + hostname;
    if (this->connected)
      return;
    this->context = zmq_ctx_new();
    this->radio = zmq_socket(this->context, ZMQ_RADIO);
    is::log::info("Connecting to {}", this->uri);
    int status = zmq_connect(this->radio, this->uri.c_str());
    assert(status == 0);
    this->connected = true;
  }

  void disconnect() {
    if (!this->connected)
      return;
    zmq_disconnect(this->radio, this->uri.c_str());
    zmq_close(this->radio);
    zmq_ctx_destroy(this->context);
    this->connected = false;
  }

  void send_packet(size_t const &length, char const &fill) {
    std::vector<char> data(length - 6, fill); // discount "topic"
    std::string body(data.begin(), data.end());
    zmq_msg_init_size(&this->msg, body.length());
    memcpy(zmq_msg_data(&this->msg), body.c_str(), body.length());
    zmq_msg_set_group(&this->msg, "topic");
    zmq_msg_send(&this->msg, this->radio, 0);
    zmq_msg_close(&this->msg);
  }

  void send_frame(size_t const &packet_length, int const &n_packets) {
    auto now_p = high_resolution_clock::now();
    for (int i = 0; i < n_packets; ++i) {
      this->send_packet(packet_length, i == (n_packets - 1) ? 'y' : 'n');

      now_p = now_p + microseconds(60);
      auto diff =
          duration_cast<nanoseconds>(now_p - high_resolution_clock::now())
              .count();
      struct timespec timeout = {.tv_sec = 0, .tv_nsec = diff - 10000};
      nanosleep(&timeout, nullptr);
      while (high_resolution_clock::now() < now_p) {
      }
    } // packets loop
  }
};

struct Dish {
  void *context;
  void *dish;
  string uri;
  zmq_msg_t msg;

  Dish() {}

  Dish(string const& port) {
    this->bind(port);
  }

  ~Dish() {
    this->unbind();
  }

  void bind(string const& port) {
    this->context = zmq_ctx_new();
    this->dish = zmq_socket(this->context, ZMQ_DISH);
    this->uri = "udp://*:" + port;
    is::log::info("Binding to {}", this->uri);
    int status = zmq_bind(this->dish, this->uri.c_str());
    assert(status == 0);
    zmq_join(this->dish, "topic");
  }

  void unbind() {
    zmq_unbind(this->dish, this->uri.c_str());
    zmq_close(this->dish);
    zmq_ctx_destroy(this->context);
  }

  bool recv_packet() {
    zmq_msg_init(&this->msg);
    zmq_msg_recv(&this->msg, this->dish, 0);
    char* body = static_cast<char*>(zmq_msg_data(&this->msg));
    bool is_last = (body[0] == 'y') ? true : false;
    zmq_msg_close(&this->msg);
    return is_last;
  }

  void recv_frame() {
    while(!this->recv_packet());
  }

  bool recv_packet_for(unsigned int const& milliseconds) {
    zmq_setsockopt(this->dish, ZMQ_RCVTIMEO, &milliseconds, sizeof(unsigned int)); // set recv timeout
    zmq_msg_init(&this->msg);
    auto rc = zmq_msg_recv(&this->msg, this->dish, 0);
    if (rc == -1) {
      unsigned int default_timeout = -1;
      zmq_setsockopt(this->dish, ZMQ_RCVTIMEO, &default_timeout, sizeof(unsigned int)); // restore socket option
      zmq_msg_close(&this->msg);
      throw std::runtime_error("Time to receive exceeded");
    }
    char* body = static_cast<char*>(zmq_msg_data(&this->msg));
    bool is_last = (body[0] == 'y') ? true : false;
    zmq_msg_close(&this->msg);
    unsigned int default_timeout = -1;
    zmq_setsockopt(this->dish, ZMQ_RCVTIMEO, &default_timeout, sizeof(unsigned int)); // restore socket option
    return is_last;
  }

  void recv_frame_for(unsigned int const& milliseconds) {
    while(!this->recv_packet_for(milliseconds)) {
    }
  }
};

struct Server {
  void *context;
  void *server;
  zmq_msg_t msg;
	
  Server(string const& tcp_port) {
    this->context = zmq_ctx_new();
    this->server = zmq_socket(this->context, ZMQ_REP);
    string uri = "tcp://*:" + tcp_port;
    int status = zmq_bind(this->server, uri.c_str());
    assert(status == 0);	
  }

	~Server() {
    zmq_close(this->server);
    zmq_ctx_destroy(this->context);
  }

  string recv() {
    zmq_msg_init(&this->msg);
    zmq_msg_recv(&this->msg, this->server, 0);
		auto begin = static_cast<char *>(zmq_msg_data(&this->msg));
		auto end = begin + zmq_msg_size(&this->msg);
    string body = string(begin, end);
    zmq_msg_close(&this->msg);
    return body;
  }

  string recv_for(unsigned int const& milliseconds) {
    zmq_setsockopt(this->server, ZMQ_RCVTIMEO, &milliseconds, sizeof(unsigned int)); // set recv timeout
    zmq_msg_init(&this->msg);
    string body;
    auto rc = zmq_msg_recv(&this->msg, this->server, 0);
    if (rc == -1) {
      unsigned int default_timeout = -1;
      zmq_setsockopt(this->server, ZMQ_RCVTIMEO, &default_timeout, sizeof(unsigned int)); // restore socket option
      zmq_msg_close(&this->msg);
      throw std::runtime_error("Time to receive exceeded");
    }
    auto begin = static_cast<char *>(zmq_msg_data(&this->msg));
    auto end = begin + zmq_msg_size(&this->msg);
    body = string(begin, end);
    zmq_msg_close(&this->msg);
    unsigned int default_timeout = -1;
    zmq_setsockopt(this->server, ZMQ_RCVTIMEO, &default_timeout, sizeof(unsigned int)); // restore socket option
    return body;
  }

  void send(std::string const &body) {
    zmq_msg_init_size(&this->msg, body.length());
    memcpy(zmq_msg_data(&this->msg), body.c_str(), body.length());
    zmq_msg_send(&this->msg, this->server, 0);
    zmq_msg_close(&this->msg);
  }
};

struct Client {
  void *context;
  void *client;
  zmq_msg_t msg;
	
  Client(string const& ip, string const& tcp_port) {
    this->context = zmq_ctx_new();
    this->client = zmq_socket(this->context, ZMQ_REQ);
    string uri = "tcp://" + ip + ":" + tcp_port;
    int status = zmq_connect(this->client, uri.c_str());
    assert(status == 0);	
  }

	~Client() {
    zmq_close(this->client);
    zmq_ctx_destroy(this->context);
  }

  string recv() {
    zmq_msg_init(&this->msg);
    zmq_msg_recv(&this->msg, this->client, 0);
		auto begin = static_cast<char *>(zmq_msg_data(&this->msg));
		auto end = begin + zmq_msg_size(&this->msg);
    string body = string(begin, end);
    zmq_msg_close(&this->msg);
    return body;
  }

  string recv_timeout(unsigned int const& milliseconds) {
    zmq_setsockopt(this->client, ZMQ_RCVTIMEO, &milliseconds, sizeof(unsigned int)); // set recv timeout
    zmq_msg_init(&this->msg);
    string body;
    auto rc = zmq_msg_recv(&this->msg, this->client, 0);
    if (rc) {
		  auto begin = static_cast<char *>(zmq_msg_data(&this->msg));
		  auto end = begin + zmq_msg_size(&this->msg);
      body = string(begin, end);
      zmq_msg_close(&this->msg);
    }
    unsigned int default_timeout = -1;
    zmq_setsockopt(this->client, ZMQ_RCVTIMEO, &default_timeout, sizeof(unsigned int)); // restore socket option
    return body;
  }

  void send(std::string const &body) {
    zmq_msg_init_size(&this->msg, body.length());
    memcpy(zmq_msg_data(&this->msg), body.c_str(), body.length());
    zmq_msg_send(&this->msg, this->client, 0);
    zmq_msg_close(&this->msg);
  }

  string request(string const &body) {
    zmq_msg_init_size(&this->msg, body.length());
    memcpy(zmq_msg_data(&this->msg), body.c_str(), body.length());
    zmq_msg_send(&this->msg, this->client, 0);
    zmq_msg_close(&this->msg);

    unsigned int timeout = 100; // ms
    zmq_setsockopt(this->client, ZMQ_RCVTIMEO, &timeout, sizeof(unsigned int)); // set recv timeout
    while (1) {
      zmq_msg_init(&this->msg);
      auto rc = zmq_msg_recv(&this->msg, this->client, 0);
      if (rc == -1) {
        zmq_msg_close(&this->msg);
        continue;
      }
      auto begin = static_cast<char *>(zmq_msg_data(&this->msg));
		  auto end = begin + zmq_msg_size(&this->msg);
      auto reply = string(begin, end);
      timeout = -1; // default timeout
      zmq_setsockopt(this->client, ZMQ_RCVTIMEO, &timeout, sizeof(unsigned int)); // set recv timeout
      zmq_msg_close(&this->msg);
      return reply;
    }
  }
};

#endif // __UTILS_HPP__
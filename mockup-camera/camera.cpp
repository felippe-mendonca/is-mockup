#include "logger.hpp"
#include "utils.hpp"
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <time.h>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

using namespace std;
using namespace std::chrono;

enum class State { IDLE, IDLE_CONFIGURED, PUBLISHING };
enum class TCP_Cmd { REQ_CONNECTION, START, STOP, ACK };
const map<string, TCP_Cmd> tcp_command = {
    {"req_connection", TCP_Cmd::REQ_CONNECTION},
    {"start", TCP_Cmd::START},
    {"stop", TCP_Cmd::STOP},
    {"ack", TCP_Cmd::ACK}};

std::string uri;

State parse_tcp_cmd(Server &server, Radio &radio, State const& state,
                    unsigned int const &timeout = -1) {
  string msg;
  try {
    msg = server.recv_for(timeout);
  } catch (...) {
    is::log::warn("Timeout");
    radio.disconnect();
    return State::IDLE;
  }
  vector<string> msg_fields;
  boost::split(msg_fields, msg, boost::is_any_of(";"),
               boost::token_compress_on);

  if (msg_fields.size() != 2) {
    is::log::warn("Invalid request size");
    return State::IDLE;
  }

  auto op_str = msg_fields[0];
  auto body = msg_fields[1];
  auto cmd = tcp_command.find(op_str);

  if (cmd == tcp_command.end()) {
    is::log::warn("Invalid request");
    return State::IDLE;
  }

  server.send("ok");
  switch (cmd->second) {

  case TCP_Cmd::REQ_CONNECTION: {
    uri = body;
    if (state != State::IDLE)
      radio.disconnect();
    return State::IDLE_CONFIGURED;
  }

  case TCP_Cmd::STOP:
    is::log::info("Stopping");
    return state == State::PUBLISHING ? State::IDLE_CONFIGURED : State::IDLE;

  case TCP_Cmd::ACK:
    return state == State::PUBLISHING
               ? State::PUBLISHING
               : (state == State::IDLE ? State::IDLE : State::IDLE_CONFIGURED);

  case TCP_Cmd::START:
    if (state == State::IDLE_CONFIGURED) {
      radio.connect(uri);
      is::log::info("Publishing");
      return State::PUBLISHING;
    } else if(state == State::PUBLISHING) {
      return State::PUBLISHING;
    }
    return State::IDLE;
  }
}

int main(int argc, char *argv[]) {

  Server server("3956"); // gvcp tcp port
  Radio radio;
  is::log::info("Running");

  auto now = high_resolution_clock::now();
  State state = State::IDLE;
  while (1) {
    switch (state) {
    case State::IDLE: {
      state = parse_tcp_cmd(server, radio, state);
      break;
    }
    case State::IDLE_CONFIGURED: {
      state = parse_tcp_cmd(server, radio, state);
      if (state == State::PUBLISHING)
        now = high_resolution_clock::now();
      break;
    }
    case State::PUBLISHING: {
      now = now + milliseconds(200);
      radio.send_frame(1372, 690);

      auto diff = duration_cast<nanoseconds>(now - high_resolution_clock::now())
                      .count();
      struct timespec timeout = {.tv_sec = 0, .tv_nsec = diff - 1000000};
      nanosleep(&timeout, nullptr);

      state = parse_tcp_cmd(server, radio, state, 0);
      while (high_resolution_clock::now() < now) {
      }
      break;
    }
    }
  }

  return 0;
}
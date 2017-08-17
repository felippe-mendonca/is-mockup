#ifndef __IP_HPP__
#define __IP_HPP__

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <sys/socket.h>

#include <map>
#include <string>
#include <bitset>
#include <iostream>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

namespace ip {

enum class src {NET, GW};

using namespace std;

map<string, string> list_all() {
  struct ifaddrs *ifap, *ifa;
  struct sockaddr_in *sa;
  char *addr;

  getifaddrs(&ifap);
  map<string, string> ips;
  for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
    if (ifa->ifa_addr->sa_family == AF_INET) {
      sa = (struct sockaddr_in *)ifa->ifa_addr;
      addr = inet_ntoa(sa->sin_addr);
      ips.insert(make_pair(ifa->ifa_name, addr));
    }
  }
  freeifaddrs(ifap);
  return ips;
}

namespace filter {

string interface(string const &interface) {
  auto ips = list_all();
  string ip;
  try {
    ip = ips.at(interface);
  } catch (std::out_of_range &e) {
    cerr << "Error: Interface not found.";
  }
  return ip;
}

string network(string const &network) {
  auto ips = list_all();
  string ip;
  vector<string> net_fields;
  boost::split(net_fields, network, boost::is_any_of("."),
               boost::token_compress_on);
  if (net_fields.size() != 4) {
		cerr << "Error: invalid network address.";
		return ip;
	}

	for (auto _ip : ips) {
    vector<string> _ip_fields;
    boost::split(_ip_fields, _ip.second, boost::is_any_of("."), boost::token_compress_on);
		if (_ip_fields.size() == 4) {
			int eq_f = 0;
			for (int i = 0; i < 4; ++i) {
				bitset<8> f_net(static_cast<uint8_t>(stoi(net_fields[i])));
				bitset<8> f_ip(static_cast<uint8_t>(stoi(_ip_fields[i])));
				if ( (f_net & f_ip) == f_net) {
					eq_f++;	
				}
			}
			if (eq_f == 4) {
				ip = _ip.second;
				break;
			}
		}
  }
  return ip;
}

} // ::ip::filter
} // ::ip

#endif // __IP_HPP__
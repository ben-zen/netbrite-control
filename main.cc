// Copyright Ben Lewis 2022

#include <climits>
#include <errno.h>
#include <experimental/net>
#include <iostream>
#include <string>

#include "nb_connect.h"

namespace net = std::experimental::net;

int main(int argc, char **argv)
{
  std::string address;
  uint16_t port;
  std::string message;
  // crack parameters
  if (argc != 4)
  {
    std::cout << "Wrong argument count: " << argc << " Expected 4." << std::endl;
    return EINVAL;
  }

  address = argv[1];
  {
    ulong temp_port = std::strtoul(argv[2], nullptr, 10);
    if (temp_port == 0 || temp_port == ULONG_MAX || temp_port > UINT16_MAX)
    {
      std::cout << "Port value is incorrect or cannot be parsed: " << argv[2] << std::endl;
      return ERANGE;
    }
    port = static_cast<uint16_t>(temp_port);
  }

  message = argv[3];

  // connect to sign
  std::cout << "Connecting to sign at " << address << ":" << port << ", and setting message: " << message << std::endl;

  nbx::net_brite sign { address, port };
  // send updated configuration
  sign.set_message(message);
  // report errors?
  return 0;
}
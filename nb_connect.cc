// Copyright Ben Lewis 2022

#include "nb_connect.h"
#include <errno.h>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <sstream>
#include <system_error>
#include <sys/socket.h>
#include <unistd.h>

using namespace nbx;

net_brite::net_brite(std::string const &address, uint16_t port) :
  m_address(address),
  m_port(port)
{
  addrinfo *info = nullptr;
  int error_code = 0;
  error_code = getaddrinfo(m_address.c_str(), std::to_string(m_port).c_str(), nullptr, &info);
  if (error_code != 0)
  {
    throw std::system_error
          { 
            std::error_code{error_code, std::system_category()},
            "getaddrinfo() failed"
          };
  }

  if (info->ai_family != AF_INET || info->ai_family != AF_INET6)
  {
    throw std::domain_error { "wrong family for address" };
  }

  std::cout << "Opening socket in domain " << info->ai_family << std::endl; 
  m_socket_fd = socket(info->ai_family, SOCK_DGRAM, IPPROTO_UDP);
  if (m_socket_fd == -1)
  {
    throw std::system_error
    {
      std::error_code { errno, std::system_category() },
      "socket() failed to create a socket"
    };
  }

  if (connect(m_socket_fd, info->ai_addr, info->ai_addrlen) != 0)
  {
    throw std::system_error
    {
      std::error_code { errno, std::system_category() },
      "connect() failed to open a connection to the required address."
    };
  }
}

net_brite::~net_brite()
{
  if (m_socket_fd != 0)
  {
    // I don't care if this fails, honestly.
    close(m_socket_fd);
  }
}

int net_brite::set_message(std::string_view message)
{
  // Build and send the message to set a rectangle the size of the whole display to the message
  int err = 0;
  std::stringstream message_buffer {};
  message_buffer << message;
  auto built_message = message_buffer.str();
  auto sent_bytes = send(m_socket_fd, built_message.c_str(), built_message.size() + 1, 0); // + 1 on size for the null terminator.
  if (sent_bytes == -1)
  {
    err = errno;
  }
  return err;
}
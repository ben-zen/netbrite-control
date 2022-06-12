// Copyright Ben Lewis 2022

#pragma once

#include <span>
#include <string>
#include <string_view>

#include "formatting.hh"

namespace nbx
{
  class net_brite
  {
  private:
    std::string m_address;
    uint16_t m_port;

    int m_socket_fd;

    u_short sequence_number{};
    u_char session_number{};
    
  public:
    net_brite(std::string const &address, uint16_t port);
    ~net_brite();
    
    int set_message(std::string_view const &message, text_color color = text_color::yellow, text_font font = text_font::proportional_11);

  private:
    int send_sign_message(std::span<uint8_t> const &message);
  };
};
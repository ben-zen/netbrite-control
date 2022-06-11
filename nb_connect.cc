// Copyright Ben Lewis 2022

#include "nb_connect.h"
#include <errno.h>
#include <limits>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <span>
#include <sstream>
#include <string_view>
#include <system_error>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

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

  if (info->ai_family != AF_INET && info->ai_family != AF_INET6)
  {
    std::stringstream message { "wrong family for address: " };
    message.flush();
    message << info->ai_family;
    throw std::domain_error { message.str() };
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

u_short crc(std::span<u_char> const &data)
{
  // crc($input,$width,  $init, $xorout,$refout, $poly,$refin,$cont)
  // crc( shift,    16, 0x0000,  0x0000,      1, 0x1021,     1,   0)
  return 0;
}

int net_brite::set_message(std::string_view const &message)
{
  // The magic numbers and message pack structure are C/O https://github.com/ben-zen/Net-Symon-Netbrite/blob/master/lib/Net/Symon/NetBrite.pm
  // Build and send the message to set a rectangle the size of the whole display to the message
  int err = 0;

  #pragma pack(push, 1)
  struct message_body
  {
    struct // header
    {
      u_char message_header[4] {0x0f, 0x00, 0x0e, 0x02};
      u_char zone_def_id;
      struct rect
      {
        u_char ul_x;
        u_char ul_y;
        u_char horizontal_extent;
        u_char vertical_extent;
      } rect;
      u_char scroll_rate_header { 0x0d };
      u_char scroll_rate;
      u_char scroll_rate_footer { 0x0 };
      u_char pause_header { 0x0c };
      u_char message_def_parameters[8] { 0x0b, 0xfe, 0x0a, 0xe8, 0x03, 0x09, 0x0e, 0x08 };
      u_char volume;
      u_char font_header { 0x07 };
      u_char font;
      u_char color_header { 0x06 };
      u_char color;
      u_char unknown_option_five[3] { 0x05, 0x00, 0x00, };
      u_char date_header {0x04};
      struct
      {
        u_short year {2012};
        u_char month {2};
        u_char day {10};
        u_char hour {19};
        u_char minute {21};
        u_char second {33};
      } date;
      u_char date_footer {0x0};
      u_char content_header[8] {0x03, 0x00, 0x00, 0x2f, 0x02, 0xff, 0x10, 0x3a};
      u_char zone_id_header { 0x1 };
      u_char zone_id;
      u_char zone_id_footer { 0x0 };
      u_char unknown_options[20] {
        0x02, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x03, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0xfe,
        0x7e, 0x00, 0x02, 0x00
      };
      u_char message_length;
      u_char unknown_message_prefix[14] {
        0x00, 0x00, 0x04,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0xfe, 0x7e, 0x00
      };
    } header;
    std::string message_text;
    u_char body_end {0x17};

    std::vector<u_char> packed_message_body {};

    message_body(u_char z_id, std::string_view const &text)
    {
      header.zone_def_id = z_id;
      // set the rectangle
      header.zone_id = z_id;
      if (text.length() > std::numeric_limits<u_char>::max())
      {
        throw std::domain_error { "Message too long; maximum length 255 characters." };
      }
      header.message_length = static_cast<u_char>(text.length());
      message_text = text;

      std::span<u_char> header_bytes(reinterpret_cast<u_char *>(&header), sizeof(header));

      packed_message_body.insert(packed_message_body.end(), header_bytes.begin(), header_bytes.end());
    };
  } s_body { 1, message }; // struct message_body;

  struct message_header
  {
    u_char header_prefix[3] { 0x16, 0x16, 0x01 };
    u_short body_length;
    u_short sequence_number;
    u_char filler[3] {};
    u_char type[2] { 0x01, 0x01 }; // type = init (??)
    u_char sign_id_fields[4] { 0x00, 0xc8, 0x01, 0x00 };
    u_char filler_char { 0x0 };
    u_char session_packet;
    u_char header_suffix[2] {0x04, 0x00};
  } s_header;
  std::span<u_char> header_span { (u_char *)&s_header, sizeof(s_header)};

  struct message_footer
  {
    u_short checksum;
    u_char message_end {0x04};
  } s_footer;
  std::span<u_char> footer_span { (u_char *)&s_footer, sizeof(s_footer)};
  #pragma pack(pop)

  // First fill the message body; the header can only be determined
  // once the body is fully built out.

  std::vector<u_char> built_message{};
  built_message.insert(built_message.end(), header_span.begin(), header_span.end());
  built_message.insert(built_message.end(), s_body.packed_message_body.begin(), s_body.packed_message_body.end());

  s_footer.checksum = crc(built_message);
  built_message.

  // The footer is then built and appended to the message.

  auto sent_bytes = send(m_socket_fd, built_message.data(), built_message.size(), 0); // + 1 on size for the null terminator.
  if (sent_bytes == -1)
  {
    err = errno;
  }
  return err;
}
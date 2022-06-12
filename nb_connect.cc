// Copyright Ben Lewis 2022

#include "nb_connect.h"
#include "formatting.hh"
#include <errno.h>
#include <limits>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <sstream>
#include <string_view>
#include <system_error>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

#include <extern/crc/include/crc.h>
using crc16_ccitt = crc::crc_base<uint16_t, 16, 0x1021, 0x0, 0x0, false, false>; // See readme for crc.h!

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
  m_socket_fd = socket(info->ai_family, SOCK_STREAM, IPPROTO_TCP);
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

int net_brite::send_sign_message(std::span<uint8_t> const &message)
{
  int err = 0;
  std::vector<uint8_t> escaped_message {};
  if (message.size() <= 8)
  {
    return ERANGE;
  }

  auto &end_of_header = message.begin() += 4;
  auto &start_of_footer = message.end() -= 4;

  escaped_message.insert(escaped_message.end(), message.begin(), end_of_header);
  // The message body needs to have specific bytes escaped before sending; it will be reconstituted on the other side?
  std::span<uint8_t> escaping_span { end_of_header, start_of_footer };
  for (auto &&byte : escaping_span)
  {
    if (byte == 0x01 || byte == 0x04 || byte == 0x10 || byte == 0x17)
    {
      escaped_message.push_back(0x10);
    }

    escaped_message.push_back(byte);
  }
  
  escaped_message.insert(escaped_message.end(), start_of_footer, message.end());
  
  auto sent_bytes = send(m_socket_fd, escaped_message.data(), escaped_message.size(), 0);
  if (sent_bytes == -1)
  {
    // send() failed.
    err = errno;
  }

  return err;
}

int net_brite::set_message(std::string_view const &message, text_color color, text_font font)
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
      scroll_speed scroll_rate;
      u_char scroll_rate_footer { 0x0 };
      u_char pause_header { 0x0c };
      u_char pause {};
      u_char message_def_parameters[7] { 0x0b, 0xfe, 0x0a, 0xe8, 0x03, 0x09, 0x0e};
      u_char volume_header { 0x08 };
      u_char volume {};
      u_char font_header { 0x07 };
      text_font font {};
      u_char color_header { 0x06 };
      text_color color {};
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
      u_short message_length;
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

    message_body(u_char z_id, std::string_view const &text, text_color color, text_font font)
    {
      header.zone_def_id = z_id;
      header.rect = { .ul_x = 0, .ul_y = 0, .horizontal_extent = 200, .vertical_extent = 24 };
      header.zone_id = z_id;
      header.scroll_rate = scroll_speed::medium;
      header.volume = 4; // allowed values 0-8
      header.color = color;
      header.font = font;
      if (text.length() > std::numeric_limits<u_short>::max())
      {
        throw std::domain_error { "Message too long; maximum length is an unsigned short." };
      }
      header.message_length = static_cast<u_short>(text.length());
      message_text = text;

      std::span<u_char> header_bytes(reinterpret_cast<u_char *>(&header), sizeof(header));

      packed_message_body.insert(packed_message_body.end(), header_bytes.begin(), header_bytes.end());
      packed_message_body.insert(packed_message_body.end(), message_text.begin(), message_text.end());
      packed_message_body.insert(packed_message_body.end(), body_end);
    };
  } s_body { 1, message, color, font }; // struct message_body;

  struct message_header
  {
    u_char header_prefix[3] { 0x16, 0x16, 0x01 };
    u_short body_length;
    u_short sequence_number;
    u_char filler[3] { 0x00, 0x01, 0x00 };
    u_char type[2] { 0x01, 0x01 }; // type = init (??)
    u_char sign_id_fields[4] { 0x00, 0xc8, 0x01, 0x00 };
    u_char filler_char { 0x0 };
    u_char session_packet;
    u_char header_suffix[2] {0x04, 0x00};

    message_header(size_t body_len, u_short seq_num, u_char pkt_num)
    {
      if (body_len > std::numeric_limits<u_short>::max())
      {
        throw std::domain_error { "Message body too large, exceeded the size of an unsigned short." };
      }
      body_length = static_cast<u_short>(body_len);
      sequence_number = seq_num;
      session_packet = pkt_num;
    };
  } s_header { s_body.packed_message_body.size(), ++sequence_number, ++session_number};
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

  s_footer.checksum = crc16_ccitt::compute_checksum_static(built_message.begin(), built_message.end());
  built_message.insert(built_message.end(), footer_span.begin(), footer_span.end());

  // The footer is then built and appended to the message.

  err = send_sign_message(built_message);

  return err;
}
#include "nb_connect.h"
#include <experimental/net>

using namespace nbx;
namespace net = std::experimental::net;

net_brite::net_brite(net::io_context &context, std::string const &address, uint16_t port) :
  m_address(address),
  m_port(port),
  m_socket(context)
{
  net::ip::udp::resolver resolver(context);
  net::connect(m_socket, resolver.resolve(m_address, std::to_string(port)));
}

void net_brite::set_message(std::string_view)
{
  // Build and send the message to set a rectangle the size of the whole display to the message
}
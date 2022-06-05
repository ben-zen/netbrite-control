#include <string>
#include <string_view>
#include <experimental/net>

namespace nbx
{
  class net_brite
  {
  private:
    std::string m_address;
    uint16_t m_port;

    std::experimental::net::ip::udp::socket m_socket;
    
  public:
    net_brite(std::experimental::net::io_context &context, std::string const &address, uint16_t port);
    ~net_brite();
    
    void set_message(std::string_view message);
  };
};
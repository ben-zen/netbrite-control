// Copyright Ben Lewis 2022

#include <string>
#include <string_view>

namespace nbx
{
  class net_brite
  {
  private:
    std::string m_address;
    uint16_t m_port;

    int m_socket_fd;
    
  public:
    net_brite(std::string const &address, uint16_t port);
    ~net_brite();
    
    int set_message(std::string_view message);
  };
};
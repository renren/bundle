#ifndef BASE_BASE_NETWORK_H__
#define BASE_BASE_NETWORK_H__

#include <vector>
#include <string>

#include "base3/basictypes.h"

namespace base {

// Represents a Unix-type network interface, with a name and single address.
// It also includes the ability to track and estimate quality.
class Network {
public:
  Network(const std::string& name, uint32 ip, uint32 mask = 0)
    : name_(name), ip_(ip), mask_(mask) {}

  // Returns the OS name of this network.  This is considered the primary key
  // that identifies each network.
  const std::string& name() const { return name_; }

  // Identifies the current IP address used by this network.
  uint32 ip() const { return ip_; }
  void set_ip(uint32 ip) { ip_ = ip; }

  // ip mask
  uint32 mask() const { return mask_; }
  void set_mask(uint32 mask) { mask_ = mask; }

  // Debugging description of this network
  std::string ToString() const;

  // for this machine
  typedef std::vector<Network*> NetworkList;

  static const std::string& local_ip();

  // Creates a network object for each network available on the machine.
  static void CreateNetworks(std::vector<Network*>& networks);

  // 是否为内网 ip 地址
  static bool IsPrivateIP(uint32 ip) {
    return ((ip >> 24) == 127) ||
      ((ip >> 24) == 10) ||
      ((ip >> 20) == ((172 << 4) | 1)) ||
      ((ip >> 16) == ((192 << 8) | 168));
  }

  static uint32 StringToIP(const std::string& hostname);
  static std::string IPToString(uint32 ip);

private:
  std::string name_;
  uint32 ip_;
  uint32 mask_;
};


}

#endif

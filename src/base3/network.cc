#include "base3/network.h"

#ifdef OS_LINUX
// #include <sys/types.h>
// #include <sys/socket.h>
// #include <netinet/in.h>
#include <net/if.h>    // struct ifconf
#include <sys/ioctl.h> // SIOCGIFCONF
#include <arpa/inet.h>
// #include <netdb.h>
#endif

#ifdef OS_WIN
#include <windows.h>
#include <winsock2.h>
#include <iptypes.h>
#include <iphlpapi.h>

#pragma comment(lib, "ws2_32")
#pragma comment(lib, "iphlpapi")
#endif

#include <string.h>
#include <errno.h>
#include <sstream>


#include "base3/logging.h"
#include "base3/common.h"

namespace base {

const std::string& Network::local_ip() {
  static std::string ip_;
  if (ip_.empty()) {
    std::vector<Network*> networks;
    CreateNetworks(networks);

    for (std::vector<Network*>::const_iterator i=networks.begin();
      i!=networks.end(); ++i) {
        Network* pn = *i;
        if (IsPrivateIP(pn->ip_))
          ip_ = IPToString(pn->ip_);
        delete *i;
    }
  }

  return ip_;
}

#ifdef OS_LINUX
void Network::CreateNetworks(std::vector<Network*>& networks) {
  int fd;
  if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    LOG(ERROR) << "socket";
    return;
  }

  struct ifconf ifc;
  ifc.ifc_len = 64 * sizeof(struct ifreq);
  ifc.ifc_buf = new char[ifc.ifc_len];

  if (ioctl(fd, SIOCGIFCONF, &ifc) < 0) {
    LOG(ERROR) << "ioctl";
    return;
  }
  ASSERT(ifc.ifc_len < static_cast<int>(64 * sizeof(struct ifreq)));

  struct ifreq* ptr = reinterpret_cast<struct ifreq*>(ifc.ifc_buf);
  struct ifreq* end =
      reinterpret_cast<struct ifreq*>(ifc.ifc_buf + ifc.ifc_len);

  while (ptr < end) {
    if (strcmp(ptr->ifr_name, "lo")) { // Ignore the loopback device
      struct sockaddr_in* inaddr =
          reinterpret_cast<struct sockaddr_in*>(&ptr->ifr_ifru.ifru_addr);
      if (inaddr->sin_family == AF_INET) {
        uint32 ip = ntohl(inaddr->sin_addr.s_addr);
        networks.push_back(new Network(std::string(ptr->ifr_name), ip));
      }
    }
#ifdef _SIZEOF_ADDR_IFREQ
    ptr = reinterpret_cast<struct ifreq*>(
        reinterpret_cast<char*>(ptr) + _SIZEOF_ADDR_IFREQ(*ptr));
#else
    ptr++;
#endif
  }

  delete [] ifc.ifc_buf;
  close(fd);
}
#endif

#ifdef OS_WIN
void Network::CreateNetworks(std::vector<Network*>& networks) {
  IP_ADAPTER_INFO info_temp;
  ULONG len = 0;
  
  if (GetAdaptersInfo(&info_temp, &len) != ERROR_BUFFER_OVERFLOW)
    return;
  IP_ADAPTER_INFO *infos = new IP_ADAPTER_INFO[len];
  if (GetAdaptersInfo(infos, &len) != NO_ERROR)
    return;

  int count = 0;
  for (IP_ADAPTER_INFO *info = infos; info != NULL; info = info->Next) {
    if (info->Type == MIB_IF_TYPE_LOOPBACK)
      continue;
    if (strcmp(info->IpAddressList.IpAddress.String, "0.0.0.0") == 0)
      continue;

    // In production, don't transmit the network name because of
    // privacy concerns. Transmit a number instead.

    std::string name;
#if defined(PRODUCTION)
    std::ostringstream ost;
    ost << count;
    name = ost.str();
    count++;
#else
    name = info->Description;
#endif

    networks.push_back(new Network(name,
        Network::StringToIP(info->IpAddressList.IpAddress.String)));
  }

  delete infos;
}

int inet_aton(const char * cp, struct in_addr * inp) {
  inp->s_addr = inet_addr(cp);
  return (inp->s_addr == INADDR_NONE) ? 0 : 1;
}
#endif

uint32 Network::StringToIP(const std::string& hostname) {
  uint32 ip = 0;
  in_addr addr;
  if (inet_aton(hostname.c_str(), &addr) != 0)
    ip = ntohl(addr.s_addr);
  return ip;
}

std::string Network::IPToString(uint32 ip) {
  std::ostringstream ost;
  ost << ((ip >> 24) & 0xff);
  ost << '.';
  ost << ((ip >> 16) & 0xff);
  ost << '.';
  ost << ((ip >> 8) & 0xff);
  ost << '.';
  ost << ((ip >> 0) & 0xff);
  return ost.str();
}

std::string Network::ToString() const {
  std::stringstream ss;
  // Print out the first space-terminated token of the network name, plus
  // the IP address.
  ss << "Net[" << name_.substr(0, name_.find(' ')) 
    << ":" << Network::IPToString(ip_) << "]";
  return ss.str();
}

}


#ifndef CWF_CONNECT_H__
#define CWF_CONNECT_H__

// open socket
// redirect stdin/stdout to socket
    
namespace cwf {
#if defined(OS_WIN)
int Connection(const char *addr, unsigned short port, const char *unixsocket);
#endif

#if defined(OS_LINUX)
int bind_socket(const char *addr, unsigned short port, const char *unixsocket, uid_t uid, gid_t gid, int mode);
#endif

int FastcgiConnect(const char *addr, unsigned short port, const char *unixsocket = 0
                    , /*uid_t uid, gid_t gid, */int mode = 0, int fork_count = 0);

}
#endif // CWF_CONNECT_H__

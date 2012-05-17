#ifdef WIN32
  #include <winsock2.h>
#else
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <arpa/inet.h> // inet_pton
#endif

#include <stdlib.h>
#include <stdio.h> // perror
#include <string.h> // memset, strchr

#ifdef WIN32
// TDOO: support AF_INET6__

int gettimeofday(struct timeval *tv, struct timezone *tz) {
  return 0; 
}

int close(int fd) {
  CloseHandle((HANDLE)fd);
  return 0;
}

int inet_pton(int af, const char *src, void *dst) {
	if (af == AF_INET) {
		int a,b,c,d;
		char more;
		struct in_addr *addr = (struct in_addr *)dst;
		if (sscanf(src, "%d.%d.%d.%d%c", &a,&b,&c,&d,&more) != 4)
			return 0;
		if (a < 0 || a > 255) return 0;
		if (b < 0 || b > 255) return 0;
		if (c < 0 || c > 255) return 0;
		if (d < 0 || d > 255) return 0;
		addr->s_addr = htonl((a<<24) | (b<<16) | (c<<8) | d);
		return 1;
#ifdef AF_INET6__
	} else if (af == AF_INET6) {
		struct in6_addr *out = (struct in6_addr *)dst;
		uint16_t words[8];
		int gapPos = -1, i, setWords=0;
		const char *dot = strchr(src, '.');
		const char *eow; /* end of words. */
		if (dot == src)
			return 0;
		else if (!dot)
			eow = src+strlen(src);
		else {
			int byte1,byte2,byte3,byte4;
			char more;
			for (eow = dot-1; eow >= src && isdigit(*eow); --eow)
				;
			++eow;

			/* We use "scanf" because some platform inet_aton()s are too lax
			 * about IPv4 addresses of the form "1.2.3" */
			if (sscanf(eow, "%d.%d.%d.%d%c",
					   &byte1,&byte2,&byte3,&byte4,&more) != 4)
				return 0;

			if (byte1 > 255 || byte1 < 0 ||
				byte2 > 255 || byte2 < 0 ||
				byte3 > 255 || byte3 < 0 ||
				byte4 > 255 || byte4 < 0)
				return 0;

			words[6] = (byte1<<8) | byte2;
			words[7] = (byte3<<8) | byte4;
			setWords += 2;
		}

		i = 0;
		while (src < eow) {
			if (i > 7)
				return 0;
			if (isdigit(*src)) {
				char *next;
				long r = strtol(src, &next, 16);
				if (next > 4+src)
					return 0;
				if (next == src)
					return 0;
				if (r<0 || r>65536)
					return 0;

				words[i++] = (uint16_t)r;
				setWords++;
				src = next;
				if (*src != ':' && src != eow)
					return 0;
				++src;
			} else if (*src == ':' && i > 0 && gapPos==-1) {
				gapPos = i;
				++src;
			} else if (*src == ':' && i == 0 && src[1] == ':' && gapPos==-1) {
				gapPos = i;
				src += 2;
			} else {
				return 0;
			}
		}

		if (setWords > 8 ||
			(setWords == 8 && gapPos != -1) ||
			(setWords < 8 && gapPos == -1))
			return 0;

		if (gapPos >= 0) {
			int nToMove = setWords - (dot ? 2 : 0) - gapPos;
			int gapLen = 8 - setWords;
			/* assert(nToMove >= 0); */
			if (nToMove < 0)
				return -1; /* should be impossible */
			memmove(&words[gapPos+gapLen], &words[gapPos],
					sizeof(uint16_t)*nToMove);
			memset(&words[gapPos], 0, sizeof(uint16_t)*gapLen);
		}
		for (i = 0; i < 8; ++i) {
			// out->s6_addr[2*i  ] = words[i] >> 8;
			// out->s6_addr[2*i+1] = words[i] & 0xff;
		}

		return 1;
#endif
	} else {
		return -1;
	}
}

#endif // WIN32

int
parse_sockaddr_port(const char *ip_as_string, struct sockaddr *out, int *outlen) {
	int port;
	char buf[128];
	const char *cp, *addr_part, *port_part;
	int is_ipv6;
	/* recognized formats are:
	 * [ipv6]:port
	 * ipv6
	 * [ipv6]
	 * ipv4:port
	 * ipv4
	 */

	cp = strchr(ip_as_string, ':');
	if (*ip_as_string == '[') {
		int len;
		if (!(cp = strchr(ip_as_string, ']'))) {
			return -1;
		}
		len = (int) ( cp-(ip_as_string + 1) );
		if (len > (int)sizeof(buf)-1) {
			return -1;
		}
		memcpy(buf, ip_as_string+1, len);
		buf[len] = '\0';
		addr_part = buf;
		if (cp[1] == ':')
			port_part = cp+2;
		else
			port_part = NULL;
		is_ipv6 = 1;
	} else if (cp && strchr(cp+1, ':')) {
		is_ipv6 = 1;
		addr_part = ip_as_string;
		port_part = NULL;
	} else if (cp) {
		is_ipv6 = 0;
		if (cp - ip_as_string > (int)sizeof(buf)-1) {
			return -1;
		}
		memcpy(buf, ip_as_string, cp-ip_as_string);
		buf[cp-ip_as_string] = '\0';
		addr_part = buf;
		port_part = cp+1;
	} else {
		addr_part = ip_as_string;
		port_part = NULL;
		is_ipv6 = 0;
	}

	if (port_part == NULL) {
		port = 0;
	} else {
		port = atoi(port_part);
		if (port <= 0 || port > 65535) {
			return -1;
		}
	}

	if (!addr_part)
		return -1; /* Should be impossible. */
#ifdef AF_INET6__
	if (is_ipv6)
	{
		struct sockaddr_in6 sin6;
		memset(&sin6, 0, sizeof(sin6));
#ifdef HAVE_STRUCT_SOCKADDR_IN6_SIN6_LEN
		sin6.sin6_len = sizeof(sin6);
#endif
		sin6.sin6_family = AF_INET6;
		sin6.sin6_port = htons(port);
		if (1 != inet_pton(AF_INET6, addr_part, &sin6.sin6_addr))
			return -1;
		if ((int)sizeof(sin6) > *outlen)
			return -1;
		memset(out, 0, *outlen);
		memcpy(out, &sin6, sizeof(sin6));
		*outlen = sizeof(sin6);
		return 0;
	}
	else
#endif
	{
		struct sockaddr_in sin;
		memset(&sin, 0, sizeof(sin));
#ifdef _EVENT_HAVE_STRUCT_SOCKADDR_IN_SIN_LEN
		sin.sin_len = sizeof(sin);
#endif
		sin.sin_family = AF_INET;
		sin.sin_port = htons(port);
		if (1 != inet_pton(AF_INET, addr_part, &sin.sin_addr))
			return -1;
		if ((int)sizeof(sin) > *outlen)
			return -1;
		memset(out, 0, *outlen);
		memcpy(out, &sin, sizeof(sin));
		*outlen = sizeof(sin);
		return 0;
	}
}

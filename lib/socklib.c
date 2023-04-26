/* socklib.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#if WIN32
#include <winsock2.h>
#else
#include <fcntl.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#endif

#if __UNIX__
#include <arpa/inet.h>
#elif __BEOS__
#include <be/net/netdb.h>
#endif

#include "compat.h"
#include "debug.h"
#include "socklib.h"
#include "srtypes.h"
#include "threadlib.h"

#if WIN32
#define DEFAULT_TIMEOUT (15 * 1000)
#define FIRST_READ_TIMEOUT (30 * 1000)
#endif

/****************************************************************************
 * Function definitions
 ****************************************************************************/
error_code
socklib_init() {
#if WIN32
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;

	wVersionRequested = MAKEWORD(2, 2);
	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0)
		return SR_ERROR_WIN32_INIT_FAILURE;
#endif

	return SR_SUCCESS;
}

/* Try to find the local interface to bind to */
error_code
read_interface(char *if_name, uint32_t *addr) {
#if defined(WIN32)
	return -1;
#else
	int fd;
	struct ifreq ifr;

	memset(&ifr, 0, sizeof(struct ifreq));
	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) >= 0) {
		ifr.ifr_addr.sa_family = AF_INET;
		strcpy(ifr.ifr_name, if_name);
		if (ioctl(fd, SIOCGIFADDR, &ifr) == 0)
			*addr = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr;
		else {
			close(fd);
			return -2;
		}
	} else
		return -1;
	close(fd);
	return 0;
#endif
}

/*
 * open's a tcp connection to host at port, host can be a dns name or IP,
 * socket_handle gets assigned to the handle for the connection
 */
error_code
socklib_open(
    HSOCKET *socket_handle, char *host, int port, char *if_name, int timeout) {
	int rc;
	struct sockaddr_in address, local;
	struct hostent *hp;
	int len;

	if (!socket_handle || !host)
		return SR_ERROR_INVALID_PARAM;

	/* On error:
	   Unix returns -1 and sets errno.
	   Windows??? */
	socket_handle->s = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_handle->s == SOCKET_ERROR) {
		debug_printf("socket() failed\n");
		WSACleanup();
		return SR_ERROR_CANT_CREATE_SOCKET;
	}

	if (if_name) {
		if (read_interface(if_name, &local.sin_addr.s_addr) != 0)
			local.sin_addr.s_addr = htonl(INADDR_ANY);
		local.sin_family = AF_INET;
		local.sin_port = htons(0);
		/* On error:
		   Unix returns -1 and sets errno.
		   Windows??? */
		debug_printf("Calling bind\n");
		if (bind(socket_handle->s, (struct sockaddr *)&local, sizeof(local))
		    == SOCKET_ERROR) {
			debug_printf("Bind failed\n");
			WSACleanup();
			closesocket(socket_handle->s);
			return SR_ERROR_CANT_BIND_ON_INTERFACE;
		}
	}

	if ((address.sin_addr.s_addr = inet_addr(host)) == INADDR_NONE) {
		debug_printf("Calling gethostbyname\n");
		hp = gethostbyname(host);
		if (hp) {
			memcpy(&address.sin_addr, hp->h_addr_list[0], hp->h_length);
		} else {
			debug_printf("resolving hostname: %s failed\n", host);
			WSACleanup();
			/* GCS Added... */
			closesocket(socket_handle->s);
			return SR_ERROR_CANT_RESOLVE_HOSTNAME;
		}
	}
	address.sin_family = AF_INET;
	address.sin_port = htons((unsigned short)port);
	len = sizeof(address);

	debug_printf("Calling connect\n");
	rc = connect(socket_handle->s, (struct sockaddr *)&address, len);
	debug_printf("Connect complete\n");
	if (rc == SOCKET_ERROR) {
		debug_printf("connect failed\n");
		/* GCS Added... */
		WSACleanup();
		closesocket(socket_handle->s);
		return SR_ERROR_CONNECT_FAILED;
	}

#ifdef WIN32
	{
		struct timeval tv = {timeout, 0};
		rc = setsockopt(
		    socket_handle->s,
		    SOL_SOCKET,
		    SO_RCVTIMEO,
		    (char *)&tv,
		    sizeof(struct timeval));
		if (rc == SOCKET_ERROR) {
			debug_printf("setsockopt failed\n");
			/* GCS Added... */
			WSACleanup();
			closesocket(socket_handle->s);
			return SR_ERROR_CANT_SET_SOCKET_OPTIONS;
		}
	}
#endif

	socket_handle->closed = FALSE;
	return SR_SUCCESS;
}

void
socklib_cleanup() {
	WSACleanup();
}

void
socklib_close(HSOCKET *socket_handle) {
	closesocket(socket_handle->s);
	socket_handle->closed = TRUE;
}

error_code
socklib_read_header(
    RIP_MANAGER_INFO *rmi, HSOCKET *socket_handle, char *buffer, int size) {
	int i;
#ifdef WIN32
	int timeout;
#endif
	int ret;
	char *t;

	if (socket_handle->closed)
		return SR_ERROR_SOCKET_CLOSED;

#ifdef WIN32
	timeout = 2 * rmi->prefs->timeout * 1000; /* Convert sec to msec */
	if (setsockopt(
	        socket_handle->s,
	        SOL_SOCKET,
	        SO_RCVTIMEO,
	        (char *)&timeout,
	        sizeof(int))
	    == SOCKET_ERROR)
		return SR_ERROR_CANT_SET_SOCKET_OPTIONS;
#endif

	memset(buffer, 0, size);
	for (i = 0; i < size; i++) {
		ret = socklib_recvall(rmi, socket_handle, &buffer[i], 1, 0);
		if (ret < 0) {
			return ret;
		}

		if (ret == 0) {
			debug_printf("http header:\n%s\n", buffer);
			return SR_ERROR_NO_HTTP_HEADER;
		}

		if (socket_handle->closed)
			return SR_ERROR_SOCKET_CLOSED;

#if defined(commentout)
		/* This is too restrictive.  Some servers do not send this. */
		if (!strstr(buffer, "icy-") && !strstr(buffer, "ice-"))
			continue;
#endif

		t = buffer + (i > 3 ? i - 3 : i);

		if (strncmp(t, "\r\n\r\n", 4) == 0)
			break;
		/* Allegedly live365 used to do this */
		if (strncmp(t, "\n\0\r\n", 4) == 0)
			break;
	}

	if (i == size) {
		debug_printf("http header:\n%s\n", buffer);
		return SR_ERROR_NO_HTTP_HEADER;
	}

	buffer[i] = '\0';

#ifdef WIN32
	timeout = rmi->prefs->timeout * 1000; /* Convert sec to msec */
	if (setsockopt(
	        socket_handle->s,
	        SOL_SOCKET,
	        SO_RCVTIMEO,
	        (char *)&timeout,
	        sizeof(timeout))
	    == SOCKET_ERROR)
		return SR_ERROR_CANT_SET_SOCKET_OPTIONS;
#endif

	return SR_SUCCESS;
}

error_code
socklib_recvall(
    RIP_MANAGER_INFO *rmi,
    HSOCKET *socket_handle,
    char *buffer,
    int size,
    int timeout) {
	int ret = 0, read = 0;
	int sock;
	fd_set fds;
	struct timeval tv;

	sock = socket_handle->s;
	FD_ZERO(&fds);
	while (size) {
		if (socket_handle->closed)
			return SR_ERROR_SOCKET_CLOSED;

		if (timeout > 0) {
/* Wait up to 'timeout' seconds for data on socket to be
   ready for read */
#if __UNIX__
			FD_SET(rmi->abort_pipe[0], &fds);
#endif
			FD_SET(sock, &fds);
			tv.tv_sec = timeout;
			tv.tv_usec = 0;
			ret = select(sock + 1, &fds, NULL, NULL, &tv);
			if (ret == SOCKET_ERROR) {
				/* This happens when I kill winamp while ripping */
				return SR_ERROR_SELECT_FAILED;
			}
			if (ret == 0) {
				return SR_ERROR_TIMEOUT;
			}
		}
#if __UNIX__
		if (FD_ISSET(rmi->abort_pipe[0], &fds)) {
			debug_printf("socklib_recvall detected write to abort pipe.\n");
			return SR_ERROR_ABORT_PIPE_SIGNALLED;
		}
#endif
		ret = recv(socket_handle->s, &buffer[read], size, 0);
		debug_printf("RECV req %5d bytes, got %5d bytes\n", size, ret);

		if (ret == SOCKET_ERROR) {
			debug_printf("RECV failed, errno = %d\n", errno);
			debug_printf("Err = %s\n", strerror(errno));
			return SR_ERROR_RECV_FAILED;
		}

		/* Got zero bytes on blocking read.  For unix this is an
		   orderly shutdown. */
		if (ret == 0) {
			debug_printf("recv received zero bytes!\n");
			break;
		}

		read += ret;
		size -= ret;
	}

	return read;
}

int
socklib_sendall(HSOCKET *socket_handle, char *buffer, int size) {
	int ret = 0, sent = 0;

	while (size) {
		if (socket_handle->closed)
			return SR_ERROR_SOCKET_CLOSED;

		ret = send(socket_handle->s, &buffer[sent], size, 0);
		if (ret == SOCKET_ERROR)
			return SR_ERROR_SEND_FAILED;

		if (ret == 0)
			break;

		sent += ret;
		size -= ret;
	}

	return sent;
}

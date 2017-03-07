/* relaylib.c
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
#if WIN32
#include <winsock2.h>
#else
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <ctype.h>
#endif

#if __UNIX__
#include <arpa/inet.h>
#elif __BEOS__
#include <be/net/netdb.h>   
#endif

#include <string.h>
#include "relaylib.h"
#include "srtypes.h"
#include "http.h"
#include "socklib.h"
#include "threadlib.h"
#include "debug.h"
#include "compat.h"
#include "rip_manager.h"
#include "cbuf2.h"

#if defined (WIN32)
#ifdef errno
#undef errno
#endif
#define errno WSAGetLastError()
#endif

/*****************************************************************************
 * Private functions
 *****************************************************************************/
static void thread_accept (void *arg);
static error_code try_port (RELAYLIB_INFO* rli, u_short port, 
			    char *if_name, char *relay_ip);
static void thread_send (void *arg);
static error_code relaylib_start_threads (RIP_MANAGER_INFO* rmi);

#define BUFSIZE (1024)

#define HTTP_HEADER_DELIM "\n"
#define ICY_METADATA_TAG "Icy-MetaData:"

static void
destroy_all_hostsocks (RIP_MANAGER_INFO* rmi)
{
    RELAY_LIST* ptr;

    threadlib_waitfor_sem (&rmi->relay_list_sem);
    while (rmi->relay_list != NULL) {
        ptr = rmi->relay_list;
        closesocket(ptr->m_sock);
        rmi->relay_list = ptr->m_next;
        if (ptr->m_buffer != NULL) {
            free (ptr->m_buffer);
            ptr->m_buffer = NULL;
        }
        free(ptr);
    }
    rmi->relay_list_len = 0;
    rmi->relay_list = NULL;
    threadlib_signal_sem (&rmi->relay_list_sem);
}

static int
tag_compare (char *str, char *tag)
{
    int i, a,b;
    int len;

    len = strlen(tag);

    for (i=0; i<len; i++) {
	a = tolower (str[i]);
	b = tolower (tag[i]);
	if ((a != b) || (a == 0))
	    return 1;
    }
    return 0;
}

static int
header_receive (int sock, int *icy_metadata)
{
    fd_set fds;
    struct timeval tv;
    int r;
    char buf[BUFSIZE+1];
    char *md;

    *icy_metadata = 0;
        
    while (1) {
	// use select to prevent deadlock on malformed http header
	// that lacks CRLF delimiter
	FD_ZERO(&fds);
        FD_SET(sock, &fds);
        tv.tv_sec = 2;
        tv.tv_usec = 0;
        r = select (sock + 1, &fds, NULL, NULL, &tv);
        if (r != 1) {
	    debug_printf ("header_receive: could not select\n");
	    break;
	}

	r = recv(sock, buf, BUFSIZE, 0);
	if (r <= 0) {
	    debug_printf ("header_receive: could not select\n");
	    break;
	}

	buf[r] = 0;
	md = strtok (buf, HTTP_HEADER_DELIM);
	while (md) {
	    debug_printf ("Got token: %s\n", md);

	    // Finished when we are at end of header: only CRLF will be there.
	    if ((md[0] == '\r') && (md[1] == 0)) {
		debug_printf ("End of header\n");
		return 0;
	    }
	
	    // Check for desired tag
	    if (tag_compare (md, ICY_METADATA_TAG) == 0) {
		for (md += strlen(ICY_METADATA_TAG); md[0] && (isdigit(md[0]) == 0); md++);
		
		if (md[0])
		    *icy_metadata = atoi(md);

		debug_printf ("client flag ICY-METADATA is %d\n", 
			      *icy_metadata);
	    }
	    
	    md = strtok (NULL, HTTP_HEADER_DELIM);
	}
    }

    return 1;
}


// Quick function to "eat" incoming data from a socket
// All data is discarded
// Returns 0 if successful or SOCKET_ERROR if error
static int
swallow_receive (int sock)
{
    fd_set fds;
    struct timeval tv;
    int ret = 0;
    char buf[BUFSIZE];
    BOOL hasmore = TRUE;
        
    FD_ZERO(&fds);
    while (hasmore) {
        // Poll the socket to see if it has anything to read
        hasmore = FALSE;
        FD_SET(sock, &fds);
        tv.tv_sec = 0;
        tv.tv_usec = 0;
        ret = select(sock + 1, &fds, NULL, NULL, &tv);
        if (ret == 1) {
            // Read and throw away data, ignoring errors
            ret = recv(sock, buf, BUFSIZE, 0);
            if (ret > 0) {
                hasmore = TRUE;
            }
            else if (ret == SOCKET_ERROR) {
                break;
            }
        }
        else if (ret == SOCKET_ERROR) {
            break;
        }
    }
        
    return ret;
}


// Makes a socket non-blocking
void
make_nonblocking (int sock)
{
#if defined (WIN32)
    long opt = 1;
    ioctlsocket(sock, FIONBIO, &opt);
#else
    int opt = fcntl(sock, F_GETFL);
    if (opt != SOCKET_ERROR) {
        fcntl(sock, F_SETFL, opt | O_NONBLOCK);
    }
#endif
}

#if defined (commentout)
error_code
relaylib_set_response_header(char *http_header)
{
    if (!http_header)
        return SR_ERROR_INVALID_PARAM;

    strcpy(m_http_header, http_header);

    return SR_SUCCESS;
}
#endif

#ifndef WIN32
void
catch_pipe(int code)
{
    //m_sock = 0;
    //m_connected = FALSE;
    // JCBUG, not sure what to do about this
}
#endif

error_code
relaylib_start (RIP_MANAGER_INFO* rmi,
		BOOL search_ports, u_short relay_port, u_short max_port, 
		u_short *port_used, char *if_name, 
		int max_connections, 
		char *relay_ip, 
		int have_metadata)
{
    int ret;
#ifdef WIN32
    WSADATA wsd;
#endif
    RELAYLIB_INFO* rli = &rmi->relaylib_info;

    /* GCS: These were globally initialized... */
    rli->m_listensock = SOCKET_ERROR;
    rli->m_running = FALSE;
    rli->m_running_accept = FALSE;
    rli->m_running_send = FALSE;

    debug_printf ("relaylib_start()\n");

    rmi->relay_list = 0;
    rmi->relay_list_len = 0;

#ifdef WIN32
    if (WSAStartup(MAKEWORD(2,2), &wsd) != 0) {
	debug_printf ("relaylib_init(): SR_ERROR_CANT_BIND_ON_PORT\n");
        return SR_ERROR_CANT_BIND_ON_PORT;
    }
#endif

    if (relay_port < 1 || !port_used) {
	debug_printf ("relaylib_init(): SR_ERROR_INVALID_PARAM\n");
        return SR_ERROR_INVALID_PARAM;
    }

#ifndef WIN32
    // catch a SIGPIPE if send fails
    signal(SIGPIPE, catch_pipe);
#endif

    rli->m_sem_not_connected = threadlib_create_sem();
    rmi->relay_list_sem = threadlib_create_sem();
    threadlib_signal_sem (&rmi->relay_list_sem);

    // NOTE: we need to signal it here in case we try to destroy
    // relaylib before the thread starts!
    threadlib_signal_sem (&rli->m_sem_not_connected);

    //m_max_connections = max_connections;
    //m_have_metadata = have_metadata;
    *port_used = 0;
    if (!search_ports)
        max_port = relay_port;

    for(;relay_port <= max_port; relay_port++) {
        ret = try_port (rli, (u_short) relay_port, if_name, relay_ip);
        if (ret == SR_ERROR_CANT_BIND_ON_PORT)
            continue;           // Keep searching.

        if (ret == SR_SUCCESS) {
            *port_used = relay_port;
            debug_printf ("Relay: Listening on port %d\n", relay_port);
            ret = SR_SUCCESS;

	    if (!rli->m_running) {
		ret = relaylib_start_threads (rmi);
	    }
	    return ret;
        } else {
            return ret;
        }
    }

    return SR_ERROR_CANT_BIND_ON_PORT;
}

static error_code
try_port (RELAYLIB_INFO* rli, u_short port, char *if_name, char *relay_ip)
{
    struct hostent *he;
    struct sockaddr_in local;

    rli->m_listensock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (rli->m_listensock == SOCKET_ERROR) {
	debug_printf ("try_port(%d) failed socket() call\n", port);
        return SR_ERROR_SOCK_BASE;
    }
    make_nonblocking(rli->m_listensock);

    if ('\0' == *relay_ip) {
	if (read_interface(if_name,&local.sin_addr.s_addr) != 0)
	    local.sin_addr.s_addr = htonl(INADDR_ANY);
    } else {
	he = gethostbyname(relay_ip);
	memcpy(&local.sin_addr, he->h_addr_list[0], he->h_length);
    }

    local.sin_family = AF_INET;
    local.sin_port = htons(port);

#ifndef WIN32
    {
        // Prevent port error when restarting quickly after a previous exit
        int opt = 1;
        setsockopt(rli->m_listensock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    }
#endif
                        
    if (bind(rli->m_listensock, (struct sockaddr *)&local, sizeof(local)) == SOCKET_ERROR)
    {
	debug_printf ("try_port(%d) failed bind() call\n", port);
        closesocket(rli->m_listensock);
        rli->m_listensock = SOCKET_ERROR;
        return SR_ERROR_CANT_BIND_ON_PORT;
    }
        
    if (listen(rli->m_listensock, 1) == SOCKET_ERROR)
    {
	debug_printf ("try_port(%d) failed listen() call\n", port);
        closesocket(rli->m_listensock);
        rli->m_listensock = SOCKET_ERROR;
        return SR_ERROR_SOCK_BASE;
    }

    debug_printf ("try_port(%d) succeeded\n", port);
    return SR_SUCCESS;
}

void
relaylib_stop (RIP_MANAGER_INFO* rmi)
{
    int ix;
    RELAYLIB_INFO* rli = &rmi->relaylib_info;

    debug_printf("relaylib_stop:start\n");
    if (!rli->m_running) {
        debug_printf("***relaylib_stop:return\n");
        return;
    }
    rli->m_running = FALSE;
    ix = 0;
    while (ix<120 && (rli->m_running_accept | rli->m_running_send)) {
        sleep(1);
        ++ix;
    }
    if (ix==120) {
        debug_printf ("relay threads refused to end, aborting\n");
        fprintf (stderr,"relay threads refused to end, aborting\n");
        exit(1);
    }

    threadlib_signal_sem (&rli->m_sem_not_connected);
    if (closesocket(rli->m_listensock) == SOCKET_ERROR) {   
        // JCBUG, what can we do?
    }
    /* Accept thread will watch for this and not try to accept anymore */
    rli->m_listensock = SOCKET_ERROR;

    debug_printf ("waiting for relay close\n");
    threadlib_waitforclose (&rli->m_hthread_accept);
    threadlib_waitforclose (&rli->m_hthread_send);
    destroy_all_hostsocks (rmi);
    threadlib_destroy_sem (&rli->m_sem_not_connected);

    debug_printf("relaylib_stop:done!\n");
}

static error_code
relaylib_start_threads (RIP_MANAGER_INFO* rmi)
{
    int ret;
    RELAYLIB_INFO* rli = &rmi->relaylib_info;

    rli->m_running = TRUE;

    ret = threadlib_beginthread (&rli->m_hthread_accept, 
				 thread_accept, (void*) rmi);
    if (ret != SR_SUCCESS) return ret;
    rli->m_running_accept = TRUE;

    ret = threadlib_beginthread (&rli->m_hthread_send, 
				 thread_send, (void*) rmi);
    if (ret != SR_SUCCESS) return ret;
    rli->m_running_send = TRUE;

    return SR_SUCCESS;
}

void
thread_accept (void* arg)
{
    int ret;
    int newsock;
    BOOL good;
    struct sockaddr_in client;
    socklen_t iAddrSize = sizeof(client);
    RELAY_LIST* newhostsock;
    int icy_metadata;
    char* client_http_header;
    RIP_MANAGER_INFO* rmi = (RIP_MANAGER_INFO*) arg;
    RELAYLIB_INFO* rli = &rmi->relaylib_info;
    STREAM_PREFS* prefs = rmi->prefs;
    int have_metadata;

    if (rmi->http_info.meta_interval == NO_META_INTERVAL) {
	have_metadata = 0;
    } else {
	have_metadata = 1;
    }

    debug_printf("thread_accept:start\n");

    while (rli->m_running)
    {
        fd_set fds;
        struct timeval tv;

        // this event will keep use from accepting while we 
	//   have a connection active
        // when a connection gets dropped, or when streamripper shuts down
        //   this event will get signaled
	debug_printf("thread_accept:waiting on m_sem_not_connected\n");
        threadlib_waitfor_sem (&rli->m_sem_not_connected);
        if (!rli->m_running) {
	    debug_printf("thread_accept:exit (no longer m_running)\n");
            break;
	}

        // Poll once per second, instead of blocking forever in 
	// accept(), so that we can regain control if relaylib_stop() 
	// called
        FD_ZERO (&fds);
        while (rli->m_listensock != SOCKET_ERROR && rli->m_running)
        {
            FD_SET (rli->m_listensock, &fds);
            tv.tv_sec = 1;
            tv.tv_usec = 0;
	    debug_printf("thread_accept:calling select()\n");
            ret = select (rli->m_listensock + 1, &fds, NULL, NULL, &tv);
	    debug_printf("thread_accept:select() returned %d\n", ret);
            if (ret == 1) {
                unsigned long num_connected;
                /* If connections are full, do nothing.  Note that 
                    m_max_connections is 0 for infinite connections allowed. */
                threadlib_waitfor_sem (&rmi->relay_list_sem);
                num_connected = rmi->relay_list_len;
                threadlib_signal_sem (&rmi->relay_list_sem);
                if (prefs->max_connections > 0 && num_connected >= (unsigned long) prefs->max_connections) {
                    continue;
                }
                /* Check for connections */
		debug_printf ("Calling accept()\n");
                newsock = accept (rli->m_listensock, (struct sockaddr *)&client, &iAddrSize);
                if (newsock != SOCKET_ERROR) {
                    // Got successful accept

                    debug_printf ("Relay: Client %d new from %s:%hu\n", newsock,
                                  inet_ntoa(client.sin_addr), ntohs(client.sin_port));

                    // Socket is new and its buffer had better have 
		    // room to hold the entire HTTP header!
                    good = FALSE;
                    if (header_receive (newsock, &icy_metadata) == 0 && rmi->cbuf2.buf != NULL) {
			int header_len;
			make_nonblocking (newsock);
			client_http_header = client_relay_header_generate (rmi, icy_metadata);
			header_len = strlen (client_http_header);
			ret = send (newsock, client_http_header, strlen(client_http_header), 0);
			debug_printf ("Relay: Sent response header to client %d (%d)\n", 
			    ret, header_len);
			client_relay_header_release (client_http_header);
			if (ret == header_len) {
                            newhostsock = malloc (sizeof(RELAY_LIST));
                            if (newhostsock != NULL) {
                                // Add new client to list (headfirst)
                                threadlib_waitfor_sem (&rmi->relay_list_sem);
                                newhostsock->m_is_new = 1;
                                newhostsock->m_sock = newsock;
                                newhostsock->m_next = rmi->relay_list;
				if (have_metadata) {
                                    newhostsock->m_icy_metadata = icy_metadata;
				} else {
                                    newhostsock->m_icy_metadata = 0;
				}

                                rmi->relay_list = newhostsock;
                                rmi->relay_list_len++;
                                threadlib_signal_sem (&rmi->relay_list_sem);
                                good = TRUE;
                            }
                        }
                    }

                    if (!good)
                    {
                        closesocket (newsock);
                        debug_printf ("Relay: Client %d disconnected (Unable to receive HTTP header) or cbuf2.buf is NULL\n", newsock);
			//if (rmi->cbuf2.buf == NULL) {
			  //  debug_printf ("In fact, cbuf2.buf is NULL\n");
			//}
                    }
                }
            }
            else if (ret == SOCKET_ERROR)
            {
                /* Something went wrong with select */
                break;
            }
        }
        threadlib_signal_sem (&rli->m_sem_not_connected);
	/* loop back up to select */
    }
    rli->m_running_accept = FALSE;
    rli->m_running = FALSE;
}

/* Sock is ready to receive, so send it from cbuf to relay */
static BOOL 
send_to_relay (RIP_MANAGER_INFO* rmi, RELAY_LIST* ptr)
{
    int ret;
    int err_errno;
    BOOL good = TRUE;
    int done = 0;

    /* For new clients, initialize cbuf pointers */
    if (ptr->m_is_new) {
	int burst_amount = 32*1024;
	//	int burst_amount = 64*1024;
	//	int burst_amount = 128*1024;
	ptr->m_offset = 0;
	ptr->m_left_to_send = 0;

	cbuf2_init_relay_entry (&rmi->cbuf2, ptr, burst_amount);
	
	ptr->m_is_new = 0;
    }

    while (!done) {
	/* If our private buffer is empty, copy some from the cbuf */
	if (!ptr->m_left_to_send) {
	    error_code rc;
	    ptr->m_offset = 0;
	    rc = cbuf2_extract_relay (&rmi->cbuf2, ptr);
	    
	    if (rc == SR_ERROR_BUFFER_EMPTY) {
		break;
	    }
	}
	/* Send from the private buffer to the client */
	debug_printf ("Relay: Sending Client %d to the client\n", 
		      ptr->m_left_to_send );
	ret = send (ptr->m_sock, ptr->m_buffer+ptr->m_offset, 
		    ptr->m_left_to_send, 0);
	debug_printf ("Relay: Sending to Client returned %d\n", ret );
	if (ret == SOCKET_ERROR) {
	    /* Sometimes windows gives me an errno of 0
	       Sometimes windows gives me an errno of 183 
	       See this thread for details: 
	       http://groups.google.com/groups?hl=en&lr=&ie=UTF-8&selm=8956d3e8.0309100905.6ba60e7f%40posting.google.com
	    */
	    err_errno = errno;
	    if (err_errno == EWOULDBLOCK || err_errno == 0 
		|| err_errno == 183)
	    {
#if defined (WIN32)
		// Client is slow.  Retry later.
		WSASetLastError (0);
#endif
	    } else {
		debug_printf ("Relay: socket error is %d\n",errno);
		good = FALSE;
	    }
	    done = 1;
	} else { 
	    // Send was successful
	    ptr->m_offset += ret;
	    ptr->m_left_to_send -= ret;
	    if (ptr->m_left_to_send < 0) {
		/* GCS: can this ever happen??? */
		debug_printf ("ptr->m_left_to_send < 0\n");
		ptr->m_left_to_send = 0;
		done = 1;
	    }
	}
    }
    return good;
}

void 
relaylib_disconnect (RIP_MANAGER_INFO* rmi, RELAY_LIST* prev, RELAY_LIST* ptr)
{
    RELAY_LIST* next = ptr->m_next;
    int sock = ptr->m_sock;

    closesocket (sock);
                                   
    // Carefully delete this client from list without 
    // affecting list order
    if (prev != NULL) {
	prev->m_next = next;
    } else {
	rmi->relay_list = next;
    }
    if (ptr->m_buffer != NULL) {
	free (ptr->m_buffer);
        ptr->m_buffer = NULL;
    }
    free (ptr);
    rmi->relay_list_len --;
}

void 
thread_send (void* arg)
{
    RELAY_LIST* prev;
    RELAY_LIST* ptr;
    RELAY_LIST* next;
    int sock;
    BOOL good;
    error_code err = SR_SUCCESS;
    RIP_MANAGER_INFO* rmi = (RIP_MANAGER_INFO*) arg;
    RELAYLIB_INFO* rli = &rmi->relaylib_info;

    while (rli->m_running) {
	threadlib_waitfor_sem (&rmi->relay_list_sem);
	ptr = rmi->relay_list;
	if (ptr != NULL) {
	    prev = NULL;
	    while (ptr != NULL) {
		sock = ptr->m_sock;
		next = ptr->m_next;

		if (swallow_receive(sock) != 0) {
		    good = FALSE;
		} else {
		    good = send_to_relay (rmi, ptr);
		}
	       
		if (!good) {
		    debug_printf ("Relay: Client %d disconnected (%s)\n", 
				  sock, strerror(errno));
		    relaylib_disconnect (rmi, prev, ptr);
		} else if (ptr != NULL) {
		    prev = ptr;
		}
		ptr = next;
	    }
	} else {
	    err = SR_ERROR_HOST_NOT_CONNECTED;
	}
	threadlib_signal_sem (&rmi->relay_list_sem);
	Sleep (50);
    }
    rli->m_running_send = FALSE;
}

/*
   _ _ _     _                         _ _
  | (_) |__ | | _____  ___ _ __   __ _| (_)_   _____
  | | | '_ \| |/ / _ \/ _ \ '_ \ / _` | | \ \ / / _ \
  | | | |_) |   <  __/  __/ |_) | (_| | | |\ V /  __/
  |_|_|_.__/|_|\_\___|\___| .__/ \__,_|_|_| \_/ \___|
                           |_|

  MIT License

  Copyright (c) 2005-2016 Fabio Busatto <fabio.busatto@gmail.com>

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

*/

#define _GNU_SOURCE
#include <dlfcn.h>
#include <errno.h>
#include <netinet/tcp.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>

int socket(int domain, int type, int protocol);

int socket(int domain, int type, int protocol) {
  static int (*orig_socket)(int, int, int) = NULL;
  int sockfd = -1, optval = 1;
  char *env;

  do {
    /* if the original function is NULL, try to resolve it or break */
    if (!orig_socket &&
        !(*(void **)(&orig_socket) = dlsym(RTLD_NEXT, "socket"))) {
      errno = EACCES;
      break;
    }

    /* call original function with parameters */
    if ((sockfd = (*orig_socket)(domain, type, protocol)) == -1)
      break;

    /* socket must be IPv4 or IPv6 */
    if ((domain != AF_INET) && (domain != AF_INET6))
      break;

    /* socket must be TCP */
    if (!(type & SOCK_STREAM))
      break;

    /* if environment variable KEEPALIVE is set to "off", break */
    if ((env = getenv("KEEPALIVE")) && !strcasecmp(env, "off"))
      break;

    /* if setting keepalive fails, break */
    if (setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval)) ==
        -1)
      break;

#ifdef TCP_KEEPCNT
    /* if environment variable KEEPCNT is set, override the default option value
     */
    if ((env = getenv("KEEPCNT"))) {
      optval = atoi(env);
      setsockopt(sockfd, SOL_TCP, TCP_KEEPCNT, &optval, sizeof(optval));
    }
#endif

#ifdef TCP_KEEPIDLE
    /* if environment variable KEEPIDLE is set, override the default option
     * value */
    if ((env = getenv("KEEPIDLE"))) {
      optval = atoi(env);
      setsockopt(sockfd, SOL_TCP, TCP_KEEPIDLE, &optval, sizeof(optval));
    }
#endif

#ifdef TCP_KEEPINTVL
    /* if environment variable KEEPINTVL is set, override the default option
     * value */
    if ((env = getenv("KEEPINTVL"))) {
      optval = atoi(env);
      setsockopt(sockfd, SOL_TCP, TCP_KEEPINTVL, &optval, sizeof(optval));
    }
#endif
  } while (0);

  return sockfd;
}

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
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int main(void);

int main() {
  int s;
  int optval;
  socklen_t optlen = sizeof(optval);

  if ((s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    perror("socket()");
    exit(EXIT_FAILURE);
  }

  if (getsockopt(s, SOL_SOCKET, SO_KEEPALIVE, &optval, &optlen) < 0) {
    perror("getsockopt()");
    close(s);
    exit(EXIT_FAILURE);
  }
  printf("SO_KEEPALIVE is %s\n", (optval ? "ON" : "OFF"));

  if (optval) {
#ifdef TCP_KEEPCNT
    if (getsockopt(s, SOL_TCP, TCP_KEEPCNT, &optval, &optlen) < 0) {
      perror("getsockopt()");
      close(s);
      exit(EXIT_FAILURE);
    }
    printf("TCP_KEEPCNT   = %d\n", optval);
#endif

#ifdef TCP_KEEPIDLE
    if (getsockopt(s, SOL_TCP, TCP_KEEPIDLE, &optval, &optlen) < 0) {
      perror("getsockopt()");
      close(s);
      exit(EXIT_FAILURE);
    }
    printf("TCP_KEEPIDLE  = %d\n", optval);
#endif

#ifdef TCP_KEEPINTVL
    if (getsockopt(s, SOL_TCP, TCP_KEEPINTVL, &optval, &optlen) < 0) {
      perror("getsockopt()");
      close(s);
      exit(EXIT_FAILURE);
    }
    printf("TCP_KEEPINTVL = %d\n", optval);
#endif
  }

  close(s);

  exit(EXIT_SUCCESS);
}

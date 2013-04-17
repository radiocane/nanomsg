/*
    Copyright (c) 2013 250bpm s.r.o.

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom
    the Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
    IN THE SOFTWARE.
*/

#ifndef NN_USOCK_INCLUDED
#define NN_USOCK_INCLUDED

/*  Import the definition of nn_iovec. */
#include "../nn.h"

#include "callback.h"
#include "worker.h"
#include "ctx.h"

#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/socket.h>
#undef _GNU_SOURCE

/*  OS-level sockets. */

/*  Callback types generated by nn_usock. */
#define NN_USOCK_CLOSED 1
#define NN_USOCK_CONNECTED 2
#define NN_USOCK_ACCEPTED 3
#define NN_USOCK_SENT 4
#define NN_USOCK_RECEIVED 5
#define NN_USOCK_ERROR 6

/*  Maximum number of iovecs that can be passed to nn_usock_send function. */
#define NN_USOCK_MAX_IOVCNT 3

/*  Size of the buffer used for batch-reads of inbound data. To keep the
    performance optimal make sure that this value is larger than network MTU. */
#define NN_USOCK_BATCH_SIZE 2048

struct nn_usock {

    /*  This class is sink of events. */
    struct nn_callback in_callback;

    /*  This class is source of events. */
    struct nn_callback *out_callback;

    /*  AIO context the socket belongs to. */
    struct nn_ctx *ctx;

    /*  The worker thread the usock is associated with. */
    struct nn_worker *worker;

    /*  The underlying OS socket and handle that represents it in the poller. */
    int s;
    struct nn_worker_fd wfd;

    /*  The state the usock's state machine is in. This value is accessed
        solely from the worker thread. */
    int state;

    /*  Members related to receiving data. */
    struct {

        /*  The buffer being filled in at the moment. */
        uint8_t *buf;
        size_t len;

        /*  Buffer for batch-reading inbound data. */
        uint8_t *batch;

        /*  Size of the batch buffer. */
        size_t batch_len;

        /*  Current position in the batch buffer. The data preceding this
            position were already received by the user. The data that follow
            will be received in the future. */
        size_t batch_pos;
    } in;

    /*  Members related to sending data. */
    struct {

        /*  msghdr being sent at the moment. */
        struct msghdr hdr;

        /*  List of buffers being sent at the moment. Referenced from 'hdr'. */
        struct iovec iov [NN_USOCK_MAX_IOVCNT];
    } out;

    /*  Asynchronous tasks for the worker. */
    struct nn_worker_task wtask_connect;
    struct nn_worker_task wtask_connected;
    struct nn_worker_task wtask_accept;
    struct nn_worker_task wtask_send;
    struct nn_worker_task wtask_recv;
    struct nn_worker_task wtask_close;

    /*  Asynchronous callback tasks. */ 
    struct nn_ctx_task ctask_accepted;
    struct nn_ctx_task ctask_connected;
    struct nn_ctx_task ctask_sent;
    struct nn_ctx_task ctask_received;
    struct nn_ctx_task ctask_error;

    /*  When accepting a new connection, the pointer to the object to associate
        the new connection with is stored here. */
    struct nn_usock *newsock;
    struct nn_callback *newcallback;
};

int nn_usock_init (struct nn_usock *self, int domain, int type, int protocol,
    struct nn_ctx *ctx, struct nn_callback *callback);
void nn_usock_close (struct nn_usock *self);

int nn_usock_setsockopt (struct nn_usock *self, int level, int optname,
    const void *optval, size_t optlen);

int nn_usock_bind (struct nn_usock *self, const struct sockaddr *addr,
    size_t addrlen);
int nn_usock_listen (struct nn_usock *self, int backlog);
void nn_usock_accept (struct nn_usock *self, struct nn_usock *newsock,
    struct nn_callback *newcallback);
void nn_usock_connect (struct nn_usock *self, const struct sockaddr *addr,
    size_t addrlen);

void nn_usock_send (struct nn_usock *self, const struct nn_iovec *iov,
    int iovcnt);
void nn_usock_recv (struct nn_usock *self, void *buf, size_t len);

#endif


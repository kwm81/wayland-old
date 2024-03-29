/*
 * Copyright © 2012 Collabora, Ltd.
 * Copyright © 2012 Intel Corporation
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */

#define _GNU_SOURCE

#include "../config.h"

#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dlfcn.h>
#include <errno.h>
#include <stdarg.h>
#include <fcntl.h>
#include <stdio.h>

#ifdef HAVE_SYS_EPOLL_H
#include <sys/epoll.h>
#elif HAVE_SYS_EVENT_H
#include <sys/event.h>
#include <sys/types.h>
#endif

#include "wayland-private.h"
#include "test-runner.h"
#include "wayland-os.h"

static int fall_back;

static int (*real_socket)(int, int, int);
static int wrapped_calls_socket;

static int (*real_fcntl)(int, int, ...);
static int wrapped_calls_fcntl;

static ssize_t (*real_recvmsg)(int, struct msghdr *, int);
static int wrapped_calls_recvmsg;

#ifdef HAVE_SYS_EPOLL_H
static int (*real_epoll_create1)(int);
static int wrapped_calls_epoll_create1;
#elif HAVE_SYS_EVENT_H
static int (*real_kqueue)(void);
static int wrapped_calls_kqueue;
#endif

static void
init_fallbacks(int do_fallbacks)
{
	fall_back = do_fallbacks;
	real_socket = dlsym(RTLD_NEXT, "socket");
	real_fcntl = dlsym(RTLD_NEXT, "fcntl");
	real_recvmsg = dlsym(RTLD_NEXT, "recvmsg");
#ifdef HAVE_SYS_EPOLL_H
	real_epoll_create1 = dlsym(RTLD_NEXT, "epoll_create1");
#elif HAVE_SYS_EVENT_H
	real_kqueue = dlsym(RTLD_NEXT, "kqueue");
#endif
}

__attribute__ ((visibility("default"))) int
socket(int domain, int type, int protocol)
{
	wrapped_calls_socket++;

#ifdef SOCK_CLOEXEC
	if (fall_back && (type & SOCK_CLOEXEC)) {
		errno = EINVAL;
		return -1;
	}
#endif

	return real_socket(domain, type, protocol);
}

__attribute__ ((visibility("default"))) int
fcntl(int fd, int cmd, ...)
{
	va_list ap;
	void *arg;

	wrapped_calls_fcntl++;

#ifdef F_DUPFD_CLOEXEC
	if (fall_back && (cmd == F_DUPFD_CLOEXEC)) {
		errno = EINVAL;
		return -1;
	}
#endif

	va_start(ap, cmd);
	arg = va_arg(ap, void*);
	va_end(ap);

	return real_fcntl(fd, cmd, arg);
}

__attribute__ ((visibility("default"))) ssize_t
recvmsg(int sockfd, struct msghdr *msg, int flags)
{
	wrapped_calls_recvmsg++;

#ifdef MSG_CMSG_CLOEXEC
	if (fall_back && (flags & MSG_CMSG_CLOEXEC)) {
		errno = EINVAL;
		return -1;
	}
#endif

	return real_recvmsg(sockfd, msg, flags);
}

#ifdef HAVE_SYS_EPOLL_H
__attribute__ ((visibility("default"))) int
epoll_create1(int flags)
{
	wrapped_calls_epoll_create1++;

	if (fall_back) {
		wrapped_calls_epoll_create1++; /* epoll_create() not wrapped */
		errno = EINVAL;
		return -1;
	}

	return real_epoll_create1(flags);
}
#elif HAVE_SYS_EVENT_H
__attribute__ ((visibility("default"))) int
kqueue(void)
{
       wrapped_calls_kqueue++;

       return real_kqueue();
}
#endif

static void
do_os_wrappers_socket_cloexec(int n)
{
	int fd;
	int nr_fds;

	nr_fds = count_open_fds();

	/* simply create a socket that closes on exec */
	fd = wl_os_socket_cloexec(PF_LOCAL, SOCK_STREAM, 0);
	assert(fd >= 0);

	/*
	 * Must have 2 calls if falling back, but must also allow
	 * falling back without a forced fallback.
	 */
	assert(wrapped_calls_socket > n);

	exec_fd_leak_check(nr_fds);
}

TEST(os_wrappers_socket_cloexec)
{
	/* normal case */
	init_fallbacks(0);
	do_os_wrappers_socket_cloexec(0);
}

#ifdef SOCK_CLOEXEC
TEST(os_wrappers_socket_cloexec_fallback)
{
	/* forced fallback */
	init_fallbacks(1);
	do_os_wrappers_socket_cloexec(1);
}
#endif

static void
do_os_wrappers_dupfd_cloexec(int n)
{
	int base_fd;
	int fd;
	int nr_fds;

	nr_fds = count_open_fds();

	base_fd = socket(PF_LOCAL, SOCK_STREAM, 0);
	assert(base_fd >= 0);

	fd = wl_os_dupfd_cloexec(base_fd, 13);
	assert(fd >= 13);

	close(base_fd);

	/*
	 * Must have 4 calls if falling back, but must also allow
	 * falling back without a forced fallback.
	 */
	assert(wrapped_calls_fcntl > n);

	exec_fd_leak_check(nr_fds);
}

TEST(os_wrappers_dupfd_cloexec)
{
	init_fallbacks(0);
	do_os_wrappers_dupfd_cloexec(0);
}

#ifdef F_DUPFD_CLOEXEC
TEST(os_wrappers_dupfd_cloexec_fallback)
{
	init_fallbacks(1);
	do_os_wrappers_dupfd_cloexec(3);
}
#endif

struct marshal_data {
	struct wl_connection *read_connection;
	struct wl_connection *write_connection;
	int s[2];
	uint32_t read_mask;
	uint32_t write_mask;
	union {
		int h[3];
	} value;
	int nr_fds_begin;
	int nr_fds_conn;
	int wrapped_calls;
};

static void
setup_marshal_data(struct marshal_data *data)
{
	assert(wl_os_socketpair_cloexec(AF_UNIX, SOCK_STREAM, 0, data->s) == 0);

	data->read_connection = wl_connection_create(data->s[0]);
	assert(data->read_connection);

	data->write_connection = wl_connection_create(data->s[1]);
	assert(data->write_connection);
}

static void
marshal_demarshal(struct marshal_data *data, 
		  void (*func)(void), int size, const char *format, ...)
{
	struct wl_closure *closure;
	static const int opcode = 4444;
	static struct wl_object sender = { NULL, NULL, 1234 };
	struct wl_message message = { "test", format, NULL };
	struct wl_map objects;
	struct wl_object object = { NULL, &func, 1234 };
	va_list ap;
	uint32_t msg[1] = { 1234 };

	va_start(ap, format);
	closure = wl_closure_vmarshal(&sender, opcode, ap, &message);
	va_end(ap);

	assert(closure);
	assert(wl_closure_send(closure, data->write_connection) == 0);
	wl_closure_destroy(closure);
	assert(wl_connection_flush(data->write_connection) == size);

	assert(wl_connection_read(data->read_connection) == size);

	wl_map_init(&objects, WL_MAP_SERVER_SIDE);
	object.id = msg[0];
	closure = wl_connection_demarshal(data->read_connection,
					  size, &objects, &message);
	assert(closure);
	wl_closure_invoke(closure, WL_CLOSURE_INVOKE_SERVER, &object, 0, data);
	wl_closure_destroy(closure);
}

static void
validate_recvmsg_h(struct marshal_data *data,
		   struct wl_object *object, int fd1, int fd2, int fd3)
{
	struct stat buf1, buf2;

	assert(fd1 >= 0);
	assert(fd2 >= 0);
	assert(fd3 >= 0);

	assert(fd1 != data->value.h[0]);
	assert(fd2 != data->value.h[1]);
	assert(fd3 != data->value.h[2]);

	assert(fstat(fd3, &buf1) == 0);
	assert(fstat(data->value.h[2], &buf2) == 0);
	assert(buf1.st_dev == buf2.st_dev);
	assert(buf1.st_ino == buf2.st_ino);

	/* close the original file descriptors */
	close(data->value.h[0]);
	close(data->value.h[1]);
	close(data->value.h[2]);

	/* the dup'd (received) fds should still be open */
	assert(count_open_fds() == data->nr_fds_conn + 3);

	/*
	 * Must have 2 calls if falling back, but must also allow
	 * falling back without a forced fallback.
	 */
	assert(wrapped_calls_recvmsg > data->wrapped_calls);

	if (data->wrapped_calls == 0 && wrapped_calls_recvmsg > 1)
		printf("recvmsg fell back unforced.\n");

	/* all fds opened during the test in any way should be gone on exec */
	exec_fd_leak_check(data->nr_fds_begin);
}

static void
do_os_wrappers_recvmsg_cloexec(int n)
{
	struct marshal_data data;

	data.nr_fds_begin = count_open_fds();
	data.wrapped_calls = n;

	setup_marshal_data(&data);
	data.nr_fds_conn = count_open_fds();

	assert(pipe(data.value.h) >= 0);

	data.value.h[2] = open("/dev/zero", O_RDONLY);
	assert(data.value.h[2] >= 0);

	marshal_demarshal(&data, (void *) validate_recvmsg_h,
			  8, "hhh", data.value.h[0], data.value.h[1],
			  data.value.h[2]);
}

TEST(os_wrappers_recvmsg_cloexec)
{
	init_fallbacks(0);
	do_os_wrappers_recvmsg_cloexec(0);
}

#ifdef MSG_CMSG_CLOEXEC
TEST(os_wrappers_recvmsg_cloexec_fallback)
{
	init_fallbacks(1);
	do_os_wrappers_recvmsg_cloexec(1);
}
#endif

static void
do_os_wrappers_epoll_create_cloexec(int n)
{
	int fd;
	int nr_fds;

	nr_fds = count_open_fds();

#ifdef HAVE_SYS_EPOLL_H
	fd = wl_os_epoll_create_cloexec();
#elif HAVE_SYS_EVENT_H
	fd = wl_os_kqueue_create_cloexec();
#endif
	assert(fd >= 0);

#ifdef EPOLL_CLOEXEC
#ifdef HAVE_SYS_EPOLL_H
	assert(wrapped_calls_epoll_create1 == n);
#else
	assert(wrapped_calls_kqueue == n);
#endif
#else
	printf("No epoll_create1.\n");
#endif

	exec_fd_leak_check(nr_fds);
}

TEST(os_wrappers_epoll_create_cloexec)
{
	init_fallbacks(0);
	do_os_wrappers_epoll_create_cloexec(1);
}

TEST(os_wrappers_epoll_create_cloexec_fallback)
{
	init_fallbacks(1);
	do_os_wrappers_epoll_create_cloexec(2);
}

/* FIXME: add tests for wl_os_accept_cloexec() */

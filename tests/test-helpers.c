/*
 * Copyright © 2012 Collabora, Ltd.
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

#include "../config.h"

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#include <assert.h>
#include <errno.h>
#include <dirent.h>
#include <stdio.h>
#include <unistd.h>

#include "test-runner.h"

#ifdef __FreeBSD__
/* FreeBSD uses fdescfs (which must be mounted using:
 *    mount -t fdescfs fdescfs /dev/fd
 * before the test suite can be run). */
#define OPEN_FDS_DIR "/dev/fd"
#else
/* Linux. */
#define OPEN_FDS_DIR "/proc/self/fd"
#endif

int
count_open_fds(void)
{
	DIR *dir;
	struct dirent *ent;
	int count = 0;

	dir = opendir(OPEN_FDS_DIR);
	assert(dir && "opening " OPEN_FDS_DIR " failed.");

	errno = 0;
	while ((ent = readdir(dir))) {
		const char *s = ent->d_name;
		if (s[0] == '.' && (s[1] == 0 || (s[1] == '.' && s[2] == 0)))
			continue;
		count++;
	}
	assert(errno == 0 && "reading " OPEN_FDS_DIR " failed.");

	closedir(dir);

	return count;
}

void
exec_fd_leak_check(int nr_expected_fds)
{
	const char *exe = "./exec-fd-leak-checker";
	char number[16] = { 0 };

	snprintf(number, sizeof number - 1, "%d", nr_expected_fds);
	execl(exe, exe, number, (char *)NULL);
	assert(0 && "execing fd leak checker failed");
}

/*
 * Copyright (C) 2025 Intel Corporation.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/queue.h>
#include <sys/epoll.h>
#include <sys/un.h>
#include <sys/socket.h>
#include "vdmabuf_protocal.h"
#include "vdmabuf_common.h"

static bool enable_debug = false;

void enable_debug_log(bool debug)
{
        enable_debug = debug;
}

void log_print(bool debug, FILE *stream, char *fmt, ...)
{
        if (debug && !enable_debug)
                return;

        va_list argl;
        va_start(argl, fmt);
        vfprintf(stream, fmt, argl);
        va_end(argl);
}

int send_command(int socket, void *header, int header_len, void *data, int data_len, int fd)
{
	struct msghdr msg = { 0 };
	struct iovec io[1];
	struct cmsghdr *cmsg;
	char buf[CMSG_SPACE(sizeof(int))];
	int ret;
	if (!header || !header_len)
		return -1;

	ret = send(socket, header, header_len, 0);
	if (ret != header_len) {
		ERROR("Failed to send message: %s\n", strerror(errno));
		return -1;
	}

	if (data == NULL || data_len == 0)
		return 0;

	memset(buf, 0, sizeof(buf));

	io[0].iov_base = data;
	io[0].iov_len = data_len;

	msg.msg_iov = io;
	msg.msg_iovlen = 1;

	if (fd >= 0) {
		msg.msg_control = buf;
		msg.msg_controllen = sizeof(buf);

		cmsg = CMSG_FIRSTHDR(&msg);
		cmsg->cmsg_level = SOL_SOCKET;
		cmsg->cmsg_type = SCM_RIGHTS;
		cmsg->cmsg_len = CMSG_LEN(sizeof(int));

		memcpy(CMSG_DATA(cmsg), &fd, sizeof(int));
	} else {
		msg.msg_control = NULL;
		msg.msg_controllen = 0;
	}

	ret = sendmsg(socket, &msg, 0);
	if (ret < 0) {
		ERROR("Failed to send message: %s\n", strerror(errno));
		return -1;
	}
	return 0;
}

int recv_resp(int socket, struct vdmabuf_resp_hdr *hdr, void *data_out, int data_len, int *fd_out)
{
	int fd = -1;
	char *data = NULL;
	int len;
	int ret = 0;
	struct msghdr msg = {0};
	struct cmsghdr *cmsg = NULL;
	struct iovec io[1];
	char ctrl_buf[CMSG_SPACE(sizeof(fd))];

	if (!hdr)
		return -1;
	if (data_len > 0 && !data_out)
		return -1;
	ret = recv(socket, hdr, sizeof(*hdr), 0);
	if (ret <= 0) {
		ERROR("socket read failure\n, ret:%d\n", ret);
	}

	DEBUG("req, cmd:%d, data_size:%d, fd_count:%d\n", hdr->type, hdr->data_size, hdr->fd_count);
	if (hdr->data_size > 0 || hdr->fd_count > 0) {
		data = malloc(hdr->data_size);
		if (!data) {
			ERROR("malloc failure in recv_resp\n");
			return -1;
		}
		io[0].iov_base = data;
		io[0].iov_len = hdr->data_size;
		msg.msg_iov = io;
		msg.msg_iovlen = 1;
		msg.msg_control = ctrl_buf;
		msg.msg_controllen = sizeof(ctrl_buf);
		cmsg = CMSG_FIRSTHDR(&msg);
		ret = recvmsg(socket, &msg, 0);
		if (ret < 0) {
			ERROR("socket read failure ret:%d\n", ret);
			free(data);
			return -1;
		}
		if (hdr->fd_count > 0) {
			fd = *((int *)CMSG_DATA(cmsg));
			if (fd < 0) {
				ERROR("recv_resp, recv fd:%d\n", fd);
				free(data);
				return -1;
			}
		}
	}
	if (fd_out)
		*fd_out = fd;
	if (data_out && data) {
		if (data_len > hdr->data_size)
			len = hdr->data_size;
		else
			len = data_len;
		memcpy(data_out, data, len);
	}
	return 0;
}

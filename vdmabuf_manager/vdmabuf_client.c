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
#include "vdmabuf_client.h"

static int vdmabuf_attach(int fd, const char *name)
{
	struct vdmabuf_req_hdr req = {0};
	struct vdmabuf_resp_hdr resp = {0};
	struct attach_resp_data resp_data = {0};
	struct attach_req_data attach_data = {0};
	int ret = 0;
	if (fd < 0 || !name) {
		return -1;
	}
	req.type = VDMABUF_ATTACH;
	req.data_size = sizeof(attach_data);
	req.fd_count = 0;
	memcpy(attach_data.name, name, MAX_VM_NAME_LEN);

	ret = send_command(fd, &req, sizeof(req), &attach_data, req.data_size, NO_FD);
	if (ret < 0)
		return -1;
	ret = recv_resp(fd, &resp, &resp_data, sizeof(resp_data), NULL);
	if (ret < 0 || resp_data.status != VDMABUF_SUCCESS)
		return -1;
	return 0;
}

int vdmabuf_alloc(int fd, int size)
{
	struct vdmabuf_req_hdr req = {0};
	struct alloc_req_data alloc_data = {0};
	struct vdmabuf_resp_hdr resp = {0};
	struct alloc_resp_data resp_data = {0};
	int fd_out = -1;
	int ret = 0;
	if (fd < 0 || size <= 0) {
		return -1;
	}
	if (size % 4096)
		alloc_data.size = (size / 4096 +1 ) * 4096;
	else
		alloc_data.size = size;
	req.type = VDMABUF_ALLOC;
	req.data_size = sizeof(alloc_data);
	req.fd_count = 0;

	ret = send_command(fd, &req, sizeof(req), &alloc_data , req.data_size, NO_FD);
	if (ret < 0)
		return -1;
	ret = recv_resp(fd, &resp, &resp_data, sizeof(resp_data), &fd_out);
	if (ret < 0 || resp_data.status != VDMABUF_SUCCESS)
		return -1;
	return fd_out;
}

int vdmabuf_export(int fd, int buf_fd, virtio_vdmabuf_buf_id_t *buf_id)
{
	struct vdmabuf_req_hdr req = {0};
	struct export_req_data export_data = {0};
	struct vdmabuf_resp_hdr resp = {0};
	struct export_resp_data resp_data = {0};
	int ret = 0;
	if (fd < 0 || buf_fd < 0 || !buf_id) {
		return -1;
	}
	req.type = VDMABUF_EXPORT;
	req.data_size = sizeof (export_data);
	req.fd_count = 1;

	ret = send_command(fd, &req, sizeof(req), &export_data , req.data_size, buf_fd);
	if (ret < 0)
		return -1;
	ret = recv_resp(fd, &resp, &resp_data, sizeof(resp_data), NULL);
	if (ret < 0 || resp_data.status != VDMABUF_SUCCESS)
		return -1;
	memcpy(buf_id, &resp_data.buf_id, sizeof(*buf_id));
	return 0;
}

int vdmabuf_unexport(int fd, virtio_vdmabuf_buf_id_t *buf_id)
{
	struct vdmabuf_req_hdr req = {0};
	struct unexport_req_data unexport_data = {0};
	struct vdmabuf_resp_hdr resp = {0};
	struct unexport_resp_data resp_data = {0};
	int ret = 0;
	if (fd < 0 || !buf_id) {
		return -1;
	}
	memcpy(&unexport_data.buf_id, buf_id, sizeof(*buf_id));
	req.type = VDMABUF_UNEXPORT;
	req.data_size = sizeof(unexport_data);
	req.fd_count = 0;

	ret = send_command(fd, &req, sizeof(req), &unexport_data , req.data_size, NO_FD);
	if (ret < 0)
		return -1;
	ret = recv_resp(fd, &resp, &resp_data, sizeof(resp_data), NULL);
	if (ret < 0 || resp_data.status != VDMABUF_SUCCESS)
		return -1;
	return 0;
}

int vdmabuf_import(int fd, virtio_vdmabuf_buf_id_t *buf_id)
{
	struct vdmabuf_req_hdr req = {0};
	struct import_req_data import_data = {0};
	struct vdmabuf_resp_hdr resp = {0};
	struct import_resp_data resp_data = {0};
	int fd_out = -1;
	int ret = 0;
	if (fd < 0 || !buf_id) {
		return -1;
	}
	memcpy(&import_data.buf_id, buf_id, sizeof(*buf_id));
	req.type = VDMABUF_IMPORT;
	req.data_size = sizeof(import_data);
	req.fd_count = 0;

	ret = send_command(fd, &req, sizeof(req), &import_data , req.data_size, NO_FD);
	if (ret < 0)
		return -1;
	ret = recv_resp(fd, &resp, &resp_data, sizeof(resp_data), &fd_out);
	if (ret < 0 || resp_data.status != VDMABUF_SUCCESS)
		return -1;
	return fd_out;
}

int connect_vdmabuf_server(const char *name, bool backend)
{
	struct sockaddr_un sa = {0};
	int socket_fd = 0;
	int ret = 0;
	char *path = NULL;

	if (backend) {
		path = DEFAULT_VDMABUF_BE_PATH;
	} else {
		path = DEFAULT_VDMABUF_FE_PATH;
	}
	sa.sun_family = AF_UNIX;
	memcpy(sa.sun_path, path, sizeof(sa.sun_path));
	socket_fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
	if (socket_fd < 0) {
		ERROR("fail to set up socket :%s\n",strerror(errno));
		return -1;
	}
	ret = connect(socket_fd, (struct sockaddr *)&sa, sizeof(struct sockaddr_un));
	if (ret < 0) {
		ERROR("fail to connect server :%s\n",strerror(errno));
		return -1;
	}
	DEBUG("connect server success\n");
	if (name && strlen(name) > 0) {
		ret = vdmabuf_attach(socket_fd, name);
		if (ret < 0) {
			close(socket_fd);
			socket_fd = 0;
		}
	}

	return socket_fd;
}

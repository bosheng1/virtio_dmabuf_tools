/*
 * Copyright (C) 2025 Intel Corporation.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "vdmabuf_ioctl.h"

int ioctl_alloc(int fd, uint32_t size)
{
	struct virtio_vdmabuf_alloc attr = {0};
	int ret = 0;
	if (fd < 0 || size == 0) {
		return -1;
	}
	attr.size = size;
	ret = ioctl(fd, VIRTIO_VDMABUF_IOCTL_ALLOC_FD, &attr);
	if (ret < 0) {
		return ret;
	}
	return attr.fd;
}

int ioctl_attach(int fd, char *name)
{
	struct virtio_vdmabuf_attach attr = {0};
	int ret = 0;
	if (fd < 0 || !name) {
		return -1;
	}
	memcpy(attr.name, name, MAX_VM_NAME_LEN);
	ret = ioctl(fd, VIRTIO_VDMABUF_IOCTL_ATTACH, &attr);
	if (ret < 0) {
		return ret;
	}
	return ret;
}

int ioctl_export(int fd, int bo_fd, int priv_size, char *priv, virtio_vdmabuf_buf_id_t *buf_id)
{
	struct virtio_vdmabuf_export attr = {0};
	int ret = 0;
	if (fd < 0 || bo_fd < 0 || !buf_id)
		return -1;
	if (priv_size > 0 && !priv)
		return -1;
	attr.fd = bo_fd;
	attr.sz_priv = priv_size;
	attr.priv = priv;

	ret = ioctl(fd, VIRTIO_VDMABUF_IOCTL_EXPORT, &attr);
	if (ret < 0) {
		return ret;
	}
	memcpy(buf_id, &attr.buf_id, sizeof(virtio_vdmabuf_buf_id_t));
	return ret;
}

int ioctl_unexport(int fd, virtio_vdmabuf_buf_id_t *buf_id)
{
	struct virtio_vdmabuf_unexport attr = {0};
	int ret = 0;
	if (fd < 0 || !buf_id)
		return -1;
	memcpy(&attr.buf_id, buf_id, sizeof(virtio_vdmabuf_buf_id_t));

	ret = ioctl(fd, VIRTIO_VDMABUF_IOCTL_UNEXPORT, &attr);
	if (ret < 0) {
		return ret;
	}
	return ret;
}

int ioctl_import(int fd, virtio_vdmabuf_buf_id_t *buf_id)
{
	struct virtio_vdmabuf_import attr = {0};
	int ret = 0;
	if (fd < 0 || !buf_id)
		return -1;
	memcpy(&attr.buf_id, buf_id, sizeof(virtio_vdmabuf_buf_id_t));

	ret = ioctl(fd, VIRTIO_VDMABUF_IOCTL_IMPORT, &attr);
	if (ret < 0) {
		return ret;
	}
	return attr.fd;
}

int ioctl_query_size(int fd, virtio_vdmabuf_buf_id_t *buf_id, int *size)
{
	struct virtio_vdmabuf_query_bufinfo attr = {0};
	int ret = 0;
	if (fd < 0 || !buf_id || !size)
		return -1;
	memcpy(&attr.buf_id, buf_id, sizeof(virtio_vdmabuf_buf_id_t));

	attr.subcmd = VIRTIO_VDMABUF_QUERY_SIZE;
	ret = ioctl(fd, VIRTIO_VDMABUF_IOCTL_QUERY_BUFINFO, &attr);
	if (ret < 0) {
		return ret;
	}
	*size = attr.info;

	return ret;
}

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
#include <sys/epoll.h>
#include <sys/un.h>
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

int vdmabuf_open(vdmabuf_init_parameter setup)
{
        int ret = 0;
        int fd = -1;
        if (!setup.guest) {
		fd= open("/dev/virtio-vdmabuf-be", O_RDWR);
		if (fd <= 0) {
			ERROR("can not open virtio-vdmabuf-be");
			return -1;
		}
                ret = ioctl_attach(fd, setup.vm);
		if (ret < 0) {
			ERROR("failed to attach VM:%s, ret:%d", setup.vm, ret);
			return ret;
		}
	} else {
		fd= open("/dev/virtio-vdmabuf", O_RDWR);
		if (fd <= 0) {
			ERROR("can not open virtio-vdmabuf");
			return -1;
		}
	}
}

void vdmabuf_close(int handle)
{
        close(handle);
}

int vdmabuf_alloc(int handle, size_t size, int *bo_fd)
{
        int ret = 0;

        if (handle < 0 || size <=0 || !bo_fd) {
                return -EINVAL;
        }
        ret = ioctl_alloc(handle, size);
        if (ret < 0) {
                return ret;
        } else {
                *bo_fd = ret;
                return 0;
        }
}

int vdmabuf_export(int handle, int bo_fd, virtio_vdmabuf_buf_id_t *buf_id)
{
        int ret = 0;

        if (handle < 0 || bo_fd < 0 || !buf_id) {
                return -EINVAL;
        }
        ret = ioctl_export(handle, bo_fd, 0, NULL, buf_id);
        return ret;
}

int vdmabuf_unexport(int handle, virtio_vdmabuf_buf_id_t *buf_id)
{
        int ret = 0;

        if (handle < 0 || !buf_id) {
                return -EINVAL;
        }
        ret = ioctl_unexport(handle, buf_id);
        return ret;
}

int vdmabuf_import(int handle, virtio_vdmabuf_buf_id_t *buf_id, int *bo_fd)
{
        int ret = 0;

        if (handle < 0 || !buf_id || !bo_fd) {
                return -EINVAL;
        }
        ret = ioctl_import(handle, buf_id);
        if (ret < 0) {
                return ret;
        } else {
                *bo_fd = ret;
                return 0;
        }
}


/*
 * Copyright (C) 2025 Intel Corporation.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#ifndef VDMABUF_CLIENT_H
#define VDMABUF_CLIENT_H
#include "vdmabuf_protocal.h"

int connect_vdmabuf_server(const char *name, bool backend);
int vdmabuf_import(int fd, virtio_vdmabuf_buf_id_t *buf_id);
int vdmabuf_unexport(int fd, virtio_vdmabuf_buf_id_t *buf_id);
int vdmabuf_export(int fd, int buf_fd, virtio_vdmabuf_buf_id_t *buf_id);
int vdmabuf_alloc(int fd, int size);

#endif

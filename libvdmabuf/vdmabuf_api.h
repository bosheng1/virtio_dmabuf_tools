/*
 * Copyright (C) 2025 Intel Corporation.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#ifndef VDMABUF_API_H
#define VDMABUF_API_H

#define MAX_SIZE_PRIV_DATA 192
#define MAX_VM_NAME_LEN 16

typedef struct {
	uint64_t id;
	int rng_key[2];
} virtio_vdmabuf_buf_id_t;

typedef struct {
	bool guest;
	char vm[MAX_VM_NAME_LEN];
} vdmabuf_init_parameter;

int vdmabuf_open(vdmabuf_init_parameter setup);
void vdmabuf_close(int handle);
int vdmabuf_alloc(int handle, size_t size, int *bo_fd);
int vdmabuf_export(int handle, int bo_fd, virtio_vdmabuf_buf_id_t *buf_id);
int vdmabuf_unexport(int handle, virtio_vdmabuf_buf_id_t *buf_id);
int vdmabuf_import(int handle, virtio_vdmabuf_buf_id_t *buf_id, int *bo_fd);

#endif

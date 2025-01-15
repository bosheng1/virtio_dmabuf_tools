/*
 * Copyright (C) 2025 Intel Corporation.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#ifndef VDMABUF_PROTOCOL_H
#define VDMABUF_PROTOCOL_H

#define DEFAULT_VDMABUF_FE_PATH "/dev/socket/vdmabuf"
#define DEFAULT_VDMABUF_BE_PATH "/tmp/vdmabuf"

#define MAX_SIZE_PRIV_DATA 192
#define MAX_VM_NAME_LEN 16

typedef struct {
	uint64_t id;
	int rng_key[2];
} virtio_vdmabuf_buf_id_t;

struct virtio_vdmabuf_e_hdr {
	virtio_vdmabuf_buf_id_t buf_id;
	int size;
};

struct virtio_vdmabuf_e_data {
	struct virtio_vdmabuf_e_hdr hdr;
	void  *data;
};

enum vdmabuf_type {
	VDMABUF_ALLOC = 0x01,
	VDMABUF_EXPORT,
	VDMABUF_UNEXPORT,
	VDMABUF_ATTACH,
	VDMABUF_IMPORT,
	VDMABUF_QUERY,
	VDMABUF_IMPORT_EVENT,
};

struct vdmabuf_req_hdr {
	uint32_t type;
	uint32_t data_size;
	uint32_t fd_count;
};

struct vdmabuf_resp_hdr {
	uint32_t type;
	uint32_t data_size;
	uint32_t fd_count;
};

enum cmd_status {
	VDMABUF_SUCCESS = 0x0,
	VDMABUF_FAILURE,
};

struct alloc_req_data {
	int size;
};

struct alloc_resp_data {
	int status;
};

struct export_req_data {
	int reserve;
};

struct export_resp_data {
	int status;
	virtio_vdmabuf_buf_id_t buf_id;
};

struct unexport_req_data {
	virtio_vdmabuf_buf_id_t buf_id;
};

struct unexport_resp_data {
	int status;
};

struct import_req_data {
	virtio_vdmabuf_buf_id_t buf_id;
};

struct import_resp_data {
	int status;
};

struct attach_req_data {
	char name[MAX_VM_NAME_LEN];
};

struct attach_resp_data {
	int status;
};

struct query_req_data {
	virtio_vdmabuf_buf_id_t buf_id;
};

struct query_resp_data {
	int status;
	int size;
	int priv_size;
	uint8_t priv[];
};
#endif

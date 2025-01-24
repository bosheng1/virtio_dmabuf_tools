/*
 * Copyright (C) 2025 Intel Corporation.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#ifndef VDMABUF_IOCTL_H
#define VDMABUF_IOCTL_H

#define MAX_SIZE_PRIV_DATA 192
#define MAX_VM_NAME_LEN 16

#define VDMABUF_PRODUCER 0x1
#define VDMABUF_CONSUMER 0x2

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

#define VIRTIO_VDMABUF_IOCTL_IMPORT \
_IOC(_IOC_NONE, 'G', 2, sizeof(struct virtio_vdmabuf_import))

struct virtio_vdmabuf_import {
	virtio_vdmabuf_buf_id_t buf_id;
	int flags;
	int fd;
};

#define VIRTIO_VDMABUF_IOCTL_EXPORT \
_IOC(_IOC_NONE, 'G', 4, sizeof(struct virtio_vdmabuf_export))
struct virtio_vdmabuf_export {
	int fd;
	virtio_vdmabuf_buf_id_t buf_id;
	int sz_priv;
	char *priv;
};

#define VIRTIO_VDMABUF_IOCTL_ALLOC_FD \
_IOC(_IOC_NONE, 'G', 5, sizeof(struct virtio_vdmabuf_alloc))
struct virtio_vdmabuf_alloc {
	uint32_t size;
	int fd;
};

#define VHOST_VDMABUF_SET_ID \
_IOC(_IOC_NONE, 'G', 6, sizeof(struct vhost_vdmabuf_set))
struct vhost_vdmabuf_set {
	uint64_t vmid;
	char name[MAX_VM_NAME_LEN];
};

#define VIRTIO_VDMABUF_IOCTL_ATTACH \
_IOC(_IOC_NONE, 'G', 7, sizeof(struct virtio_vdmabuf_attach))
struct virtio_vdmabuf_attach {
	char name[MAX_VM_NAME_LEN];
};

#define VIRTIO_VDMABUF_IOCTL_ROLE \
_IOC(_IOC_NONE, 'G', 3, sizeof(struct virtio_vdmabuf_role))
struct virtio_vdmabuf_role {
	int role;
};
#define VIRTIO_VDMABUF_IOCTL_SHARED_MEM \
_IOC(_IOC_NONE, 'G', 8, sizeof(struct virtio_vdmabuf_smem))
struct virtio_vdmabuf_smem {
	uint64_t hva;
	uint64_t gpa;
	uint64_t size;
};

#define VIRTIO_VDMABUF_IOCTL_UNEXPORT \
_IOC(_IOC_NONE, 'G', 9, sizeof(struct virtio_vdmabuf_unexport))
struct virtio_vdmabuf_unexport {
	virtio_vdmabuf_buf_id_t buf_id;
};

enum virtio_vdmabuf_query_cmd {
	VIRTIO_VDMABUF_QUERY_SIZE = 0x10,
	VIRTIO_VDMABUF_QUERY_PRIV_INFO_SIZE,
	VIRTIO_VDMABUF_QUERY_PRIV_INFO,
};

#define VIRTIO_VDMABUF_IOCTL_QUERY_BUFINFO \
_IOC(_IOC_NONE, 'G', 10, sizeof(struct virtio_vdmabuf_query_bufinfo))
struct virtio_vdmabuf_query_bufinfo {
	virtio_vdmabuf_buf_id_t buf_id;
	int subcmd;
	unsigned long info;
};
#endif

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

/**
 * @brief Buffer ID represents dmabuf
 */
typedef struct {
	uint64_t id;
	int rng_key[2];
} virtio_vdmabuf_buf_id_t;

/**
 * @brief vdmabuf init parameter
 * @param[in] guest it's runing on Guest OS when guest is true, runing on SOS when guest is false
 * @param[in] vm  VM name, e.g VM1, it's needed on SOS
 */
typedef struct {
	bool guest;
	char vm[MAX_VM_NAME_LEN];
} vdmabuf_init_parameter;

/**
 * @brief open vdmabuf device and return handle, called by both producer and consumer.
 * @param[in]  setup parameter for init
 * @return handle used in following operation
 */
int vdmabuf_open(vdmabuf_init_parameter setup);

/**
 * @brief close vdmabuf device, called by both producer and consumer.
 * @param[in]  handle clean up specified handle
 */
void vdmabuf_close(int handle);

/**
 * @brief alloc buffer, called by producer.
 * @param[in]  handle  specified vdmabuf
 * @param[in]  size    size of buffer
 * @param[out]  bo_fd   dma buffer fd
 * @return  int  error when ret is less than zero
 */
int vdmabuf_alloc(int handle, size_t size, int *bo_fd);

/**
 * @brief free buffer, called by producer.
 * @param[in]  handle  specified vdmabuf
 * @param[in]  bo_fd   dma buffer fd
 * @return  int  error when ret is less than zero
 */
void vdmabuf_free(int handle, int bo_fd);

/**
 * @brief export buffer to remote, called by producer.
 * @param[in]  handle  specified vdmabuf
 * @param[in]  bo_fd   dma buffer fd
 * @param[out]  buf_id   buffer ID of bo_fd
 * @return  int  error when ret is less than zero
 */
int vdmabuf_export(int handle, int bo_fd, virtio_vdmabuf_buf_id_t *buf_id);

/**
 * @brief unexport buffer when shared buffer is not used used, called by producer.
 * @param[in]  handle  specified vdmabuf
 * @param[in]  buf_id   buffer ID
 * @return  int  error when ret is less than zero
 */
int vdmabuf_unexport(int handle, virtio_vdmabuf_buf_id_t *buf_id);

/**
 * @brief import buffer from remote, called by consumer.
 * @param[in]  handle  specified vdmabuf
 * @param[in]  buf_id  buffer ID
 * @param[out]  bo_fd  imported dmabuf fd
 * @return  int  error when ret is less than zero
 */
int vdmabuf_import(int handle, virtio_vdmabuf_buf_id_t *buf_id, int *bo_fd);

/**
 * @brief query buffer size.
 * @param[in]  handle  specified vdmabuf
 * @param[in]  buf_id  buffer ID
 * @param[out]  size   size for buffer
 * @return  int  error when ret is less than zero
 */
int vdmabuf_query_size(int handle, virtio_vdmabuf_buf_id_t *buf_id, int *size);

#endif

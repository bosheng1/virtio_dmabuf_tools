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
#include <sys/mman.h>
#include "vdmabuf_protocal.h"
#include "vdmabuf_common.h"
#include "vdmabuf_client.h"

int main(int argc, char **argv)
{
	char name[128] = {0};
	int ret = 0;
	int socket_fd = -1;
	int bo_fd = -1;
	bool backend = false;
	unsigned char bufid[16] = {0};
	int size = 1024*1024*10;
	virtio_vdmabuf_buf_id_t buf_id;
	if (argc > 1) {
		backend = true;
		memcpy(name, argv[1], 128);
		printf("backend, connect to VM:%s\n", name);
	}
	socket_fd = connect_vdmabuf_server(name, backend);
	if (socket_fd < 0) {
		printf("can not connect vdmabuf manager\n");
		return -1;
	}

	bo_fd = vdmabuf_alloc(socket_fd, size);
	if (bo_fd < 0) {
		printf("alloc buffer failure\n");
	}

	int *addr = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, bo_fd, 0);
	if (!addr) {
		printf("failed to mmap fd");
		return -1;
	}
	for (int i = 0; i < size /4 ; i++) {
		addr[i] = i;
	}

	ret = vdmabuf_export(socket_fd, bo_fd, &buf_id);
	if (ret < 0) {
		printf("export buffer failure\n");
	}
	memcpy(bufid, &buf_id, sizeof(bufid));
	printf("export buf size is:%d buf_id is: ", size);
	for (int i = 0; i < 16; i++) {
		printf("%d ",  bufid[i]);
	}
	printf("\n");
	sleep(30);
	ret = vdmabuf_unexport(socket_fd, &buf_id);
	if (ret < 0) {
		printf("unexport buffer failure\n");
	}
	sleep(3);
}

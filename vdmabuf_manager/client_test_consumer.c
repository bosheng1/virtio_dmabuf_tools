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

static void usage(const char *progname) {
	printf("Usage: %s [OPTIONS]\n"
			"Options:\n"
			"-h, --help \tPrint this help\n"
			"-i, --id \tbuf id\n"
			"-s, --size \tbuf size\n"
			"-m, --vm \tspecify vm name to monitor\n",
			progname);
}

const char *opts = "hism:";
const struct option options[] = {
	{"help", no_argument, NULL, 'h'},
	{"id", required_argument, NULL, 'i'},
	{"size", required_argument, NULL, 's'},
	{"vm", required_argument, NULL, 'm'},
	{NULL, 0, NULL, 0},
};

int main(int argc, char **argv)
{
	int ret = 0;
	char *vm_name = NULL;
	char *id = NULL;
	int bufid_num = 0;
	bool backend = true;
	char *tmp;
	virtio_vdmabuf_buf_id_t buf_id;
	unsigned char buf_id_recv[16] = {0};
	int socket_fd = -1;
	char c;
	int size;

	while ((c = getopt_long(argc, argv, opts, options, NULL)) != -1) {
		int ret = EXIT_FAILURE;
		switch (c) {
			case 'm':
				vm_name = optarg;
				printf("vm, optarg:%s\n", optarg);
				break;
			case 'i':
				id = optarg;
				printf("vm, buf id:%s\n", id);
				while ((tmp = strsep(&id, " ")) != NULL) {
					buf_id_recv[bufid_num] = atoi(tmp);
					bufid_num++;
					if (bufid_num == 16) {
						break;
					}
				}
				break;
			case 's':
				size = atoi(optarg);
				printf("vm, size:%d\n", size);
				break;
			case 'h':
				ret = EXIT_SUCCESS;
				/* fall through */
			default:
				usage(argv[0]);
				return ret;
		}
	}
	if (bufid_num != 16)
		return -1;
	memcpy(&buf_id, buf_id_recv, 16);
	if (vm_name)
		backend = true;
	else
		backend = false;
	socket_fd = connect_vdmabuf_server(vm_name, backend);
	if (socket_fd < 0) {
		printf("can not connect vdmabuf manager\n");
		return -1;
	}

	ret = vdmabuf_import(socket_fd, &buf_id);
	if (ret < 0) {
		printf("can not import buf id:%s\n", id);
		return -1;
	}
	int bo_fd = ret;

	int *addr = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, bo_fd, 0);
	if (!addr) {
		printf("failed to mmap fd:%d", bo_fd);
		return -1;
	}
	printf("print first 20 data: ");
	for (int i = 0; i < 20 ; i++) {
		printf("%d ", addr[i]);
	}
	printf("\n");
	close(bo_fd);
	return 0;
}

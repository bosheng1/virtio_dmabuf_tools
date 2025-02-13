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
#include <sys/epoll.h>
#include <getopt.h>
#include "vdmabuf_api.h"

#define DATA 18

static void usage(const char *progname) {
	printf("Usage: %s [OPTIONS]\n"
			"Options:\n"
			"-h, --help \tPrint this help\n"
			"-b, --backend \tused in SOS side\n"
			"-i, --id \tused in SOS side\n"
			"-m, --vm \tspecify vm name to monitor\n",
			progname);
}

const char *opts = "hbm:";
const struct option options[] = {
	{"help", no_argument, NULL, 'h'},
	{"backend", no_argument, NULL, 'b'},
	{"id", required_argument, NULL, 'i'},
	{"vm", required_argument, NULL, 'm'},
	{NULL, 0, NULL, 0},
};

int main(int argc, char **argv) {
	char *vm_name = "VM1";
	int ret;
	bool backend = false;
	int handle;
	int bo_fd;
	char c;
	int size = 0;
	char *id = NULL;
	bool id_get = false;
	virtio_vdmabuf_buf_id_t buf_id;
	uint64_t buf_id_recv[2] = {0};

	while ((c = getopt_long(argc, argv, opts, options, NULL)) != -1) {
		int ret = EXIT_FAILURE;
		switch (c) {
			case 'm':
				vm_name = optarg;
				break;
			case 'b':
				backend = true;
				break;
			case 'i':
				id = optarg;
				char *tmp;
				int bufid_num = 0;
				while ((tmp = strsep(&id, " ")) != NULL) {
					char *end;
					buf_id_recv[bufid_num] = strtoul(tmp, &end, 10);
					if (*end != '\0') {
						printf("invalid buf id\n");
						return -1;
					}
					bufid_num++;
					if (bufid_num == 2) {
						break;
					}
				}
				if (bufid_num != 2) {
					printf("invalid buf id\n");
				}
				id_get = true;
				memcpy(&buf_id, buf_id_recv, sizeof(buf_id));
				break;
			case 'h':
				ret = EXIT_SUCCESS;
				/* fall through */
			default:
				usage(argv[0]);
				return ret;
		}
	}
	if (!id_get) {
		printf("no id specified\n");
		return -1;
	}
	printf("backend:%d, vm_name:%s\n", backend, vm_name);
	vdmabuf_init_parameter setup;
	if (backend) {
		setup.guest = false;
		memcpy(setup.vm, vm_name, MAX_VM_NAME_LEN);
	} else {
		setup.guest = true;
	}

	ret = vdmabuf_open(setup);
	if (ret < 0) {
		printf("failed to open vdmabuf ret:%d\n", ret);
		return -1;
	}
	handle = ret;

	ret = vdmabuf_query_size(handle, &buf_id, &size);
	if (ret < 0) {
		printf("failed to query size\n");
		return -1;
	}
	printf("buf size is :%d\n", size);

	ret = vdmabuf_import(handle, &buf_id, &bo_fd);
	if (ret < 0) {
		printf("failed to import buffer\n");
		return -1;
	}
	unsigned char *addr = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, bo_fd, 0);
	if (!addr) {
		printf("vdmabuf: mmap failure\n");
		return -1;
	}
	printf("print first four bytes data\n");
	for(int i = 0; i < 4; i++){
		printf(" %d ", addr[i]);
	}
	printf("\n");
	bool data_check = true;
	for(int i = 0; i < size; i++){
		if (addr[i] != DATA) {
			data_check = false;
			break;
		}
	}
	if (data_check) {
		printf("TEST PASS\n");
	}
	munmap(addr, size);
	return 0;
}


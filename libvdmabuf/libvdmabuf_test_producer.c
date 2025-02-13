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
#include <getopt.h>
#include "vdmabuf_api.h"

#define SIZE 4096
#define DATA 18
static void usage(const char *progname) {
	printf("Usage: %s [OPTIONS]\n"
			"Options:\n"
			"-h, --help \tPrint this help\n"
			"-b, --backend \tused in SOS side\n"
			"-m, --vm \tspecify vm name to monitor\n",
			progname);
}

const char *opts = "hbm:";
const struct option options[] = {
	{"help", no_argument, NULL, 'h'},
	{"backend", no_argument, NULL, 'b'},
	{"vm", required_argument, NULL, 'm'},
	{NULL, 0, NULL, 0},
};

int main(int argc, char **argv) {
	char *vm_name = "VM1";
	int ret;
	int handle;
	bool backend = false;
	char c;
	int bo_fd;
	virtio_vdmabuf_buf_id_t buf_id;
	while ((c = getopt_long(argc, argv, opts, options, NULL)) != -1) {
		int ret = EXIT_FAILURE;
		switch (c) {
			case 'm':
				vm_name = optarg;
				break;
			case 'b':
				backend = true;
				break;
			case 'h':
				ret = EXIT_SUCCESS;
				/* fall through */
			default:
				usage(argv[0]);
				return ret;
		}
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

	ret = vdmabuf_alloc(handle, SIZE, &bo_fd);
	if (ret < 0 || bo_fd < 0) {
		printf("failed to alloc vdmabuf buffer ret:%d\n", ret);
		return -1;
	}

	ret = vdmabuf_export(handle, bo_fd, &buf_id);
	if (ret < 0) {
		printf("failed to export vdmabuf buffer ret:%d\n", ret);
		return -1;
	}

	printf("export buf size is: %d buf_id is: ", SIZE);

	uint64_t bufid[2] = {0};
	memcpy(bufid, &buf_id, sizeof(bufid));
	for (int i = 0; i < 2; i++) {
		printf("%lu ",  bufid[i]);
	}
	printf("\n");

	char *addr = mmap(0, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, bo_fd, 0);
	if (!addr) {
		printf("failed to mmap fd\n");
		return -1;
	}
	memset(addr, DATA, SIZE);

	sleep(20);
	ret = vdmabuf_unexport(handle, &buf_id);
	if (ret < 0) {
		printf("failed to unexport vdmabuf buffer ret:%d\n", ret);
		return -1;
	}

	munmap(addr, SIZE);
	vdmabuf_free(handle, bo_fd);
	vdmabuf_close(handle);

	printf("TEST PASS\n");
	return 0;
}

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
#include <getopt.h>
#include "vdmabuf_ioctl.h"

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
	bool backend = false;
	int fd;
	char c;
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
	if (backend) {
		fd= open("/dev/virtio-vdmabuf-be", O_RDWR);
		if (fd <= 0) {
			printf("can not open virtio-vdmabuf\n");
			return -1;
		}
		struct virtio_vdmabuf_attach attach;
		memcpy(attach.name, vm_name, MAX_VM_NAME_LEN);
		ret = ioctl(fd, VIRTIO_VDMABUF_IOCTL_ATTACH, &attach);
		if (ret < 0) {
			printf("failed to attach VM:%s, ret:%d\n", vm_name, ret);
			return -1;
		}
	} else {
		fd= open("/dev/virtio-vdmabuf", O_RDWR);
		if (fd <= 0) {
			printf("can not open virtio-vdmabuf\n");
			return -1;
		}
	}
	struct virtio_vdmabuf_role role;
	role.role = VDMABUF_PRODUCER;
	ret = ioctl(fd, VIRTIO_VDMABUF_IOCTL_ROLE, &role);
	if (ret < 0) {
		printf("failed to set role, ret:%d\n", ret);
//		return -1;
	}
	struct virtio_vdmabuf_alloc amsg = {0};
	amsg.size = SIZE;
	ret = ioctl(fd, VIRTIO_VDMABUF_IOCTL_ALLOC_FD, &amsg);
	if (ret < 0) {
		printf("failed to alloc, ret:%d\n", ret);
		return -1;
	}
	int bo_fd = amsg.fd;
	char *addr = mmap(0, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, bo_fd, 0);
	if (!addr) {
		printf("failed to mmap fd\n");
		return -1;
	}
	memset(addr, DATA, SIZE);

	struct virtio_vdmabuf_export emsg = {0};

	emsg.fd = bo_fd;
	emsg.sz_priv = 4;
	emsg.priv = (char *)&amsg.size;

	ret = ioctl(fd, VIRTIO_VDMABUF_IOCTL_EXPORT, &emsg);
	if (ret < 0) {
		printf("failed to export, ret:%d\n", ret);
		return -1;
	}
	sleep(2);
	struct virtio_vdmabuf_unexport unexport = {0};

	unexport.buf_id = emsg.buf_id;

	ret = ioctl(fd, VIRTIO_VDMABUF_IOCTL_UNEXPORT, &unexport);
	if (ret < 0) {
		printf("failed to unexport, ret:%d\n", ret);
		return -1;
	}
	printf("TEST PASS\n");

	close(fd);
	return 0;
}

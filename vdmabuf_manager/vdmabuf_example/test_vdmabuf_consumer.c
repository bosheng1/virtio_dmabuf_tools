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
#include "vdmabuf_ioctl.h"

#define DATA 18
void vdmabuf_event_read(int fd)
{
	struct virtio_vdmabuf_e_hdr hdr;
	char msg[256];
	bool data_check = true;
	int ret = 0;
	int i = 0;
	ret = read(fd, &msg, 256);
	if (ret < sizeof(hdr)) {
		printf("vdmabuf: event read failure, ret:%d\n", ret);
		return;
	}
	memcpy(&hdr, msg, sizeof(hdr));
	printf("vdmabuf: read hdr: ret:%d  hdr priv size:%d\n",  ret, hdr.size);

	int len;
	memcpy(&len, msg + sizeof(hdr), sizeof(len));
	printf("vdmabuf: recv priv size:%d\n", len);

	struct virtio_vdmabuf_query_bufinfo query_msg = {0};
	memcpy(&query_msg.buf_id, &hdr.buf_id, 16);
	query_msg.subcmd = VIRTIO_VDMABUF_QUERY_PRIV_INFO_SIZE;
	ret = ioctl(fd, VIRTIO_VDMABUF_IOCTL_QUERY_BUFINFO, &query_msg);
	if (ret < 0) {
		printf("query priv size failure:%d\n", ret);
	}
	printf("query priv size:%ld\n",query_msg.info);
	query_msg.subcmd = VIRTIO_VDMABUF_QUERY_SIZE;
	ret = ioctl(fd, VIRTIO_VDMABUF_IOCTL_QUERY_BUFINFO, &query_msg);
	if (ret < 0) {
		printf("query priv size failure:%d\n", ret);
	}
	printf("query buf size:%ld\n",query_msg.info);
	len = query_msg.info;
	struct virtio_vdmabuf_import import_msg = {0};
	memcpy(&import_msg.buf_id, &hdr.buf_id, 16);
	ret = ioctl(fd, VIRTIO_VDMABUF_IOCTL_IMPORT, &import_msg);
	if (ret < 0) {
		printf("vdmabuf: import failure:%d\n", ret);
		return;
	}
	int bo_fd = import_msg.fd;
	unsigned char *addr = mmap(0, len, PROT_READ | PROT_WRITE, MAP_SHARED, bo_fd, 0);
	if (!addr) {
		printf("vdmabuf: mmap failure\n");
		close(bo_fd);
		return;
	}
	printf("print first four bytes data\n");
	for(i = 0; i < 4; i++){
		printf(" %d ", addr[i]);
	}
	printf("\n");

	for(i = 0; i < len; i++){
		if (addr[i] != DATA) {
			data_check = false;
			printf("data check error, index:%d, expected:%d, actual:%d\n", i, DATA, addr[i]);
			break;
		}
	}
	if (data_check) {
		printf("TEST PASS\n");
	}

	munmap(addr, len);
	close(bo_fd);
}
#define MAX_EVENTS 10
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
	struct epoll_event ev, events[MAX_EVENTS];
	int epoll_fd;
	int nfds;
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
	role.role = VDMABUF_CONSUMER;
	ret = ioctl(fd, VIRTIO_VDMABUF_IOCTL_ROLE, &role);
	if (ret < 0) {
		printf("failed to set role, ret:%d\n", ret);
		return -1;
	}
	epoll_fd = epoll_create1(0);
	if (epoll_fd < 0) {
		printf("failed to create epoll fd\n");
		return -1;
	}
	ev.events = EPOLLIN;
	ev.data.fd = fd;
	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) == -1) {
		printf("failed to add virtio-vdmabuf fd to epoll\n");
		return -1;
	}
	while (1) {
		nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
		if (nfds < 0) {
			printf("epoll error in wait\n");
			break;
		}
		for (int i = 0; i < nfds; ++i) {
			if (events[i].data.fd == fd) {
				printf("new event receive\n");
				vdmabuf_event_read(fd);
			}
		}
	}
}


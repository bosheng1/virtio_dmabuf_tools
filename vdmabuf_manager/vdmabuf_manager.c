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
#include <sys/ioctl.h>
#include <sys/queue.h>
#include "vdmabuf_protocal.h"
#include "vdmabuf_ioctl.h"
#include "vdmabuf_common.h"

#define MAX_USER 20
#define MAX_VM 10

struct vdmabuf_buf {
	TAILQ_ENTRY(vdmabuf_buf) link;
	virtio_vdmabuf_buf_id_t buf_id;
};

struct vdmabuf_client {
	int socket_fd;
	bool is_connected;
	char vm_name[MAX_VM_NAME_LEN];
	TAILQ_HEAD(vdmabuf_buf_list, vdmabuf_buf) buf_list;
	int dev_fd;
	bool backend;
};

struct vm_info {
	char vm_name[MAX_VM_NAME_LEN];
	int fd;
	bool backend;
};

struct vdmabuf_server {
	int socket_fd;
	int epoll_fd;
	int backend;
	struct vdmabuf_client clients[MAX_USER];
	struct vm_info vms[MAX_VM];
	int vm_num;
};

struct vdmabuf_server *server;

static void usage(const char *progname) {
	INFO("Usage: %s [OPTIONS]\n"
			"Options:\n"
			"-h, --help \tPrint this help\n"
			"-d, --help \tPrint more log\n"
			"-b, --backend \tused in SOS side\n"
			"-m, --vm \tspecify vm name to monitor\n",
			progname);
}

const char *opts = "hdbm:";
const struct option options[] = {
	{"help", no_argument, NULL, 'h'},
	{"debug", no_argument, NULL, 'd'},
	{"backend", no_argument, NULL, 'b'},
	{"vm", required_argument, NULL, 'm'},
	{NULL, 0, NULL, 0},
};

static int setup_server(int epoll_fd, bool backend) {
	int ret;
	struct epoll_event ev;
	struct sockaddr_un addr = {0};
	int socket_fd;
	char *socket_path = NULL;
		
	if (backend) {
		socket_path = DEFAULT_VDMABUF_BE_PATH;
		unlink(DEFAULT_VDMABUF_BE_PATH);
	} else {
		socket_path = DEFAULT_VDMABUF_FE_PATH;
		unlink(DEFAULT_VDMABUF_FE_PATH);
	}

	socket_fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
	if (socket_fd < 0) {
		ERROR("failed to create socket: %s\n", strerror(errno));
		return -1;
	}
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);
	ret = bind(socket_fd, (const struct sockaddr *)&addr, sizeof(addr));
	if (ret < 0) {
		ERROR("failed to bind socket: %s\n", strerror(errno));
		close(socket_fd);
		return -1;
	}

	ret = listen(socket_fd, MAX_USER);
	if (ret < 0) {
		ERROR("failed to bind socket: %s\n", strerror(errno));
		close(socket_fd);
		return -1;
	}
	ev.events = EPOLLIN;
	ev.data.fd = socket_fd;
	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socket_fd, &ev) == -1) {
		ERROR("failed to add virtio-vdmabuf fd to epoll\n");
		close(socket_fd);
		return -1;
	}
	return socket_fd;
}

static int find_device_fd(char *name)
{
	int i = 0;
	if (!server || !name)
		return -1;
	for(i = 0; i < server->vm_num; i++) {
		if (strncmp(server->vms[i].vm_name, name,  MAX_VM_NAME_LEN) == 0) {
			return server->vms[i].fd;
		}
	}
	return -1;
}

static int process_client_request(struct vdmabuf_client *client, struct vdmabuf_req_hdr *req, char *data, int data_fd)
{
	int ret = 0;
	int fd = -1;
	int status = VDMABUF_SUCCESS;
	struct vdmabuf_resp_hdr resp = {0};
	if (!client)
		return -1;
	if (!req)
		return -1;
	fd = client->dev_fd;
	if (req->type != VDMABUF_ATTACH && fd <= 0)
		return -1;
	if (req->type == VDMABUF_ALLOC) {
		uint32_t size = 0;
		struct alloc_resp_data alloc_data = {0};
		if (!data)
			return -1;
		memcpy(&size, data, 4);
		ret = ioctl_alloc(fd, size);
		DEBUG("process client alloc request: ret:%d\n", ret);
		resp.data_size = sizeof(alloc_data);
		if (ret >= 0) {
			resp.fd_count = 1;
			alloc_data.status = status;
			ret = send_command(client->socket_fd, &resp, sizeof(resp), &alloc_data, sizeof(alloc_data), ret);
		} else {
			resp.fd_count = 0;
			status = VDMABUF_FAILURE;
			alloc_data.status = status;
			ret = send_command(client->socket_fd, &resp, sizeof(resp), &alloc_data, sizeof(alloc_data), NO_FD);
		}
	} else if (req->type == VDMABUF_EXPORT) {
		char data[MAX_SIZE_PRIV_DATA] = {0};
		struct export_resp_data export_data = {0};
		virtio_vdmabuf_buf_id_t buf_id;
		if (req->data_size) {
			memcpy(data, data, req->data_size);
		}
		ret = ioctl_export(fd, data_fd, req->data_size, data, &buf_id);
		DEBUG("process client export request: ret:%d\n", ret);
		resp.data_size = sizeof(export_data);
		resp.fd_count = 0;
		if (ret < 0) {
			status = VDMABUF_FAILURE;
		} else {
			struct vdmabuf_buf *buf_entry = malloc(sizeof (struct vdmabuf_buf));
			if (buf_entry) {
				memcpy(&buf_entry->buf_id, &buf_id, sizeof(buf_id));
				TAILQ_INSERT_TAIL(&client->buf_list, buf_entry, link);
			}
		}
		export_data.status = status;
		memcpy(&export_data.buf_id, &buf_id, sizeof(buf_id));
		ret = send_command(client->socket_fd, &resp, sizeof(resp), &export_data, sizeof(export_data), NO_FD);

	} else if (req->type == VDMABUF_UNEXPORT) {
		virtio_vdmabuf_buf_id_t buf_id;
		struct unexport_resp_data unexport_data = {0};
		if (req->data_size) {
			memcpy(&buf_id, data, req->data_size);
		}
		ret = ioctl_unexport(fd, &buf_id);
		DEBUG("process client unexport request: ret:%d\n", ret);
		resp.data_size = sizeof(unexport_data);
		resp.fd_count = 0;
		if (ret < 0) {
			status = VDMABUF_FAILURE;
		} else {
			struct vdmabuf_buf *buf_entry = NULL;
			bool found = false;
			TAILQ_FOREACH(buf_entry, &client->buf_list, link)  {
				if (strncmp((char *)&buf_entry->buf_id, (char *)&buf_id, sizeof(buf_id)) == 0) {
					found = true;
					break;
				}
			}
			if (found) {
				TAILQ_REMOVE(&client->buf_list, buf_entry, link);
				free(buf_entry);
				buf_entry = NULL;
			}
		}
		unexport_data.status = status;
		ret = send_command(client->socket_fd, &resp, sizeof(resp), &unexport_data, sizeof(unexport_data), NO_FD);
	} else if (req->type == VDMABUF_ATTACH) {
		struct attach_resp_data attach_data = {0};
		if (!data || !req->data_size)
			return -1;

		memcpy(client->vm_name, data, MAX_VM_NAME_LEN);
		client->dev_fd = find_device_fd(client->vm_name);
		ERROR("process client attach request: ret:%d\n", client->dev_fd);
		resp.data_size = sizeof(attach_data);
		resp.fd_count = 0;
		if (client->dev_fd < 0)
			status = VDMABUF_FAILURE;
		attach_data.status = status;
		ret = send_command(client->socket_fd, &resp, sizeof(resp), &attach_data, sizeof(attach_data), NO_FD);
	} else if (req->type == VDMABUF_IMPORT) {
		virtio_vdmabuf_buf_id_t buf_id;
		struct import_resp_data import_data = {0};
		if (req->data_size) {
			memcpy(&buf_id, data, req->data_size);
		}
		ret = ioctl_import(fd, &buf_id);
		ERROR("process client import request: ret:%d\n", ret);
		resp.data_size = sizeof(import_data);
		resp.fd_count = 1;
		if (ret < 0) {
			resp.fd_count = 0;
			status = VDMABUF_FAILURE;
			ret = send_command(client->socket_fd, &resp, sizeof(resp), &import_data, sizeof(import_data), NO_FD);
		} else {
			ret = send_command(client->socket_fd, &resp, sizeof(resp), &import_data, sizeof(import_data), ret);
		}
	} else {
		ERROR("unknow client cmd:%d\n", req->type);
		return -1;
	}
	return 1;
}

static int device_read(void *ptr) {
	struct vm_info *vm = ptr;
	struct virtio_vdmabuf_e_hdr hdr;
	char msg[256];
	int ret = 0;
	int len = 0;
	int i = 0, j = 0;
	unsigned char bufid[16] = {0};
	ret = read(vm->fd, &msg, 256);
	if (ret < sizeof(hdr))
		return -1;
	len = ret;
	while(1) {
		if (j + sizeof(hdr) > len) {
			break;
		}
		memcpy(&hdr, msg + j, sizeof(hdr));
		memcpy(bufid, &hdr.buf_id, sizeof(bufid));
		DEBUG("vdmabuf: recv device event, build id: ");
		for (i = 0; i < 16; i++) {
			DEBUG("%u ",  bufid[i]);
		}
		DEBUG("\n  priv size:%d\n", hdr.size);
		j = j + sizeof(hdr) +  hdr.size;

	}
	memcpy(&hdr, msg, sizeof(hdr));

	return 0;
}

static int client_read(void *ptr) {
	struct vdmabuf_client *client = ptr;
	int ret = 0;
	struct vdmabuf_req_hdr req = {0};
	int fd = -1;
	char *data = NULL;
	struct msghdr msg = {0};
	struct cmsghdr *cmsg = NULL;
	struct iovec io[1];
	char ctrl_buf[CMSG_SPACE(sizeof(fd))];

	DEBUG("start to read client, %p\n", client);

	if (!client)
		return -1;
	if (!client->is_connected)
		return -1;

	ret = recv(client->socket_fd, &req, sizeof(req), 0);
	if (ret <= 0) {
		ERROR("client read failure\n, ret:%d\n", ret);
	}

	DEBUG("req, cmd:%d, data_size:%d, fd_count:%d\n", req.type, req.data_size, req.fd_count);
	if (req.data_size > 0 || req.fd_count > 0) {
		data = malloc(req.data_size);
		if (!data) {
			ERROR("client_read malloc failure\n");
			return -1;
		}
		io[0].iov_base = data;
		io[0].iov_len = req.data_size;
		msg.msg_iov = io;
		msg.msg_iovlen = 1;
		msg.msg_control = ctrl_buf;
		msg.msg_controllen = sizeof(ctrl_buf);
		cmsg = CMSG_FIRSTHDR(&msg);
		ret = recvmsg(client->socket_fd, &msg, 0);
		if (ret < 0) {
			ERROR("client read failure\n, ret:%d\n", ret);
			free(data);
			return -1;
		}
		if (req.fd_count > 0) {
			fd = *((int *)CMSG_DATA(cmsg));
			DEBUG("read client, recv fd:%d\n", fd);
			if (fd < 0) {
				ERROR("read client, recv fd:%d\n", fd);
				free(data);
				return -1;
			}
		}
	}
	ret = process_client_request(client, &req, data, fd);
	return ret;
}

static void client_disconnect(struct vdmabuf_server *server, void *ptr) {
	struct vdmabuf_client *client = ptr;

	INFO("start to disconnect client, %p\n", client);
	if (!client)
		return;
	if (!client->is_connected)
		return;
	epoll_ctl(server->epoll_fd, EPOLL_CTL_DEL, client->socket_fd, NULL);
	close(client->socket_fd);
	struct vdmabuf_buf *buf_entry = NULL;
	virtio_vdmabuf_buf_id_t buf_id;
	if (!TAILQ_EMPTY(&client->buf_list)) {
		buf_entry = TAILQ_FIRST(&client->buf_list);
		if (buf_entry) {
			TAILQ_REMOVE(&client->buf_list, buf_entry, link);
			memcpy(&buf_id, &buf_entry->buf_id, sizeof(buf_id));
			ioctl_unexport(client->dev_fd, &buf_id);
			free(buf_entry);
			buf_entry = NULL;
		}
	}
	client->is_connected = false;
}

static void client_connect(struct vdmabuf_server *server) {
	int i = 0;
	struct epoll_event ev;
	struct vdmabuf_client *client = NULL;
	INFO("start to connect client\n");
	int cfd = accept(server->socket_fd, NULL, NULL);
	if (cfd < 0) {
		ERROR("accept failed %s\n", strerror(errno));
		return;
	}

	for (i = 0; i < MAX_USER; i++) {
		if (!server->clients[i].is_connected) {
			client = &server->clients[i];
			break;
		}
	}
	if (!client) {
		ERROR("failed to accept user due to max client limit\n");
		close(cfd);
		return;
	}

	client->socket_fd = cfd;
	client->dev_fd = -1;
	client->backend = server->backend;
	if (!client->backend)
		client->dev_fd = server->vms[0].fd;

	TAILQ_INIT(&client->buf_list);
	ev.events = POLLIN;

	ev.data.fd = cfd;

	if (epoll_ctl(server->epoll_fd, EPOLL_CTL_ADD, cfd, &ev)) {
		ERROR("epoll_ctl add failed: %s\n", strerror(errno));
		close(cfd);
		return;
	}

	client->is_connected = true;
}

static struct vdmabuf_client * find_client(int fd) {
	int i = 0;
	for (i = 0; i < MAX_USER; i++) {
		if (server->clients[i].is_connected && server->clients[i].socket_fd == fd) {
			return &server->clients[i];
		}
	}
	return NULL;
}

static bool is_client(int fd) {
	int i = 0;
	for (i = 0; i < MAX_USER; i++) {
		if (server->clients[i].is_connected && server->clients[i].socket_fd == fd) {
			return true;
		}
	}
	return false;
}

static struct vm_info *find_device(int fd) {
	int i = 0;
	for (i = 0; i < server->vm_num; i++) {
		if (server->vms[i].fd == fd) {
			return &server->vms[i];
		}
	}
	return NULL;
}

static bool is_device(int fd) {
	int i = 0;
	for (i = 0; i < server->vm_num; i++) {
		if (server->vms[i].fd == fd) {
			return true;
		}
	}
	return false;
}

int main(int argc, char **argv) {
	bool backend = false;
	int socket_fd = 0;
	struct epoll_event ev;
	int c;
	int ret = 0;
	int dev_fd = 0;
	int epoll_fd;
	int nfds = 0;
	char *vm_name = NULL;
	char *str = NULL;
	int i = 0;
	int flags;
	bool debug = false;

	while ((c = getopt_long(argc, argv, opts, options, NULL)) != -1) {
		int ret = EXIT_FAILURE;
		switch (c) {
			case 'b':
				backend = true;
				break;
			case 'd':
				debug = true;
				break;
			case 'm':
				vm_name = optarg;
				break;
			case 'h':
				ret = EXIT_SUCCESS;
				/* fall through */
			default:
				usage(argv[0]);
				return ret;
		}
	}
	if (vm_name && !backend) {
		usage(argv[0]);
		return -1;
	}
	enable_debug_log(debug);
	DEBUG("backend:%d vm name:%s\n", backend, vm_name);
	epoll_fd = epoll_create1(0);
	if (epoll_fd < 0) {
		ERROR("failed to create epoll fd\n");
		return -1;
	}
	socket_fd = setup_server(epoll_fd, backend);
	if (socket_fd < 0) {
		return -1;
	}

	server = malloc(sizeof(struct vdmabuf_server));
	if (!server) {
		ERROR("server malloc failure\n");
		return -1;
	}
	memset(server, 0, sizeof(*server));
	server->epoll_fd = epoll_fd;
	server->socket_fd = socket_fd;
	server->backend = backend;
	if (vm_name) {
		while ((str = strsep(&vm_name, ",")) != NULL) {
			memcpy(server->vms[server->vm_num].vm_name, str, MAX_VM_NAME_LEN);
			server->vm_num++;
			if (server->vm_num == MAX_VM)
				break;
		}
	}

	if (backend) {
		for (i = 0; i < server->vm_num; i++) {
			dev_fd = open("/dev/virtio-vdmabuf-be", O_RDWR);
			if (dev_fd < 0) {
				ERROR("failed to open virtio-vdmabuf-be node\n");
				return -1;
			}
			ret = ioctl_attach(dev_fd, server->vms[i].vm_name);
			if (ret < 0) {
				close(dev_fd);
				continue;
			}
			struct virtio_vdmabuf_role role;
			role.role = VDMABUF_CONSUMER | VDMABUF_PRODUCER;
			ret = ioctl(dev_fd, VIRTIO_VDMABUF_IOCTL_ROLE, &role);
			if (ret < 0) {
				ERROR("failed to set role, ret:%d", ret);
				return -1;
			}
			ev.events = EPOLLIN;
			ev.data.fd = dev_fd;
			if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, dev_fd, &ev) == -1) {
				ERROR("failed to add device fd to epoll\n");
			}
			flags = fcntl(dev_fd, F_GETFL);
			fcntl(dev_fd, F_SETFL, O_NONBLOCK | flags);
			server->vms[i].fd = dev_fd;
			server->vms[i].backend = 1;
		}
	} else {
		dev_fd = open("/dev/virtio-vdmabuf", O_RDWR);
		if (dev_fd < 0) {
			ERROR("failed to open virtio-vdmabuf node\n");
			return -1;
		}
		struct virtio_vdmabuf_role role;
		role.role = VDMABUF_CONSUMER | VDMABUF_PRODUCER;
		ret = ioctl(dev_fd, VIRTIO_VDMABUF_IOCTL_ROLE, &role);
		if (ret < 0) {
			ERROR("failed to set role, ret:%d", ret);
			return -1;
		}
		ev.events = EPOLLIN;
		ev.data.fd = dev_fd;
		if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, dev_fd, &ev) == -1) {
			ERROR("failed to add device fd to epoll\n");
		}
		flags = fcntl(dev_fd, F_GETFL);
		fcntl(dev_fd, F_SETFL, O_NONBLOCK | flags);
		server->vm_num = 1;
		server->vms[0].fd = dev_fd;
		server->vms[0].backend = 0;
	}
	DEBUG("start to recv event\n");

	while (1) {
		struct epoll_event ev;
		struct vdmabuf_client *client;
		struct vm_info *vm = NULL;
		nfds = epoll_wait(epoll_fd, &ev, 1, -1);
		if (nfds < 0) {
			if (errno == EINTR)
				continue;
			ERROR("epoll wait error, %s\n", strerror(errno));
			break;
		}
		DEBUG("ev fd:%d, events:%x\n", ev.data.fd, ev.events);
		if (ev.data.fd == server->socket_fd) {
			client_connect(server);
			continue;
		}
		if (is_device(ev.data.fd)) {
			vm = find_device(ev.data.fd);
			if (ev.events & POLLIN) {
				device_read(vm);
			}
			continue;
		}
		if (is_client(ev.data.fd)) {
			client = find_client(ev.data.fd);
			if (ev.events & POLLHUP) {
				client_disconnect(server, client);
			} else if (ev.events & POLLIN) {
				client_read(client);
			}
			continue;
		}
	}
}

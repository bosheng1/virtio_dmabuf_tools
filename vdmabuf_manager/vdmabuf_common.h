/*
 * Copyright (C) 2025 Intel Corporation.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#ifndef VDMABUF_COMMON_H
#define VDMABUF_COMMON_H
#include <stdarg.h>
#include <stdio.h>

#define NO_FD -1
void enable_debug_log(bool debug);
void log_print(bool debug, FILE *stream, char *fmt, ...);
#define DEBUG(FMT, ...)                                               \
        log_print(true, stdout, "DEBUG: %s: line %d: " FMT, __func__, \
                      __LINE__, ##__VA_ARGS__)
#define INFO(FMT, ...) \
        log_print(false, stdout, "INFO: " FMT, ##__VA_ARGS__)

#define ERROR(FMT, ...) \
        log_print(false, stderr, "ERROR: " FMT, ##__VA_ARGS__)

#define DUMP(FMT, ...) \
        log_print(true, stdout, "" FMT, ##__VA_ARGS__)

int send_command(int socket, void *header, int header_len, void *data, int data_len, int fd);
int recv_resp(int socket, struct vdmabuf_resp_hdr *hdr, void *data_out, int data_len, int *fd_out);

#endif

// network/network.h
#ifndef NETWORK_H
#define NETWORK_H

#include <time.h>
#include "../platform/platform.h"
#include "../models/models.h"
#include "../utils/utils.h"
#include "../protocol/protocol.h"

/* ================ 网络常量定义 ================ */
#define DEFAULT_PORT 8080
#define MAX_CLIENTS 100
#define BUFFER_SIZE 4096
#define SELECT_TIMEOUT 5 // select超时时间（秒）

/* ================ 函数声明 ================ */

/* TCP服务器函数 */
int tcp_server_init(int port);
int tcp_server_start(void);
void tcp_server_stop(void);
socket_t tcp_server_get_fd(void);
int tcp_server_is_running(void);

/* 事件循环函数 */
void event_loop_init(void);
void event_loop_run(socket_t server_fd);
void event_loop_stop(void);
void event_loop_remove_fd(socket_t client_fd);

/* 客户端处理函数 */
void client_handler_init(void);
void client_handler_handle(socket_t client_fd);
void client_handler_send(socket_t client_fd, const char *data);
void client_handler_broadcast(const char *data, socket_t exclude_fd);
void client_handler_close(socket_t client_fd);

/* TCP客户端函数 */
socket_t tcp_connect(const char *server_ip, int server_port);
int tcp_send(socket_t sockfd, const char *data, size_t len);
int tcp_receive(socket_t sockfd, char *buffer, size_t buffer_size);
void tcp_close(socket_t sockfd);

/* 网络工具函数 */
int set_socket_nonblocking(socket_t sockfd);
const char *get_client_ip(socket_t client_fd);
int get_client_port(socket_t client_fd);

#endif /* NETWORK_H */

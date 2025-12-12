// network/network.h
#ifndef NETWORK_H
#define NETWORK_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include <errno.h>
#include <time.h>
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
int tcp_server_get_fd(void);
int tcp_server_is_running(void);

/* 事件循环函数 */
void event_loop_init(void);
void event_loop_run(int server_fd);
void event_loop_stop(void);
void event_loop_remove_fd(int client_fd);

/* 客户端处理函数 */
void client_handler_init(void);
void client_handler_handle(int client_fd);
void client_handler_send(int client_fd, const char *data);
void client_handler_broadcast(const char *data, int exclude_fd);
void client_handler_close(int client_fd);

/* TCP客户端函数 */
int tcp_connect(const char *server_ip, int server_port);
int tcp_send(int sockfd, const char *data, size_t len);
int tcp_receive(int sockfd, char *buffer, size_t buffer_size);
void tcp_close(int sockfd);

/* 网络工具函数 */
int set_socket_nonblocking(int sockfd);
const char *get_client_ip(int client_fd);
int get_client_port(int client_fd);

#endif /* NETWORK_H */

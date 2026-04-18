// network/tcp_server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <signal.h>
#endif
#include "network.h"

static socket_t server_fd = SOCKET_INVALID;
static volatile int server_running = 0;

#ifndef _WIN32
/* 信号处理函数 */
static void signal_handler(int sig)
{
	if (sig == SIGINT || sig == SIGTERM)
	{
		server_running = 0;
	}
}

/* 设置信号处理 */
static void setup_signals(void)
{
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);
	signal(SIGPIPE, SIG_IGN); // 忽略管道断开信号
}
#else
static void setup_signals(void)
{
}
#endif

/* 初始化TCP服务器 */
int tcp_server_init(int port)
{
	if (SOCKET_IS_VALID(server_fd))
	{
		LOG_WARN("Server is already initialized");
		return -1;
	}

	setup_signals();

	if (platform_socket_init() < 0)
	{
		LOG_ERROR("Failed to initialize socket layer");
		return -1;
	}

	// 创建套接字
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (SOCKET_IS_INVALID(server_fd))
	{
		LOG_ERROR("Failed to create socket: %s", platform_socket_error_message());
		platform_socket_cleanup();
		return -1;
	}

	// 设置SO_REUSEADDR选项
	int opt = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt)) < 0)
	{
		LOG_ERROR("Failed to set SO_REUSEADDR: %s", platform_socket_error_message());
		platform_socket_close(server_fd);
		platform_socket_cleanup();
		server_fd = SOCKET_INVALID;
		return -1;
	}

	// 绑定地址
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(port);

	if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
		LOG_ERROR("Failed to bind to port %d: %s", port, platform_socket_error_message());
		platform_socket_close(server_fd);
		platform_socket_cleanup();
		server_fd = SOCKET_INVALID;
		return -1;
	}

	LOG_INFO("TCP server initialized on port %d", port);
	return 0;
}

/* 启动TCP服务器 */
int tcp_server_start(void)
{
	if (SOCKET_IS_INVALID(server_fd))
	{
		LOG_ERROR("Server not initialized");
		return -1;
	}

	// 开始监听
	if (listen(server_fd, 10) < 0)
	{
		LOG_ERROR("Failed to listen: %s", platform_socket_error_message());
		return -1;
	}

	server_running = 1;
	LOG_INFO("TCP server started, listening for connections...");
	return 0;
}

/* 停止TCP服务器 */
void tcp_server_stop(void)
{
	if (SOCKET_IS_VALID(server_fd))
	{
		LOG_INFO("Closing server socket...");
		platform_socket_close(server_fd);
		platform_socket_cleanup();
		server_fd = SOCKET_INVALID;
	}
	server_running = 0;
}

/* 获取服务器文件描述符 */
socket_t tcp_server_get_fd(void)
{
	return server_fd;
}

/* 获取服务器运行状态 */
int tcp_server_is_running(void)
{
	return server_running;
}

/* 设置套接字为非阻塞模式 */
int set_socket_nonblocking(socket_t sockfd)
{
	if (platform_socket_set_nonblocking(sockfd) < 0)
	{
		LOG_ERROR("Failed to set socket non-blocking: %s", platform_socket_error_message());
		return -1;
	}

	return 0;
}

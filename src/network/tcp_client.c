/**
 * @file tcp_client.c
 * @brief TCP客户端实现
 *
 * 实现TCP客户端连接功能，用于连接到服务器。
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "network.h"
#include "../utils/utils.h"

/**
 * @brief 创建TCP套接字
 *
 * @return int 成功返回套接字描述符，失败返回-1
 */
static socket_t create_tcp_socket(void)
{
	socket_t sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (SOCKET_IS_INVALID(sockfd))
	{
		LOG_ERROR("Failed to create socket: %s", platform_socket_error_message());
		return SOCKET_INVALID;
	}

	// 设置套接字为非阻塞模式
	if (platform_socket_set_nonblocking(sockfd) < 0)
	{
		LOG_ERROR("Failed to set non-blocking mode: %s", platform_socket_error_message());
		platform_socket_close(sockfd);
		return SOCKET_INVALID;
	}

	return sockfd;
}

/**
 * @brief 连接到TCP服务器
 *
 * @param sockfd 套接字描述符
 * @param server_ip 服务器IP地址
 * @param server_port 服务器端口
 * @return int 成功返回0，失败返回-1
 */
static int connect_to_server(socket_t sockfd, const char *server_ip, int server_port)
{
	struct sockaddr_in server_addr;

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(server_port);

	if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0)
	{
		LOG_ERROR("Invalid server IP address: %s", server_ip);
		return -1;
	}

	// 尝试连接
	if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
		if (!platform_socket_in_progress())
		{
			LOG_ERROR("Failed to connect to server: %s", platform_socket_error_message());
			return -1;
		}

		// 等待连接完成
		fd_set write_fds;
		struct timeval timeout;

		FD_ZERO(&write_fds);
		FD_SET(sockfd, &write_fds);

		timeout.tv_sec = 5; // 5秒超时
		timeout.tv_usec = 0;

		if (select(platform_select_nfds(sockfd), NULL, &write_fds, NULL, &timeout) <= 0)
		{
			LOG_ERROR("Connection timeout or error");
			return -1;
		}

		// 检查连接是否成功
		int error = 0;
		socket_len_t len = sizeof(error);
		if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (char *)&error, &len) < 0 || error)
		{
			LOG_ERROR("Connection failed: %s", platform_socket_error_message_code(error));
			return -1;
		}
	}

	return 0;
}

socket_t tcp_connect(const char *server_ip, int server_port)
{
	socket_t sockfd;

	if (!server_ip)
	{
		LOG_ERROR("Server IP is NULL");
		return SOCKET_INVALID;
	}

	if (platform_socket_init() < 0)
	{
		LOG_ERROR("Failed to initialize socket layer");
		return SOCKET_INVALID;
	}

	// 创建套接字
	sockfd = create_tcp_socket();
	if (SOCKET_IS_INVALID(sockfd))
	{
		platform_socket_cleanup();
		return SOCKET_INVALID;
	}

	// 连接到服务器
	if (connect_to_server(sockfd, server_ip, server_port) < 0)
	{
		platform_socket_close(sockfd);
		platform_socket_cleanup();
		return SOCKET_INVALID;
	}

	LOG_INFO("Connected to server %s:%d", server_ip, server_port);
	return sockfd;
}

int tcp_send(socket_t sockfd, const char *data, size_t len)
{
	if (SOCKET_IS_INVALID(sockfd) || !data || len == 0)
	{
		LOG_ERROR("Invalid parameters");
		return -1;
	}

	size_t total_sent = 0;
	socket_io_result_t sent;

	while (total_sent < len)
	{
		sent = platform_socket_send(sockfd, data + total_sent, len - total_sent);
		if (sent < 0)
		{
			if (platform_socket_would_block())
			{
				// 非阻塞模式下，稍后重试
				continue;
			}
			else
			{
				LOG_ERROR("Failed to send data: %s", platform_socket_error_message());
				return -1;
			}
		}
		else if (sent == 0)
		{
			LOG_ERROR("Connection closed by peer");
			return -1;
		}

		total_sent += sent;
	}

	return 0;
}

int tcp_receive(socket_t sockfd, char *buffer, size_t buffer_size)
{
	if (SOCKET_IS_INVALID(sockfd) || !buffer || buffer_size == 0)
	{
		LOG_ERROR("Invalid parameters");
		return -1;
	}

	socket_io_result_t received = platform_socket_recv(sockfd, buffer, buffer_size - 1);
	if (received < 0)
	{
		if (platform_socket_would_block())
		{
			// 非阻塞模式下，没有数据可读
			return 0;
		}
		else
		{
			LOG_ERROR("Failed to receive data: %s", platform_socket_error_message());
			return -1;
		}
	}
	else if (received == 0)
	{
		// 连接已关闭（对端关闭连接），视为错误/断开
		LOG_DEBUG("Peer closed connection on sockfd=%lld", SOCKET_ID(sockfd));
		return -1;
	}

	buffer[received] = '\0';
	return received;
}

void tcp_close(socket_t sockfd)
{
	if (SOCKET_IS_VALID(sockfd))
	{
		platform_socket_close(sockfd);
		platform_socket_cleanup();
	}
}

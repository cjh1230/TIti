// network/client_handler.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "network.h"
#include "../protocol/protocol.h"
#include "../core/core.h"
/* 初始化客户端处理器 */
void client_handler_init(void)
{
	LOG_DEBUG("Client handler initialized");
}

/* 处理客户端数据 */
void client_handler_handle(int client_fd)
{
	char buffer[BUFFER_SIZE];
	ssize_t bytes_read;

	// 读取数据
	bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);

	if (bytes_read > 0)
	{
		buffer[bytes_read] = '\0';

		LOG_DEBUG("Received %zd bytes from client %d: %s",
				  bytes_read, client_fd, buffer);

		// 更新最后活动时间
		connection_manager_update_active(client_fd);

		// 解析消息
		Message *msg = parse_message(buffer);
		if (msg)
		{
			// 调用命令处理器处理消息
			handle_raw_message(client_fd, buffer);

			// 释放消息内存
			safe_free((void **)&msg);
		}
		else
		{
			// 解析失败，返回错误
			char *response = build_error_msg(ERROR_SERVER_ERROR, "Invalid message format");
			if (response)
			{
				client_handler_send(client_fd, response);
				free(response);
			}
		}
	}
	else if (bytes_read == 0)
	{
		// 客户端断开连接
		LOG_INFO("Client disconnected: fd=%d", client_fd);
		client_handler_close(client_fd);
	}
	else
	{
		// 读取错误
		if (errno != EWOULDBLOCK && errno != EAGAIN)
		{
			LOG_ERROR("Read error from fd=%d: %s", client_fd, strerror(errno));
			client_handler_close(client_fd);
		}
	}
}

/* 发送数据到客户端 */
void client_handler_send(int client_fd, const char *data)
{
	if (!data || strlen(data) == 0)
	{
		return;
	}

	ssize_t bytes_sent = write(client_fd, data, strlen(data));

	if (bytes_sent < 0)
	{
		LOG_ERROR("Failed to send to fd=%d: %s", client_fd, strerror(errno));
	}
	else
	{
		LOG_DEBUG("Sent %zd bytes to fd=%d", bytes_sent, client_fd);
	}
}

/* 广播数据到所有客户端 */
void client_handler_broadcast(const char *data, int exclude_fd)
{
	if (!data || strlen(data) == 0)
	{
		return;
	}

	LOG_DEBUG("Broadcasting message to all clients");

	// 通过连接管理器获取所有客户端
	int client_count = 0;
	Client **clients = connection_manager_get_all(&client_count);

	if (!clients)
	{
		return;
	}

	for (int i = 0; i < client_count; i++)
	{
		Client *client = clients[i];
		if (client->sockfd != exclude_fd && client->status >= CLIENT_STATUS_CONNECTED)
		{
			client_handler_send(client->sockfd, data);
		}
	}

	safe_free((void **)&clients);
}

/* 关闭客户端连接 */
void client_handler_close(int client_fd)
{
	if (client_fd > 0)
	{
		close(client_fd);
		LOG_DEBUG("Closed connection: fd=%d", client_fd);

		/* 确保事件循环不再使用该fd */
		event_loop_remove_fd(client_fd);
	}
}

/* 获取客户端IP地址 */
const char *get_client_ip(int client_fd)
{
	static char ip_str[INET_ADDRSTRLEN];
	struct sockaddr_in addr;
	socklen_t addr_len = sizeof(addr);

	if (getpeername(client_fd, (struct sockaddr *)&addr, &addr_len) == 0)
	{
		inet_ntop(AF_INET, &addr.sin_addr, ip_str, sizeof(ip_str));
		return ip_str;
	}

	strcpy(ip_str, "unknown");
	return ip_str;
}

/* 获取客户端端口 */
int get_client_port(int client_fd)
{
	struct sockaddr_in addr;
	socklen_t addr_len = sizeof(addr);

	if (getpeername(client_fd, (struct sockaddr *)&addr, &addr_len) == 0)
	{
		return ntohs(addr.sin_port);
	}

	return -1;
}

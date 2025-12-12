// network/event_loop.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include "network.h"
#include "../core/core.h"
// 客户端连接管理
static int client_fds[MAX_CLIENTS];
static int client_count = 0;
static volatile int loop_running = 0;

/* 初始化事件循环 */
void event_loop_init(void)
{
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		client_fds[i] = -1;
	}
	client_count = 0;
	loop_running = 0;

	LOG_DEBUG("Event loop initialized");
}

/* 添加客户端到事件循环 */
static void add_client(int client_fd)
{
	if (client_count >= MAX_CLIENTS)
	{
		LOG_WARN("Maximum clients reached (%d), rejecting connection", MAX_CLIENTS);
		close(client_fd);
		return;
	}

	// 获取客户端IP和端口
	const char *client_ip = get_client_ip(client_fd);
	int client_port = get_client_port(client_fd);

	// 添加到连接管理器
	connection_manager_add_from_fd(client_fd, client_ip, client_port);

	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (client_fds[i] == -1)
		{
			client_fds[i] = client_fd;
			client_count++;

			// 设置为非阻塞
			set_socket_nonblocking(client_fd);

			LOG_INFO("New client connected: fd=%d, IP=%s:%d, total=%d",
					 client_fd, client_ip, client_port, client_count);
			return;
		}
	}
}

/* 从事件循环移除客户端 */
static void remove_client(int client_fd)
{
	connection_manager_remove(client_fd);

	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (client_fds[i] == client_fd)
		{
			client_fds[i] = -1;
			client_count--;

			LOG_INFO("Client removed from event loop: fd=%d, remaining=%d", client_fd, client_count);
			return;
		}
	}
}

/* 公共接口：从事件循环中移除指定的客户端fd（供其他模块调用） */
void event_loop_remove_fd(int client_fd)
{
	if (client_fd <= 0)
		return;

	/* 如果文件描述符仍在client_fds数组中，移除之 */
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (client_fds[i] == client_fd)
		{
			client_fds[i] = -1;
			client_count--;
			LOG_INFO("Event loop removed fd=%d, remaining=%d", client_fd, client_count);
			break;
		}
	}

	/* 从连接管理器中移除对应客户端 */
	connection_manager_remove(client_fd);
}

/* 接受新连接 */
static int accept_connection(int server_fd)
{
	struct sockaddr_in client_addr;
	socklen_t addr_len = sizeof(client_addr);

	int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);

	if (client_fd < 0)
	{
		if (errno != EWOULDBLOCK && errno != EAGAIN)
		{
			LOG_ERROR("Failed to accept connection: %s", strerror(errno));
		}
		return -1;
	}

	return client_fd;
}

/* 运行事件循环 */
void event_loop_run(int server_fd)
{
	fd_set read_fds;
	int max_fd;
	struct timeval timeout;

	loop_running = 1;
	LOG_INFO("Event loop started");

	while (loop_running && tcp_server_is_running())
	{
		// 清空文件描述符集合
		FD_ZERO(&read_fds);

		// 添加服务器套接字
		FD_SET(server_fd, &read_fds);
		max_fd = server_fd;

		// 添加所有客户端套接字
		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			int fd = client_fds[i];
			if (fd > 0)
			{
				FD_SET(fd, &read_fds);
				if (fd > max_fd)
				{
					max_fd = fd;
				}
			}
		}

		// 设置超时
		timeout.tv_sec = SELECT_TIMEOUT;
		timeout.tv_usec = 0;

		// 等待事件
		int activity = select(max_fd + 1, &read_fds, NULL, NULL, &timeout);

		if (activity < 0 && errno != EINTR)
		{
			LOG_ERROR("select error: %s", strerror(errno));
			break;
		}

		if (activity == 0)
		{
			// 超时，执行定期任务
			continue;
		}

		// 处理新连接
		if (FD_ISSET(server_fd, &read_fds))
		{
			int new_client = accept_connection(server_fd);
			if (new_client > 0)
			{
				add_client(new_client);
			}
		}

		// 处理客户端数据
		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			int client_fd = client_fds[i];
			if (client_fd > 0 && FD_ISSET(client_fd, &read_fds))
			{
				client_handler_handle(client_fd);
			}
		}
	}

	LOG_INFO("Event loop stopped");
}

/* 停止事件循环 */
void event_loop_stop(void)
{
	loop_running = 0;

	// 关闭所有客户端连接
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (client_fds[i] > 0)
		{
			int fd = client_fds[i];
			close(fd);
			remove_client(fd);
		}
	}
	client_count = 0;
}

/* 获取客户端数量 */
int event_loop_client_count(void)
{
	return client_count;
}

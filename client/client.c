/**
 * @file client.c
 * @brief 客户端核心实现
 *
 * 实现客户端的核心功能，包括连接管理、消息发送和接收等。
 */

#include "client.h"
#include "../network/network.h"

/**
 * @brief 接收消息线程函数
 *
 * 持续接收来自服务器的消息并处理。
 *
 * @param arg 客户端结构体指针
 * @return void* 线程返回值
 */
static void *recv_thread_func(void *arg)
{
	AppClient *client = (AppClient *)arg;
	char buffer[1024];
	int bytes_received;

	while (client->running)
	{
		bytes_received = tcp_receive(client->sockfd, buffer, sizeof(buffer) - 1);
		if (bytes_received < 0)
		{
			if (client->running)
			{
				LOG_ERROR("Connection lost to server");
				pthread_mutex_lock(&client->state_lock);
				client->state = CLIENT_DISCONNECTED;
				pthread_mutex_unlock(&client->state_lock);
			}
			break;
		}

		/* 没有数据可读，短暂休眠后继续轮询，避免忙等待或误判断线 */
		if (bytes_received == 0)
		{
			usleep(100 * 1000); /* 100ms */
			continue;
		}

		buffer[bytes_received] = '\0';

		/* 可见化调试：在终端直接打印收到的原始消息，便于定位解析问题 */
		printf("[RECV RAW] %s\n", buffer);
		fflush(stdout);
		LOG_DEBUG("Raw message received (%zd bytes): %s", (ssize_t)bytes_received, buffer);

		// 解析消息
		Message *msg = parse_message(buffer);
		if (!msg)
		{
			LOG_ERROR("Failed to parse message: %s", buffer);
			continue;
		}

		/* 可见化调试：打印解析后的类型和内容，确认解析器行为 */
		printf("[PARSED] type=%s | content=%s\n", msg->type, msg->content);
		fflush(stdout);

		// 处理消息
		if (strcmp(msg->type, MSG_TYPE_OK) == 0)
		{
			printf("[SUCCESS] %s\n", msg->content);

			/* 如果是响应，内容通常为 "code|message"，按 code 解析判断成功 */
			{
				char *sep = strchr(msg->content, '|');
				int code = -1;
				if (sep)
				{
					char codebuf[16] = {0};
					size_t n = sep - msg->content;
					if (n >= sizeof(codebuf))
						n = sizeof(codebuf) - 1;
					memcpy(codebuf, msg->content, n);
					code = atoi(codebuf);
				}
				/* code == 0 (RESPONSE_SUCCESS) 视为登录/操作成功 */
				if (code == 0)
				{
					pthread_mutex_lock(&client->state_lock);
					client->state = CLIENT_AUTHENTICATED;
					pthread_mutex_unlock(&client->state_lock);
					LOG_INFO("Client authenticated locally: %s", client->username);
					printf("[STATE] client state set to AUTHENTICATED (code=%d)\n", code);
					fflush(stdout);
				}
			}
		}
		else if (strcmp(msg->type, MSG_TYPE_ERROR) == 0)
		{
			printf("[ERROR] %s\n", msg->content);
		}
		else if (strcmp(msg->type, MSG_TYPE_MSG) == 0)
		{
			printf("[MESSAGE] %s -> %s: %s\n", msg->sender, msg->receiver, msg->content);
		}
		else if (strcmp(msg->type, MSG_TYPE_BROADCAST) == 0)
		{
			printf("[BROADCAST] %s: %s\n", msg->sender, msg->content);
		}
		else if (strcmp(msg->type, MSG_TYPE_GROUP) == 0)
		{
			printf("[GROUP] %s -> %s: %s\n", msg->sender, msg->receiver, msg->content);
		}
		else if (strcmp(msg->type, MSG_TYPE_HISTORY) == 0)
		{
			printf("[HISTORY] %s\n", msg->content);
		}
		else if (strcmp(msg->type, MSG_TYPE_STATUS) == 0)
		{
			printf("[STATUS] %s\n", msg->content);
		}

		free_message(msg);
	}

	return NULL;
}

int client_init(AppClient *client, const char *server_ip, int server_port)
{
	if (!client || !server_ip)
	{
		LOG_ERROR("Invalid parameters");
		return -1;
	}

	memset(client, 0, sizeof(AppClient));
	safe_strcpy(client->server_ip, server_ip, sizeof(client->server_ip));
	client->server_port = server_port;
	client->state = CLIENT_DISCONNECTED;
	client->sockfd = -1;
	client->running = false;

	// 初始化互斥锁
	if (pthread_mutex_init(&client->state_lock, NULL) != 0)
	{
		LOG_ERROR("Failed to initialize state mutex");
		return -1;
	}

	return 0;
}

int client_connect(AppClient *client)
{
	if (!client)
	{
		LOG_ERROR("Invalid client");
		return -1;
	}

	pthread_mutex_lock(&client->state_lock);

	if (client->state != CLIENT_DISCONNECTED)
	{
		LOG_ERROR("Client already connected or connecting");
		pthread_mutex_unlock(&client->state_lock);
		return -1;
	}

	client->state = CLIENT_CONNECTING;
	pthread_mutex_unlock(&client->state_lock);

	// 创建套接字并连接
	client->sockfd = tcp_connect(client->server_ip, client->server_port);
	if (client->sockfd < 0)
	{
		LOG_ERROR("Failed to connect to server");
		pthread_mutex_lock(&client->state_lock);
		client->state = CLIENT_DISCONNECTED;
		pthread_mutex_unlock(&client->state_lock);
		return -1;
	}

	pthread_mutex_lock(&client->state_lock);
	client->state = CLIENT_CONNECTED;
	pthread_mutex_unlock(&client->state_lock);

	LOG_INFO("Connected to server %s:%d", client->server_ip, client->server_port);
	return 0;
}

int client_disconnect(AppClient *client)
{
	if (!client)
	{
		LOG_ERROR("Invalid client");
		return -1;
	}

	pthread_mutex_lock(&client->state_lock);

	if (client->state == CLIENT_DISCONNECTED)
	{
		pthread_mutex_unlock(&client->state_lock);
		return 0;
	}

	// 停止接收线程
	client->running = false;

	// 关闭套接字
	if (client->sockfd >= 0)
	{
		close(client->sockfd);
		client->sockfd = -1;
	}

	client->state = CLIENT_DISCONNECTED;
	memset(client->username, 0, sizeof(client->username));

	pthread_mutex_unlock(&client->state_lock);

	// 等待接收线程结束
	if (client->recv_thread)
	{
		pthread_join(client->recv_thread, NULL);
		client->recv_thread = 0;
	}

	LOG_INFO("Disconnected from server");
	return 0;
}

int client_login(AppClient *client, const char *username, const char *password)
{
	if (!client || !username || !password)
	{
		LOG_ERROR("Invalid parameters");
		return -1;
	}

	pthread_mutex_lock(&client->state_lock);

	if (client->state != CLIENT_CONNECTED)
	{
		LOG_ERROR("Client not connected");
		pthread_mutex_unlock(&client->state_lock);
		return -1;
	}

	pthread_mutex_unlock(&client->state_lock);

	// 构建登录消息
	char *login_msg = build_login_msg(username, password);
	if (!login_msg)
	{
		LOG_ERROR("Failed to build login message");
		return -1;
	}

	// 发送登录消息
	if (tcp_send(client->sockfd, login_msg, strlen(login_msg)) < 0)
	{
		LOG_ERROR("Failed to send login message");
		free(login_msg);
		return -1;
	}

	free(login_msg);

	// 保存用户名
	safe_strcpy(client->username, username, sizeof(client->username));

	/* 等待认证响应：避免用户在收到服务器确认前立刻发送消息导致竞态。
	   最多等待 5 秒（每 100ms 检查一次）。 */
	for (int i = 0; i < 50; i++)
	{
		pthread_mutex_lock(&client->state_lock);
		ClientState st = client->state;
		pthread_mutex_unlock(&client->state_lock);
		if (st == CLIENT_AUTHENTICATED)
		{
			return 0;
		}
		usleep(100 * 1000);
	}

	LOG_WARN("Login timed out or not authenticated within wait period");
	return -1;
}

int client_logout(AppClient *client)
{
	if (!client)
	{
		LOG_ERROR("Invalid client");
		return -1;
	}

	pthread_mutex_lock(&client->state_lock);

	if (client->state != CLIENT_AUTHENTICATED)
	{
		LOG_ERROR("Client not authenticated");
		printf("[STATE] client state=%d\n", client->state);
		fflush(stdout);
		pthread_mutex_unlock(&client->state_lock);
		return -1;
	}

	pthread_mutex_unlock(&client->state_lock);

	// 构建登出消息
	char *logout_msg = build_logout_msg(client->username);
	if (!logout_msg)
	{
		LOG_ERROR("Failed to build logout message");
		return -1;
	}

	// 发送登出消息
	if (tcp_send(client->sockfd, logout_msg, strlen(logout_msg)) < 0)
	{
		LOG_ERROR("Failed to send logout message");
		free(logout_msg);
		return -1;
	}

	free(logout_msg);

	pthread_mutex_lock(&client->state_lock);
	client->state = CLIENT_CONNECTED;
	memset(client->username, 0, sizeof(client->username));
	pthread_mutex_unlock(&client->state_lock);

	return 0;
}

int client_send_message(AppClient *client, const char *receiver, const char *content)
{
	if (!client || !receiver || !content)
	{
		LOG_ERROR("Invalid parameters");
		return -1;
	}

	pthread_mutex_lock(&client->state_lock);

	if (client->state != CLIENT_AUTHENTICATED)
	{
		LOG_ERROR("Client not authenticated");
		pthread_mutex_unlock(&client->state_lock);
		return -1;
	}

	pthread_mutex_unlock(&client->state_lock);

	// 构建消息
	char *msg = build_text_msg(client->username, receiver, content);
	if (!msg)
	{
		LOG_ERROR("Failed to build message");
		return -1;
	}

	// 发送消息
	if (tcp_send(client->sockfd, msg, strlen(msg)) < 0)
	{
		LOG_ERROR("Failed to send message");
		free(msg);
		return -1;
	}

	free(msg);
	return 0;
}

int client_send_broadcast(AppClient *client, const char *content)
{
	if (!client || !content)
	{
		LOG_ERROR("Invalid parameters");
		return -1;
	}

	pthread_mutex_lock(&client->state_lock);

	if (client->state != CLIENT_AUTHENTICATED)
	{
		LOG_ERROR("Client not authenticated");
		pthread_mutex_unlock(&client->state_lock);
		return -1;
	}

	pthread_mutex_unlock(&client->state_lock);

	// 构建广播消息
	char *msg = build_broadcast_msg(client->username, content);
	if (!msg)
	{
		LOG_ERROR("Failed to build broadcast message");
		return -1;
	}

	// 发送消息
	if (tcp_send(client->sockfd, msg, strlen(msg)) < 0)
	{
		LOG_ERROR("Failed to send broadcast message");
		free(msg);
		return -1;
	}

	free(msg);
	return 0;
}

int client_send_group_message(AppClient *client, const char *group_name, const char *content)
{
	if (!client || !group_name || !content)
	{
		LOG_ERROR("Invalid parameters");
		return -1;
	}

	pthread_mutex_lock(&client->state_lock);

	if (client->state != CLIENT_AUTHENTICATED)
	{
		LOG_ERROR("Client not authenticated");
		pthread_mutex_unlock(&client->state_lock);
		return -1;
	}

	pthread_mutex_unlock(&client->state_lock);

	// 构建群组消息
	char *msg = build_group_msg(client->username, group_name, content);
	if (!msg)
	{
		LOG_ERROR("Failed to build group message");
		return -1;
	}

	// 发送消息
	if (tcp_send(client->sockfd, msg, strlen(msg)) < 0)
	{
		LOG_ERROR("Failed to send group message");
		free(msg);
		return -1;
	}

	free(msg);
	return 0;
}

int client_request_history(AppClient *client, const char *target,
						   const char *start_time, const char *end_time)
{
	if (!client || !target)
	{
		LOG_ERROR("Invalid parameters");
		return -1;
	}

	pthread_mutex_lock(&client->state_lock);

	if (client->state != CLIENT_AUTHENTICATED)
	{
		LOG_ERROR("Client not authenticated");
		pthread_mutex_unlock(&client->state_lock);
		return -1;
	}

	pthread_mutex_unlock(&client->state_lock);

	// 构建历史记录请求
	char *msg = build_history_request(client->username, target, start_time, end_time);
	if (!msg)
	{
		LOG_ERROR("Failed to build history request");
		return -1;
	}

	// 发送请求
	if (tcp_send(client->sockfd, msg, strlen(msg)) < 0)
	{
		LOG_ERROR("Failed to send history request");
		free(msg);
		return -1;
	}

	free(msg);
	return 0;
}

int client_request_status(AppClient *client)
{
	if (!client)
	{
		LOG_ERROR("Invalid client");
		return -1;
	}

	pthread_mutex_lock(&client->state_lock);

	if (client->state != CLIENT_AUTHENTICATED)
	{
		LOG_ERROR("Client not authenticated");
		pthread_mutex_unlock(&client->state_lock);
		return -1;
	}

	pthread_mutex_unlock(&client->state_lock);

	// 构建状态请求
	char *msg = build_status_request(client->username);
	if (!msg)
	{
		LOG_ERROR("Failed to build status request");
		return -1;
	}

	// 发送请求
	if (tcp_send(client->sockfd, msg, strlen(msg)) < 0)
	{
		LOG_ERROR("Failed to send status request");
		free(msg);
		return -1;
	}

	free(msg);
	return 0;
}

int client_start(AppClient *client)
{
	if (!client)
	{
		LOG_ERROR("Invalid client");
		return -1;
	}

	pthread_mutex_lock(&client->state_lock);

	if (client->state != CLIENT_CONNECTED && client->state != CLIENT_AUTHENTICATED)
	{
		LOG_ERROR("Client not connected");
		pthread_mutex_unlock(&client->state_lock);
		return -1;
	}

	if (client->running)
	{
		LOG_ERROR("Client already running");
		pthread_mutex_unlock(&client->state_lock);
		return -1;
	}

	client->running = true;
	pthread_mutex_unlock(&client->state_lock);

	// 创建接收线程
	if (pthread_create(&client->recv_thread, NULL, recv_thread_func, client) != 0)
	{
		LOG_ERROR("Failed to create receive thread");
		client->running = false;
		return -1;
	}

	return 0;
}

int client_stop(AppClient *client)
{
	if (!client)
	{
		LOG_ERROR("Invalid client");
		return -1;
	}

	pthread_mutex_lock(&client->state_lock);
	client->running = false;
	pthread_mutex_unlock(&client->state_lock);

	// 等待接收线程结束
	if (client->recv_thread)
	{
		pthread_join(client->recv_thread, NULL);
		client->recv_thread = 0;
	}

	return 0;
}

void client_cleanup(AppClient *client)
{
	if (!client)
	{
		return;
	}

	// 停止客户端
	client_stop(client);

	// 断开连接
	client_disconnect(client);

	// 销毁互斥锁
	pthread_mutex_destroy(&client->state_lock);

	memset(client, 0, sizeof(Client));
}

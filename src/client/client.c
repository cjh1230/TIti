/**
 * @file client.c
 * @brief 客户端核心实现
 *
 * 实现客户端的核心功能，包括连接管理、消息发送和接收等。
 */

#include "client.h"
#include "../network/network.h"
#include <stdarg.h>

/**
 * @brief 从响应内容中提取面向用户显示的文本
 *
 * 服务器响应通常使用 "code|message" 格式；如果没有分隔符，则直接返回原内容。
 *
 * @param content 响应内容，可为 NULL
 * @return const char* 可显示的消息文本
 */
static const char *response_message_text(const char *content)
{
	const char *sep;
	//如果传进来的形参为NULL 返回""空字符串
	if (!content)
	{
		return "";
	}
	//查找'|'符号 如果存在sep指向'|' 如果不存在sep为NULL
	sep = strchr(content, '|');
	//如果sep存在 并且下一位不是\0 就返回下一位的地址
	if (sep && *(sep + 1) != '\0')
	{
		return sep + 1;
	}
	//没找到 返回原字符串
	return content;
}

/**
 * @brief 输出一行客户端消息
 *
 * 优先使用上层注册的回调，未注册回调时退回到标准输出。
 *
 * @param client 客户端结构体指针
 * @param line 要输出的文本，可为 NULL
 */
static void client_emit_line(AppClient *client, const char *line)
{
	ClientMessageCallback callback = NULL;
	void *userdata = NULL;
	//空指针检查
	if (!client)
	{
		return;
	}

	/* 只在读取回调指针和上下文时持锁，避免回调内部再次操作 client 时形成死锁 */
	platform_mutex_lock(&client->state_lock);
	callback = client->message_callback;
	userdata = client->message_callback_userdata;
	platform_mutex_unlock(&client->state_lock);
	
	if (callback)
	{
		callback(userdata, line ? line : "");
	}
	else
	{
		printf("%s\n", line ? line : "");
		fflush(stdout);
	}
}

/**
 * @brief 按 printf 风格格式化并输出客户端消息
 *
 * @param client 客户端结构体指针
 * @param format 格式化字符串
 */
static void client_emitf(AppClient *client, const char *format, ...)
{
	char line[512];
	va_list args;

	va_start(args, format);
	vsnprintf(line, sizeof(line), format, args);
	va_end(args);

	client_emit_line(client, line);
}

/**
 * @brief 接收消息线程函数
 *
 * 持续接收来自服务器的消息并处理。
 *
 * @param arg 客户端结构体指针
 * @return void* 线程返回值
 */
static platform_thread_return_t PLATFORM_THREAD_CALL recv_thread_func(void *arg)
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
				platform_mutex_lock(&client->state_lock);
				client->state = CLIENT_DISCONNECTED;
				platform_mutex_unlock(&client->state_lock);
			}
			break;
		}

		/* 没有数据可读，短暂休眠后继续轮询，避免忙等待或误判断线 */
		if (bytes_received == 0)
		{
			platform_sleep_ms(100);
			continue;
		}

		buffer[bytes_received] = '\0';

#ifdef CLIENT_DEBUG_RECV
		printf("[RECV RAW] %s\n", buffer);
		fflush(stdout);
#endif
		LOG_DEBUG("Raw message received (%d bytes): %s", bytes_received, buffer);

		// 解析消息
		Message *msg = parse_message(buffer);
		if (!msg)
		{
			LOG_ERROR("Failed to parse message: %s", buffer);
			continue;
		}

#ifdef CLIENT_DEBUG_RECV
		printf("[PARSED] type=%s | content=%s\n", msg->type, msg->content);
		fflush(stdout);
#endif

		// 处理消息
		if (strcmp(msg->type, MSG_TYPE_OK) == 0)
		{
			/* OK 响应既要展示给用户，也可能驱动本地认证状态切换 */
			client_emitf(client, "成功: %s", response_message_text(msg->content));

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
					platform_mutex_lock(&client->state_lock);
					client->state = CLIENT_AUTHENTICATED;
					platform_mutex_unlock(&client->state_lock);
					LOG_INFO("Client authenticated locally: %s", client->username);
#ifdef CLIENT_DEBUG_RECV
					printf("[STATE] client state set to AUTHENTICATED (code=%d)\n", code);
					fflush(stdout);
#endif
				}
			}
		}
		else if (strcmp(msg->type, MSG_TYPE_ERROR) == 0)
		{
			client_emitf(client, "错误: %s", response_message_text(msg->content));
		}
		else if (strcmp(msg->type, MSG_TYPE_MSG) == 0)
		{
			client_emitf(client, "%s: %s", msg->sender, msg->content);
		}
		else if (strcmp(msg->type, MSG_TYPE_BROADCAST) == 0)
		{
			client_emitf(client, "广播 %s: %s", msg->sender, msg->content);
		}
		else if (strcmp(msg->type, MSG_TYPE_GROUP) == 0)
		{
			client_emitf(client, "群组 %s %s: %s", msg->receiver, msg->sender, msg->content);
		}
		else if (strcmp(msg->type, MSG_TYPE_HISTORY) == 0)
		{
			client_emitf(client, "历史记录:\n%s", msg->content);
		}
		else if (strcmp(msg->type, MSG_TYPE_STATUS) == 0)
		{
			client_emit_line(client, msg->content);
		}

		free_message(msg);
	}

	return PLATFORM_THREAD_RETURN_VALUE;
}

/**
 * @brief 初始化客户端实例
 *
 * 清空结构体、保存服务器地址，并初始化状态锁。调用成功后客户端处于未连接状态。
 *
 * @param client 客户端结构体指针
 * @param server_ip 服务器 IP 地址
 * @param server_port 服务器端口
 * @return int 成功返回 0，失败返回 -1
 */
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
	/* 初始化为未连接状态，连接成功后再逐步切换到 CONNECTED/AUTHENTICATED */
	client->state = CLIENT_DISCONNECTED;
	client->sockfd = SOCKET_INVALID;
	client->running = false;

	// 初始化互斥锁
	if (platform_mutex_init(&client->state_lock) != 0)
	{
		LOG_ERROR("Failed to initialize state mutex");
		return -1;
	}

	return 0;
}

/**
 * @brief 连接服务器
 *
 * 仅允许从 CLIENT_DISCONNECTED 状态发起连接，连接成功后进入 CLIENT_CONNECTED 状态。
 *
 * @param client 客户端结构体指针
 * @return int 成功返回 0，失败返回 -1
 */
int client_connect(AppClient *client)
{
	if (!client)
	{
		LOG_ERROR("Invalid client");
		return -1;
	}

	platform_mutex_lock(&client->state_lock);

	/* 同一客户端实例只允许从未连接状态发起一次连接流程 */
	if (client->state != CLIENT_DISCONNECTED)
	{
		LOG_ERROR("Client already connected or connecting");
		platform_mutex_unlock(&client->state_lock);
		return -1;
	}

	client->state = CLIENT_CONNECTING;
	platform_mutex_unlock(&client->state_lock);

	// 创建套接字并连接服务器
	client->sockfd = tcp_connect(client->server_ip, client->server_port);
	if (SOCKET_IS_INVALID(client->sockfd))
	{
		LOG_ERROR("Failed to connect to server");
		platform_mutex_lock(&client->state_lock);
		client->state = CLIENT_DISCONNECTED;
		platform_mutex_unlock(&client->state_lock);
		return -1;
	}

	platform_mutex_lock(&client->state_lock);
	client->state = CLIENT_CONNECTED;
	platform_mutex_unlock(&client->state_lock);

	LOG_INFO("Connected to server %s:%d", client->server_ip, client->server_port);
	return 0;
}

/**
 * @brief 断开服务器连接
 *
 * 停止接收线程、关闭套接字、清空登录用户名，并把状态恢复为 CLIENT_DISCONNECTED。
 * 已断开时重复调用也会视为成功。
 *
 * @param client 客户端结构体指针
 * @return int 成功返回 0，失败返回 -1
 */
int client_disconnect(AppClient *client)
{
	if (!client)
	{
		LOG_ERROR("Invalid client");
		return -1;
	}

	platform_mutex_lock(&client->state_lock);

	/* 已经断开时视为成功，便于 cleanup 等路径重复调用 */
	if (client->state == CLIENT_DISCONNECTED)
	{
		platform_mutex_unlock(&client->state_lock);
		return 0;
	}

	// 停止接收线程
	client->running = false;

	// 先关闭套接字，唤醒可能阻塞在接收中的线程
	if (SOCKET_IS_VALID(client->sockfd))
	{
		tcp_close(client->sockfd);
		client->sockfd = SOCKET_INVALID;
	}

	client->state = CLIENT_DISCONNECTED;
	memset(client->username, 0, sizeof(client->username));

	platform_mutex_unlock(&client->state_lock);

	// 锁外等待接收线程结束，避免线程退出时访问状态锁产生互相等待
	if (platform_thread_is_valid(client->recv_thread))
	{
		platform_thread_join(client->recv_thread);
		client->recv_thread = PLATFORM_THREAD_NULL;
	}

	LOG_INFO("Disconnected from server");
	return 0;
}

/**
 * @brief 登录服务器
 *
 * 发送登录请求后等待接收线程把状态更新为 CLIENT_AUTHENTICATED，最多等待 5 秒。
 *
 * @param client 客户端结构体指针
 * @param username 用户名
 * @param password 密码
 * @return int 认证成功返回 0，失败或超时返回 -1
 */
int client_login(AppClient *client, const char *username, const char *password)
{
	if (!client || !username || !password)
	{
		LOG_ERROR("Invalid parameters");
		return -1;
	}

	platform_mutex_lock(&client->state_lock);

	/* 登录前必须已建立 TCP 连接，但还未认证 */
	if (client->state != CLIENT_CONNECTED)
	{
		LOG_ERROR("Client not connected");
		platform_mutex_unlock(&client->state_lock);
		return -1;
	}

	platform_mutex_unlock(&client->state_lock);

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

	// 保存用户名，后续发送消息和构建请求时作为 sender 使用
	safe_strcpy(client->username, username, sizeof(client->username));

	/* 等待认证响应：避免用户在收到服务器确认前立刻发送消息导致竞态。
	   最多等待 5 秒（每 100ms 检查一次）。 */
	for (int i = 0; i < 50; i++)
	{
		platform_mutex_lock(&client->state_lock);
		ClientState st = client->state;
		platform_mutex_unlock(&client->state_lock);
		if (st == CLIENT_AUTHENTICATED)
		{
			return 0;
		}
		platform_sleep_ms(100);
	}

	LOG_WARN("Login timed out or not authenticated within wait period");
	return -1;
}

/**
 * @brief 登出当前用户
 *
 * 发送登出请求后清空本地用户名，TCP 连接保持可用，状态退回 CLIENT_CONNECTED。
 *
 * @param client 客户端结构体指针
 * @return int 成功返回 0，失败返回 -1
 */
int client_logout(AppClient *client)
{
	if (!client)
	{
		LOG_ERROR("Invalid client");
		return -1;
	}

	platform_mutex_lock(&client->state_lock);

	/* 只有已认证用户才需要向服务器发送登出请求 */
	if (client->state != CLIENT_AUTHENTICATED)
	{
		LOG_ERROR("Client not authenticated");
		platform_mutex_unlock(&client->state_lock);
		return -1;
	}

	platform_mutex_unlock(&client->state_lock);

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

	platform_mutex_lock(&client->state_lock);
	/* 登出后保持 TCP 连接，可继续使用同一连接重新登录 */
	client->state = CLIENT_CONNECTED;
	memset(client->username, 0, sizeof(client->username));
	platform_mutex_unlock(&client->state_lock);

	return 0;
}

/**
 * @brief 发送私聊消息
 *
 * 仅允许已认证用户发送，sender 使用当前保存的登录用户名。
 *
 * @param client 客户端结构体指针
 * @param receiver 接收者用户名
 * @param content 消息内容
 * @return int 成功返回 0，失败返回 -1
 */
int client_send_message(AppClient *client, const char *receiver, const char *content)
{
	if (!client || !receiver || !content)
	{
		LOG_ERROR("Invalid parameters");
		return -1;
	}

	platform_mutex_lock(&client->state_lock);

	/* 所有业务消息都要求先完成登录认证 */
	if (client->state != CLIENT_AUTHENTICATED)
	{
		LOG_ERROR("Client not authenticated");
		platform_mutex_unlock(&client->state_lock);
		return -1;
	}

	platform_mutex_unlock(&client->state_lock);

	// 构建私聊消息，sender 使用当前登录用户名
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

/**
 * @brief 发送广播消息
 *
 * 广播消息由服务器分发给在线用户。
 *
 * @param client 客户端结构体指针
 * @param content 消息内容
 * @return int 成功返回 0，失败返回 -1
 */
int client_send_broadcast(AppClient *client, const char *content)
{
	if (!client || !content)
	{
		LOG_ERROR("Invalid parameters");
		return -1;
	}

	platform_mutex_lock(&client->state_lock);

	if (client->state != CLIENT_AUTHENTICATED)
	{
		LOG_ERROR("Client not authenticated");
		platform_mutex_unlock(&client->state_lock);
		return -1;
	}

	platform_mutex_unlock(&client->state_lock);

	// 构建广播消息，服务器会负责分发给在线用户
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

/**
 * @brief 发送群组消息
 *
 * 群组名会作为协议中的 receiver 字段发送给服务器。
 *
 * @param client 客户端结构体指针
 * @param group_name 群组名
 * @param content 消息内容
 * @return int 成功返回 0，失败返回 -1
 */
int client_send_group_message(AppClient *client, const char *group_name, const char *content)
{
	if (!client || !group_name || !content)
	{
		LOG_ERROR("Invalid parameters");
		return -1;
	}

	platform_mutex_lock(&client->state_lock);

	if (client->state != CLIENT_AUTHENTICATED)
	{
		LOG_ERROR("Client not authenticated");
		platform_mutex_unlock(&client->state_lock);
		return -1;
	}

	platform_mutex_unlock(&client->state_lock);

	// 构建群组消息，receiver 字段承载群组名
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

/**
 * @brief 请求历史消息
 *
 * start_time 和 end_time 可为 NULL，具体时间范围由协议层和服务器解释。
 *
 * @param client 客户端结构体指针
 * @param target 目标用户或群组
 * @param start_time 开始时间，可为 NULL
 * @param end_time 结束时间，可为 NULL
 * @return int 成功返回 0，失败返回 -1
 */
int client_request_history(AppClient *client, const char *target,
						   const char *start_time, const char *end_time)
{
	if (!client || !target)
	{
		LOG_ERROR("Invalid parameters");
		return -1;
	}

	platform_mutex_lock(&client->state_lock);

	if (client->state != CLIENT_AUTHENTICATED)
	{
		LOG_ERROR("Client not authenticated");
		platform_mutex_unlock(&client->state_lock);
		return -1;
	}

	platform_mutex_unlock(&client->state_lock);

	// 构建历史记录请求，时间范围参数可为空，由协议层处理
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

/**
 * @brief 请求在线状态信息
 *
 * 服务器返回的 STATUS 消息会由接收线程统一输出或回调给上层。
 *
 * @param client 客户端结构体指针
 * @return int 成功返回 0，失败返回 -1
 */
int client_request_status(AppClient *client)
{
	if (!client)
	{
		LOG_ERROR("Invalid client");
		return -1;
	}

	platform_mutex_lock(&client->state_lock);

	if (client->state != CLIENT_AUTHENTICATED)
	{
		LOG_ERROR("Client not authenticated");
		platform_mutex_unlock(&client->state_lock);
		return -1;
	}

	platform_mutex_unlock(&client->state_lock);

	// 构建在线状态请求，服务器返回 STATUS 消息后由接收线程展示
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

/**
 * @brief 启动接收线程
 *
 * 接收线程负责读取服务器响应和推送消息；登录流程也依赖它接收认证结果。
 *
 * @param client 客户端结构体指针
 * @return int 成功返回 0，失败返回 -1
 */
int client_start(AppClient *client)
{
	if (!client)
	{
		LOG_ERROR("Invalid client");
		return -1;
	}

	platform_mutex_lock(&client->state_lock);

	/* 接收线程可以在连接后启动；登录响应也依赖它来更新认证状态 */
	if (client->state != CLIENT_CONNECTED && client->state != CLIENT_AUTHENTICATED)
	{
		LOG_ERROR("Client not connected");
		platform_mutex_unlock(&client->state_lock);
		return -1;
	}

	if (client->running)
	{
		LOG_ERROR("Client already running");
		platform_mutex_unlock(&client->state_lock);
		return -1;
	}

	client->running = true;
	platform_mutex_unlock(&client->state_lock);

	// 创建接收线程，负责持续读取服务器推送和响应
	if (platform_thread_create(&client->recv_thread, recv_thread_func, client) != 0)
	{
		LOG_ERROR("Failed to create receive thread");
		client->running = false;
		return -1;
	}

	return 0;
}

/**
 * @brief 设置消息输出回调
 *
 * 注册回调后，接收到的可显示消息会通过回调交给上层；传入 NULL 可恢复默认控制台输出。
 *
 * @param client 客户端结构体指针
 * @param callback 消息回调函数，可为 NULL
 * @param userdata 传递给回调的用户数据
 */
void client_set_message_callback(AppClient *client,
								 ClientMessageCallback callback,
								 void *userdata)
{
	if (!client)
	{
		return;
	}

	/* 回调可为空；为空时 client_emit_line 会恢复为控制台输出 */
	platform_mutex_lock(&client->state_lock);
	client->message_callback = callback;
	client->message_callback_userdata = userdata;
	platform_mutex_unlock(&client->state_lock);
}

/**
 * @brief 停止接收线程
 *
 * 仅停止后台接收循环，不主动关闭套接字；需要断开连接时调用 client_disconnect。
 *
 * @param client 客户端结构体指针
 * @return int 成功返回 0，失败返回 -1
 */
int client_stop(AppClient *client)
{
	if (!client)
	{
		LOG_ERROR("Invalid client");
		return -1;
	}

	platform_mutex_lock(&client->state_lock);
	/* 停止后台接收循环，但不主动断开 TCP 连接 */
	client->running = false;
	platform_mutex_unlock(&client->state_lock);

	// 等待接收线程结束，防止 client 结构体被清理时线程仍在访问
	if (platform_thread_is_valid(client->recv_thread))
	{
		platform_thread_join(client->recv_thread);
		client->recv_thread = PLATFORM_THREAD_NULL;
	}

	return 0;
}

/**
 * @brief 释放客户端资源
 *
 * 依次停止后台线程、断开连接、销毁互斥锁，最后清空客户端结构体。
 *
 * @param client 客户端结构体指针
 */
void client_cleanup(AppClient *client)
{
	if (!client)
	{
		return;
	}

	// 停止客户端后台线程
	client_stop(client);

	// 断开网络连接并清空登录状态
	client_disconnect(client);

	// 销毁互斥锁
	platform_mutex_destroy(&client->state_lock);

	memset(client, 0, sizeof(AppClient));
}

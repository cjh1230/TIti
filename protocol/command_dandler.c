/**
 * @file command_dandler.c
 * @brief 命令处理器实现
 *
 * 处理解析后的各种命令消息，包括登录、登出、发送消息、广播、历史查询等。
 * 每个命令都有对应的处理函数，处理完成后会发送响应给客户端。
 *
 * 主要功能：
 * 1. 用户认证管理（登录/登出）
 * 2. 消息发送处理（私聊/广播/群组）
 * 3. 历史记录查询
 * 4. 状态查询
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include "protocol.h"
#include "../core/core.h"
#include "../storage/storage.h"
#include "../utils/utils.h"

/**
 * @brief 发送消息到指定socket
 *
 * @param sockfd 目标socket
 * @param message 要发送的消息
 * @return int 成功返回0，失败返回-1
 */
static int send_to_socket(int sockfd, const char *message)
{
	if (sockfd < 0 || !message)
		return -1;

	size_t len = strlen(message);
	ssize_t bytes_sent = send(sockfd, message, len, 0);

	if (bytes_sent < 0)
	{
		LOG_ERROR("Failed to send to socket %d: %s", sockfd, strerror(errno));
		return -1;
	}

	LOG_DEBUG("Sent %zd bytes to socket %d", bytes_sent, sockfd);
	return 0;
}

/**
 * @brief 处理登录命令
 *
 * 解析消息内容获取用户名和密码，进行认证。
 * 认证成功后设置客户端认证状态。
 *
 * @param client_fd 客户端文件描述符
 * @param msg 登录消息
 * @return int 成功返回0，失败返回错误码
 */
static int handle_login(int client_fd, Message *msg)
{
	if (!msg || !is_login_msg(msg))
	{
		LOG_ERROR("Invalid login message");
		return -1;
	}

	// 登录消息的内容是密码
	const char *username = msg->sender;
	const char *password = msg->content;

	if (!username || !password || strlen(password) == 0)
	{
		LOG_ERROR("Missing username or password in login request");
		char *error_msg = build_error_msg(ERROR_AUTH_FAILED, "Missing username or password");
		if (error_msg)
		{
			send_to_socket(client_fd, error_msg);
			free(error_msg);
		}
		return ERROR_AUTH_FAILED;
	}

	LOG_DEBUG("Processing login request: user=%s", username);

	// 执行认证
	int auth_result = session_manager_authenticate(client_fd, username, password);

	if (auth_result)
	{
		// 认证成功
		LOG_INFO("User logged in successfully: %s (fd=%d)", username, client_fd);

		// 发送成功响应
		char *success_msg = build_success_msg("Login successful");
		if (success_msg)
		{
			send_to_socket(client_fd, success_msg);
			free(success_msg);
		}

		// 广播用户上线通知（可选）
		char *online_notify = build_user_online_msg(username);
		if (online_notify)
		{
			// TODO: 需要广播给其他用户
			free(online_notify);
		}

		return 0;
	}
	else
	{
		// 认证失败
		LOG_WARN("Login failed for user: %s (fd=%d)", username, client_fd);
		char *error_msg = build_error_msg(ERROR_AUTH_FAILED, "Invalid username or password");
		if (error_msg)
		{
			send_to_socket(client_fd, error_msg);
			free(error_msg);
		}
		return ERROR_AUTH_FAILED;
	}
}

/**
 * @brief 处理登出命令
 *
 * 处理用户登出请求，重置客户端认证状态。
 *
 * @param client_fd 客户端文件描述符
 * @param msg 登出消息
 * @return int 成功返回0，失败返回-1
 */
static int handle_logout(int client_fd, Message *msg)
{
	if (!msg || !is_logout_msg(msg))
	{
		LOG_ERROR("Invalid logout message");
		return -1;
	}

	const char *username = msg->sender;
	LOG_DEBUG("Processing logout request: user=%s", username);

	// 执行登出
	session_manager_logout(client_fd);

	// 发送成功响应
	char *success_msg = build_success_msg("Logout successful");
	if (success_msg)
	{
		send_to_socket(client_fd, success_msg);
		free(success_msg);
	}

	// 广播用户下线通知（可选）
	char *offline_notify = build_user_offline_msg(username);
	if (offline_notify)
	{
		// TODO: 需要广播给其他用户
		free(offline_notify);
	}

	LOG_INFO("User logged out: %s (fd=%d)", username, client_fd);
	return 0;
}

/**
 * @brief 处理发送私聊消息命令
 *
 * 验证发送者权限，然后路由消息给接收者。
 *
 * @param client_fd 客户端文件描述符
 * @param msg 私聊消息
 * @return int 成功返回0，失败返回错误码
 */
static int handle_send_message(int client_fd, Message *msg)
{
	if (!msg || !is_private_msg(msg))
	{
		LOG_ERROR("Invalid private message");
		return -1;
	}

	// 检查发送者是否已认证
	if (!session_manager_is_authenticated(client_fd))
	{
		LOG_WARN("Unauthorized message attempt from fd=%d", client_fd);
		char *error_msg = build_error_msg(ERROR_AUTH_FAILED, "Please login first");
		if (error_msg)
		{
			send_to_socket(client_fd, error_msg);
			free(error_msg);
		}
		return ERROR_AUTH_FAILED;
	}

	const char *sender = session_manager_get_username(client_fd);
	if (!sender)
	{
		LOG_ERROR("Failed to get sender username for fd=%d", client_fd);
		return -1;
	}

	// 确保消息中的发送者是当前认证用户
	if (strcmp(sender, msg->sender) != 0)
	{
		LOG_WARN("Message sender mismatch: expected %s, got %s", sender, msg->sender);
		char *error_msg = build_error_msg(ERROR_AUTH_FAILED, "Sender mismatch");
		if (error_msg)
		{
			send_to_socket(client_fd, error_msg);
			free(error_msg);
		}
		return ERROR_AUTH_FAILED;
	}

	LOG_DEBUG("Processing private message: %s -> %s", msg->sender, msg->receiver);

	// 路由消息（message_router.c中实现）
	extern int route_message(Message * msg); // 需要message_router.c中的函数

	int route_result = route_message(msg);

	if (route_result == 0)
	{
		// 发送成功响应给发送者
		char *success_msg = build_success_msg("Message sent successfully");
		if (success_msg)
		{
			send_to_socket(client_fd, success_msg);
			free(success_msg);
		}
		return 0;
	}
	else
	{
		// 发送失败响应
		const char *error_str = "Failed to send message";
		if (route_result == ERROR_USER_OFFLINE)
			error_str = "User is offline";
		else if (route_result == ERROR_USER_NOT_FOUND)
			error_str = "User not found";

		char *error_msg = build_error_msg(route_result, error_str);
		if (error_msg)
		{
			send_to_socket(client_fd, error_msg);
			free(error_msg);
		}
		return route_result;
	}
}

/**
 * @brief 处理广播消息命令
 *
 * 验证发送者权限，然后广播消息给所有在线用户。
 *
 * @param client_fd 客户端文件描述符
 * @param msg 广播消息
 * @return int 成功返回0，失败返回错误码
 */
static int handle_broadcast(int client_fd, Message *msg)
{
	if (!msg || !is_broadcast_msg(msg))
	{
		LOG_ERROR("Invalid broadcast message");
		return -1;
	}

	// 检查发送者是否已认证
	if (!session_manager_is_authenticated(client_fd))
	{
		LOG_WARN("Unauthorized broadcast attempt from fd=%d", client_fd);
		char *error_msg = build_error_msg(ERROR_AUTH_FAILED, "Please login first");
		if (error_msg)
		{
			send_to_socket(client_fd, error_msg);
			free(error_msg);
		}
		return ERROR_AUTH_FAILED;
	}

	const char *sender = session_manager_get_username(client_fd);
	if (!sender)
	{
		LOG_ERROR("Failed to get sender username for fd=%d", client_fd);
		return -1;
	}

	// 确保消息中的发送者是当前认证用户
	if (strcmp(sender, msg->sender) != 0)
	{
		LOG_WARN("Broadcast sender mismatch: expected %s, got %s", sender, msg->sender);
		char *error_msg = build_error_msg(ERROR_AUTH_FAILED, "Sender mismatch");
		if (error_msg)
		{
			send_to_socket(client_fd, error_msg);
			free(error_msg);
		}
		return ERROR_AUTH_FAILED;
	}

	LOG_DEBUG("Processing broadcast message from: %s", sender);

	// 路由消息（message_router.c中实现）
	extern int route_message(Message * msg);

	int route_result = route_message(msg);

	if (route_result == 0)
	{
		// 发送成功响应
		char *success_msg = build_success_msg("Broadcast sent successfully");
		if (success_msg)
		{
			send_to_socket(client_fd, success_msg);
			free(success_msg);
		}
		return 0;
	}
	else
	{
		char *error_msg = build_error_msg(ERROR_SERVER_ERROR, "Failed to broadcast message");
		if (error_msg)
		{
			send_to_socket(client_fd, error_msg);
			free(error_msg);
		}
		return ERROR_SERVER_ERROR;
	}
}

/**
 * @brief 处理历史记录查询命令
 *
 * TODO: 等待history_manager实现
 *
 * @param client_fd 客户端文件描述符
 * @param msg 历史查询消息
 * @return int 暂返回-1
 */
static int handle_history_request(int client_fd, Message *msg)
{
	if (!msg || !is_history_request(msg))
	{
		LOG_ERROR("Invalid history request");
		return -1;
	}

	// 检查用户是否已认证
	if (!session_manager_is_authenticated(client_fd))
	{
		LOG_WARN("Unauthorized history request from fd=%d", client_fd);
		char *error_msg = build_error_msg(ERROR_AUTH_FAILED, "Please login first");
		if (error_msg)
		{
			send_to_socket(client_fd, error_msg);
			free(error_msg);
		}
		return ERROR_AUTH_FAILED;
	}

	// 解析查询参数
	// 内容格式：target|start_time|end_time
	char *content_copy = strdup(msg->content);
	if (!content_copy)
	{
		LOG_ERROR("Failed to duplicate content");
		return -1;
	}

	char *target = strtok(content_copy, "|");
	char *start_time = strtok(NULL, "|");
	char *end_time = strtok(NULL, "|");

	LOG_DEBUG("History request: user=%s, target=%s, start=%s, end=%s",
			  msg->sender, target ? target : "all",
			  start_time ? start_time : "none",
			  end_time ? end_time : "none");

	free(content_copy);

	// TODO: 实现历史记录查询功能
	char *response = build_error_msg(ERROR_SERVER_ERROR, "History feature not implemented yet");
	if (response)
	{
		send_to_socket(client_fd, response);
		free(response);
	}

	return -1;
}

/**
 * @brief 处理状态查询命令
 *
 * 返回当前服务器状态和用户状态信息。
 *
 * @param client_fd 客户端文件描述符
 * @param msg 状态查询消息
 * @return int 成功返回0，失败返回-1
 */
static int handle_status_request(int client_fd, Message *msg)
{
	if (!msg || !is_status_request(msg))
	{
		LOG_ERROR("Invalid status request");
		return -1;
	}

	const char *username = msg->sender;
	LOG_DEBUG("Processing status request from: %s", username);

	// 构建状态信息
	char status_info[512];
	int online_count = 0;

	// 获取在线用户列表
	char **online_users = NULL;
	int user_count = 0;
	if (session_manager_get_online_users(&online_users, &user_count))
	{
		online_count = user_count;
	}

	// 构建状态消息
	snprintf(status_info, sizeof(status_info),
			 "Server Status:\n"
			 "- Uptime: TODO\n"
			 "- Connected clients: %d\n"
			 "- Online users: %d\n"
			 "- Total users: %d\n"
			 "- Your status: %s",
			 connection_manager_count(),
			 online_count,
			 user_store_count(),
			 session_manager_is_authenticated(client_fd) ? "Online" : "Offline");

	// 清理在线用户列表
	if (online_users)
	{
		for (int i = 0; i < user_count; i++)
		{
			free(online_users[i]);
		}
		free(online_users);
	}

	// 发送状态响应
	char *response = build_success_msg(status_info);
	if (response)
	{
		send_to_socket(client_fd, response);
		free(response);
	}

	return 0;
}

/**
 * @brief 处理群组消息命令
 *
 * TODO: 等待群组管理模块实现
 *
 * @param client_fd 客户端文件描述符
 * @param msg 群组消息
 * @return int 暂返回-1
 */
static int handle_group_message(int client_fd, Message *msg)
{
	if (!msg || !is_group_msg(msg))
	{
		LOG_ERROR("Invalid group message");
		return -1;
	}

	// 检查发送者是否已认证
	if (!session_manager_is_authenticated(client_fd))
	{
		LOG_WARN("Unauthorized group message attempt from fd=%d", client_fd);
		char *error_msg = build_error_msg(ERROR_AUTH_FAILED, "Please login first");
		if (error_msg)
		{
			send_to_socket(client_fd, error_msg);
			free(error_msg);
		}
		return ERROR_AUTH_FAILED;
	}

	LOG_DEBUG("Processing group message: %s -> %s", msg->sender, msg->receiver);

	// TODO: 实现群组消息处理
	char *response = build_error_msg(ERROR_SERVER_ERROR, "Group feature not implemented yet");
	if (response)
	{
		send_to_socket(client_fd, response);
		free(response);
	}

	return -1;
}

/**
 * @brief 主命令处理函数
 *
 * 根据消息类型分发到对应的命令处理函数。
 *
 * @param client_fd 客户端文件描述符
 * @param msg 要处理的消息
 * @return int 成功返回0，失败返回错误码
 */
int handle_command(int client_fd, Message *msg)
{
	if (!msg)
	{
		LOG_ERROR("NULL message for command handling");
		return -1;
	}

	LOG_DEBUG("Handling command: fd=%d, type=%s", client_fd, msg->type);

	// 获取命令类型
	CommandType cmd_type = get_command_type(msg->type);

	// 根据命令类型处理
	switch (cmd_type)
	{
	case CMD_LOGIN:
		return handle_login(client_fd, msg);

	case CMD_LOGOUT:
		return handle_logout(client_fd, msg);

	case CMD_SEND_MSG:
		return handle_send_message(client_fd, msg);

	case CMD_BROADCAST:
		return handle_broadcast(client_fd, msg);

	case CMD_JOIN_GROUP:
	case CMD_LEAVE_GROUP:
		return handle_group_message(client_fd, msg);

	case CMD_GET_HISTORY:
		return handle_history_request(client_fd, msg);

	case CMD_GET_STATUS:
		return handle_status_request(client_fd, msg);

	case CMD_UNKNOWN:
		// 检查是否是响应消息（ERROR/OK）
		if (strcmp(msg->type, MSG_TYPE_ERROR) == 0 ||
			strcmp(msg->type, MSG_TYPE_OK) == 0)
		{
			LOG_DEBUG("Response message received, no action needed");
			return 0;
		}

		LOG_WARN("Unknown command type: %s", msg->type);
		char *error_msg = build_error_msg(ERROR_SERVER_ERROR, "Unknown command type");
		if (error_msg)
		{
			send_to_socket(client_fd, error_msg);
			free(error_msg);
		}
		return ERROR_SERVER_ERROR;
	}

	LOG_ERROR("Unhandled command type: %s", msg->type);
	return -1;
}

/**
 * @brief 处理原始消息字符串
 *
 * 将原始消息字符串解析为Message结构，然后调用命令处理器。
 * 这是一个便捷函数，用于简化消息处理流程。
 *
 * @param client_fd 客户端文件描述符
 * @param raw_message 原始消息字符串
 * @return int 成功返回0，失败返回错误码
 */
int handle_raw_message(int client_fd, const char *raw_message)
{
	if (!raw_message || client_fd < 0)
	{
		LOG_ERROR("Invalid parameters for raw message handling");
		return -1;
	}

	// 解析消息
	Message *msg = parse_message(raw_message);
	if (!msg)
	{
		LOG_ERROR("Failed to parse message: %s", raw_message);
		char *error_msg = build_error_msg(ERROR_SERVER_ERROR, "Failed to parse message");
		if (error_msg)
		{
			send_to_socket(client_fd, error_msg);
			free(error_msg);
		}
		return -1;
	}

	// 处理命令
	int result = handle_command(client_fd, msg);

	// 清理消息
	safe_free((void **)&msg);

	return result;
}

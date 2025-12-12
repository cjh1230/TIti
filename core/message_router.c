/**
 * @file message_router.c
 * @brief 消息路由模块实现
 *
 * 负责根据消息类型和接收者将消息路由到正确的客户端。
 * 支持私聊、群聊、广播三种消息类型的路由。
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include "core.h"
#include "../protocol/protocol.h"
#include "../utils/utils.h"

/**
 * @brief 发送消息到指定的socket描述符
 *
 * @param sockfd 目标socket描述符
 * @param message 要发送的消息字符串
 * @return int 成功返回0，失败返回-1
 */
static int send_to_socket(int sockfd, const char *message)
{
	if (sockfd < 0 || !message)
	{
		return -1;
	}

	size_t len = strlen(message);
	ssize_t bytes_sent = send(sockfd, message, len, 0);

	if (bytes_sent < 0)
	{
		LOG_ERROR("Failed to send message to socket %d", sockfd);
		return -1;
	}

	if ((size_t)bytes_sent != len)
	{
		LOG_WARN("Partial send to socket %d: %zd/%zu bytes",
				 sockfd, bytes_sent, len);
		return -1;
	}

	LOG_DEBUG("Sent %zd bytes to socket %d", bytes_sent, sockfd);
	return 0;
}

/**
 * @brief 路由私聊消息
 *
 * 将私聊消息发送给指定的接收者。
 * 1. 检查接收者是否在线
 * 2. 查找接收者的客户端连接
 * 3. 发送消息给接收者
 *
 * @param msg 要路由的消息
 * @return int 成功返回0，失败返回错误码
 */
static int route_private_message(Message *msg)
{
	if (!msg || !is_private_msg(msg))
	{
		LOG_ERROR("Invalid private message");
		return -1;
	}

	// 检查接收者是否在线
	if (!session_manager_is_user_online(msg->receiver))
	{
		LOG_WARN("User %s is not online, cannot deliver message", msg->receiver);
		return ERROR_USER_OFFLINE;
	}

	// 查找接收者的客户端连接
	Client *receiver = connection_manager_find_by_username(msg->receiver);
	if (!receiver)
	{
		LOG_ERROR("Failed to find client for user: %s", msg->receiver);
		return ERROR_USER_NOT_FOUND;
	}

	// 构建要发送的消息
	char *serialized_msg = serialize_message(msg);
	if (!serialized_msg)
	{
		LOG_ERROR("Failed to serialize message");
		return -1;
	}

	// 发送消息
	int result = send_to_socket(receiver->sockfd, serialized_msg);

	// 更新消息状态
	if (result == 0)
	{
		msg->is_delivered = 1;
		LOG_INFO("Private message delivered: %s -> %s", msg->sender, msg->receiver);
	}
	else
	{
		LOG_ERROR("Failed to deliver private message: %s -> %s",
				  msg->sender, msg->receiver);
	}

	free(serialized_msg);
	return result;
}

/**
 * @brief 路由广播消息
 *
 * 将广播消息发送给所有在线且已认证的用户（除了发送者自己）。
 *
 * @param msg 要广播的消息
 * @return int 成功返回0，失败返回-1
 */
static int route_broadcast_message(Message *msg)
{
	if (!msg || !is_broadcast_msg(msg))
	{
		LOG_ERROR("Invalid broadcast message");
		return -1;
	}

	// 获取所有客户端
	int client_count = 0;
	Client **clients = connection_manager_get_all(&client_count);
	if (!clients || client_count == 0)
	{
		LOG_WARN("No clients available for broadcast");
		if (clients)
			free(clients);
		return -1;
	}

	// 序列化消息
	char *serialized_msg = serialize_message(msg);
	if (!serialized_msg)
	{
		LOG_ERROR("Failed to serialize broadcast message");
		free(clients);
		return -1;
	}

	int success_count = 0;
	int total_eligible = 0;

	// 发送给所有已认证的客户端（除了发送者）
	for (int i = 0; i < client_count; i++)
	{
		Client *client = clients[i];
		if (!client || client->status != CLIENT_STATUS_AUTHENTICATED)
		{
			continue;
		}

		total_eligible++;

		// 不发送给自己
		if (strcmp(client->username, msg->sender) == 0)
		{
			continue;
		}

		if (send_to_socket(client->sockfd, serialized_msg) == 0)
		{
			success_count++;
			LOG_DEBUG("Broadcast delivered to: %s", client->username);
		}
		else
		{
			LOG_WARN("Failed to deliver broadcast to: %s", client->username);
		}
	}

	LOG_INFO("Broadcast delivered: %d/%d users, from: %s",
			 success_count, total_eligible, msg->sender);

	free(serialized_msg);
	free(clients);

	// 如果至少发送给了一个用户，就算成功
	return (success_count > 0) ? 0 : -1;
}

/**
 * @brief 路由群组消息
 *
 * TODO: 等待群组管理模块实现
 *
 * @param msg 群组消息
 * @return int 暂返回-1
 */
static int route_group_message(Message *msg)
{
	if (!msg || !is_group_msg(msg))
	{
		LOG_ERROR("Invalid group message");
		return -1;
	}

	LOG_WARN("Group message routing not implemented yet");
	return -1;
}

/**
 * @brief 主路由函数
 *
 * 根据消息类型将消息路由到正确的处理函数。
 *
 * @param msg 要路由的消息
 * @return int 成功返回0，失败返回错误码
 */
int route_message(Message *msg)
{
	if (!msg)
	{
		LOG_ERROR("NULL message for routing");
		return -1;
	}

	LOG_DEBUG("Routing message: id=%d, type=%s, sender=%s, receiver=%s",
			  msg->message_id, msg->type, msg->sender, msg->receiver);

	// 根据消息类型路由
	if (is_private_msg(msg))
	{
		return route_private_message(msg);
	}
	else if (is_broadcast_msg(msg))
	{
		return route_broadcast_message(msg);
	}
	else if (is_group_msg(msg))
	{
		return route_group_message(msg);
	}
	else if (is_login_msg(msg) || is_logout_msg(msg) ||
			 is_history_request(msg) || is_status_request(msg))
	{
		// 这些消息不需要路由，由command handler处理
		LOG_DEBUG("Command message, skipping routing: %s", msg->type);
		return 0;
	}

	LOG_ERROR("Unknown message type for routing: %s", msg->type);
	return -1;
}

/**
 * @brief 发送消息给指定用户
 *
 * 这是一个便捷函数，用于直接发送消息字符串给指定用户。
 *
 * @param username 目标用户名
 * @param message_str 要发送的消息字符串
 * @return int 成功返回0，失败返回错误码
 */
int send_to_user(const char *username, const char *message_str)
{
	if (!username || !message_str)
	{
		LOG_ERROR("Invalid parameters for send_to_user");
		return -1;
	}

	Client *client = connection_manager_find_by_username(username);
	if (!client)
	{
		LOG_WARN("User %s not found or not online", username);
		return ERROR_USER_OFFLINE;
	}

	if (client->status != CLIENT_STATUS_AUTHENTICATED)
	{
		LOG_WARN("User %s is not authenticated", username);
		return ERROR_AUTH_FAILED;
	}

	int result = send_to_socket(client->sockfd, message_str);

	if (result == 0)
	{
		LOG_DEBUG("Message sent to user %s: %s", username, message_str);
	}

	return result;
}

/**
 * @brief 发送响应消息给客户端
 *
 * @param client_fd 客户端文件描述符
 * @param code 响应码
 * @param type 响应类型（OK/ERROR）
 * @param message 响应消息
 * @return int 成功返回0，失败返回-1
 */
int send_response(int client_fd, int code, const char *type, const char *message)
{
	if (client_fd < 0 || !type || !message)
	{
		return -1;
	}

	char *response_msg = build_response_msg(code, type, message);
	if (!response_msg)
	{
		LOG_ERROR("Failed to build response message");
		return -1;
	}

	int result = send_to_socket(client_fd, response_msg);
	free(response_msg);

	return result;
}

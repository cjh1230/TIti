
/**
 * @file session_manager.c
 * @brief 会话管理器实现
 *
 * 本文件实现了会话管理功能，包括用户认证、登出、在线状态检查等。
 * 会话管理器负责处理用户登录认证流程，维护用户在线状态，
 * 并提供查询在线用户的功能。
 *
 * 主要功能：
 * 1. 用户认证与登出
 * 2. 会话状态管理
 * 3. 在线用户查询
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "core.h"
#include "../storage/storage.h"
#include "../protocol/protocol.h"

/**
 * @brief 用户认证
 *
 * 验证用户提供的用户名和密码，如果验证成功，则更新客户端状态为已认证。
 * 认证流程包括：
 * 1. 验证参数有效性
 * 2. 查找客户端连接
 * 3. 检查是否已认证
 * 4. 验证用户凭证
 * 5. 设置客户端认证信息
 * 6. 发送认证成功响应
 *
 * @param fd 客户端的文件描述符
 * @param username 用户名
 * @param password 密码
 * @return int 认证成功返回1，失败返回0
 */
int session_manager_authenticate(int fd, const char *username, const char *password)
{
	if (!username || !password)
	{
		LOG_ERROR("Invalid authentication parameters");
		return 0;
	}

	// 查找客户端
	Client *client = connection_manager_find_by_fd(fd);
	if (!client)
	{
		LOG_ERROR("Client not found for fd=%d", fd);
		return 0;
	}

	// 检查是否已经认证
	if (client->status == CLIENT_STATUS_AUTHENTICATED)
	{
		LOG_WARN("Client already authenticated: fd=%d, username=%s", fd, client->username);
		return 1;
	}

	// 验证用户凭证
	if (!user_store_authenticate(username, password))
	{
		LOG_WARN("Authentication failed for user: %s", username);
		return 0;
	}

	// 获取用户信息
	User *user = user_store_find_by_username(username);
	if (!user)
	{
		LOG_ERROR("User not found after successful authentication: %s", username);
		return 0;
	}

	// 设置客户端认证信息
	connection_manager_set_auth(fd, user->user_id, username);

	LOG_INFO("User authenticated successfully: %s (fd=%d)", username, fd);

	// 发送认证成功响应
	char *response = build_success_msg("Login successful");
	if (response)
	{
		// TODO: 这里需要发送给客户端
		free(response);
	}

	return 1;
}

/**
 * @brief 用户登出
 *
 * 处理用户登出请求，重置客户端状态为未认证状态。
 * 登出流程包括：
 * 1. 查找客户端连接
 * 2. 检查客户端是否已认证
 * 3. 重置客户端认证信息
 * 4. 发送登出成功响应
 *
 * @param fd 客户端的文件描述符
 */
void session_manager_logout(int fd)
{
	Client *client = connection_manager_find_by_fd(fd);
	if (!client)
	{
		LOG_WARN("Client not found for logout: fd=%d", fd);
		return;
	}

	if (client->status != CLIENT_STATUS_AUTHENTICATED)
	{
		LOG_WARN("Client not authenticated: fd=%d", fd);
		return;
	}

	LOG_INFO("User logging out: %s (fd=%d)", client->username, fd);

	// 重置客户端状态
	client->user_id = -1;
	memset(client->username, 0, sizeof(client->username));
	client->status = CLIENT_STATUS_CONNECTED;

	// 发送登出响应
	char *response = build_success_msg("Logout successful");
	if (response)
	{
		// TODO: 这里需要发送给客户端
		free(response);
	}
}

/**
 * @brief 检查是否已认证
 *
 * 检查指定文件描述符对应的客户端是否已经通过认证。
 *
 * @param fd 客户端的文件描述符
 * @return int 已认证返回1，未认证或客户端不存在返回0
 */
int session_manager_is_authenticated(int fd)
{
	Client *client = connection_manager_find_by_fd(fd);
	if (!client)
	{
		return 0;
	}
	return client->status == CLIENT_STATUS_AUTHENTICATED;
}

/**
 * @brief 获取用户ID
 *
 * 获取已认证客户端对应的用户ID。
 *
 * @param fd 客户端的文件描述符
 * @return int 成功返回用户ID，未认证或客户端不存在返回-1
 */
int session_manager_get_user_id(int fd)
{
	Client *client = connection_manager_find_by_fd(fd);
	if (!client || client->status != CLIENT_STATUS_AUTHENTICATED)
	{
		return -1;
	}
	return client->user_id;
}

/**
 * @brief 获取用户名
 *
 * 获取已认证客户端对应的用户名。
 *
 * @param fd 客户端的文件描述符
 * @return const char* 成功返回用户名字符串指针，未认证或客户端不存在返回NULL
 */
const char *session_manager_get_username(int fd)
{
	Client *client = connection_manager_find_by_fd(fd);
	if (!client || client->status != CLIENT_STATUS_AUTHENTICATED)
	{
		return NULL;
	}
	return client->username;
}

/**
 * @brief 检查用户名是否在线
 *
 * 检查指定用户名对应的客户端是否在线且已认证。
 *
 * @param username 要检查的用户名
 * @return int 在线返回1，不在线或用户不存在返回0
 */
int session_manager_is_user_online(const char *username)
{
	if (!username)
		return 0;

	Client *client = connection_manager_find_by_username(username);
	return (client != NULL && client->status == CLIENT_STATUS_AUTHENTICATED);
}

/**
 * @brief 获取所有在线用户
 *
 * 获取所有已认证的在线用户名列表，返回动态分配的用户名数组。
 * 调用者负责释放返回的数组和其中的字符串。
 *
 * @param usernames 输出参数，用于返回用户名数组
 * @param count 输出参数，用于返回在线用户数量
 * @return int 成功返回1，失败返回0
 */
int session_manager_get_online_users(char ***usernames, int *count)
{
	if (!usernames || !count)
		return 0;

	*usernames = NULL;
	*count = 0;

	int total = 0;
	Client **all = connection_manager_get_all(&total);
	if (!all || total == 0)
	{
		if (all)
			free(all);
		return 0;
	}

	// 先计算在线的数量
	int online_count = 0;
	for (int i = 0; i < total; ++i)
	{
		if (all[i] && all[i]->status == CLIENT_STATUS_AUTHENTICATED)
			online_count++;
	}

	if (online_count == 0)
	{
		free(all);
		return 0;
	}

	// 分配并填充用户名列表
	char **list = (char **)safe_malloc(sizeof(char *) * online_count);
	if (!list)
	{
		free(all);
		return 0;
	}

	int idx = 0;
	for (int i = 0; i < total && idx < online_count; ++i)
	{
		if (all[i] && all[i]->status == CLIENT_STATUS_AUTHENTICATED)
		{
			list[idx++] = strdup(all[i]->username);
		}
	}

	free(all);

	*usernames = list;
	*count = online_count;
	return 1;
}

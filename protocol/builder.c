/**
 * @file builder.c
 * @brief 协议消息构建器实现
 *
 * 本文件实现了各种类型的消息构建函数，用于生成符合协议格式的消息字符串。
 * 支持构建的消息类型包括：登录、登出、普通文本消息、广播消息、群组消息、
 * 历史记录请求、状态查询请求、响应消息以及系统通知等。
 *
 * 所有构建的消息都遵循统一的格式：type|sender|receiver|timestamp|content
 *
 * 主要功能：
 * 1. 构建各种类型的用户消息
 * 2. 构建服务器响应消息
 * 3. 构建系统通知消息
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "protocol.h"
#include "../utils/utils.h"

/**
 * @brief 构建登录消息
 *
 * 根据提供的用户名和密码构建登录消息，格式为：
 * LOGIN|username|server|timestamp|password
 *
 * @param username 用户名
 * @param password 密码
 * @return char* 成功返回登录消息字符串，失败返回NULL
 */
char *build_login_msg(const char *username, const char *password)
{
	if (!username || !password)
	{
		LOG_ERROR("Invalid parameters for login message");
		return NULL;
	}

	if (!is_valid_username(username))
	{
		LOG_ERROR("Invalid username: %s", username);
		return NULL;
	}

	// 获取当前时间戳
	char *timestamp = get_current_timestamp();
	if (!timestamp)
	{
		LOG_ERROR("Failed to get timestamp");
		return NULL;
	}

	// 构建消息内容
	char content[256];
	snprintf(content, sizeof(content), "%s|%s|%s|%s|%s\n",
			 MSG_TYPE_LOGIN, username, "server", timestamp, password);

	char *result = strdup(content);
	safe_free((void **)&timestamp);

	LOG_DEBUG("Built login message for user: %s", username);
	return result;
}

/**
 * @brief 构建登出消息
 *
 * 根据提供的用户名构建登出消息，格式为：
 * LOGOUT|username|server|timestamp|
 *
 * @param username 用户名
 * @return char* 成功返回登出消息字符串，失败返回NULL
 */
char *build_logout_msg(const char *username)
{
	if (!username)
	{
		LOG_ERROR("Invalid username for logout message");
		return NULL;
	}

	if (!is_valid_username(username))
	{
		LOG_ERROR("Invalid username: %s", username);
		return NULL;
	}

	char *timestamp = get_current_timestamp();
	if (!timestamp)
	{
		LOG_ERROR("Failed to get timestamp");
		return NULL;
	}

	char content[256];
	snprintf(content, sizeof(content), "%s|%s|%s|%s|\n",
			 MSG_TYPE_LOGOUT, username, "server", timestamp);

	char *result = strdup(content);
	safe_free((void **)&timestamp);

	LOG_DEBUG("Built logout message for user: %s", username);
	return result;
}

/**
 * @brief 构建普通文本消息
 *
 * 根据提供的发送者、接收者和内容构建普通文本消息，格式为：
 * MSG|sender|receiver|timestamp|content
 *
 * 注意：内容会被转义处理，以确保特殊字符不会干扰消息解析。
 *
 * @param sender 发送者用户名
 * @param receiver 接收者用户名
 * @param content 消息内容
 * @return char* 成功返回消息字符串，失败返回NULL
 */
char *build_text_msg(const char *sender, const char *receiver,
					 const char *content)
{
	if (!sender || !receiver || !content)
	{
		LOG_ERROR("Invalid parameters for text message");
		return NULL;
	}

	if (!is_valid_username(sender))
	{
		LOG_ERROR("Invalid sender: %s", sender);
		return NULL;
	}

	if (strlen(content) > MAX_CONTENT_LEN - 1)
	{
		LOG_ERROR("Message content too long: %zu", strlen(content));
		return NULL;
	}

	char *timestamp = get_current_timestamp();
	if (!timestamp)
	{
		LOG_ERROR("Failed to get timestamp");
		return NULL;
	}

	// 转义内容中的特殊字符
	char *escaped_content = escape_field(content);
	if (!escaped_content)
	{
		LOG_ERROR("Failed to escape content");
		safe_free((void **)&timestamp);
		return NULL;
	}

	char msg[512];
	snprintf(msg, sizeof(msg), "%s|%s|%s|%s|%s\n",
			 MSG_TYPE_MSG, sender, receiver, timestamp, escaped_content);

	char *result = strdup(msg);
	safe_free((void **)&timestamp);
	safe_free((void **)&escaped_content);

	LOG_DEBUG("Built text message: %s -> %s", sender, receiver);
	return result;
}

/**
 * @brief 构建广播消息
 *
 * 根据提供的发送者和内容构建广播消息，格式为：
 * BROADCAST|sender|broadcast|timestamp|content
 *
 * 广播消息会发送给所有在线用户，内容会被转义处理。
 *
 * @param sender 发送者用户名
 * @param content 消息内容
 * @return char* 成功返回广播消息字符串，失败返回NULL
 */
char *build_broadcast_msg(const char *sender, const char *content)
{
	if (!sender || !content)
	{
		LOG_ERROR("Invalid parameters for broadcast message");
		return NULL;
	}

	if (!is_valid_username(sender))
	{
		LOG_ERROR("Invalid sender: %s", sender);
		return NULL;
	}

	if (strlen(content) > MAX_CONTENT_LEN - 1)
	{
		LOG_ERROR("Broadcast content too long: %zu", strlen(content));
		return NULL;
	}

	char *timestamp = get_current_timestamp();
	if (!timestamp)
	{
		LOG_ERROR("Failed to get timestamp");
		return NULL;
	}

	char *escaped_content = escape_field(content);
	if (!escaped_content)
	{
		LOG_ERROR("Failed to escape content");
		safe_free((void **)&timestamp);
		return NULL;
	}

	char msg[512];
	snprintf(msg, sizeof(msg), "%s|%s|%s|%s|%s\n",
			 MSG_TYPE_BROADCAST, sender, RECEIVER_BROADCAST,
			 timestamp, escaped_content);

	char *result = strdup(msg);
	safe_free((void **)&timestamp);
	safe_free((void **)&escaped_content);

	LOG_DEBUG("Built broadcast message from: %s", sender);
	return result;
}

/**
 * @brief 构建群组消息
 *
 * 根据提供的发送者、群组名和内容构建群组消息，格式为：
 * GROUP|sender|group:群组名|timestamp|content
 *
 * 群组消息会发送给指定群组的所有成员，内容会被转义处理。
 *
 * @param sender 发送者用户名
 * @param group_name 群组名称
 * @param content 消息内容
 * @return char* 成功返回群组消息字符串，失败返回NULL
 */
char *build_group_msg(const char *sender, const char *group_name,
					  const char *content)
{
	if (!sender || !group_name || !content)
	{
		LOG_ERROR("Invalid parameters for group message");
		return NULL;
	}

	if (!is_valid_username(sender))
	{
		LOG_ERROR("Invalid sender: %s", sender);
		return NULL;
	}

	if (strlen(group_name) > MAX_GROUPNAME_LEN - 1)
	{
		LOG_ERROR("Group name too long: %s", group_name);
		return NULL;
	}

	if (strlen(content) > MAX_CONTENT_LEN - 1)
	{
		LOG_ERROR("Group message content too long: %zu", strlen(content));
		return NULL;
	}

	char *timestamp = get_current_timestamp();
	if (!timestamp)
	{
		LOG_ERROR("Failed to get timestamp");
		return NULL;
	}

	// 构建接收者标识：group:群组名
	char receiver[64];
	snprintf(receiver, sizeof(receiver), "%s%s",
			 RECEIVER_GROUP_PREFIX, group_name);

	char *escaped_content = escape_field(content);
	if (!escaped_content)
	{
		LOG_ERROR("Failed to escape content");
		safe_free((void **)&timestamp);
		return NULL;
	}

	char msg[512];
	snprintf(msg, sizeof(msg), "%s|%s|%s|%s|%s\n",
			 MSG_TYPE_GROUP, sender, receiver, timestamp, escaped_content);

	char *result = strdup(msg);
	safe_free((void **)&timestamp);
	safe_free((void **)&escaped_content);

	LOG_DEBUG("Built group message: %s -> group:%s", sender, group_name);
	return result;
}

/**
 * @brief 构建历史记录请求消息
 *
 * 根据提供的用户名、目标用户/群组和时间范围构建历史记录请求消息，格式为：
 * HISTORY|username|server|timestamp|target|start_time|end_time
 *
 * 内容部分包含目标用户/群组和时间范围，用于查询特定时间段的历史记录。
 *
 * @param username 请求用户名
 * @param target 查询目标（用户名或群组名）
 * @param start_time 开始时间（可为NULL）
 * @param end_time 结束时间（可为NULL）
 * @return char* 成功返回历史记录请求消息字符串，失败返回NULL
 */
char *build_history_request(const char *username, const char *target,
							const char *start_time, const char *end_time)
{
	if (!username || !target)
	{
		LOG_ERROR("Invalid parameters for history request");
		return NULL;
	}

	if (!is_valid_username(username))
	{
		LOG_ERROR("Invalid username: %s", username);
		return NULL;
	}

	char *timestamp = get_current_timestamp();
	if (!timestamp)
	{
		LOG_ERROR("Failed to get timestamp");
		return NULL;
	}

	// 构建内容：target|start_time|end_time
	char content[512];
	snprintf(content, sizeof(content), "%s|%s|%s",
			 target,
			 start_time ? start_time : "",
			 end_time ? end_time : "");

	char msg[1024];
	snprintf(msg, sizeof(msg), "%s|%s|%s|%s|%s\n",
			 MSG_TYPE_HISTORY, username, "server", timestamp, content);

	char *result = strdup(msg);
	safe_free((void **)&timestamp);

	LOG_DEBUG("Built history request: %s -> %s", username, target);
	return result;
}

/**
 * @brief 构建状态查询请求消息
 *
 * 根据提供的用户名构建状态查询请求消息，格式为：
 * STATUS|username|server|timestamp|
 *
 * 用于查询用户或系统的状态信息。
 *
 * @param username 请求用户名
 * @return char* 成功返回状态查询请求消息字符串，失败返回NULL
 */
char *build_status_request(const char *username)
{
	if (!username)
	{
		LOG_ERROR("Invalid username for status request");
		return NULL;
	}

	if (!is_valid_username(username))
	{
		LOG_ERROR("Invalid username: %s", username);
		return NULL;
	}

	char *timestamp = get_current_timestamp();
	if (!timestamp)
	{
		LOG_ERROR("Failed to get timestamp");
		return NULL;
	}

	char msg[256];
	snprintf(msg, sizeof(msg), "%s|%s|%s|%s|\n",
			 MSG_TYPE_STATUS, username, "server", timestamp);

	char *result = strdup(msg);
	safe_free((void **)&timestamp);

	LOG_DEBUG("Built status request for: %s", username);
	return result;
}

/**
 * @brief 构建响应消息
 *
 * 根据提供的响应码、类型和消息内容构建响应消息，格式为：
 * type|server|client|timestamp|code|message
 *
 * 响应消息用于向客户端返回操作结果或错误信息。
 *
 * @param code 响应码
 * @param type 响应类型（OK或ERROR）
 * @param message 响应消息内容
 * @return char* 成功返回响应消息字符串，失败返回NULL
 */
char *build_response_msg(int code, const char *type, const char *message)
{
	if (!type || !message)
	{
		LOG_ERROR("Invalid parameters for response message");
		return NULL;
	}

	// 验证响应类型
	if (strcmp(type, MSG_TYPE_OK) != 0 &&
		strcmp(type, MSG_TYPE_ERROR) != 0)
	{
		LOG_ERROR("Invalid response type: %s", type);
		return NULL;
	}

	char *timestamp = get_current_timestamp();
	if (!timestamp)
	{
		LOG_ERROR("Failed to get timestamp");
		return NULL;
	}

	// 构建内容：code|message
	char content[256];
	snprintf(content, sizeof(content), "%d|%s", code, message);

	char msg[512];
	snprintf(msg, sizeof(msg), "%s|server|client|%s|%s\n",
			 type, timestamp, content);

	char *result = strdup(msg);
	safe_free((void **)&timestamp);

	LOG_DEBUG("Built response: type=%s, code=%d", type, code);
	return result;
}

/**
 * @brief 构建成功响应消息
 *
 * 构建表示操作成功的响应消息，使用OK类型和成功响应码。
 *
 * @param message 响应消息内容（可为NULL，默认为"Success"）
 * @return char* 成功返回成功响应消息字符串，失败返回NULL
 */
char *build_success_msg(const char *message)
{
	return build_response_msg(RESPONSE_SUCCESS, MSG_TYPE_OK,
							  message ? message : "Success");
}

/**
 * @brief 构建错误响应消息
 *
 * 构建表示操作失败的响应消息，使用ERROR类型和指定的错误码。
 * 如果未提供消息内容，会根据错误码使用默认的错误消息。
 *
 * @param error_code 错误码
 * @param message 错误消息内容（可为NULL）
 * @return char* 成功返回错误响应消息字符串，失败返回NULL
 */
char *build_error_msg(int error_code, const char *message)
{
	const char *error_msg = message;

	// 如果没有提供消息，使用默认错误消息
	if (!message)
	{
		switch (error_code)
		{
		case ERROR_AUTH_FAILED:
			error_msg = "Authentication failed";
			break;
		case ERROR_USER_NOT_FOUND:
			error_msg = "User not found";
			break;
		case ERROR_USER_OFFLINE:
			error_msg = "User is offline";
			break;
		case ERROR_GROUP_FULL:
			error_msg = "Group is full";
			break;
		case ERROR_SERVER_ERROR:
			error_msg = "Server internal error";
			break;
		default:
			error_msg = "Unknown error";
			break;
		}
	}

	return build_response_msg(error_code, MSG_TYPE_ERROR, error_msg);
}

/**
 * @brief 从Response结构体构建响应消息
 *
 * 根据提供的Response结构体构建响应消息，根据响应码确定消息类型。
 *
 * @param resp Response结构体指针
 * @return char* 成功返回响应消息字符串，失败返回NULL
 */
char *build_response_from_struct(const Response *resp)
{
	if (!resp)
	{
		LOG_ERROR("NULL response structure");
		return NULL;
	}

	// 确定消息类型
	const char *type = (resp->code == RESPONSE_SUCCESS) ? MSG_TYPE_OK : MSG_TYPE_ERROR;

	return build_response_msg(resp->code, type, resp->message);
}

/**
 * @brief 构建用户上线通知消息
 *
 * 构建用户上线的广播通知消息，格式为：
 * BROADCAST|server|broadcast|timestamp|username is now online
 *
 * @param username 上线用户名
 * @return char* 成功返回上线通知消息字符串，失败返回NULL
 */
char *build_user_online_msg(const char *username)
{
	if (!username || !is_valid_username(username))
	{
		LOG_ERROR("Invalid username for online notification");
		return NULL;
	}

	char *timestamp = get_current_timestamp();
	if (!timestamp)
	{
		LOG_ERROR("Failed to get timestamp");
		return NULL;
	}

	char content[128];
	snprintf(content, sizeof(content), "%s is now online", username);

	char msg[512];
	snprintf(msg, sizeof(msg), "%s|server|%s|%s|%s\n",
			 MSG_TYPE_BROADCAST, RECEIVER_BROADCAST, timestamp, content);

	char *result = strdup(msg);
	safe_free((void **)&timestamp);

	LOG_DEBUG("Built online notification for: %s", username);
	return result;
}

/**
 * @brief 构建用户下线通知消息
 *
 * 构建用户下线的广播通知消息，格式为：
 * BROADCAST|server|broadcast|timestamp|username is now offline
 *
 * @param username 下线用户名
 * @return char* 成功返回下线通知消息字符串，失败返回NULL
 */
char *build_user_offline_msg(const char *username)
{
	if (!username || !is_valid_username(username))
	{
		LOG_ERROR("Invalid username for offline notification");
		return NULL;
	}

	char *timestamp = get_current_timestamp();
	if (!timestamp)
	{
		LOG_ERROR("Failed to get timestamp");
		return NULL;
	}

	char content[128];
	snprintf(content, sizeof(content), "%s is now offline", username);

	char msg[512];
	snprintf(msg, sizeof(msg), "%s|server|%s|%s|%s\n",
			 MSG_TYPE_BROADCAST, RECEIVER_BROADCAST, timestamp, content);

	char *result = strdup(msg);
	safe_free((void **)&timestamp);

	LOG_DEBUG("Built offline notification for: %s", username);
	return result;
}

/**
 * @brief 构建系统通知消息
 *
 * 构建系统广播通知消息，格式为：
 * BROADCAST|server|broadcast|timestamp|content
 *
 * 内容会被转义处理，以确保特殊字符不会干扰消息解析。
 *
 * @param content 系统通知内容
 * @return char* 成功返回系统通知消息字符串，失败返回NULL
 */
char *build_system_notification(const char *content)
{
	if (!content)
	{
		LOG_ERROR("Invalid content for system notification");
		return NULL;
	}

	if (strlen(content) > MAX_CONTENT_LEN - 1)
	{
		LOG_ERROR("System notification too long: %zu", strlen(content));
		return NULL;
	}

	char *timestamp = get_current_timestamp();
	if (!timestamp)
	{
		LOG_ERROR("Failed to get timestamp");
		return NULL;
	}

	char *escaped_content = escape_field(content);
	if (!escaped_content)
	{
		LOG_ERROR("Failed to escape content");
		safe_free((void **)&timestamp);
		return NULL;
	}

	char msg[512];
	snprintf(msg, sizeof(msg), "%s|server|%s|%s|%s\n",
			 MSG_TYPE_BROADCAST, RECEIVER_BROADCAST,
			 timestamp, escaped_content);

	char *result = strdup(msg);
	safe_free((void **)&timestamp);
	safe_free((void **)&escaped_content);

	LOG_DEBUG("Built system notification");
	return result;
}

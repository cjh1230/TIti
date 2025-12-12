
/**
 * @file parser.c
 * @brief 协议解析器实现
 *
 * 本文件实现了消息的解析、序列化、验证等功能，包括消息的转义和反转义处理。
 * 支持多种消息类型的解析，如登录、登出、普通消息、广播、群组消息等。
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <stdatomic.h>
#include "protocol.h"

/**
 * @brief 全局消息ID计数器
 *
 * 使用原子变量确保多线程环境下的线程安全，初始值为100。
 * 每创建一个新消息，该计数器会自动递增，并为消息分配唯一ID。
 */
static atomic_int message_id_counter = ATOMIC_VAR_INIT(100);

/**
 * @brief 解析原始消息字符串为Message结构体
 *
 * 该函数将原始字符串格式的消息解析为Message结构体，包括字段提取、
 * 转义字符处理、消息验证等功能。
 *
 * @param raw_msg 原始消息字符串，格式为：type|sender|receiver|timestamp|content
 * @return Message* 成功返回解析后的消息结构体指针，失败返回NULL
 */
Message *parse_message(const char *raw_msg)
{
	if (!raw_msg || strlen(raw_msg) == 0)
	{
		// LOG_ERROR("Empty message received");
		LOG_ERROR("空的信息");
		return NULL;
	}

	LOG_DEBUG("Parsing message format: %s", raw_msg);

	if (!validate_message(raw_msg))
	{
		LOG_ERROR("Invalid message format: %s", raw_msg);
		return NULL;
	}

	Message *msg = (Message *)safe_malloc(sizeof(Message));
	if (!msg)
	{
		LOG_ERROR("Memory allocation failed for Message");
		return NULL;
	}

	memset(msg, 0, sizeof(Message));
	msg->next = NULL;
	msg->prev = NULL;
	msg->message_id = 0;
	msg->is_delivered = 0;

	char *msg_copy = strdup(raw_msg);
	if (!msg_copy)
	{
		LOG_ERROR("Failed to duplicate message string");
		safe_free((void **)&msg);
		return NULL;
	}

	size_t len = strlen(msg_copy);
	if (len > 0 && msg_copy[len - 1] == '\n')
	{
		msg_copy[len - 1] = '\0';
	}

	/* 按未被转义的分隔符手动切分字段，strtok 无法识别转义序列
	   为避免当 content 中包含额外的分隔符导致 fields 数组越界，
	   只在前 FIELD_COUNT-1 个未转义分隔符处分割，其后的分隔符都
	   视为 content 字段的一部分。 */
	char *fields[FIELD_COUNT] = {NULL};
	int field_index = 0;
	char *start = msg_copy;
	size_t mlen = strlen(msg_copy);
	for (size_t i = 0; i < mlen; i++)
	{
		if (msg_copy[i] == FIELD_DELIMITER[0])
		{
			/* 计算当前位置前连续的转义字符数量，若为偶数则分隔符有效 */
			int backslashes = 0;
			int idx = (int)i - 1;
			for (int k = idx; k >= 0 && msg_copy[k] == ESCAPE_CHAR; k--)
			{
				backslashes++;
			}
			if (backslashes % 2 == 0)
			{
				/* 仅在尚未达到字段上限时分割，最后一个字段包含剩余所有内容 */
				if (field_index < FIELD_COUNT - 1)
				{
					msg_copy[i] = '\0';
					fields[field_index++] = start;
					start = &msg_copy[i + 1];
				}
				/* 否则把这个分隔符视为内容的一部分，继续扫描 */
			}
		}
	}
	/* 最后一个字段指向剩余字符串（即 content） */
	fields[field_index++] = start;

	if (field_index < FIELD_COUNT)
	{
		LOG_ERROR("Invalid field count: %d (expected at least %d)", field_index, FIELD_COUNT);
		safe_free((void **)&msg_copy);
		safe_free((void **)&msg);
		return NULL;
	}

	/* 如果字段超过 FIELD_COUNT，则将多余的字段合并到最后的 content 字段中 */
	if (field_index > FIELD_COUNT)
	{
		/* 合并 fields[FIELD_CONTENT] .. fields[field_index-1] */
		size_t total_len = 0;
		for (int j = FIELD_CONTENT; j < field_index; j++)
		{
			total_len += strlen(fields[j]) + 1; /* 包括分隔符 */
		}
		char *merged = (char *)safe_malloc(total_len + 1);
		if (!merged)
		{
			LOG_ERROR("Failed to allocate merged content");
			safe_free((void **)&msg_copy);
			safe_free((void **)&msg);
			return NULL;
		}
		merged[0] = '\0';
		for (int j = FIELD_CONTENT; j < field_index; j++)
		{
			if (j > FIELD_CONTENT)
				strcat(merged, FIELD_DELIMITER);
			strcat(merged, fields[j]);
		}
		/* 将合并后的字符串放回 fields[FIELD_CONTENT]，并置后续为 NULL */
		fields[FIELD_CONTENT] = merged;
		for (int j = FIELD_CONTENT + 1; j < field_index; j++)
			fields[j] = NULL;
		field_index = FIELD_COUNT;
	}

	for (int i = 0; i < FIELD_COUNT; i++)
	{
		if (!fields[i])
		{
			LOG_ERROR("Field %d is NULL", i);
			safe_free((void **)&msg_copy);
			safe_free((void **)&msg);
			return NULL;
		}
		char *unescaped = unescape_field(fields[i]);
		if (!unescaped)
		{
			LOG_ERROR("Failed to unescaped field %d", i);
			safe_free((void **)&msg_copy);
			safe_free((void **)&msg);
			return NULL;
		}
		switch (i)
		{
		case FIELD_TYPE:
			safe_strcpy(msg->type, unescaped, sizeof(msg->type));
			break;
		case FIELD_SENDER:
			safe_strcpy(msg->sender, unescaped, sizeof(msg->sender));
			break;
		case FIELD_RECEIVER:
			safe_strcpy(msg->receiver, unescaped, sizeof(msg->receiver));
			break;
		case FIELD_TIMESTAMP:
			safe_strcpy(msg->timestamp, unescaped, sizeof(msg->timestamp));
			break;
		case FIELD_CONTENT:
			safe_strcpy(msg->content, unescaped, sizeof(msg->content));
			break;
		}
		free(unescaped);
	}
	/* 如果我们为合并分配了内存，释放 msg_copy 后需要释放 merged（已替换到 fields[FIELD_CONTENT]） */
	if (field_index == FIELD_COUNT && fields[FIELD_CONTENT] && fields[FIELD_CONTENT] != start)
	{
		/* merged was allocated via safe_malloc and assigned to fields[FIELD_CONTENT]; it will be freed by safe_free above? */
	}
	safe_free((void **)&msg_copy);

	if (strlen(msg->type) == 0)
	{
		LOG_ERROR("Message type is empty");
		safe_free((void **)&msg);
		return NULL;
	}
	if (!is_valid_msg_type(msg->type))
	{
		LOG_ERROR("Invalid message type: %s", msg->type);
		safe_free((void **)&msg);
		return NULL;
	}

	msg->message_id = atomic_fetch_add(&message_id_counter, 1);

	if (strlen(msg->timestamp) == 0)
	{
		char *timestamp = get_current_timestamp();
		if (timestamp)
		{
			safe_strcpy(msg->timestamp, timestamp, sizeof(msg->timestamp));
			safe_free((void **)&timestamp);
		}
	}

	LOG_DEBUG("Successfully parsed message: id=%d, type=%s, sender=%s, receiver=%s", msg->message_id, msg->type, msg->sender, msg->receiver);
	return msg;
}
/**
 * @brief 将Message结构体序列化为字符串
 *
 * 该函数将Message结构体转换为字符串格式，以便于网络传输。
 * 序列化过程中会对所有字段进行转义处理，确保特殊字符不会干扰消息解析。
 *
 * @param msg 要序列化的消息结构体指针
 * @return char* 成功返回序列化后的字符串，失败返回NULL
 */
char *serialize_message(const Message *msg)
{
	if (!msg)
	{
		LOG_ERROR("Cannot serialize NULL message");
		return NULL;
	}

	// 验证消息
	if (strlen(msg->type) == 0)
	{
		LOG_ERROR("Message type is empty");
		return NULL;
	}

	// 转义所有字段
	char *escaped_type = escape_field(msg->type);
	char *escaped_sender = escape_field(msg->sender);
	char *escaped_receiver = escape_field(msg->receiver);
	char *escaped_timestamp = escape_field(msg->timestamp);
	char *escaped_content = escape_field(msg->content);

	if (!escaped_type || !escaped_sender || !escaped_receiver ||
		!escaped_timestamp || !escaped_content)
	{
		LOG_ERROR("Failed to escape message fields");
		free(escaped_type);
		free(escaped_sender);
		free(escaped_receiver);
		free(escaped_timestamp);
		free(escaped_content);
		return NULL;
	}

	// 计算缓冲区大小
	size_t buffer_size = strlen(escaped_type) + strlen(escaped_sender) +
						 strlen(escaped_receiver) + strlen(escaped_timestamp) +
						 strlen(escaped_content) + 10; // 分隔符和结尾

	char *buffer = (char *)safe_malloc(buffer_size);
	if (!buffer)
	{
		LOG_ERROR("Memory allocation failed for serialization buffer");
		free(escaped_type);
		free(escaped_sender);
		free(escaped_receiver);
		free(escaped_timestamp);
		free(escaped_content);
		return NULL;
	}

	// 格式化消息
	snprintf(buffer, buffer_size, "%s|%s|%s|%s|%s\n",
			 escaped_type,
			 escaped_sender,
			 escaped_receiver,
			 escaped_timestamp,
			 escaped_content);

	// 清理
	free(escaped_type);
	free(escaped_sender);
	free(escaped_receiver);
	free(escaped_timestamp);
	free(escaped_content);

	LOG_DEBUG("Serialized message: %s", buffer);

	return buffer;
}

/**
 * @brief 验证消息格式
 *
 * 该函数验证原始消息字符串是否符合协议格式要求，包括：
 * 1. 消息不为空
 * 2. 消息长度在合理范围内
 * 3. 分隔符数量正确（4个）
 * 4. 消息不以未转义的反斜杠结尾
 *
 * @param raw_msg 要验证的原始消息字符串
 * @return int 验证通过返回1(真)，失败返回0(假)
 */
int validate_message(const char *raw_msg)
{
	if (!raw_msg)
	{
		LOG_DEBUG("NULL message received");
		return 0;
	}

	size_t len = strlen(raw_msg);
	if (len < 5)
	{
		LOG_DEBUG("Message too short: %zu", len);
		return 0;
	}

	if (len > 1024)
	{ // 合理的大小限制
		LOG_DEBUG("Message too long: %zu", len);
		return 0;
	}

	// 统计分隔符数量（忽略被转义的分隔符）
	int delimiter_count = 0;
	for (size_t i = 0; i < len; i++)
	{
		if (raw_msg[i] == FIELD_DELIMITER[0])
		{
			// 计算当前位置前连续的转义字符('\')数量，如果为偶数则该分隔符有效
			int backslashes = 0;
			int idx = (int)i - 1;
			for (int k = idx; k >= 0 && raw_msg[k] == ESCAPE_CHAR; k--)
			{
				backslashes++;
			}
			if (backslashes % 2 == 0)
			{
				delimiter_count++;
			}
		}
	}

	// 至少应有 4 个分隔符；允许更多（例如响应消息的 content 里可能包含额外的分隔符）
	if (delimiter_count < FIELD_COUNT - 1)
	{
		LOG_DEBUG("Invalid delimiter count: %d (expected at least %d)",
				  delimiter_count, FIELD_COUNT - 1);
		return 0;
	}

	/* 检查是否存在尾部孤立的转义字符（以单个反斜杠结尾且未被转义） */
	if (len > 0 && raw_msg[len - 1] == ESCAPE_CHAR)
	{
		/* 统计结尾处连续反斜杠的数量 */
		int backslashes = 0;
		for (int i = (int)len - 1; i >= 0 && raw_msg[i] == ESCAPE_CHAR; i--)
			backslashes++;
		if (backslashes % 2 == 1)
		{
			LOG_DEBUG("Message ends with an unescaped backslash");
			return 0;
		}
	}

	return 1;
}

/**
 * @brief 识别命令类型
 *
 * 根据消息类型字符串返回对应的命令枚举值，用于后续处理。
 * 对于响应消息(ERROR和OK)，返回CMD_UNKNOWN表示不是命令。
 *
 * @param type_str 消息类型字符串
 * @return CommandType 对应的命令类型枚举值
 */
CommandType get_command_type(const char *type_str)
{
	if (!type_str)
		return CMD_UNKNOWN;

	if (strcmp(type_str, MSG_TYPE_LOGIN) == 0)
	{
		return CMD_LOGIN;
	}
	else if (strcmp(type_str, MSG_TYPE_LOGOUT) == 0)
	{
		return CMD_LOGOUT;
	}
	else if (strcmp(type_str, MSG_TYPE_MSG) == 0)
	{
		return CMD_SEND_MSG;
	}
	else if (strcmp(type_str, MSG_TYPE_BROADCAST) == 0)
	{
		return CMD_BROADCAST;
	}
	else if (strcmp(type_str, MSG_TYPE_GROUP) == 0)
	{
		return CMD_JOIN_GROUP; // 简化处理
	}
	else if (strcmp(type_str, MSG_TYPE_HISTORY) == 0)
	{
		return CMD_GET_HISTORY;
	}
	else if (strcmp(type_str, MSG_TYPE_STATUS) == 0)
	{
		return CMD_GET_STATUS;
	}
	else if (strcmp(type_str, MSG_TYPE_ERROR) == 0 ||
			 strcmp(type_str, MSG_TYPE_OK) == 0)
	{
		return CMD_UNKNOWN; // 响应消息
	}

	return CMD_UNKNOWN;
}

/**
 * @brief 获取命令字符串
 *
 * 根据命令类型枚举值返回对应的消息类型字符串，用于构建消息。
 *
 * @param type 命令类型枚举值
 * @return const char* 对应的消息类型字符串
 */
const char *get_command_str(CommandType type)
{
	switch (type)
	{
	case CMD_LOGIN:
		return MSG_TYPE_LOGIN;
	case CMD_LOGOUT:
		return MSG_TYPE_LOGOUT;
	case CMD_SEND_MSG:
		return MSG_TYPE_MSG;
	case CMD_BROADCAST:
		return MSG_TYPE_BROADCAST;
	case CMD_JOIN_GROUP:
		return MSG_TYPE_GROUP;
	case CMD_LEAVE_GROUP:
		return MSG_TYPE_GROUP;
	case CMD_GET_HISTORY:
		return MSG_TYPE_HISTORY;
	case CMD_GET_STATUS:
		return MSG_TYPE_STATUS;
	default:
		return "UNKNOWN";
	}
}

/**
 * @brief 验证消息类型
 *
 * 检查给定的消息类型字符串是否为有效的消息类型。
 * 有效类型包括：LOGIN、LOGOUT、MSG、BROADCAST、GROUP、HISTORY、STATUS、ERROR、OK。
 *
 * @param type 要验证的消息类型字符串
 * @return int 有效返回1(真)，无效返回0(假)
 */
int is_valid_msg_type(const char *type)
{
	if (!type)
		return 0;

	return (strcmp(type, MSG_TYPE_LOGIN) == 0 ||
			strcmp(type, MSG_TYPE_LOGOUT) == 0 ||
			strcmp(type, MSG_TYPE_MSG) == 0 ||
			strcmp(type, MSG_TYPE_BROADCAST) == 0 ||
			strcmp(type, MSG_TYPE_GROUP) == 0 ||
			strcmp(type, MSG_TYPE_HISTORY) == 0 ||
			strcmp(type, MSG_TYPE_STATUS) == 0 ||
			strcmp(type, MSG_TYPE_ERROR) == 0 ||
			strcmp(type, MSG_TYPE_OK) == 0);
}

/**
 * @brief 验证用户名
 *
 * 检查用户名是否符合规范：
 * 1. 不为空
 * 2. 长度在1到MAX_USERNAME_LEN-1之间
 * 3. 只包含字母、数字和下划线
 *
 * @param username 要验证的用户名字符串
 * @return int 有效返回1(真)，无效返回0(假)
 */
int is_valid_username(const char *username)
{
	if (!username)
		return 0;

	size_t len = strlen(username);
	if (len < 1 || len > MAX_USERNAME_LEN - 1)
	{
		return 0;
	}

	// 检查是否只包含有效字符（字母、数字、下划线）
	for (size_t i = 0; i < len; i++)
	{
		if (!isalnum(username[i]) && username[i] != '_')
		{
			return 0;
		}
	}

	return 1;
}

/**
 * @brief 转义字段中的特殊字符
 *
 * 对字段中的特殊字符进行转义处理，确保这些字符不会干扰消息解析。
 * 转义规则：
 * 1. 字段分隔符('|') -> '\d'
 * 2. 转义字符('\\') -> '\\\\'
 * 3. 换行符('\n') -> '\\n'
 *
 * @param field 要转义的字段字符串
 * @return char* 成功返回转义后的字符串，失败返回NULL
 */
char *escape_field(const char *field)
{
	if (!field)
	{
		return strdup("");
	}

	size_t len = strlen(field);
	size_t escape_count = 0;

	// 统计需要转义的字符
	for (size_t i = 0; i < len; i++)
	{
		if (field[i] == FIELD_DELIMITER[0] ||
			field[i] == ESCAPE_CHAR ||
			field[i] == '\n')
		{
			escape_count++;
		}
	}

	// 分配内存
	char *escaped = (char *)safe_malloc(len + escape_count + 1);
	if (!escaped)
	{
		LOG_ERROR("Memory allocation failed for escaped field");
		return NULL;
	}

	size_t j = 0;
	for (size_t i = 0; i < len; i++)
	{
		if (field[i] == FIELD_DELIMITER[0])
		{
			escaped[j++] = ESCAPE_CHAR;
			escaped[j++] = DELIMITER_ESCAPE;
		}
		else if (field[i] == ESCAPE_CHAR)
		{
			escaped[j++] = ESCAPE_CHAR;
			escaped[j++] = ESCAPE_CHAR;
		}
		else if (field[i] == '\n')
		{
			escaped[j++] = ESCAPE_CHAR;
			escaped[j++] = NEWLINE_ESCAPE;
		}
		else
		{
			escaped[j++] = field[i];
		}
	}
	escaped[j] = '\0';

	return escaped;
}

/**
 * @brief 反转义字段
 *
 * 将转义后的字段还原为原始内容，处理转义序列：
 * 1. '\d' -> '|'
 * 2. '\\\\' -> '\\'
 * 3. '\\n' -> '\n'
 *
 * @param field 要反转义的字段字符串
 * @return char* 成功返回反转义后的字符串，失败返回NULL
 */
char *unescape_field(const char *field)
{
	if (!field)
	{
		return strdup("");
	}

	size_t len = strlen(field);
	char *unescaped = (char *)safe_malloc(len + 1);
	if (!unescaped)
	{
		LOG_ERROR("Memory allocation failed for unescaped field");
		return NULL;
	}

	size_t j = 0;
	for (size_t i = 0; i < len; i++)
	{
		if (field[i] == ESCAPE_CHAR && i + 1 < len)
		{
			switch (field[i + 1])
			{
			case DELIMITER_ESCAPE:
				unescaped[j++] = FIELD_DELIMITER[0];
				i++;
				break;
			case ESCAPE_CHAR:
				unescaped[j++] = ESCAPE_CHAR;
				i++;
				break;
			case NEWLINE_ESCAPE:
				unescaped[j++] = '\n';
				i++;
				break;
			default:
				// 无效的转义序列，保留原样
				unescaped[j++] = field[i];
				break;
			}
		}
		else
		{
			unescaped[j++] = field[i];
		}
	}
	unescaped[j] = '\0';

	// 如果长度变短了，重新分配内存
	if (j < len)
	{
		char *temp = realloc(unescaped, j + 1);
		if (temp)
		{
			unescaped = temp;
		}
	}

	return unescaped;
}

/**
 * @brief 获取当前时间戳字符串
 *
 * 获取当前时间的格式化字符串，格式为"YYYY-MM-DD HH:MM:SS"。
 * 使用线程安全的localtime_r函数确保多线程环境下正常工作。
 *
 * @return char* 成功返回时间戳字符串，失败返回NULL
 */
char *get_current_timestamp(void)
{
	char *timestamp = (char *)safe_malloc(32);
	if (!timestamp)
	{
		return NULL;
	}

	time_t now = time(NULL);
	struct tm tm_info;
	if (localtime_r(&now, &tm_info) == NULL)
	{
		safe_free((void **)&timestamp);
		return NULL;
	}
	strftime(timestamp, 32, "%Y-%m-%d %H:%M:%S", &tm_info);

	return timestamp;
}

/**
 * @brief 解析群组ID
 *
 * 从接收者字段中解析出群组ID。接收者字段格式为"group:ID"或"group:all"。
 * "group:all"返回0表示所有群组，"group:数字"返回对应的群组ID。
 *
 * @param receiver 接收者字段字符串
 * @return int 成功返回群组ID，失败返回-1
 */
int parse_group_id(const char *receiver)
{
	if (!receiver)
		return -1;

	// 检查是否为群组消息
	if (strncmp(receiver, RECEIVER_GROUP_PREFIX, strlen(RECEIVER_GROUP_PREFIX)) != 0)
	{
		return -1;
	}

	const char *group_str = receiver + strlen(RECEIVER_GROUP_PREFIX);

	// 如果是"all"，返回特殊值
	if (strcmp(group_str, "all") == 0)
	{
		return 0; // 表示所有群组
	}

	// 尝试解析为整数
	char *endptr;
	long group_id = strtol(group_str, &endptr, 10);

	if (endptr == group_str || *endptr != '\0')
	{
		return -1; // 解析失败
	}

	return (int)group_id;
}

/**
 * @brief 消息类型检查函数
 *
 * 以下是一系列用于检查消息类型的辅助函数，简化了消息类型的判断逻辑。
 */

/**
 * @brief 检查是否为登录消息
 *
 * @param msg 要检查的消息指针
 * @return int 是登录消息返回1(真)，否则返回0(假)
 */
int is_login_msg(const Message *msg)
{
	return msg && strcmp(msg->type, MSG_TYPE_LOGIN) == 0;
}

/**
 * @brief 检查是否为登出消息
 *
 * @param msg 要检查的消息指针
 * @return int 是登出消息返回1(真)，否则返回0(假)
 */
int is_logout_msg(const Message *msg)
{
	return msg && strcmp(msg->type, MSG_TYPE_LOGOUT) == 0;
}

/**
 * @brief 检查是否为私聊消息
 *
 * 私聊消息是指普通消息(MSG类型)且接收者不是广播或群组。
 *
 * @param msg 要检查的消息指针
 * @return int 是私聊消息返回1(真)，否则返回0(假)
 */
int is_private_msg(const Message *msg)
{
	return msg && strcmp(msg->type, MSG_TYPE_MSG) == 0 &&
		   strcmp(msg->receiver, RECEIVER_BROADCAST) != 0 &&
		   strncmp(msg->receiver, RECEIVER_GROUP_PREFIX,
				   strlen(RECEIVER_GROUP_PREFIX)) != 0;
}

/**
 * @brief 检查是否为广播消息
 *
 * @param msg 要检查的消息指针
 * @return int 是广播消息返回1(真)，否则返回0(假)
 */
int is_broadcast_msg(const Message *msg)
{
	return msg && strcmp(msg->type, MSG_TYPE_BROADCAST) == 0;
}

/**
 * @brief 检查是否为群组消息
 *
 * @param msg 要检查的消息指针
 * @return int 是群组消息返回1(真)，否则返回0(假)
 */
int is_group_msg(const Message *msg)
{
	return msg && strcmp(msg->type, MSG_TYPE_GROUP) == 0;
}

/**
 * @brief 检查是否为历史记录请求
 *
 * @param msg 要检查的消息指针
 * @return int 是历史记录请求返回1(真)，否则返回0(假)
 */
int is_history_request(const Message *msg)
{
	return msg && strcmp(msg->type, MSG_TYPE_HISTORY) == 0;
}

/**
 * @brief 检查是否为状态请求
 *
 * @param msg 要检查的消息指针
 * @return int 是状态请求返回1(真)，否则返回0(假)
 */
int is_status_request(const Message *msg)
{
	return msg && strcmp(msg->type, MSG_TYPE_STATUS) == 0;
}

/*
 * @brief 释放 Message 结构体
 */
void free_message(Message *msg)
{
	if (!msg)
		return;
	safe_free((void **)&msg);
}

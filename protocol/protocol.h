#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdio.h>
#include "../models/models.h"
#include "../utils/utils.h"

#define FIELD_DELIMITER "|"
#define ESCAPE_CHAR '\\'
#define DELIMITER_ESCAPE '|'
#define NEWLINE_ESCAPE 'n'

#define PROTOCOL_VERSION "1.0"

#define FIELD_TYPE 0	  // 消息类型
#define FIELD_SENDER 1	  // 发送者
#define FIELD_RECEIVER 2  // 接收者
#define FIELD_TIMESTAMP 3 // 时间戳
#define FIELD_CONTENT 4	  // 内容
#define FIELD_COUNT 5	  // 字段总数

Message *parse_message(const char *raw_msg);
char *serialize_message(const Message *msg);

/* 命令类型识别 */
CommandType get_command_type(const char *type_str);
const char *get_command_str(CommandType type);

/* 消息构建相关 - 直接返回格式化字符串 */
char *build_login_msg(const char *username, const char *password);
char *build_logout_msg(const char *username);
char *build_text_msg(const char *sender, const char *receiver,
					 const char *content);
char *build_broadcast_msg(const char *sender, const char *content);
char *build_group_msg(const char *sender, const char *group_name,
					  const char *content);
char *build_history_request(const char *username, const char *target,
							const char *start_time, const char *end_time);
char *build_status_request(const char *username);

/* 额外的构建器函数原型 */
char *build_response_from_struct(const Response *resp);
char *build_user_online_msg(const char *username);
char *build_user_offline_msg(const char *username);
char *build_system_notification(const char *content);

/* 响应消息构建 - 根据你的Response结构体 */
char *build_response_msg(int code, const char *type, const char *message);
char *build_success_msg(const char *message);
char *build_error_msg(int error_code, const char *message);

/* 协议验证 */
int validate_message(const char *raw_msg);
int is_valid_msg_type(const char *type);
int is_valid_username(const char *username);

/* 辅助函数 */
char *escape_field(const char *field);
char *unescape_field(const char *field);
char *get_current_timestamp(void);
int parse_group_id(const char *receiver);

/* 释放消息结构体内存 */
void free_message(Message *msg);

/* 消息类型检查 */
int is_login_msg(const Message *msg);
int is_logout_msg(const Message *msg);
int is_private_msg(const Message *msg);
int is_broadcast_msg(const Message *msg);
int is_group_msg(const Message *msg);
int is_history_request(const Message *msg);
int is_status_request(const Message *msg);

int handle_command(int client_fd, Message *msg);
int handle_raw_message(int client_fd, const char *raw_message);
#endif /*PROTOCOL_H*/

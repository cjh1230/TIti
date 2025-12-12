// core/core.h
#ifndef CORE_H
#define CORE_H

#include <time.h>
#include "../models/models.h"
#include "../utils/utils.h"
#include "../protocol/protocol.h"

/* ================ 连接管理器函数 ================ */

/* 客户端管理 */
Client *connection_manager_find_by_fd(int fd);
Client *connection_manager_find_by_username(const char *username);
Client *connection_manager_find_by_user_id(int user_id);
void connection_manager_add_from_fd(int sockfd, const char *ip, int port);
void connection_manager_remove(int fd);
int connection_manager_count(void);

/* 客户端状态 */
void connection_manager_update_active(int fd);
int connection_manager_set_auth(int fd, int user_id, const char *username);
void connection_manager_set_status(int fd, int status);

/* 工具函数 */
void connection_manager_print_all(void);
void connection_manager_cleanup(void);
Client **connection_manager_get_all(int *out_count);

/* ================ 会话管理器函数 ================ */

int session_manager_authenticate(int fd, const char *username, const char *password);
void session_manager_logout(int fd);
int session_manager_is_authenticated(int fd);
int session_manager_get_user_id(int fd);
const char *session_manager_get_username(int fd);

/* 检查用户在线状态和获取在线用户列表（测试/工具） */
int session_manager_is_user_online(const char *username);
int session_manager_get_online_users(char ***usernames, int *count);

/* ================ 消息路由器函数 ================ */

int message_router_route(Message *msg);
int message_router_send_to_user(const char *username, const char *content, const char *sender);
int message_router_broadcast(const char *content, const char *sender);
int message_router_send_to_group(const char *group_name, const char *content, const char *sender);

int route_message(Message *msg);
int send_to_user(const char *username, const char *message_str);
int send_response(int client_fd, int code, const char *type, const char *message);
#endif /* CORE_H */

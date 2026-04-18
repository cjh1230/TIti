// core/core.h
#ifndef CORE_H
#define CORE_H

#include <time.h>
#include "../models/models.h"
#include "../utils/utils.h"
#include "../protocol/protocol.h"

/* ================ 连接管理器函数 ================ */

/* 客户端管理 */
Client *connection_manager_find_by_fd(socket_t fd);
Client *connection_manager_find_by_username(const char *username);
void connection_manager_add_from_fd(socket_t sockfd, const char *ip, int port);
void connection_manager_remove(socket_t fd);
int connection_manager_count(void);

/* 客户端状态 */
void connection_manager_update_active(socket_t fd);
int connection_manager_set_auth(socket_t fd, int user_id, const char *username);
void connection_manager_set_status(socket_t fd, int status);

/* 工具函数 */
void connection_manager_print_all(void);
void connection_manager_cleanup(void);
Client **connection_manager_get_all(int *out_count);

/* ================ 会话管理器函数 ================ */

int session_manager_authenticate(socket_t fd, const char *username, const char *password);
void session_manager_logout(socket_t fd);
int session_manager_is_authenticated(socket_t fd);
int session_manager_get_user_id(socket_t fd);
const char *session_manager_get_username(socket_t fd);

/* 检查用户在线状态和获取在线用户列表（测试/工具） */
int session_manager_is_user_online(const char *username);
int session_manager_get_online_users(char ***usernames, int *count);

/* ================ 消息路由器函数 ================ */

int route_message(Message *msg);
#endif /* CORE_H */

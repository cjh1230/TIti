#ifndef STORAGE_H
#define STORAGE_H

#include "../models/models.h"

/* ================ 用户存储函数 ================ */

/* 用户管理 */
User *user_store_find_by_username(const char *username);
User *user_store_find_by_id(int user_id);
int user_store_add(const char *username, const char *password);
int user_store_remove(const char *username);
int user_store_count(void);

/* 用户验证 */
int user_store_authenticate(const char *username, const char *password);
int user_store_change_password(const char *username, const char *new_password);

/* 用户列表 */
User **user_store_get_all(int *count);
void user_store_print_all(void);

/* 测试辅助：初始化默认用户 */
void user_store_init_defaults(void);

/* ================ 历史消息存储函数 ================ */

int history_manager_save(Message *msg);
Message **history_manager_get_for_user(const char *username, int *count);
Message **history_manager_get_for_group(const char *group_name, int *count);
Message **history_manager_get_recent(int limit, int *count);
void history_manager_cleanup_old(time_t before_time);

/* 初始化/清理 */
void storage_init(void);
void storage_cleanup(void);

#endif /* STORAGE_H */

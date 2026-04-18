#ifndef STORAGE_H
#define STORAGE_H

#include "../models/models.h"

/* ================ 用户存储函数 ================ */

/* 用户管理 */
User *user_store_find_by_username(const char *username);
User *user_store_find_by_id(int user_id);
int user_store_add(const char *username, const char *password);
int user_store_count(void);

/* 用户验证 */
int user_store_authenticate(const char *username, const char *password);

/* 用户列表 */
void user_store_print_all(void);

/* 测试辅助：初始化默认用户 */
void user_store_init_defaults(void);

/* 初始化/清理 */
void storage_init(void);
void storage_cleanup(void);

#endif /* STORAGE_H */

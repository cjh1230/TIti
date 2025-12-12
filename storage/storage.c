/**
 * @file storage.c
 * @brief 存储模块初始化/清理实现
 */

#include "storage.h"
#include "../utils/utils.h"

/* 初始化存储子模块 */
void storage_init(void)
{
	/* 目前仅初始化默认用户，未来可扩展为加载持久化数据 */
	user_store_init_defaults();

	LOG_INFO("Storage initialized");
}

/* 清理存储子模块（占位） */
void storage_cleanup(void)
{
	/* 目前没有持久化资源需要清理 */
	LOG_INFO("Storage cleaned up");
}

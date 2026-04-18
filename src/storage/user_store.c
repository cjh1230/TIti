/**
 * @file user_store.c
 * @brief 用户存储实现
 * 
 * 本文件实现了用户数据的存储和管理功能，使用链表结构存储用户信息。
 * 提供了用户创建、查找、添加、认证等功能。
 * 
 * 注意：当前实现使用明文密码存储，实际项目中应使用密码哈希。
 * 
 * 主要功能：
 * 1. 用户创建与管理
 * 2. 用户认证
 * 3. 用户信息查询
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "storage.h"
#include "../utils/utils.h"

/**
 * @brief 全局用户链表头指针
 * 
 * 指向用户链表的第一个节点，所有用户信息都通过这个链表访问。
 */
static User *user_list_head = NULL;

/**
 * @brief 用户ID计数器
 * 
 * 用于为新注册的用户分配唯一ID，初始值为1000，每次分配后自动递增。
 */
static int user_id_counter = 1000;

/**
 * @brief 创建新用户
 * 
 * 根据提供的用户名和密码创建新用户，并分配唯一ID。
 * 新用户默认为激活状态，并记录注册时间。
 * 
 * @param username 用户名
 * @param password 密码
 * @return User* 成功返回用户结构体指针，失败返回NULL
 */
static User *create_user(const char *username, const char *password)
{
	User *user = (User *)safe_malloc(sizeof(User));
	if (!user)
	{
		LOG_ERROR("Failed to allocate memory for user");
		return NULL;
	}

	memset(user, 0, sizeof(User));
	safe_strcpy(user->username, username, sizeof(user->username));
	safe_strcpy(user->password, password, sizeof(user->password));
	user->user_id = user_id_counter++;
	user->register_time = time(NULL);
	user->is_active = 1;

	return user;
}

/**
 * @brief 根据用户名查找用户
 * 
 * 遍历用户链表，查找具有指定用户名的用户。
 * 
 * @param username 要查找的用户名
 * @return User* 找到返回用户结构体指针，未找到返回NULL
 */
User *user_store_find_by_username(const char *username)
{
	if (!username)
		return NULL;

	User *current = user_list_head;
	while (current != NULL)
	{
		if (strcmp(current->username, username) == 0)
		{
			return current;
		}
		current = current->next;
	}

	return NULL;
}

/**
 * @brief 根据用户ID查找用户
 * 
 * 遍历用户链表，查找具有指定用户ID的用户。
 * 
 * @param user_id 要查找的用户ID
 * @return User* 找到返回用户结构体指针，未找到返回NULL
 */
User *user_store_find_by_id(int user_id)
{
	User *current = user_list_head;
	while (current != NULL)
	{
		if (current->user_id == user_id)
		{
			return current;
		}
		current = current->next;
	}

	return NULL;
}

/**
 * @brief 添加用户
 * 
 * 根据提供的用户名和密码添加新用户到存储中。
 * 添加流程包括：
 * 1. 验证参数有效性
 * 2. 检查用户是否已存在
 * 3. 创建新用户
 * 4. 将用户添加到链表头部
 * 
 * @param username 用户名
 * @param password 密码
 * @return int 成功返回1，失败返回0
 */
int user_store_add(const char *username, const char *password)
{
	if (!username || !password)
	{
		LOG_ERROR("Invalid parameters for user addition");
		return 0;
	}

	// 检查用户是否已存在
	if (user_store_find_by_username(username) != NULL)
	{
		LOG_WARN("User already exists: %s", username);
		return 0;
	}

	// 创建新用户
	User *user = create_user(username, password);
	if (!user)
	{
		return 0;
	}

	// 添加到链表头部
	user->next = user_list_head;
	user_list_head = user;

	LOG_INFO("User added: %s (id=%d)", username, user->user_id);
	return 1;
}

/**
 * @brief 用户认证
 * 
 * 验证用户提供的用户名和密码是否正确。
 * 认证流程包括：
 * 1. 查找用户
 * 2. 检查用户是否激活
 * 3. 验证密码
 * 
 * 注意：当前实现使用明文密码比较，实际项目中应使用密码哈希。
 * 
 * @param username 用户名
 * @param password 密码
 * @return int 认证成功返回1，失败返回0
 */
int user_store_authenticate(const char *username, const char *password)
{
	if (!username || !password)
	{
		return 0;
	}

	User *user = user_store_find_by_username(username);
	if (!user)
	{
		LOG_WARN("User not found: %s", username);
		return 0;
	}

	if (!user->is_active)
	{
		LOG_WARN("User account is inactive: %s", username);
		return 0;
	}

	// 简单密码比较（实际项目应该使用哈希）
	if (strcmp(user->password, password) == 0)
	{
		LOG_INFO("User authenticated: %s", username);
		return 1;
	}

	LOG_WARN("Authentication failed for user: %s", username);
	return 0;
}

/**
 * @brief 初始化默认用户（用于测试）
 * 
 * 添加一些默认用户账号用于测试和演示。
 * 默认用户包括：admin、alice、bob和charlie。
 */
void user_store_init_defaults(void)
{
	user_store_add("admin", "admin123");
	user_store_add("alice", "alice123");
	user_store_add("bob", "bob123");
	user_store_add("charlie", "charlie123");

	LOG_INFO("Initialized default users");
}

/**
 * @brief 获取用户数量
 * 
 * 遍历用户链表，统计当前存储的用户总数。
 * 
 * @return int 用户总数
 */
int user_store_count(void)
{
	int count = 0;
	User *current = user_list_head;

	while (current != NULL)
	{
		count++;
		current = current->next;
	}

	return count;
}

/**
 * @brief 打印所有用户（调试用）
 * 
 * 遍历用户链表，打印每个用户的详细信息，
 * 包括ID、用户名、注册时间和激活状态。
 */
void user_store_print_all(void)
{
	printf("=== Registered Users (%d) ===\n", user_store_count());

	User *current = user_list_head;
	while (current != NULL)
	{
		char time_buf[32];
		strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S",
				 localtime(&current->register_time));

		printf("ID: %d, Username: %s, Registered: %s, Active: %s\n",
			   current->user_id, current->username,
			   time_buf, current->is_active ? "Yes" : "No");
		current = current->next;
	}
	printf("==============================\n");
}

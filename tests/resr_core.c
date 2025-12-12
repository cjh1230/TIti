// tests/test_core.c
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../core/core.h"
#include "../storage/storage.h"
#include "../utils/utils.h"

void test_connection_manager()
{
	printf("Testing Connection Manager...\n");

	// 模拟添加客户端
	connection_manager_add_from_fd(10, "192.168.1.100", 12345);
	connection_manager_add_from_fd(11, "192.168.1.101", 12346);

	// 测试查找
	Client *client = connection_manager_find_by_fd(10);
	assert(client != NULL);
	assert(client->sockfd == 10);
	assert(strcmp(client->remote_ip, "192.168.1.100") == 0);

	// 测试计数
	assert(connection_manager_count() == 2);

	// 测试认证设置
	connection_manager_set_auth(10, 1001, "testuser");
	client = connection_manager_find_by_fd(10);
	assert(client->user_id == 1001);
	assert(strcmp(client->username, "testuser") == 0);
	assert(client->status == CLIENT_STATUS_AUTHENTICATED);

	// 测试按用户名查找
	client = connection_manager_find_by_username("testuser");
	assert(client != NULL);
	assert(client->sockfd == 10);

	// 测试移除
	connection_manager_remove(10);
	assert(connection_manager_count() == 1);
	assert(connection_manager_find_by_fd(10) == NULL);

	printf("  ✓ Connection Manager tests passed\n");

	// 清理
	connection_manager_remove(11);
}

void test_user_store()
{
	printf("Testing User Store...\n");

	// 初始化默认用户
	user_store_init_defaults();

	// 测试查找
	User *user = user_store_find_by_username("admin");
	assert(user != NULL);
	assert(strcmp(user->username, "admin") == 0);

	// 测试认证
	assert(user_store_authenticate("admin", "admin123") == 1);
	assert(user_store_authenticate("admin", "wrongpass") == 0);
	assert(user_store_authenticate("nonexistent", "pass") == 0);

	// 测试添加新用户
	assert(user_store_add("newuser", "newpass") == 1);
	assert(user_store_find_by_username("newuser") != NULL);

	// 测试重复添加
	assert(user_store_add("newuser", "anotherpass") == 0);

	// 测试用户数量
	assert(user_store_count() >= 5); // 4默认 + 1新增

	printf("  ✓ User Store tests passed\n");

	// 打印用户列表（调试）
	user_store_print_all();
}

void test_integration()
{
	printf("Testing Integration...\n");

	// 创建客户端
	connection_manager_add_from_fd(20, "192.168.1.102", 12347);

	// 模拟认证流程
	// 1. 验证用户
	assert(user_store_authenticate("alice", "alice123") == 1);

	// 2. 获取用户信息
	User *user = user_store_find_by_username("alice");
	assert(user != NULL);

	// 3. 设置客户端认证状态
	connection_manager_set_auth(20, user->user_id, user->username);

	// 4. 验证认证状态
	Client *client = connection_manager_find_by_fd(20);
	assert(client != NULL);
	assert(client->status == CLIENT_STATUS_AUTHENTICATED);
	assert(strcmp(client->username, "alice") == 0);

	printf("  ✓ Integration tests passed\n");

	// 清理
	connection_manager_remove(20);
}

int main()
{
	set_log_file(NULL);
	set_log_level(LOG_INFO);

	printf("=== Core Module Tests ===\n\n");

	test_connection_manager();
	test_user_store();
	test_integration();

	printf("\n=== All core tests passed! ===\n");

	return 0;
}

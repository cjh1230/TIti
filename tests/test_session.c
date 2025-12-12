// tests/test_session.c
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../core/core.h"
#include "../storage/storage.h"

int main()
{
	printf("=== Session Manager Test ===\n\n");

	// 初始化用户存储
	printf("Initializing user store...\n");
	user_store_init_defaults();
	user_store_print_all();

	// 添加测试客户端
	printf("\nAdding test clients...\n");
	connection_manager_add_from_fd(100, "192.168.1.100", 12345);
	connection_manager_add_from_fd(101, "192.168.1.101", 12346);

	// 测试1：正确认证
	printf("\nTest 1: Correct authentication...\n");
	int result = session_manager_authenticate(100, "alice", "alice123");
	assert(result == 1);
	printf("✓ Alice authenticated successfully\n");

	// 验证认证状态
	assert(session_manager_is_authenticated(100) == 1);
	assert(strcmp(session_manager_get_username(100), "alice") == 0);
	assert(session_manager_get_user_id(100) > 0);

	// 测试2：错误密码
	printf("\nTest 2: Wrong password...\n");
	result = session_manager_authenticate(101, "bob", "wrongpass");
	assert(result == 0);
	printf("✓ Wrong password rejected\n");

	// 测试3：不存在的用户
	printf("\nTest 3: Non-existent user...\n");
	result = session_manager_authenticate(101, "nonexistent", "pass");
	assert(result == 0);
	printf("✓ Non-existent user rejected\n");

	// 测试4：重复认证
	printf("\nTest 4: Re-authentication...\n");
	result = session_manager_authenticate(100, "alice", "alice123");
	assert(result == 1); // 应该成功（已经认证）
	printf("✓ Re-authentication handled\n");

	// 测试5：用户是否在线
	printf("\nTest 5: Check if user is online...\n");
	assert(session_manager_is_user_online("alice") == 1);
	assert(session_manager_is_user_online("bob") == 0);
	printf("✓ Online status checked\n");

	// 测试6：登出
	printf("\nTest 6: Logout...\n");
	session_manager_logout(100);
	assert(session_manager_is_authenticated(100) == 0);
	assert(session_manager_get_username(100) == NULL);
	printf("✓ Logout successful\n");

	// 测试7：再次登录
	printf("\nTest 7: Login after logout...\n");
	result = session_manager_authenticate(100, "bob", "bob123");
	assert(result == 1);
	assert(session_manager_is_authenticated(100) == 1);
	assert(strcmp(session_manager_get_username(100), "bob") == 0);
	printf("✓ Login after logout successful\n");

	// 清理
	printf("\nCleaning up...\n");
	connection_manager_remove(100);
	connection_manager_remove(101);
	connection_manager_cleanup();

	printf("\n=== All session tests passed! ===\n");
	return 0;
}

// tests/test_connection.c - 使用正确的函数名
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../models/models.h"

// 声明实际的连接管理器函数（根据你的头文件）
void connection_manager_add_from_fd(int sockfd, const char *ip, int port);
Client *connection_manager_find_by_fd(int fd);
void connection_manager_remove(int fd);
int connection_manager_count(void);
void connection_manager_set_status(int fd, int status);
void connection_manager_print_all(void);
void connection_manager_cleanup(void);

int main()
{
	printf("=== Connection Manager Test ===\n\n");

	// 测试1：添加客户端
	printf("Test 1: Adding clients...\n");
	connection_manager_add_from_fd(100, "192.168.1.100", 12345);
	connection_manager_add_from_fd(101, "192.168.1.101", 12346);

	int count = connection_manager_count();
	printf("Client count: %d\n", count);
	assert(count == 2);
	printf("✓ Added 2 clients successfully\n\n");

	// 测试2：查找客户端
	printf("Test 2: Finding clients...\n");
	Client *client1 = connection_manager_find_by_fd(100);
	Client *client2 = connection_manager_find_by_fd(101);

	assert(client1 != NULL);
	assert(client2 != NULL);
	assert(client1->sockfd == 100);
	assert(client2->sockfd == 101);
	assert(strcmp(client1->remote_ip, "192.168.1.100") == 0);
	assert(strcmp(client2->remote_ip, "192.168.1.101") == 0);
	printf("✓ Found clients by fd\n\n");

	// 测试3：设置客户端状态
	printf("Test 3: Setting client status...\n");
	connection_manager_set_status(100, CLIENT_STATUS_AUTHENTICATED);

	client1 = connection_manager_find_by_fd(100);
	assert(client1->status == CLIENT_STATUS_AUTHENTICATED);
	printf("✓ Set client status\n\n");

	// 测试4：移除客户端
	printf("Test 4: Removing client...\n");
	connection_manager_remove(100);

	count = connection_manager_count();
	assert(count == 1);
	assert(connection_manager_find_by_fd(100) == NULL);
	assert(connection_manager_find_by_fd(101) != NULL);
	printf("✓ Removed client successfully\n\n");

	// 测试5：打印所有客户端
	printf("Test 5: Printing all clients...\n");
	connection_manager_print_all();
	printf("✓ Printed client list\n\n");

	// 清理
	printf("Cleaning up...\n");
	connection_manager_cleanup();
	count = connection_manager_count();
	assert(count == 0);

	printf("\n=== All tests passed! ===\n");
	return 0;
}

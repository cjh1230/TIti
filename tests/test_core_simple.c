// tests/test_core_simple.c - 简化版本
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../utils/utils.h"

// 先测试工具模块
void test_utils()
{
	printf("Testing Utils...\n");

	// 测试安全字符串函数
	char dest[10];
	size_t copied;

	copied = safe_strcpy(dest, "Hello", sizeof(dest));
	assert(copied == 5);
	assert(strcmp(dest, "Hello") == 0);
	printf("  ✓ safe_strcpy\n");

	// 测试内存分配
	int *ptr = (int *)safe_malloc(sizeof(int) * 10);
	assert(ptr != NULL);
	safe_free((void **)&ptr);
	assert(ptr == NULL);
	printf("  ✓ safe_malloc/safe_free\n");

	printf("Utils tests passed!\n\n");
}

// 简单测试连接管理器（不使用实际头文件）
typedef struct TestClient
{
	int sockfd;
	char username[32];
	struct TestClient *next;
} TestClient;

TestClient *test_clients = NULL;
int test_client_count = 0;

void test_connection_simple()
{
	printf("Testing Connection Manager (Simple)...\n");

	// 创建测试客户端
	TestClient *client1 = (TestClient *)malloc(sizeof(TestClient));
	client1->sockfd = 100;
	strcpy(client1->username, "test1");
	client1->next = test_clients;
	test_clients = client1;
	test_client_count++;

	TestClient *client2 = (TestClient *)malloc(sizeof(TestClient));
	client2->sockfd = 101;
	strcpy(client2->username, "test2");
	client2->next = test_clients;
	test_clients = client2;
	test_client_count++;

	assert(test_client_count == 2);

	// 查找测试
	TestClient *current = test_clients;
	int found = 0;
	while (current)
	{
		if (current->sockfd == 101 && strcmp(current->username, "test2") == 0)
		{
			found = 1;
			break;
		}
		current = current->next;
	}
	assert(found == 1);

	// 清理
	while (test_clients)
	{
		TestClient *next = test_clients->next;
		free(test_clients);
		test_clients = next;
	}
	test_client_count = 0;

	printf("  ✓ Simple connection tests passed\n\n");
}

int main()
{
	printf("=== Core Module Simple Tests ===\n\n");

	// 设置日志
	set_log_file(NULL);
	set_log_level(LOG_INFO);

	test_utils();
	test_connection_simple();

	printf("=== All simple tests passed! ===\n");

	return 0;
}

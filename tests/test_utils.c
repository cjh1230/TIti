// tests/test_utils.c - 创建这个文件
#include <stdio.h>
#include <string.h>
#include "../utils/utils.h"

int main()
{
	set_log_file(NULL);
	set_log_level(LOG_DEBUG);

	printf("=== Utils Module Tests ===\n\n");

	// 测试日志
	LOG_DEBUG("Debug message test");
	LOG_INFO("Info message test");
	LOG_WARN("Warning message test");
	LOG_ERROR("Error message test");

	// 测试时间函数
	char time_buf[32];
	get_current_time(time_buf, sizeof(time_buf));
	printf("Current time: %s\n", time_buf);

	// 测试安全字符串函数
	char dest[10];
	size_t copied = safe_strcpy(dest, "Hello", sizeof(dest));
	printf("Copied %zu characters: %s\n", copied, dest);

	// 测试连接和溢出
	safe_strcpy(dest, "Hello", sizeof(dest));
	copied = safe_strcat(dest, " World", sizeof(dest));
	printf("After concat: %s (copied %zu)\n", dest, copied);

	// 测试内存安全函数
	int *ptr = (int *)safe_malloc(sizeof(int) * 10);
	printf("Allocated memory: %p\n", (void *)ptr);
	safe_free((void **)&ptr);
	printf("Freed memory, pointer is now: %p\n", (void *)ptr);

	printf("\n=== All utils tests completed ===\n");
	return 0;
}

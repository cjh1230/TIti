// tests/test_builder.c
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "../protocol/protocol.h"
#include "../utils/utils.h"

void test_build_login()
{
	printf("Testing build_login_msg...\n");

	char *msg = build_login_msg("alice", "password123");
	assert(msg != NULL);

	// 验证消息格式
	assert(strstr(msg, "LOGIN|alice|server") != NULL);
	assert(strstr(msg, "password123") != NULL);

	printf("  ✓ Built login message: %s", msg);
	free(msg);
}

void test_build_text_msg()
{
	printf("Testing build_text_msg...\n");

	char *msg = build_text_msg("alice", "bob", "Hello Bob!");
	assert(msg != NULL);

	assert(strstr(msg, "MSG|alice|bob") != NULL);
	assert(strstr(msg, "Hello Bob!") != NULL);

	printf("  ✓ Built text message: %s", msg);
	free(msg);
}

void test_build_broadcast()
{
	printf("Testing build_broadcast_msg...\n");

	char *msg = build_broadcast_msg("admin", "System maintenance in 5 minutes");
	assert(msg != NULL);

	assert(strstr(msg, "BROADCAST|admin|*") != NULL);
	assert(strstr(msg, "System maintenance") != NULL);

	printf("  ✓ Built broadcast message: %s", msg);
	free(msg);
}

void test_build_group_msg()
{
	printf("Testing build_group_msg...\n");

	char *msg = build_group_msg("charlie", "dev-team", "Meeting at 3 PM");
	assert(msg != NULL);

	assert(strstr(msg, "GROUP|charlie|group:dev-team") != NULL);
	assert(strstr(msg, "Meeting at 3 PM") != NULL);

	printf("  ✓ Built group message: %s", msg);
	free(msg);
}

void test_build_responses()
{
	printf("Testing build_response_msg...\n");

	// 测试成功响应
	char *success = build_success_msg("Login successful");
	assert(success != NULL);
	assert(strstr(success, "OK|server|client") != NULL);
	assert(strstr(success, "0|Login successful") != NULL);
	printf("  ✓ Built success response: %s", success);
	free(success);

	// 测试错误响应
	char *error = build_error_msg(ERROR_USER_NOT_FOUND, NULL);
	assert(error != NULL);
	assert(strstr(error, "ERROR|server|client") != NULL);
	assert(strstr(error, "1002|User not found") != NULL);
	printf("  ✓ Built error response: %s", error);
	free(error);

	// 测试自定义错误消息
	char *custom_error = build_error_msg(ERROR_AUTH_FAILED, "Invalid credentials");
	assert(custom_error != NULL);
	assert(strstr(custom_error, "1001|Invalid credentials") != NULL);
	printf("  ✓ Built custom error response: %s", custom_error);
	free(custom_error);
}

void test_build_history_request()
{
	printf("Testing build_history_request...\n");

	char *msg = build_history_request("alice", "bob", "2024-01-15", "2024-01-16");
	assert(msg != NULL);

	assert(strstr(msg, "HISTORY|alice|server") != NULL);
	assert(strstr(msg, "bob|2024-01-15|2024-01-16") != NULL);

	printf("  ✓ Built history request: %s", msg);
	free(msg);
}

void test_build_status_request()
{
	printf("Testing build_status_request...\n");

	char *msg = build_status_request("alice");
	assert(msg != NULL);

	assert(strstr(msg, "STATUS|alice|server") != NULL);

	printf("  ✓ Built status request: %s", msg);
	free(msg);
}

void test_build_notifications()
{
	printf("Testing build_user_online_msg...\n");

	char *online_msg = build_user_online_msg("alice");
	assert(online_msg != NULL);
	assert(strstr(online_msg, "alice is now online") != NULL);
	printf("  ✓ Built online notification: %s", online_msg);
	free(online_msg);

	char *offline_msg = build_user_offline_msg("bob");
	assert(offline_msg != NULL);
	assert(strstr(offline_msg, "bob is now offline") != NULL);
	printf("  ✓ Built offline notification: %s", offline_msg);
	free(offline_msg);

	char *sys_msg = build_system_notification("Server will restart at midnight");
	assert(sys_msg != NULL);
	assert(strstr(sys_msg, "Server will restart") != NULL);
	printf("  ✓ Built system notification: %s", sys_msg);
	free(sys_msg);
}

void test_escape_in_builder()
{
	printf("Testing escape in builder...\n");

	// 测试包含特殊字符的内容
	char *msg = build_text_msg("alice", "bob", "Hello|World\nNew line");
	assert(msg != NULL);

	// 验证转义
	assert(strstr(msg, "Hello\\|World\\nNew line") != NULL);

	printf("  ✓ Built message with escape: %s", msg);
	free(msg);
}

int main()
{
	set_log_file(NULL);
	set_log_level(LOG_INFO);

	printf("=== Protocol Builder Tests ===\n\n");

	test_build_login();
	test_build_text_msg();
	test_build_broadcast();
	test_build_group_msg();
	test_build_responses();
	test_build_history_request();
	test_build_status_request();
	test_build_notifications();
	test_escape_in_builder();

	printf("\n=== All builder tests passed! ===\n");

	return 0;
}

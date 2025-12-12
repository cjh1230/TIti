// tests/test_protocol.c
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "../protocol/protocol.h"
#include "../utils/utils.h"

void test_parse_basic()
{
	printf("Testing basic parse...\n");

	// 测试登录消息
	const char *login_msg = "LOGIN|alice|server|2024-01-15 10:30:00|password123\n";
	Message *msg = parse_message(login_msg);

	assert(msg != NULL);
	assert(strcmp(msg->type, "LOGIN") == 0);
	assert(strcmp(msg->sender, "alice") == 0);
	assert(strcmp(msg->receiver, "server") == 0);
	assert(strcmp(msg->content, "password123") == 0);

	printf("  ✓ Login message parsed\n");
	safe_free((void **)&msg);
}

void test_parse_with_escape()
{
	printf("Testing parse with escape characters...\n");

	// 测试包含特殊字符的消息
	const char *msg_str = "MSG|bob|alice|2024-01-15 10:35:00|Hello\\|World\\nNew line\n";
	Message *msg = parse_message(msg_str);

	assert(msg != NULL);
	assert(strcmp(msg->content, "Hello|World\nNew line") == 0);

	printf("  ✓ Message with escape characters parsed\n");
	safe_free((void **)&msg);
}

void test_serialize()
{
	printf("Testing serialize...\n");

	// 创建消息
	Message msg;
	memset(&msg, 0, sizeof(Message));
	strcpy(msg.type, "MSG");
	strcpy(msg.sender, "alice");
	strcpy(msg.receiver, "bob");
	strcpy(msg.timestamp, "2024-01-15 10:30:00");
	strcpy(msg.content, "Hello Bob!");

	// 序列化
	char *serialized = serialize_message(&msg);
	assert(serialized != NULL);

	// 验证序列化结果
	assert(strstr(serialized, "MSG|alice|bob|2024-01-15 10:30:00|Hello Bob!") != NULL);

	printf("  ✓ Message serialized: %s", serialized);
	free(serialized);
}

void test_escape_unescape()
{
	printf("Testing escape/unescape...\n");

	const char *test_cases[] = {
		"Hello|World",
		"Test\\Backslash",
		"Line1\nLine2",
		"Normal",
		NULL};

	for (int i = 0; test_cases[i] != NULL; i++)
	{
		char *escaped = escape_field(test_cases[i]);
		char *unescaped = unescape_field(escaped);

		assert(strcmp(test_cases[i], unescaped) == 0);

		printf("  ✓ '%s' -> '%s' -> '%s'\n",
			   test_cases[i], escaped, unescaped);

		free(escaped);
		free(unescaped);
	}
}

void test_command_type()
{
	printf("Testing command type recognition...\n");

	struct
	{
		const char *type;
		CommandType expected;
	} tests[] = {
		{"LOGIN", CMD_LOGIN},
		{"LOGOUT", CMD_LOGOUT},
		{"MSG", CMD_SEND_MSG},
		{"BROADCAST", CMD_BROADCAST},
		{"GROUP", CMD_JOIN_GROUP},
		{"HISTORY", CMD_GET_HISTORY},
		{"STATUS", CMD_GET_STATUS},
		{"ERROR", CMD_UNKNOWN},
		{"OK", CMD_UNKNOWN},
		{"UNKNOWN", CMD_UNKNOWN}};

	for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); i++)
	{
		CommandType result = get_command_type(tests[i].type);
		assert(result == tests[i].expected);
		printf("  ✓ %s -> %d\n", tests[i].type, result);
	}
}

void test_validation()
{
	printf("Testing message validation...\n");

	struct
	{
		const char *msg;
		int should_pass;
	} tests[] = {
		{"LOGIN|alice|server|time|pass\n", 1},
		{"MSG|bob|alice|time|hello\n", 1},
		{"TYPE|sender|receiver|time|content\n", 1}, // 类型验证单独进行
		{"TOO|FEW|FIELDS\n", 0},
		{"TOO|MANY|FIELDS|EXTRA|EXTRA|EXTRA\n", 0},
		{"", 0},
		{NULL, 0}};

	for (int i = 0; tests[i].msg != NULL || i == 6; i++)
	{
		int result = validate_message(tests[i].msg);
		if (result == tests[i].should_pass)
		{
			printf("  ✓ Validation %s for: %s\n",
				   result ? "passed" : "failed",
				   tests[i].msg ? tests[i].msg : "NULL");
		}
		else
		{
			printf("  ✗ Validation failed for: %s (got %d, expected %d)\n",
				   tests[i].msg ? tests[i].msg : "NULL",
				   result, tests[i].should_pass);
			exit(1);
		}
	}
}

int main()
{
	set_log_file(NULL);
	set_log_level(LOG_INFO);

	printf("=== Protocol Module Tests ===\n\n");

	test_parse_basic();
	test_parse_with_escape();
	test_serialize();
	test_escape_unescape();
	test_command_type();
	test_validation();

	printf("\n=== All tests passed! ===\n");

	return 0;
}

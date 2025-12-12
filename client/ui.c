/**
 * @file ui.c
 * @brief 客户端用户界面实现
 *
 * 实现客户端的用户界面功能，处理用户输入和命令。
 */

#include "ui.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

/**
 * @brief 去除字符串两端的空白字符
 *
 * @param str 要处理的字符串
 * @return char* 处理后的字符串
 */
static char *trim_whitespace(char *str)
{
	char *end;

	// 去除前导空白
	while (isspace((unsigned char)*str))
	{
		str++;
	}

	if (*str == 0)
	{ // 全是空白字符
		return str;
	}

	// 去除尾部空白
	end = str + strlen(str) - 1;
	while (end > str && isspace((unsigned char)*end))
	{
		end--;
	}

	// 写入新的终止符
	*(end + 1) = '\0';

	return str;
}

void ui_show_welcome(void)
{
	printf("========================================\n");
	printf("  欢迎使用ITit聊天客户端\n");
	printf("========================================\n");
	printf("\n");
	printf("输入 'help' 查看可用命令\n");
	printf("\n");
}

void ui_show_help(void)
{
	printf("可用命令:\n");
	printf("  connect <ip> <port>    - 连接到服务器\n");
	printf("  disconnect            - 断开与服务器的连接\n");
	printf("  login <user> <pass>   - 登录到服务器\n");
	printf("  logout                - 从服务器登出\n");
	printf("  send <user> <msg>     - 发送消息给指定用户\n");
	printf("  broadcast <msg>        - 发送广播消息\n");
	printf("  group <group> <msg>    - 发送群组消息\n");
	printf("  history <target>       - 查询与目标用户或群组的历史记录\n");
	printf("  status                - 查询服务器状态\n");
	printf("  help                  - 显示此帮助信息\n");
	printf("  quit                  - 退出客户端\n");
}

void ui_show_status(AppClient *client)
{
	const char *state_str;

	switch (client->state)
	{
	case CLIENT_DISCONNECTED:
		state_str = "未连接";
		break;
	case CLIENT_CONNECTING:
		state_str = "连接中";
		break;
	case CLIENT_CONNECTED:
		state_str = "已连接";
		break;
	case CLIENT_AUTHENTICATED:
		state_str = "已登录";
		break;
	case CLIENT_ERROR:
		state_str = "错误";
		break;
	default:
		state_str = "未知";
		break;
	}

	printf("当前状态: %s\n", state_str);
	if (client->state >= CLIENT_CONNECTED)
	{
		printf("服务器: %s:%d\n", client->server_ip, client->server_port);
	}
	if (client->state == CLIENT_AUTHENTICATED)
	{
		printf("用户名: %s\n", client->username);
	}
}

int ui_handle_input(AppClient *client)
{
	char input[1024];
	char *cmd;

	printf("> ");
	fflush(stdout);

	if (fgets(input, sizeof(input), stdin) == NULL)
	{
		return -1; // EOF或错误
	}

	// 去除换行符
	input[strcspn(input, "\n")] = '\0';

	// 去除空白字符
	cmd = trim_whitespace(input);

	// 空命令
	if (strlen(cmd) == 0)
	{
		return 0;
	}

	// 处理命令
	if (strncmp(cmd, "connect", 7) == 0)
	{
		return ui_cmd_connect(client, cmd);
	}
	else if (strncmp(cmd, "disconnect", 10) == 0)
	{
		return ui_cmd_disconnect(client, cmd);
	}
	else if (strncmp(cmd, "login", 5) == 0)
	{
		return ui_cmd_login(client, cmd);
	}
	else if (strncmp(cmd, "logout", 6) == 0)
	{
		return ui_cmd_logout(client, cmd);
	}
	else if (strncmp(cmd, "send", 4) == 0)
	{
		return ui_cmd_send(client, cmd);
	}
	else if (strncmp(cmd, "broadcast", 9) == 0)
	{
		return ui_cmd_broadcast(client, cmd);
	}
	else if (strncmp(cmd, "group", 5) == 0)
	{
		return ui_cmd_group(client, cmd);
	}
	else if (strncmp(cmd, "history", 7) == 0)
	{
		return ui_cmd_history(client, cmd);
	}
	else if (strncmp(cmd, "status", 6) == 0)
	{
		return ui_cmd_status(client, cmd);
	}
	else if (strncmp(cmd, "help", 4) == 0)
	{
		return ui_cmd_help(client, cmd);
	}
	else if (strncmp(cmd, "quit", 4) == 0)
	{
		return ui_cmd_quit(client, cmd);
	}
	else
	{
		printf("未知命令: %s\n", cmd);
		printf("输入 'help' 查看可用命令\n");
		return 0;
	}
}

int ui_cmd_connect(AppClient *client, const char *cmd)
{
	char ip[32];
	int port;

	if (sscanf(cmd, "connect %31s %d", ip, &port) != 2)
	{
		printf("用法: connect <ip> <port>\n");
		return 0;
	}

	if (client_connect(client) == 0)
	{
		/* 启动接收线程，开始处理来自服务器的响应 */
		if (client_start(client) == 0)
		{
			printf("连接成功\n");
		}
		else
		{
			printf("连接成功，但启动接收线程失败\n");
		}
	}
	else
	{
		printf("连接失败\n");
	}

	return 0;
}

int ui_cmd_disconnect(AppClient *client, const char *cmd)
{
	(void)cmd;
	if (client->state == CLIENT_DISCONNECTED)
	{
		printf("未连接到服务器\n");
		return 0;
	}

	if (client_disconnect(client) == 0)
	{
		printf("断开连接成功\n");
	}
	else
	{
		printf("断开连接失败\n");
	}

	return 0;
}

int ui_cmd_login(AppClient *client, const char *cmd)
{
	char username[32];
	char password[32];

	if (sscanf(cmd, "login %31s %31s", username, password) != 2)
	{
		printf("用法: login <username> <password>\n");
		return 0;
	}

	if (client_login(client, username, password) == 0)
	{
		printf("登录请求已发送\n");
	}
	else
	{
		printf("登录失败\n");
	}

	return 0;
}

int ui_cmd_logout(AppClient *client, const char *cmd)
{
	(void)cmd;
	if (client->state != CLIENT_AUTHENTICATED)
	{
		printf("未登录\n");
		return 0;
	}

	if (client_logout(client) == 0)
	{
		printf("登出请求已发送\n");
	}
	else
	{
		printf("登出失败\n");
	}

	return 0;
}

int ui_cmd_send(AppClient *client, const char *cmd)
{
	char receiver[32];
	/* content not used here; message is taken from msg_start */

	// 找到消息内容开始位置
	const char *msg_start = cmd + 5; // 跳过 "send "
	while (*msg_start && !isspace(*msg_start))
	{
		msg_start++;
	}
	while (*msg_start && isspace(*msg_start))
	{
		msg_start++;
	}

	if (sscanf(cmd, "send %31s", receiver) != 1 || strlen(msg_start) == 0)
	{
		printf("用法: send <username> <message>\n");
		return 0;
	}

	if (client_send_message(client, receiver, msg_start) == 0)
	{
		printf("消息已发送\n");
	}
	else
	{
		printf("发送消息失败\n");
	}

	return 0;
}

int ui_cmd_broadcast(AppClient *client, const char *cmd)
{
	// 找到消息内容开始位置
	const char *msg_start = cmd + 10; // 跳过 "broadcast "
	while (*msg_start && isspace(*msg_start))
	{
		msg_start++;
	}

	if (strlen(msg_start) == 0)
	{
		printf("用法: broadcast <message>\n");
		return 0;
	}

	if (client_send_broadcast(client, msg_start) == 0)
	{
		printf("广播消息已发送\n");
	}
	else
	{
		printf("发送广播消息失败\n");
	}

	return 0;
}

int ui_cmd_group(AppClient *client, const char *cmd)
{
	char group_name[32];
	/* content not used here; message is taken from msg_start */

	// 找到消息内容开始位置
	const char *msg_start = cmd + 6; // 跳过 "group "
	while (*msg_start && !isspace(*msg_start))
	{
		msg_start++;
	}
	while (*msg_start && isspace(*msg_start))
	{
		msg_start++;
	}

	if (sscanf(cmd, "group %31s", group_name) != 1 || strlen(msg_start) == 0)
	{
		printf("用法: group <groupname> <message>\n");
		return 0;
	}

	if (client_send_group_message(client, group_name, msg_start) == 0)
	{
		printf("群组消息已发送\n");
	}
	else
	{
		printf("发送群组消息失败\n");
	}

	return 0;
}

int ui_cmd_history(AppClient *client, const char *cmd)
{
	char target[32];

	if (sscanf(cmd, "history %31s", target) != 1)
	{
		printf("用法: history <target>\n");
		return 0;
	}

	if (client_request_history(client, target, NULL, NULL) == 0)
	{
		printf("历史记录请求已发送\n");
	}
	else
	{
		printf("请求历史记录失败\n");
	}

	return 0;
}

int ui_cmd_status(AppClient *client, const char *cmd)
{
	(void)cmd;
	if (client_request_status(client) == 0)
	{
		printf("状态请求已发送\n");
	}
	else
	{
		printf("请求状态失败\n");
	}

	return 0;
}

int ui_cmd_help(AppClient *client, const char *cmd)
{
	(void)client;
	(void)cmd;
	ui_show_help();
	return 0;
}

int ui_cmd_quit(AppClient *client, const char *cmd)
{
	(void)client;
	(void)cmd;
	printf("再见!\n");
	return -1; // 返回-1表示退出
}

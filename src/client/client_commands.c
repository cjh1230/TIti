#include "client_commands.h"
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static char *trim_whitespace(char *str)
{
	char *end;

	while (isspace((unsigned char)*str))
	{
		str++;
	}

	if (*str == '\0')
	{
		return str;
	}

	end = str + strlen(str) - 1;
	while (end > str && isspace((unsigned char)*end))
	{
		end--;
	}

	*(end + 1) = '\0';
	return str;
}

static int command_matches(const char *cmd, const char *name)
{
	size_t len = strlen(name);

	return strncmp(cmd, name, len) == 0 &&
		   (cmd[len] == '\0' || isspace((unsigned char)cmd[len]));
}

static const char *command_args(const char *cmd)
{
	while (*cmd && !isspace((unsigned char)*cmd))
	{
		cmd++;
	}
	while (*cmd && isspace((unsigned char)*cmd))
	{
		cmd++;
	}
	return cmd;
}

static void command_write(ClientCommandContext *ctx, const char *line)
{
	if (ctx && ctx->write_line)
	{
		ctx->write_line(ctx->userdata, line ? line : "");
	}
}

static void command_writef(ClientCommandContext *ctx, const char *format, ...)
{
	char line[512];
	va_list args;

	va_start(args, format);
	vsnprintf(line, sizeof(line), format, args);
	va_end(args);

	command_write(ctx, line);
}

static void notify_state_changed(AppClient *client, ClientCommandContext *ctx)
{
	if (ctx && ctx->state_changed)
	{
		ctx->state_changed(ctx->userdata, client, ctx->active_receiver);
	}
}

static void command_show_help(ClientCommandContext *ctx)
{
	command_write(ctx, "可用命令:");
	command_write(ctx, "  connect [ip] [port]    - 连接服务器，默认 127.0.0.1:8080，别名 c");
	command_write(ctx, "  disconnect             - 断开连接，别名 d");
	command_write(ctx, "  login <user> <pass>    - 登录，别名 l");
	command_write(ctx, "  logout                 - 登出");
	command_write(ctx, "  to <user>              - 设置聊天对象，之后直接输入内容即可发送");
	command_write(ctx, "  send <user> <msg>      - 发送私聊消息，别名 s");
	command_write(ctx, "  broadcast <msg>        - 发送广播消息，别名 b");
	command_write(ctx, "  group <group> <msg>    - 发送群组消息，别名 g");
	command_write(ctx, "  history <target>       - 查询历史记录，别名 h");
	command_write(ctx, "  status                 - 查询服务器状态，别名 st");
	command_write(ctx, "  help                   - 显示帮助，别名 ?");
	command_write(ctx, "  quit                   - 退出客户端，别名 q");
}

static int send_to_active_receiver(AppClient *client, ClientCommandContext *ctx, const char *message)
{
	ClientState state;

	if (!ctx || ctx->active_receiver[0] == '\0')
	{
		return 0;
	}

	platform_mutex_lock(&client->state_lock);
	state = client->state;
	platform_mutex_unlock(&client->state_lock);

	if (state != CLIENT_AUTHENTICATED)
	{
		command_write(ctx, "请先连接并登录，再发送消息");
		return 1;
	}

	if (client_send_message(client, ctx->active_receiver, message) == 0)
	{
		command_write(ctx, "消息已发送");
	}
	else
	{
		command_write(ctx, "发送消息失败");
	}

	return 1;
}

static int command_connect(AppClient *client, ClientCommandContext *ctx, const char *cmd)
{
	char ip[32] = "127.0.0.1";
	int port = 8080;
	const char *args = command_args(cmd);

	if (*args != '\0' && sscanf(args, "%31s %d", ip, &port) < 1)
	{
		command_write(ctx, "用法: connect [ip] [port]");
		return 0;
	}

	safe_strcpy(client->server_ip, ip, sizeof(client->server_ip));
	client->server_port = port;

	if (client_connect(client) == 0)
	{
		if (client_start(client) == 0)
		{
			command_write(ctx, "连接成功");
		}
		else
		{
			command_write(ctx, "连接成功，但启动接收线程失败");
		}
	}
	else
	{
		command_write(ctx, "连接失败");
	}

	notify_state_changed(client, ctx);
	return 0;
}

static int command_disconnect(AppClient *client, ClientCommandContext *ctx)
{
	if (client->state == CLIENT_DISCONNECTED)
	{
		command_write(ctx, "未连接到服务器");
		return 0;
	}

	if (client_disconnect(client) == 0)
	{
		command_write(ctx, "断开连接成功");
	}
	else
	{
		command_write(ctx, "断开连接失败");
	}

	notify_state_changed(client, ctx);
	return 0;
}

static int command_login(AppClient *client, ClientCommandContext *ctx, const char *cmd)
{
	char username[32];
	char password[32];
	const char *args = command_args(cmd);

	if (sscanf(args, "%31s %31s", username, password) != 2)
	{
		command_write(ctx, "用法: login <username> <password>");
		return 0;
	}

	if (client_login(client, username, password) == 0)
	{
		command_write(ctx, "登录成功");
	}
	else
	{
		command_write(ctx, "登录失败");
	}

	notify_state_changed(client, ctx);
	return 0;
}

static int command_logout(AppClient *client, ClientCommandContext *ctx)
{
	if (client->state != CLIENT_AUTHENTICATED)
	{
		command_write(ctx, "未登录");
		return 0;
	}

	if (client_logout(client) == 0)
	{
		command_write(ctx, "登出请求已发送");
	}
	else
	{
		command_write(ctx, "登出失败");
	}

	notify_state_changed(client, ctx);
	return 0;
}

static int command_send(AppClient *client, ClientCommandContext *ctx, const char *cmd)
{
	char receiver[32];
	const char *msg_start = command_args(cmd);

	while (*msg_start && !isspace((unsigned char)*msg_start))
	{
		msg_start++;
	}
	while (*msg_start && isspace((unsigned char)*msg_start))
	{
		msg_start++;
	}

	if (sscanf(cmd, "%*s %31s", receiver) != 1 || strlen(msg_start) == 0)
	{
		command_write(ctx, "用法: send <username> <message>");
		return 0;
	}

	if (client_send_message(client, receiver, msg_start) == 0)
	{
		command_write(ctx, "消息已发送");
	}
	else
	{
		command_write(ctx, "发送消息失败");
	}

	return 0;
}

static int command_broadcast(AppClient *client, ClientCommandContext *ctx, const char *cmd)
{
	const char *msg_start = command_args(cmd);

	if (strlen(msg_start) == 0)
	{
		command_write(ctx, "用法: broadcast <message>");
		return 0;
	}

	if (client_send_broadcast(client, msg_start) == 0)
	{
		command_write(ctx, "广播消息已发送");
	}
	else
	{
		command_write(ctx, "发送广播消息失败");
	}

	return 0;
}

static int command_group(AppClient *client, ClientCommandContext *ctx, const char *cmd)
{
	char group_name[32];
	const char *msg_start = command_args(cmd);

	while (*msg_start && !isspace((unsigned char)*msg_start))
	{
		msg_start++;
	}
	while (*msg_start && isspace((unsigned char)*msg_start))
	{
		msg_start++;
	}

	if (sscanf(cmd, "%*s %31s", group_name) != 1 || strlen(msg_start) == 0)
	{
		command_write(ctx, "用法: group <groupname> <message>");
		return 0;
	}

	if (client_send_group_message(client, group_name, msg_start) == 0)
	{
		command_write(ctx, "群组消息已发送");
	}
	else
	{
		command_write(ctx, "发送群组消息失败");
	}

	return 0;
}

static int command_history(AppClient *client, ClientCommandContext *ctx, const char *cmd)
{
	char target[32];

	if (sscanf(cmd, "%*s %31s", target) != 1)
	{
		command_write(ctx, "用法: history <target>");
		return 0;
	}

	if (client_request_history(client, target, NULL, NULL) == 0)
	{
		command_write(ctx, "历史记录请求已发送");
	}
	else
	{
		command_write(ctx, "请求历史记录失败");
	}

	return 0;
}

static int command_to(AppClient *client, ClientCommandContext *ctx, const char *cmd)
{
	char receiver[32];
	(void)client;

	if (sscanf(cmd, "%*s %31s", receiver) != 1)
	{
		if (ctx->active_receiver[0] != '\0')
		{
			command_writef(ctx, "当前聊天对象: %s", ctx->active_receiver);
		}
		else
		{
			command_write(ctx, "用法: to <username>");
		}
		return 0;
	}

	safe_strcpy(ctx->active_receiver, receiver, sizeof(ctx->active_receiver));
	command_writef(ctx, "当前聊天对象: %s", ctx->active_receiver);
	notify_state_changed(client, ctx);
	return 0;
}

static int command_status(AppClient *client, ClientCommandContext *ctx)
{
	if (client_request_status(client) == 0)
	{
		command_write(ctx, "状态请求已发送");
	}
	else
	{
		command_write(ctx, "请求状态失败");
	}

	return 0;
}

void client_command_context_init(ClientCommandContext *ctx,
								 ClientCommandWriteFn write_line,
								 void *userdata)
{
	if (!ctx)
	{
		return;
	}

	memset(ctx, 0, sizeof(*ctx));
	ctx->write_line = write_line;
	ctx->userdata = userdata;
}

void client_command_set_state_callback(ClientCommandContext *ctx,
									   ClientCommandStateFn state_changed)
{
	if (ctx)
	{
		ctx->state_changed = state_changed;
	}
}

int client_command_execute(AppClient *client, ClientCommandContext *ctx, const char *input)
{
	char buffer[1024];
	char *cmd;

	if (!ctx || !input)
	{
		return 0;
	}

	safe_strcpy(buffer, input, sizeof(buffer));
	cmd = trim_whitespace(buffer);
	if (strlen(cmd) == 0)
	{
		return 0;
	}

	if (command_matches(cmd, "help") || command_matches(cmd, "?"))
	{
		command_show_help(ctx);
		return 0;
	}
	if (command_matches(cmd, "quit") || command_matches(cmd, "q"))
	{
		command_write(ctx, "再见!");
		return -1;
	}
	if (!client)
	{
		command_write(ctx, "客户端未初始化");
		return 0;
	}

	if (command_matches(cmd, "connect") || command_matches(cmd, "c"))
	{
		return command_connect(client, ctx, cmd);
	}
	if (command_matches(cmd, "disconnect") || command_matches(cmd, "d"))
	{
		return command_disconnect(client, ctx);
	}
	if (command_matches(cmd, "login") || command_matches(cmd, "l"))
	{
		return command_login(client, ctx, cmd);
	}
	if (command_matches(cmd, "logout"))
	{
		return command_logout(client, ctx);
	}
	if (command_matches(cmd, "to"))
	{
		return command_to(client, ctx, cmd);
	}
	if (command_matches(cmd, "send") || command_matches(cmd, "s"))
	{
		return command_send(client, ctx, cmd);
	}
	if (command_matches(cmd, "broadcast") || command_matches(cmd, "b"))
	{
		return command_broadcast(client, ctx, cmd);
	}
	if (command_matches(cmd, "group") || command_matches(cmd, "g"))
	{
		return command_group(client, ctx, cmd);
	}
	if (command_matches(cmd, "history") || command_matches(cmd, "h"))
	{
		return command_history(client, ctx, cmd);
	}
	if (command_matches(cmd, "status") || command_matches(cmd, "st"))
	{
		return command_status(client, ctx);
	}
	if (send_to_active_receiver(client, ctx, cmd))
	{
		return 0;
	}

	command_writef(ctx, "未知命令: %s", cmd);
	command_write(ctx, "输入 'help' 查看可用命令");
	return 0;
}

const char *client_command_state_label(AppClient *client)
{
	ClientState state;

	if (!client)
	{
		return "未知";
	}

	platform_mutex_lock(&client->state_lock);
	state = client->state;
	platform_mutex_unlock(&client->state_lock);

	switch (state)
	{
	case CLIENT_DISCONNECTED:
		return "未连接";
	case CLIENT_CONNECTING:
		return "连接中";
	case CLIENT_CONNECTED:
		return "已连接";
	case CLIENT_AUTHENTICATED:
		return "已登录";
	case CLIENT_ERROR:
		return "错误";
	default:
		return "未知";
	}
}

const char *client_command_active_receiver(ClientCommandContext *ctx)
{
	if (!ctx || ctx->active_receiver[0] == '\0')
	{
		return "";
	}

	return ctx->active_receiver;
}

#include "ui.h"
#include "client_commands.h"
#include <stdio.h>
#include <string.h>

static ClientCommandContext command_ctx;
static int command_ctx_initialized = 0;

static void cli_write_line(void *userdata, const char *line)
{
	(void)userdata;
	printf("%s\n", line ? line : "");
}

static void ensure_command_context(void)
{
	if (!command_ctx_initialized)
	{
		client_command_context_init(&command_ctx, cli_write_line, NULL);
		command_ctx_initialized = 1;
	}
}

static void ui_show_prompt(AppClient *client)
{
	char username[32];
	const char *active_receiver;

	ensure_command_context();

	platform_mutex_lock(&client->state_lock);
	safe_strcpy(username, client->username, sizeof(username));
	platform_mutex_unlock(&client->state_lock);

	if (client->state == CLIENT_AUTHENTICATED && username[0] != '\0')
	{
		printf("%s", username);
	}
	else
	{
		printf("%s", client_command_state_label(client));
	}

	active_receiver = client_command_active_receiver(&command_ctx);
	if (active_receiver[0] != '\0')
	{
		printf("->%s", active_receiver);
	}

	printf("> ");
	fflush(stdout);
}

void ui_show_welcome(void)
{
	printf("========================================\n");
	printf("  欢迎使用ITit聊天客户端\n");
	printf("========================================\n\n");
	printf("输入 'help' 查看可用命令\n\n");
}

void ui_show_help(void)
{
	ensure_command_context();
	client_command_execute(NULL, &command_ctx, "help");
}

void ui_show_status(AppClient *client)
{
	const char *active_receiver;

	ensure_command_context();

	printf("当前状态: %s\n", client_command_state_label(client));
	if (client->state >= CLIENT_CONNECTED)
	{
		printf("服务器: %s:%d\n", client->server_ip, client->server_port);
	}
	if (client->state == CLIENT_AUTHENTICATED)
	{
		printf("用户名: %s\n", client->username);
	}

	active_receiver = client_command_active_receiver(&command_ctx);
	if (active_receiver[0] != '\0')
	{
		printf("当前聊天对象: %s\n", active_receiver);
	}
}

int ui_handle_input(AppClient *client)
{
	char input[1024];

	ensure_command_context();
	ui_show_prompt(client);

	if (fgets(input, sizeof(input), stdin) == NULL)
	{
		return -1;
	}

	input[strcspn(input, "\n")] = '\0';
	return client_command_execute(client, &command_ctx, input);
}

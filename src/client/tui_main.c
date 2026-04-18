#include "client.h"
#include "client_commands.h"
#include "../tui/tui.h"
#include <stdio.h>
#include <string.h>

typedef struct
{
	AppClient *client;
	ClientCommandContext *commands;
} TuiClientUi;

static void tui_update_status(TuiClientUi *ui)
{
	char username[32];
	char status[256];
	const char *receiver;

	platform_mutex_lock(&ui->client->state_lock);
	safe_strcpy(username, ui->client->username, sizeof(username));
	platform_mutex_unlock(&ui->client->state_lock);

	receiver = client_command_active_receiver(ui->commands);
	snprintf(status, sizeof(status), "%s | user=%s | server=%s:%d | to=%s",
			 client_command_state_label(ui->client),
			 username[0] ? username : "-",
			 ui->client->server_ip,
			 ui->client->server_port,
			 receiver[0] ? receiver : "-");
	tui_set_status(status);
}

static void tui_command_write(void *userdata, const char *line)
{
	(void)userdata;
	tui_draw_system(line);
}

static void tui_command_state_changed(void *userdata, AppClient *client, const char *active_receiver)
{
	(void)client;
	(void)active_receiver;
	tui_update_status((TuiClientUi *)userdata);
}

static void tui_client_message(void *userdata, const char *line)
{
	TuiClientUi *ui = (TuiClientUi *)userdata;
	tui_draw_message("server", line);
	tui_update_status(ui);
}

int main(void)
{
	AppClient client;
	ClientCommandContext commands;
	TuiClientUi ui;
	char input[1024];
	int result = 0;

	set_log_file("client.log");
	set_log_level(LOG_INFO);

	if (client_init(&client, "127.0.0.1", 8080) != 0)
	{
		fprintf(stderr, "初始化客户端失败\n");
		return 1;
	}

	client_command_context_init(&commands, tui_command_write, NULL);
	ui.client = &client;
	ui.commands = &commands;
	commands.userdata = &ui;
	client_command_set_state_callback(&commands, tui_command_state_changed);
	client_set_message_callback(&client, tui_client_message, &ui);

	if (tui_init() != 0)
	{
		client_cleanup(&client);
		return 1;
	}

	tui_update_status(&ui);
	tui_draw_system("ITit TUI ready. 输入 help 查看命令。");
	tui_draw_help();

	while (tui_read_input(input, sizeof(input)) >= 0)
	{
		result = client_command_execute(&client, &commands, input);
		tui_update_status(&ui);
		if (result != 0)
		{
			break;
		}
	}

	client_stop(&client);
	client_cleanup(&client);
	tui_shutdown();
	return 0;
}

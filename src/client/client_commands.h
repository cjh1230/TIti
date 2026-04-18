#ifndef CLIENT_COMMANDS_H
#define CLIENT_COMMANDS_H

#include "client.h"

typedef void (*ClientCommandWriteFn)(void *userdata, const char *line);
typedef void (*ClientCommandStateFn)(void *userdata, AppClient *client, const char *active_receiver);

typedef struct
{
	char active_receiver[32];
	ClientCommandWriteFn write_line;
	ClientCommandStateFn state_changed;
	void *userdata;
} ClientCommandContext;

void client_command_context_init(ClientCommandContext *ctx,
								 ClientCommandWriteFn write_line,
								 void *userdata);
void client_command_set_state_callback(ClientCommandContext *ctx,
									   ClientCommandStateFn state_changed);
int client_command_execute(AppClient *client, ClientCommandContext *ctx, const char *input);
const char *client_command_state_label(AppClient *client);
const char *client_command_active_receiver(ClientCommandContext *ctx);

#endif /* CLIENT_COMMANDS_H */

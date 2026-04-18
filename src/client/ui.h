#ifndef UI_H
#define UI_H

#include "client.h"

void ui_show_welcome(void);
void ui_show_help(void);
void ui_show_status(AppClient *client);
int ui_handle_input(AppClient *client);

#endif /* UI_H */

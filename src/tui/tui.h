#ifndef TUI_H
#define TUI_H

int tui_init(void);
void tui_shutdown(void);
void tui_clear(void);
void tui_set_status(const char *status);
void tui_draw_system(const char *message);
void tui_draw_error(const char *message);
void tui_draw_help(void);
void tui_draw_message(const char *sender, const char *message);
int tui_read_input(char *buffer, int size);

#endif /* TUI_H */

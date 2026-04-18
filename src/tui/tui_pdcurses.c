#ifdef _WIN32

#include "tui.h"
#include "../platform/platform.h"
#include "../utils/utils.h"
#include <curses.h>
#include <locale.h>
#include <string.h>

static WINDOW *status_win;
static WINDOW *message_win;
static WINDOW *input_win;
static char current_status[256];
static platform_mutex_t tui_lock = PLATFORM_MUTEX_INITIALIZER;

static void destroy_windows(void)
{
	if (status_win)
	{
		delwin(status_win);
		status_win = NULL;
	}
	if (message_win)
	{
		delwin(message_win);
		message_win = NULL;
	}
	if (input_win)
	{
		delwin(input_win);
		input_win = NULL;
	}
}

static void draw_status_locked(void)
{
	if (!status_win)
	{
		return;
	}

	werase(status_win);
	if (has_colors())
	{
		wbkgd(status_win, COLOR_PAIR(2));
	}
	mvwprintw(status_win, 0, 0, "%s", current_status[0] ? current_status : "ITit TUI");
	wclrtoeol(status_win);
	wrefresh(status_win);
}

static void create_layout_locked(void)
{
	int rows;
	int cols;
	int message_rows;

	getmaxyx(stdscr, rows, cols);
	if (rows < 4)
	{
		rows = 4;
	}
	if (cols < 20)
	{
		cols = 20;
	}

	destroy_windows();
	message_rows = rows - 2;
	status_win = newwin(1, cols, 0, 0);
	message_win = newwin(message_rows, cols, 1, 0);
	input_win = newwin(1, cols, rows - 1, 0);

	scrollok(message_win, TRUE);
	keypad(input_win, TRUE);
	draw_status_locked();
	wrefresh(message_win);
	wrefresh(input_win);
}

static void write_text_locked(const char *text)
{
	const char *p = text ? text : "";
	const char *newline;

	while (*p)
	{
		newline = strchr(p, '\n');
		if (newline)
		{
			waddnstr(message_win, p, (int)(newline - p));
			waddch(message_win, '\n');
			p = newline + 1;
		}
		else
		{
			waddstr(message_win, p);
			break;
		}
	}
	waddch(message_win, '\n');
	wrefresh(message_win);
}

static void draw_line_locked(const char *prefix, const char *message, int color_pair)
{
	if (!message_win)
	{
		return;
	}

	if (prefix && prefix[0] != '\0')
	{
		if (has_colors() && color_pair > 0)
		{
			wattron(message_win, COLOR_PAIR(color_pair));
		}
		wprintw(message_win, "%s", prefix);
		if (has_colors() && color_pair > 0)
		{
			wattroff(message_win, COLOR_PAIR(color_pair));
		}
		wprintw(message_win, ": ");
	}

	write_text_locked(message);
}

int tui_init(void)
{
	setlocale(LC_ALL, "");
	initscr();
	cbreak();
	noecho();
	keypad(stdscr, TRUE);
	scrollok(stdscr, TRUE);

	if (has_colors())
	{
		start_color();
		init_pair(1, COLOR_GREEN, COLOR_BLACK);
		init_pair(2, COLOR_BLACK, COLOR_CYAN);
		init_pair(3, COLOR_RED, COLOR_BLACK);
		init_pair(4, COLOR_YELLOW, COLOR_BLACK);
	}

	platform_mutex_lock(&tui_lock);
	create_layout_locked();
	platform_mutex_unlock(&tui_lock);
	return 0;
}

void tui_shutdown(void)
{
	platform_mutex_lock(&tui_lock);
	destroy_windows();
	platform_mutex_unlock(&tui_lock);
	endwin();
}

void tui_clear(void)
{
	platform_mutex_lock(&tui_lock);
	werase(message_win);
	wrefresh(message_win);
	platform_mutex_unlock(&tui_lock);
}

void tui_set_status(const char *status)
{
	platform_mutex_lock(&tui_lock);
	safe_strcpy(current_status, status ? status : "", sizeof(current_status));
	draw_status_locked();
	platform_mutex_unlock(&tui_lock);
}

void tui_draw_system(const char *message)
{
	platform_mutex_lock(&tui_lock);
	draw_line_locked("system", message, 4);
	platform_mutex_unlock(&tui_lock);
}

void tui_draw_error(const char *message)
{
	platform_mutex_lock(&tui_lock);
	draw_line_locked("error", message, 3);
	platform_mutex_unlock(&tui_lock);
}

void tui_draw_help(void)
{
	tui_draw_system("c [ip] [port] | l <user> <pass> | to <user> | text | b <msg> | st | q");
}

void tui_draw_message(const char *sender, const char *message)
{
	platform_mutex_lock(&tui_lock);
	draw_line_locked(sender ? sender : "system", message, 1);
	platform_mutex_unlock(&tui_lock);
}

int tui_read_input(char *buffer, int size)
{
	int result;

	if (!buffer || size <= 0)
	{
		return -1;
	}

	platform_mutex_lock(&tui_lock);
	werase(input_win);
	mvwprintw(input_win, 0, 0, "> ");
	wclrtoeol(input_win);
	wrefresh(input_win);
	echo();
	result = wgetnstr(input_win, buffer, size - 1);
	noecho();
	werase(input_win);
	wrefresh(input_win);
	platform_mutex_unlock(&tui_lock);

	if (result == ERR)
	{
		return -1;
	}

	return (int)strlen(buffer);
}

#endif /* _WIN32 */

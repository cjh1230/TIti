/**
 * @file utils/logger.c
 * @brief 日志系统实现文件
 *
 * 本文件实现了日志记录功能，支持多种日志级别、颜色输出和文件记录。
 * 日志系统可以输出到控制台或文件，并提供了客户端和消息相关的专用日志函数。
 *
 * @author 开发团队
 * @date 2025
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include "utils.h"
#include "../models/models.h"

/** 当前日志级别，低于此级别的日志将被忽略 */
static LogLevel current_log_level = LOG_INFO;
/** 日志输出文件指针，默认为NULL表示输出到标准输出 */
/* 默认输出到标准输出，避免未初始化导致的崩溃 */
static FILE *log_file = NULL;
__attribute__((constructor)) static void logger_init_default(void)
{
	/* 将默认输出设置为 stdout，除非后来被 set_log_file 覆盖 */
	log_file = stdout;
}

/* 日志互斥，确保在多线程下写入不会交错 */
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

/** 日志级别字符串数组，用于在日志中显示级别名称 */
static const char *log_level_strings[] = {
	"DEBUG", "INFO", "WARNING", "ERROR", "FATAL"};

/** 日志级别颜色代码数组，用于在终端中显示不同颜色的日志 */
static const char *log_level_colors[] = {
	"\x1b[36m", // CYAN for DEBUG
	"\x1b[32m", // GREEN for INFO
	"\x1b[33m", // YELLOW for WARNING
	"\x1b[31m", // RED for ERROR
	"\x1b[35m"	// MAGENTA for FATAL
};

/* 级别数组长度，用于边界检查 */
static const int LOG_LEVEL_COUNT = sizeof(log_level_strings) / sizeof(log_level_strings[0]);

/**
 * @brief 设置日志级别
 *
 * 设置日志系统显示的最低级别。低于此级别的日志将被忽略。
 *
 * @param level 要设置的最低日志级别
 */
void set_log_level(LogLevel level)
{
	current_log_level = level;
}

/**
 * @brief 设置日志输出文件
 *
 * 指定日志输出文件。如果设置为NULL或空字符串，日志将输出到标准输出。
 * 如果打开文件失败，则回退到标准错误输出。
 *
 * @param filename 日志文件路径，NULL或空字符串表示输出到标准输出
 */
void set_log_file(const char *filename)
{
	pthread_mutex_lock(&log_mutex);
	/* 如果已有打开的日志文件且不是标准输出/错误，则关闭它 */
	if (log_file && log_file != stdout && log_file != stderr)
	{
		fclose(log_file);
	}

	/* 如果文件名为空或NULL，则使用标准输出 */
	if (filename == NULL || strlen(filename) == 0)
	{
		log_file = stdout;
	}
	else
	{
		/* 尝试以追加模式打开日志文件 */
		log_file = fopen(filename, "a");
		if (!log_file)
		{
			/* 打开失败时输出错误信息并回退到标准错误 */
			fprintf(stderr, "Failed to open log file: %s\n", filename);
			log_file = stderr;
		}
	}
	pthread_mutex_unlock(&log_mutex);
}

/**
 * @brief 记录日志消息
 *
 * 根据指定的级别记录格式化的日志消息。消息会输出到控制台或
 * 由set_log_file指定的文件中。控制台输出带有颜色，文件输出则不带颜色。
 *
 * @param level 日志级别
 * @param format 格式化字符串，类似于printf的格式
 * @param ... 可变参数，对应format中的占位符
 */
void log_message(LogLevel level, const char *format, ...)
{
	/* 如果日志级别低于当前设置的最低级别，则忽略此日志 */
	if (level < current_log_level)
	{
		return;
	}

	/* 保守检查 level 边界，防止无效枚举值导致越界访问 */
	if (level < 0 || level >= LOG_LEVEL_COUNT)
	{
		level = LOG_INFO;
	}

	/* 获取当前时间字符串 */
	char time_buf[32];
	get_current_time(time_buf, sizeof(time_buf));

	/* 获取当前日志目标并确保在访问/写入期间不被并发修改 */
	pthread_mutex_lock(&log_mutex);
	if (!log_file)
	{
		log_file = stdout;
	}

	/* 格式化日志前缀（基于当前输出目标决定是否需要颜色） */
	char prefix[256];
	if (log_file == stdout || log_file == stderr)
	{
		/* 控制台输出：带颜色 */
		snprintf(prefix, sizeof(prefix), "%s[%s]%s %s: ",
				 log_level_colors[level],
				 time_buf,
				 "\x1b[0m",
				 log_level_strings[level]);
	}
	else
	{
		/* 文件输出：无颜色 */
		snprintf(prefix, sizeof(prefix), "[%s] %s: ",
				 time_buf, log_level_strings[level]);
	}

	/* 获取可变参数并在持锁期间写入，避免在写入过程中被 set_log_file 干扰 */
	va_list args;
	va_start(args, format);

	fprintf(log_file, "%s", prefix);
	vfprintf(log_file, format, args);
	fprintf(log_file, "\n");
	fflush(log_file);

	va_end(args);
	pthread_mutex_unlock(&log_mutex);
}

/* 日志宏在头文件中定义，不应在实现文件重复定义 */

/**
 * @brief 记录客户端相关事件
 *
 * 记录与客户端相关的事件日志，包括客户端ID、用户ID、用户名和IP地址等信息。
 *
 * @param event 事件描述字符串
 * @param client 客户端结构指针，可以为NULL
 */
void log_client_event(const char *event, Client *client)
{
	if (client)
	{
		/* 记录客户端详细信息 */
		LOG_INFO("%s: client_id=%d, user_id=%d, username=%s, ip=%s:%d",
				 event, client->client_id, client->user_id,
				 client->username, client->remote_ip, client->remote_port);
	}
	else
	{
		/* 如果客户端为NULL，只记录事件描述 */
		LOG_INFO("%s", event);
	}
}

/**
 * @brief 记录消息相关事件
 *
 * 记录与消息相关的事件日志，包括消息ID、类型、发送者和接收者等信息。
 *
 * @param event 事件描述字符串
 * @param msg 消息结构指针，可以为NULL
 */
void log_message_event(const char *event, Message *msg)
{
	if (msg)
	{
		/* 记录消息详细信息 */
		LOG_INFO("%s: msg_id=%d, type=%s, sender=%s, receiver=%s",
				 event, msg->message_id, msg->type,
				 msg->sender, msg->receiver);
	}
}

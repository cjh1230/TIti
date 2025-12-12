/**
 * @file utils/time_utils.c
 * @brief 时间处理工具函数实现
 *
 * 本文件实现了时间相关的工具函数，包括获取当前时间、解析时间戳和格式化时间等功能。
 * 所有函数都考虑了跨平台兼容性和安全性。
 *
 * @author 开发团队
 * @date 2025
 */

#include "utils.h"
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/**
 * @brief 获取当前时间字符串
 *
 * 获取当前系统时间并按照指定格式(YYYY-MM-DD HH:MM:SS)格式化为字符串。
 * 该函数考虑了线程安全性，在不同平台上使用适当的线程安全函数。
 *
 * @param buffer 存储时间字符串的缓冲区
 * @param buf_size 缓冲区大小
 */
void get_current_time(char *buffer, size_t buf_size)
{
	/* 检查参数有效性 */
	if (!buffer || buf_size == 0)
		return;

	/* 获取当前时间 */
	time_t now = time(NULL);
	struct tm tm_info;

	/* 根据平台使用线程安全的本地时间转换函数 */
#if defined(_POSIX_THREAD_SAFE_FUNCTIONS) || defined(__linux__)
	/* Linux/Unix系统使用线程安全的localtime_r函数 */
	localtime_r(&now, &tm_info);
#else
	/* 其他系统使用标准localtime函数并复制结果 */
	struct tm *tmp = localtime(&now);
	if (tmp)
		tm_info = *tmp;
#endif

	/* 将时间格式化为字符串 */
	if (strftime(buffer, buf_size, "%Y-%m-%d %H:%M:%S", &tm_info) == 0)
	{
		/* 如果缓冲区太小，确保至少为空字符串 */
		buffer[0] = '\0';
	}
}

/**
 * @brief 解析时间戳字符串
 *
 * 将时间戳字符串转换为time_t类型的时间值。
 * 支持多种常见的时间格式，包括ISO 8601格式和Unix时间戳。
 *
 * @param timestamp 时间戳字符串
 * @return 成功返回对应的时间值，失败返回-1
 */
time_t parse_timestamp(const char *timestamp)
{
	if (!timestamp || strlen(timestamp) == 0)
		return -1;

	/* 尝试解析为Unix时间戳（纯数字） */
	char *endptr;
	long ts = strtol(timestamp, &endptr, 10);
	if (*endptr == '\0')
	{
		/* 纯数字，假设为Unix时间戳 */
		return (time_t)ts;
	}

	/* 尝试解析ISO 8601格式 (YYYY-MM-DD HH:MM:SS) */
	struct tm tm_info = {0};
	if (sscanf(timestamp, "%d-%d-%d %d:%d:%d",
			   &tm_info.tm_year, &tm_info.tm_mon, &tm_info.tm_mday,
			   &tm_info.tm_hour, &tm_info.tm_min, &tm_info.tm_sec) == 6)
	{
		/* 调整tm_year和tm_mon为mktime期望的值 */
		tm_info.tm_year -= 1900;
		tm_info.tm_mon -= 1;

		/* 转换为time_t */
		return mktime(&tm_info);
	}

	/* 无法解析的时间格式 */
	return -1;
}

/**
 * @brief 格式化时间
 *
 * 将time_t类型的时间按照指定格式格式化为字符串。
 * 使用标准的strftime格式字符串。
 *
 * @param t 要格式化的时间
 * @param format 时间格式字符串，遵循strftime的格式
 * @return 返回格式化后的字符串，需要调用者释放内存
 */
char *format_time(time_t t, const char *format)
{
	if (!format || strlen(format) == 0)
		return NULL;

	/* 获取时间的结构化表示 */
	struct tm tm_info;
#if defined(_POSIX_THREAD_SAFE_FUNCTIONS) || defined(__linux__)
	/* Linux/Unix系统使用线程安全的localtime_r函数 */
	localtime_r(&t, &tm_info);
#else
	/* 其他系统使用标准localtime函数并复制结果 */
	struct tm *tmp = localtime(&t);
	if (!tmp)
		return NULL;
	tm_info = *tmp;
#endif

	/* 计算格式化后字符串的最大可能长度 */
	/* 根据strftime文档，结果字符串不会超过max(初始缓冲区大小，格式字符串长度*2) */
	size_t buf_size = strlen(format) * 2 + 1;
	char *result = safe_malloc(buf_size);
	if (!result)
		return NULL;

	/* 格式化时间 */
	size_t len = strftime(result, buf_size, format, &tm_info);
	if (len == 0)
	{
		/* 格式化失败，释放内存并返回NULL */
		safe_free((void **)&result);
		return NULL;
	}

	return result;
}

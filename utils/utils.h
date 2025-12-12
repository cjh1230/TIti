/**
 * @file utils/utils.h
 * @brief 通用工具函数库头文件
 *
 * 本文件定义了一系列实用的工具函数，包括日志记录、字符串操作、
 * 时间处理、内存管理和网络验证等功能。所有函数都经过安全设计，
 * 防止缓冲区溢出和其他常见的安全问题。
 *
 * @author 开发团队
 * @date 2025
 */

#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <time.h>

/**
 * @brief 日志级别枚举
 *
 * 定义了五个日志级别，从低到高分别为：
 * - LOG_DEBUG: 调试信息
 * - LOG_INFO: 一般信息
 * - LOG_WARNING: 警告信息
 * - LOG_ERROR: 错误信息
 * - LOG_FATAL: 致命错误
 */
typedef enum
{
	LOG_DEBUG = 0, /**< 调试级别 */
	LOG_INFO,	   /**< 信息级别 */
	LOG_WARNING,   /**< 警告级别 */
	LOG_ERROR,	   /**< 错误级别 */
	LOG_FATAL	   /**< 致命错误级别 */
} LogLevel;

/*
 * @defgroup 日志函数
 * @brief 提供日志记录功能的函数集合
 * @{
 */

/**
 * @brief 记录日志消息
 *
 * 根据指定的级别记录格式化的日志消息。消息会输出到控制台或
 * 由set_log_file指定的文件中。
 *
 * @param level 日志级别
 * @param format 格式化字符串，类似于printf的格式
 * @param ... 可变参数，对应format中的占位符
 */
void log_message(LogLevel level, const char *format, ...);

/**
 * @brief 设置日志级别
 *
 * 设置日志系统显示的最低级别。低于此级别的日志将被忽略。
 *
 * @param level 要设置的最低日志级别
 */
void set_log_level(LogLevel level);

/**
 * @brief 设置日志文件
 *
 * 指定日志输出文件。如果设置为NULL，日志将输出到标准错误。
 *
 * @param filename 日志文件路径，NULL表示输出到控制台
 */
void set_log_file(const char *filename);

/* @} */

/* 简化版日志宏：方便在代码中直接使用不同级别的日志记录 */
#define LOG_DEBUG(...) log_message(LOG_DEBUG, __VA_ARGS__)
#define LOG_INFO(...) log_message(LOG_INFO, __VA_ARGS__)
#define LOG_WARN(...) log_message(LOG_WARNING, __VA_ARGS__)
#define LOG_ERROR(...) log_message(LOG_ERROR, __VA_ARGS__)
#define LOG_FATAL(...) log_message(LOG_FATAL, __VA_ARGS__)

/*
 * @defgroup 安全字符串函数
 * @brief 提供安全的字符串操作函数，防止缓冲区溢出
 * @{
 */

/**
 * @brief 安全的字符串复制函数
 *
 * 将源字符串复制到目标缓冲区，确保不会发生缓冲区溢出。
 *
 * @param dest 目标缓冲区
 * @param src 源字符串
 * @param dest_size 目标缓冲区大小（包括空终止符）
 * @return 成功复制的字符数（不包括空终止符）
 */
size_t safe_strcpy(char *dest, const char *src, size_t dest_size);

/**
 * @brief 安全的字符串连接函数
 *
 * 将源字符串连接到目标字符串末尾，确保不会发生缓冲区溢出。
 *
 * @param dest 目标缓冲区，必须包含以null结尾的字符串
 * @param src 要连接的源字符串
 * @param dest_size 目标缓冲区总大小（包括空终止符）
 * @return 连接后字符串的总长度（不包括空终止符）
 */
size_t safe_strcat(char *dest, const char *src, size_t dest_size);

/**
 * @brief 安全的字符串比较函数
 *
 * 比较两个字符串，最多比较max_len个字符，防止缓冲区溢出。
 *
 * @param s1 第一个字符串
 * @param s2 第二个字符串
 * @param max_len 最大比较长度
 * @return 如果字符串相等返回0，s1<s2返回负数，s1>s2返回正数
 */
int safe_strcmp(const char *s1, const char *s2, size_t max_len);

/* @} */

/*
 * @defgroup 时间函数
 * @brief 提供时间处理和格式化功能
 * @{
 */

/**
 * @brief 获取当前时间字符串
 *
 * 获取当前时间并按照默认格式(YYYY-MM-DD HH:MM:SS)格式化为字符串。
 *
 * @param buffer 存储时间字符串的缓冲区
 * @param buf_size 缓冲区大小
 */
void get_current_time(char *buffer, size_t buf_size);

/**
 * @brief 解析时间戳字符串
 *
 * 将时间戳字符串转换为time_t类型的时间值。
 *
 * @param timestamp 时间戳字符串
 * @return 成功返回对应的时间值，失败返回-1
 */
time_t parse_timestamp(const char *timestamp);

/**
 * @brief 格式化时间
 *
 * 将time_t类型的时间按照指定格式格式化为字符串。
 *
 * @param t 要格式化的时间
 * @param format 时间格式字符串，遵循strftime的格式
 * @return 返回格式化后的字符串，需要调用者释放内存
 */
char *format_time(time_t t, const char *format);

/* @} */

/*
 * @defgroup 内存安全函数
 * @brief 提供安全的内存分配和释放功能
 * @{
 */

/**
 * @brief 安全的内存分配函数
 *
 * 分配指定大小的内存，并在分配失败时进行适当的错误处理。
 *
 * @param size 要分配的内存大小（字节）
 * @return 成功返回指向分配内存的指针，失败返回NULL
 */
void *safe_malloc(size_t size);

/**
 * @brief 安全的内存清零并分配函数
 *
 * 分配并清零指定数量和大小的内存，在分配失败时进行适当的错误处理。
 *
 * @param num 要分配的元素数量
 * @param size 每个元素的大小（字节）
 * @return 成功返回指向分配内存的指针，失败返回NULL
 */
void *safe_calloc(size_t num, size_t size);

/**
 * @brief 安全的内存释放函数
 *
 * 释放指针指向的内存，并将指针设置为NULL，防止悬空指针。
 *
 * @param ptr 指向要释放的指针的指针
 */
void safe_free(void **ptr);

/* @} */

/*
 * @defgroup 网络相关工具
 * @brief 提供网络地址和端口验证功能
 * @{
 */

/**
 * @brief 验证IP地址格式
 *
 * 检查给定的字符串是否为有效的IPv4地址格式。
 *
 * @param ip 要验证的IP地址字符串
 * @return 有效返回1，无效返回0
 */
int is_valid_ip(const char *ip);

/**
 * @brief 验证端口号
 *
 * 检查给定的端口号是否在有效范围内(1-65535)。
 *
 * @param port 要验证的端口号
 * @return 有效返回1，无效返回0
 */
int is_valid_port(int port);

/* @} */

#endif /* UTILS_H */

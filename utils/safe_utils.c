/**
 * @file utils/safe_utils.c
 * @brief 安全工具函数实现
 *
 * 本文件实现了各种安全的工具函数，包括字符串操作、内存管理和网络验证等功能。
 * 所有函数都经过安全设计，防止缓冲区溢出和其他常见的安全问题。
 *
 * @author 开发团队
 * @date 2025
 */

#include "utils.h"
#include <string.h>
#include <stdlib.h>

/**
 * @brief 安全的字符串复制函数
 *
 * 将源字符串复制到目标缓冲区，确保不会发生缓冲区溢出。
 * 与标准strcpy不同，此函数接受目标缓冲区大小作为参数。
 *
 * @param dest 目标缓冲区
 * @param src 源字符串
 * @param dest_size 目标缓冲区大小（包括空终止符）
 * @return 成功复制的字符数（不包括空终止符）
 */
size_t safe_strcpy(char *dest, const char *src, size_t dest_size)
{
	/* 检查参数有效性 */
	if (!dest || !src || dest_size == 0)
		return 0;

	/* 计算源字符串长度 */
	size_t src_len = strlen(src);
	/* 计算可以安全复制的最大长度，为目标缓冲区大小减1（保留空间给空终止符） */
	size_t copy_len = (src_len >= dest_size) ? (dest_size - 1) : src_len;

	/* 如果有内容需要复制 */
	if (copy_len > 0)
		memcpy(dest, src, copy_len);

	/* 确保字符串以空字符结尾 */
	dest[copy_len] = '\0';
	return copy_len;
}

/**
 * @brief 安全的字符串连接函数
 *
 * 将源字符串连接到目标字符串末尾，确保不会发生缓冲区溢出。
 * 目标字符串必须已经包含以null结尾的字符串。
 *
 * @param dest 目标缓冲区，必须包含以null结尾的字符串
 * @param src 要连接的源字符串
 * @param dest_size 目标缓冲区总大小（包括空终止符）
 * @return 连接后字符串的总长度（不包括空终止符）
 */
size_t safe_strcat(char *dest, const char *src, size_t dest_size)
{
	/* 检查参数有效性 */
	if (!dest || !src || dest_size == 0)
		return 0;

	/* 获取目标字符串当前长度，最多检查dest_size个字符 */
	size_t dest_len = strnlen(dest, dest_size);
	/* 如果目标字符串已经占满缓冲区，无法追加内容 */
	if (dest_len >= dest_size - 1)
		return dest_len;

	/* 计算源字符串长度 */
	size_t src_len = strlen(src);
	/* 计算可以安全追加的最大长度，保留一个字节给空终止符 */
	size_t copy_len = (src_len >= (dest_size - dest_len - 1)) ? (dest_size - dest_len - 1) : src_len;

	/* 如果有内容需要追加 */
	if (copy_len > 0)
		memcpy(dest + dest_len, src, copy_len);

	/* 确保字符串以空字符结尾 */
	dest[dest_len + copy_len] = '\0';
	return dest_len + copy_len;
}

/**
 * @brief 安全的字符串比较函数
 *
 * 比较两个字符串，最多比较max_len个字符，防止缓冲区溢出。
 * 处理了NULL指针的情况，确保不会因空指针而崩溃。
 *
 * @param s1 第一个字符串
 * @param s2 第二个字符串
 * @param max_len 最大比较长度
 * @return 如果字符串相等返回0，s1<s2返回负数，s1>s2返回正数
 */
int safe_strcmp(const char *s1, const char *s2, size_t max_len)
{
	/* 处理NULL指针情况 */
	if (!s1 || !s2)
		return (s1 == s2) ? 0 : (s1 ? 1 : -1);

	/* 使用标准strncmp进行安全比较 */
	return strncmp(s1, s2, max_len);
}

/**
 * @brief 安全的内存分配函数
 *
 * 分配指定大小的内存，并在分配失败时进行适当的错误处理。
 * 目前实现为简单的malloc封装，未来可扩展添加更多安全检查。
 *
 * @param size 要分配的内存大小（字节）
 * @return 成功返回指向分配内存的指针，失败返回NULL
 */
void *safe_malloc(size_t size)
{
	/* 目前直接调用标准malloc，未来可添加更多安全检查 */
	return malloc(size);
}

/**
 * @brief 安全的内存清零并分配函数
 *
 * 分配并清零指定数量和大小的内存，在分配失败时进行适当的错误处理。
 * 目前实现为简单的calloc封装，未来可扩展添加更多安全检查。
 *
 * @param num 要分配的元素数量
 * @param size 每个元素的大小（字节）
 * @return 成功返回指向分配内存的指针，失败返回NULL
 */
void *safe_calloc(size_t num, size_t size)
{
	/* 目前直接调用标准calloc，未来可添加更多安全检查 */
	return calloc(num, size);
}

/**
 * @brief 安全的内存释放函数
 *
 * 释放指针指向的内存，并将指针设置为NULL，防止悬空指针。
 * 这种双重指针的设计确保调用者处的指针也被置为NULL。
 *
 * @param ptr 指向要释放的指针的指针
 */
void safe_free(void **ptr)
{
	/* 检查指针有效性，防止双重释放 */
	if (!ptr || !*ptr)
		return;

	/* 释放内存 */
	free(*ptr);
	/* 将指针置为NULL，防止悬空指针 */
	*ptr = NULL;
}

/**
 * @brief 验证IP地址格式
 *
 * 检查给定的字符串是否为有效的IPv4地址格式。
 * 注意：此函数只进行基本格式验证，不严格检查每段数字的范围。
 *
 * @param ip 要验证的IP地址字符串
 * @return 有效返回1，无效返回0
 */
int is_valid_ip(const char *ip)
{
	/* 检查指针有效性 */
	if (!ip)
		return 0;
	/* 严格验证 IPv4 格式 x.x.x.x，且每段在 0-255 范围内 */
	char *copy = strdup(ip);
	if (!copy)
		return 0;
	int parts = 0;
	char *saveptr = NULL;
	char *token = strtok_r(copy, ".", &saveptr);
	while (token)
	{
		if (parts >= 4)
		{
			free(copy);
			return 0;
		}
		char *endptr = NULL;
		long v = strtol(token, &endptr, 10);
		if (endptr == token || *endptr != '\0' || v < 0 || v > 255)
		{
			free(copy);
			return 0;
		}
		parts++;
		token = strtok_r(NULL, ".", &saveptr);
	}
	free(copy);
	return (parts == 4) ? 1 : 0;
}

/**
 * @brief 验证端口号
 *
 * 检查给定的端口号是否在有效范围内(1-65535)。
 * 端口号0通常保留，65535是最大端口号。
 *
 * @param port 要验证的端口号
 * @return 有效返回1，无效返回0
 */
int is_valid_port(int port)
{
	/* 端口号必须在1到65535范围内 */
	return (port > 0 && port <= 65535) ? 1 : 0;
}

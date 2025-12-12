/**
 * @file connection_manager.c
 * @brief 连接管理器实现
 *
 * 本文件实现了基于内存的连接管理器，用于跟踪所有客户端连接。
 * 提供了添加、删除、查找客户端的功能，以及更新客户端状态的能力。
 * 使用链表结构存储客户端信息，支持通过文件描述符、用户名或用户ID查找客户端。
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "core.h"

/**
 * @brief 客户端链表头指针
 *
 * 指向客户端链表的第一个节点，所有客户端信息都通过这个链表访问。
 */
static Client *clients_head = NULL;

/**
 * @brief 当前连接的客户端数量
 *
 * 记录当前系统中连接的客户端总数。
 */
static int clients_count = 0;

/**
 * @brief 下一个客户端ID
 *
 * 用于为新连接的客户端分配唯一ID，每次分配后自动递增。
 */
static int next_client_id = 1;

/**
 * @brief 根据文件描述符查找客户端
 *
 * 遍历客户端链表，查找具有指定文件描述符的客户端。
 *
 * @param fd 要查找的文件描述符
 * @return Client* 找到返回客户端指针，未找到返回NULL
 */
Client *connection_manager_find_by_fd(int fd)
{
	Client *cur = clients_head;
	while (cur)
	{
		if (cur->sockfd == fd)
			return cur;
		cur = cur->next;
	}
	return NULL;
}

/**
 * @brief 根据用户名查找客户端
 *
 * 遍历客户端链表，查找具有指定用户名的客户端。
 *
 * @param username 要查找的用户名
 * @return Client* 找到返回客户端指针，未找到返回NULL
 */
Client *connection_manager_find_by_username(const char *username)
{
	if (!username)
		return NULL;
	Client *cur = clients_head;
	while (cur)
	{
		if (strncmp(cur->username, username, sizeof(cur->username)) == 0)
			return cur;
		cur = cur->next;
	}
	return NULL;
}

/**
 * @brief 根据用户ID查找客户端
 *
 * 遍历客户端链表，查找具有指定用户ID的客户端。
 *
 * @param user_id 要查找的用户ID
 * @return Client* 找到返回客户端指针，未找到返回NULL
 */
Client *connection_manager_find_by_user_id(int user_id)
{
	Client *cur = clients_head;
	while (cur)
	{
		if (cur->user_id == user_id)
			return cur;
		cur = cur->next;
	}
	return NULL;
}

/**
 * @brief 从文件描述符添加客户端
 *
 * 根据给定的文件描述符创建新的客户端条目，并将其添加到客户端链表中。
 * 如果文件描述符已存在，则不执行任何操作。
 *
 * @param sockfd 客户端的套接字文件描述符
 * @param ip 客户端的IP地址字符串
 * @param port 客户端的端口号
 */
void connection_manager_add_from_fd(int sockfd, const char *ip, int port)
{
	if (connection_manager_find_by_fd(sockfd))
		return;

	Client *c = (Client *)calloc(1, sizeof(Client));
	if (!c)
		return;

	c->sockfd = sockfd;
	c->client_id = next_client_id++;
	c->user_id = -1;
	c->status = CLIENT_STATUS_CONNECTED;
	c->connect_time = time(NULL);
	c->last_active = c->connect_time;
	if (ip)
		strncpy(c->remote_ip, ip, sizeof(c->remote_ip) - 1);
	c->remote_port = port;

	// insert at head
	c->next = clients_head;
	clients_head = c;
	clients_count++;
}

/**
 * @brief 移除客户端
 *
 * 根据文件描述符从客户端链表中移除对应的客户端，并释放相关资源。
 *
 * @param fd 要移除的客户端的文件描述符
 */
void connection_manager_remove(int fd)
{
	Client *prev = NULL;
	Client *cur = clients_head;
	while (cur)
	{
		if (cur->sockfd == fd)
		{
			if (prev)
				prev->next = cur->next;
			else
				clients_head = cur->next;

			free(cur);
			clients_count--;
			return;
		}
		prev = cur;
		cur = cur->next;
	}
}

/**
 * @brief 获取当前连接的客户端数量
 *
 * @return int 当前连接的客户端总数
 */
int connection_manager_count(void)
{
	return clients_count;
}

/**
 * @brief 更新客户端最后活动时间
 *
 * 根据文件描述符找到对应的客户端，并更新其最后活动时间为当前时间。
 *
 * @param fd 客户端的文件描述符
 */
void connection_manager_update_active(int fd)
{
	Client *c = connection_manager_find_by_fd(fd);
	if (c)
	{
		c->last_active = time(NULL);
	}
}

/**
 * @brief 设置客户端认证信息
 *
 * 为指定文件描述符的客户端设置用户ID和用户名，并将状态更新为已认证。
 *
 * @param fd 客户端的文件描述符
 * @param user_id 用户的ID
 * @param username 用户名
 * @return int 成功返回0，失败返回-1
 */
int connection_manager_set_auth(int fd, int user_id, const char *username)
{
	Client *c = connection_manager_find_by_fd(fd);
	if (!c)
		return -1;
	c->user_id = user_id;
	if (username)
		strncpy(c->username, username, sizeof(c->username) - 1);
	c->status = CLIENT_STATUS_AUTHENTICATED;
	return 0;
}

/**
 * @brief 设置客户端状态
 *
 * 为指定文件描述符的客户端设置新的状态。
 *
 * @param fd 客户端的文件描述符
 * @param status 要设置的新状态
 */
void connection_manager_set_status(int fd, int status)
{
	Client *c = connection_manager_find_by_fd(fd);
	if (c)
		c->status = status;
}

/**
 * @brief 获取所有客户端
 *
 * 返回一个包含所有客户端指针的动态分配数组，调用者负责释放该数组。
 * 使用safe_free释放时请传入数组地址。
 *
 * @param out_count 输出参数，用于返回客户端数量
 * @return Client** 成功返回客户端指针数组，失败返回NULL
 */
Client **connection_manager_get_all(int *out_count)
{
	if (out_count)
		*out_count = 0;

	if (clients_count == 0)
		return NULL;

	Client **arr = (Client **)calloc(clients_count, sizeof(Client *));
	if (!arr)
		return NULL;

	int i = 0;
	Client *cur = clients_head;
	while (cur && i < clients_count)
	{
		arr[i++] = cur;
		cur = cur->next;
	}

	if (out_count)
		*out_count = i;
	return arr;
}

/**
 * @brief 打印所有客户端信息
 *
 * 遍历客户端链表，打印每个客户端的基本信息，包括文件描述符、
 * 客户端ID、用户ID、用户名和状态。
 */
void connection_manager_print_all(void)
{
	Client *cur = clients_head;
	printf("[connection_manager] total=%d\n", clients_count);
	while (cur)
	{
		printf(" fd=%d id=%d user=%d name=%s status=%d\n", cur->sockfd, cur->client_id, cur->user_id, cur->username, cur->status);
		cur = cur->next;
	}
}

/**
 * @brief 清理所有客户端
 *
 * 释放客户端链表中的所有节点，重置客户端计数和链表头指针。
 * 通常在服务器关闭时调用。
 */
void connection_manager_cleanup(void)
{
	Client *cur = clients_head;
	while (cur)
	{
		Client *next = cur->next;
		free(cur);
		cur = next;
	}
	clients_head = NULL;
	clients_count = 0;
}

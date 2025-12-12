/**
 * @file client.h
 * @brief 客户端核心头文件
 * 
 * 定义客户端的核心数据结构和函数接口。
 */

#ifndef CLIENT_H
#define CLIENT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include "../protocol/protocol.h"
#include "../utils/utils.h"

/** 
 * @brief 客户端状态枚举 
 */
typedef enum {
    CLIENT_DISCONNECTED,  /**< 未连接 */
    CLIENT_CONNECTING,    /**< 连接中 */
    CLIENT_CONNECTED,     /**< 已连接 */
    CLIENT_AUTHENTICATED, /**< 已认证 */
    CLIENT_ERROR          /**< 错误状态 */
} ClientState;

/**
 * @brief 客户端结构体
 */
typedef struct {
    int sockfd;                 /**< 套接字文件描述符 */
    char server_ip[32];         /**< 服务器IP地址 */
    int server_port;            /**< 服务器端口 */
    ClientState state;          /**< 客户端状态 */
    char username[32];          /**< 用户名 */
    bool running;               /**< 运行状态标志 */
    pthread_t recv_thread;      /**< 接收线程 */
    pthread_mutex_t state_lock; /**< 状态锁 */
} AppClient;

/**
 * @brief 初始化客户端
 * 
 * @param client 客户端结构体指针
 * @param server_ip 服务器IP地址
 * @param server_port 服务器端口
 * @return int 成功返回0，失败返回-1
 */
int client_init(AppClient *client, const char *server_ip, int server_port);

/**
 * @brief 连接到服务器
 * 
 * @param client 客户端结构体指针
 * @return int 成功返回0，失败返回-1
 */
int client_connect(AppClient *client);

/**
 * @brief 断开与服务器的连接
 * 
 * @param client 客户端结构体指针
 * @return int 成功返回0，失败返回-1
 */
int client_disconnect(AppClient *client);

/**
 * @brief 用户登录
 * 
 * @param client 客户端结构体指针
 * @param username 用户名
 * @param password 密码
 * @return int 成功返回0，失败返回-1
 */
int client_login(AppClient *client, const char *username, const char *password);

/**
 * @brief 用户登出
 * 
 * @param client 客户端结构体指针
 * @return int 成功返回0，失败返回-1
 */
int client_logout(AppClient *client);

/**
 * @brief 发送消息
 * 
 * @param client 客户端结构体指针
 * @param receiver 接收者
 * @param content 消息内容
 * @return int 成功返回0，失败返回-1
 */
int client_send_message(AppClient *client, const char *receiver, const char *content);

/**
 * @brief 发送广播消息
 * 
 * @param client 客户端结构体指针
 * @param content 消息内容
 * @return int 成功返回0，失败返回-1
 */
int client_send_broadcast(AppClient *client, const char *content);

/**
 * @brief 发送群组消息
 * 
 * @param client 客户端结构体指针
 * @param group_name 群组名
 * @param content 消息内容
 * @return int 成功返回0，失败返回-1
 */
int client_send_group_message(AppClient *client, const char *group_name, const char *content);

/**
 * @brief 请求历史记录
 * 
 * @param client 客户端结构体指针
 * @param target 目标用户或群组
 * @param start_time 开始时间（可为NULL）
 * @param end_time 结束时间（可为NULL）
 * @return int 成功返回0，失败返回-1
 */
int client_request_history(AppClient *client, const char *target, 
                        const char *start_time, const char *end_time);

/**
 * @brief 请求状态信息
 * 
 * @param client 客户端结构体指针
 * @return int 成功返回0，失败返回-1
 */
int client_request_status(AppClient *client);

/**
 * @brief 启动客户端
 * 
 * @param client 客户端结构体指针
 * @return int 成功返回0，失败返回-1
 */
int client_start(AppClient *client);

/**
 * @brief 停止客户端
 * 
 * @param client 客户端结构体指针
 * @return int 成功返回0，失败返回-1
 */
int client_stop(AppClient *client);

/**
 * @brief 清理客户端资源
 * 
 * @param client 客户端结构体指针
 */
void client_cleanup(AppClient *client);

#endif /* CLIENT_H */

/**
 * @file ui.h
 * @brief 客户端用户界面头文件
 * 
 * 定义客户端用户界面的函数接口。
 */

#ifndef UI_H
#define UI_H

#include "client.h"

/**
 * @brief 显示欢迎信息
 */
void ui_show_welcome(void);

/**
 * @brief 显示帮助信息
 */
void ui_show_help(void);

/**
 * @brief 显示连接状态
 * 
 * @param client 客户端结构体指针
 */
void ui_show_status(AppClient *client);

/**
 * @brief 处理用户输入
 * 
 * @param client 客户端结构体指针
 * @return int 返回0表示继续，-1表示退出
 */
int ui_handle_input(AppClient *client);

/**
 * @brief 处理连接命令
 * 
 * @param client 客户端结构体指针
 * @param cmd 命令字符串
 * @return int 成功返回0，失败返回-1
 */
int ui_cmd_connect(AppClient *client, const char *cmd);

/**
 * @brief 处理断开连接命令
 * 
 * @param client 客户端结构体指针
 * @param cmd 命令字符串
 * @return int 成功返回0，失败返回-1
 */
int ui_cmd_disconnect(AppClient *client, const char *cmd);

/**
 * @brief 处理登录命令
 * 
 * @param client 客户端结构体指针
 * @param cmd 命令字符串
 * @return int 成功返回0，失败返回-1
 */
int ui_cmd_login(AppClient *client, const char *cmd);

/**
 * @brief 处理登出命令
 * 
 * @param client 客户端结构体指针
 * @param cmd 命令字符串
 * @return int 成功返回0，失败返回-1
 */
int ui_cmd_logout(AppClient *client, const char *cmd);

/**
 * @brief 处理发送消息命令
 * 
 * @param client 客户端结构体指针
 * @param cmd 命令字符串
 * @return int 成功返回0，失败返回-1
 */
int ui_cmd_send(AppClient *client, const char *cmd);

/**
 * @brief 处理广播命令
 * 
 * @param client 客户端结构体指针
 * @param cmd 命令字符串
 * @return int 成功返回0，失败返回-1
 */
int ui_cmd_broadcast(AppClient *client, const char *cmd);

/**
 * @brief 处理群组消息命令
 * 
 * @param client 客户端结构体指针
 * @param cmd 命令字符串
 * @return int 成功返回0，失败返回-1
 */
int ui_cmd_group(AppClient *client, const char *cmd);

/**
 * @brief 处理历史记录命令
 * 
 * @param client 客户端结构体指针
 * @param cmd 命令字符串
 * @return int 成功返回0，失败返回-1
 */
int ui_cmd_history(AppClient *client, const char *cmd);

/**
 * @brief 处理状态查询命令
 * 
 * @param client 客户端结构体指针
 * @param cmd 命令字符串
 * @return int 成功返回0，失败返回-1
 */
int ui_cmd_status(AppClient *client, const char *cmd);

/**
 * @brief 处理帮助命令
 * 
 * @param client 客户端结构体指针
 * @param cmd 命令字符串
 * @return int 成功返回0，失败返回-1
 */
int ui_cmd_help(AppClient *client, const char *cmd);

/**
 * @brief 处理退出命令
 * 
 * @param client 客户端结构体指针
 * @param cmd 命令字符串
 * @return int 成功返回0，失败返回-1
 */
int ui_cmd_quit(AppClient *client, const char *cmd);

#endif /* UI_H */

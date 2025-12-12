// server.h
#ifndef SERVER_H
#define SERVER_H

#include "models/models.h"

/* 全局服务器配置 */
extern ServerConfig server_config;

/* 服务器控制函数 */
void server_start(void);
void server_stop(void);
int server_is_running(void);

#endif /* SERVER_H */

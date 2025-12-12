/**
 * @file main.c
 * @brief 客户端主程序
 *
 * 实现客户端的主程序入口和主循环。
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "client.h"
#include "ui.h"

/**
 * @brief 全局客户端实例
 */
static AppClient g_client;

/**
 * @brief 信号处理函数
 *
 * @param sig 信号值
 */
static void signal_handler(int sig)
{
	switch (sig)
	{
	case SIGINT:
		printf("\n收到中断信号，正在退出...\n");
		client_stop(&g_client);
		client_cleanup(&g_client);
		exit(0);
		break;
	default:
		break;
	}
}

/**
 * @brief 主函数
 *
 * @param argc 命令行参数数量
 * @param argv 命令行参数数组
 * @return int 程序退出码
 */
int main(int argc, char *argv[])
{
	(void)argc;
	(void)argv;
	// 设置信号处理
	signal(SIGINT, signal_handler);

	// 初始化客户端
	if (client_init(&g_client, "127.0.0.1", 8080) != 0)
	{
		fprintf(stderr, "初始化客户端失败\n");
		return 1;
	}

	// 显示欢迎信息
	ui_show_welcome();

	// 主循环
	while (1)
	{
		// 处理用户输入
		if (ui_handle_input(&g_client) != 0)
		{
			break;
		}
	}

	// 清理资源
	client_stop(&g_client);
	client_cleanup(&g_client);

	return 0;
}

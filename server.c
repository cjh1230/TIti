// server.c
#include <stdio.h>
#include <stdlib.h>
#include "network/network.h"
#include "utils/utils.h"
#include "storage/storage.h"

/* 服务器配置 */
ServerConfig server_config = {
	.server_port = DEFAULT_PORT,
	.max_clients = MAX_CLIENTS,
	.max_history = 1000,
	.timeout_seconds = 300,
	.log_path = "server.log",
	.require_auth = 1,
	.enable_encryption = 0};

/* 打印服务器信息 */
void print_server_info(void)
{
	printf("=== Message Forward Server ===\n");
	printf("Port: %d\n", server_config.server_port);
	printf("Max clients: %d\n", server_config.max_clients);
	printf("Log file: %s\n", server_config.log_path);
	printf("Press Ctrl+C to stop the server\n\n");
}

/* 主函数 */
int main(int argc, char *argv[])
{
	// 解析命令行参数
	if (argc > 1)
	{
		server_config.server_port = atoi(argv[1]);
	}

	// 打印服务器信息
	print_server_info();

	// 设置日志
	set_log_file(server_config.log_path);
	set_log_level(LOG_INFO);

	/* 初始化存储（包括默认测试用户或从持久化加载用户） */
	storage_init();

	LOG_INFO("Server starting...");

	// 初始化TCP服务器
	if (tcp_server_init(server_config.server_port) < 0)
	{
		LOG_ERROR("Failed to initialize TCP server");
		return 1;
	}

	// 初始化事件循环
	event_loop_init();

	// 初始化客户端处理器
	client_handler_init();

	// 启动服务器
	if (tcp_server_start() < 0)
	{
		LOG_ERROR("Failed to start TCP server");
		return 1;
	}

	// 运行事件循环
	int server_fd = tcp_server_get_fd();
	if (server_fd > 0)
	{
		event_loop_run(server_fd);
	}

	// 清理资源
	LOG_INFO("Server shutting down...");
	event_loop_stop();
	tcp_server_stop();

	LOG_INFO("Server stopped");
	return 0;
}

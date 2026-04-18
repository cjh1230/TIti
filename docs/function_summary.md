# ITit 函数总览

本文档按源码目录总结 `src` 下项目自有 `.c/.h` 文件中的函数。第三方库、测试文件、编译产物不在本文档范围内。

## client

### `src/client/client.c`
文件职责：实现客户端连接、登录、消息发送、接收线程和生命周期管理。

| 函数 | 可见性 | 说明 |
| --- | --- | --- |
| `response_message_text` | static | 从服务端响应内容中提取用户可读消息文本。 |
| `client_emit_line` | static | 将接收线程产生的消息发送到回调，未设置回调时打印到终端。 |
| `client_emitf` | static | 格式化一行客户端消息并交给 `client_emit_line` 输出。 |
| `recv_thread_func` | static | 后台轮询服务器消息、解析协议并更新客户端状态或输出消息。 |
| `client_init` | public | 初始化 `AppClient`、默认服务器信息、socket 状态和状态锁。 |
| `client_connect` | public | 根据客户端保存的服务器地址建立 TCP 连接并更新状态。 |
| `client_disconnect` | public | 停止接收线程、关闭 socket 并重置客户端认证状态。 |
| `client_login` | public | 构建并发送登录消息，等待服务端确认后完成本地认证状态更新。 |
| `client_logout` | public | 构建并发送登出消息，并将本地状态退回已连接未认证。 |
| `client_send_message` | public | 向指定用户构建并发送私聊消息。 |
| `client_send_broadcast` | public | 构建并发送广播消息。 |
| `client_send_group_message` | public | 构建并发送群组消息请求。 |
| `client_request_history` | public | 构建并发送历史记录查询请求。 |
| `client_request_status` | public | 构建并发送服务端状态查询请求。 |
| `client_start` | public | 启动客户端接收线程。 |
| `client_set_message_callback` | public | 设置接收线程消息输出回调及其上下文。 |
| `client_stop` | public | 停止客户端接收线程并等待线程退出。 |
| `client_cleanup` | public | 停止客户端、断开连接、销毁锁并清空结构体。 |

### `src/client/client.h`
文件职责：声明客户端核心数据结构、状态枚举和客户端 API。

| 函数 | 可见性 | 说明 |
| --- | --- | --- |
| `client_init` | public | 声明客户端初始化接口。 |
| `client_connect` | public | 声明连接服务器接口。 |
| `client_disconnect` | public | 声明断开服务器接口。 |
| `client_login` | public | 声明登录接口。 |
| `client_logout` | public | 声明登出接口。 |
| `client_send_message` | public | 声明私聊消息发送接口。 |
| `client_send_broadcast` | public | 声明广播消息发送接口。 |
| `client_send_group_message` | public | 声明群组消息发送接口。 |
| `client_request_history` | public | 声明历史记录查询接口。 |
| `client_request_status` | public | 声明状态查询接口。 |
| `client_start` | public | 声明启动接收线程接口。 |
| `client_set_message_callback` | public | 声明接收消息回调设置接口。 |
| `client_stop` | public | 声明停止接收线程接口。 |
| `client_cleanup` | public | 声明客户端资源清理接口。 |

### `src/client/client_commands.c`
文件职责：实现 CLI 和 TUI 共用的客户端命令解析与执行逻辑。

| 函数 | 可见性 | 说明 |
| --- | --- | --- |
| `trim_whitespace` | static | 去除命令输入首尾空白。 |
| `command_matches` | static | 判断输入命令是否与指定命令名完整匹配。 |
| `command_args` | static | 返回命令名之后的参数起始位置。 |
| `command_write` | static | 通过命令上下文回调输出一行文本。 |
| `command_writef` | static | 格式化命令输出并写入上下文回调。 |
| `notify_state_changed` | static | 在状态或聊天对象变化后通知界面刷新。 |
| `command_show_help` | static | 输出共用命令帮助信息。 |
| `send_to_active_receiver` | static | 将普通输入作为消息发送给当前 `to` 设置的聊天对象。 |
| `command_connect` | static | 处理 `connect/c` 命令并启动客户端接收线程。 |
| `command_disconnect` | static | 处理 `disconnect/d` 命令并断开连接。 |
| `command_login` | static | 处理 `login/l` 命令并执行登录。 |
| `command_logout` | static | 处理 `logout` 命令并执行登出。 |
| `command_send` | static | 处理 `send/s` 命令并发送私聊消息。 |
| `command_broadcast` | static | 处理 `broadcast/b` 命令并发送广播消息。 |
| `command_group` | static | 处理 `group/g` 命令并发送群组消息请求。 |
| `command_history` | static | 处理 `history/h` 命令并发送历史查询请求。 |
| `command_to` | static | 处理 `to` 命令并设置当前聊天对象。 |
| `command_status` | static | 处理 `status/st` 命令并发送状态查询请求。 |
| `client_command_context_init` | public | 初始化命令上下文和输出回调。 |
| `client_command_set_state_callback` | public | 设置命令执行后的状态变更回调。 |
| `client_command_execute` | public | 解析并执行一行用户输入。 |
| `client_command_state_label` | public | 将客户端状态转换为用户可读标签。 |
| `client_command_active_receiver` | public | 返回当前命令上下文中的聊天对象。 |

### `src/client/client_commands.h`
文件职责：声明 CLI/TUI 共用命令上下文、回调类型和命令执行 API。

| 函数 | 可见性 | 说明 |
| --- | --- | --- |
| `client_command_context_init` | public | 声明命令上下文初始化接口。 |
| `client_command_set_state_callback` | public | 声明状态变更回调设置接口。 |
| `client_command_execute` | public | 声明共用命令执行接口。 |
| `client_command_state_label` | public | 声明客户端状态标签查询接口。 |
| `client_command_active_receiver` | public | 声明当前聊天对象查询接口。 |

### `src/client/main.c`
文件职责：普通命令行客户端入口，负责初始化客户端、日志和主输入循环。

| 函数 | 可见性 | 说明 |
| --- | --- | --- |
| `signal_handler` | static | 处理 `SIGINT`，停止并清理客户端后退出。 |
| `main` | public | 初始化命令行客户端并循环处理用户输入。 |

### `src/client/tui_main.c`
文件职责：TUI 客户端入口，负责连接 TUI 界面、客户端核心和共用命令层。

| 函数 | 可见性 | 说明 |
| --- | --- | --- |
| `tui_update_status` | static | 根据客户端状态、用户名、服务器地址和聊天对象刷新状态栏。 |
| `tui_command_write` | static | 将命令层输出追加到 TUI 消息区。 |
| `tui_command_state_changed` | static | 在命令改变状态时触发 TUI 状态栏刷新。 |
| `tui_client_message` | static | 将接收线程收到的服务端消息写入 TUI 消息区。 |
| `main` | public | 初始化 TUI 客户端并循环读取输入执行命令。 |

### `src/client/ui.c`
文件职责：普通 CLI 界面薄封装，负责显示提示符、读取 stdin 并调用共用命令层。

| 函数 | 可见性 | 说明 |
| --- | --- | --- |
| `cli_write_line` | static | 将命令层输出打印到标准输出。 |
| `ensure_command_context` | static | 懒初始化 CLI 使用的命令上下文。 |
| `ui_show_prompt` | static | 根据客户端状态和聊天对象打印 CLI 提示符。 |
| `ui_show_welcome` | public | 打印 CLI 欢迎信息。 |
| `ui_show_help` | public | 通过共用命令层输出帮助信息。 |
| `ui_show_status` | public | 打印当前连接状态、服务器、用户名和聊天对象。 |
| `ui_handle_input` | public | 读取一行用户输入并交给共用命令层执行。 |

### `src/client/ui.h`
文件职责：声明普通 CLI 界面的少量入口函数。

| 函数 | 可见性 | 说明 |
| --- | --- | --- |
| `ui_show_welcome` | public | 声明欢迎信息显示接口。 |
| `ui_show_help` | public | 声明帮助信息显示接口。 |
| `ui_show_status` | public | 声明状态显示接口。 |
| `ui_handle_input` | public | 声明用户输入处理接口。 |

## core

### `src/core/connection_manager.c`
文件职责：维护服务端当前连接客户端的链表和认证状态。

| 函数 | 可见性 | 说明 |
| --- | --- | --- |
| `connection_manager_find_by_fd` | public | 按 socket 描述符查找客户端连接。 |
| `connection_manager_find_by_username` | public | 按已认证用户名查找客户端连接。 |
| `connection_manager_add_from_fd` | public | 根据新 socket 创建并登记客户端连接。 |
| `connection_manager_remove` | public | 从连接链表中移除指定 socket 的客户端。 |
| `connection_manager_count` | public | 返回当前连接数量。 |
| `connection_manager_update_active` | public | 更新指定客户端最后活跃时间。 |
| `connection_manager_set_auth` | public | 设置客户端用户 ID、用户名和认证状态。 |
| `connection_manager_set_status` | public | 修改指定客户端的连接状态。 |
| `connection_manager_get_all` | public | 返回当前所有客户端指针数组。 |
| `connection_manager_print_all` | public | 打印当前连接列表用于调试。 |
| `connection_manager_cleanup` | public | 释放所有连接记录并重置计数。 |

### `src/core/core.h`
文件职责：聚合核心模块的连接管理、会话管理和消息路由接口声明。

| 函数 | 可见性 | 说明 |
| --- | --- | --- |
| `connection_manager_find_by_fd` | public | 声明按 socket 查找连接的接口。 |
| `connection_manager_find_by_username` | public | 声明按用户名查找连接的接口。 |
| `connection_manager_add_from_fd` | public | 声明新增连接记录接口。 |
| `connection_manager_remove` | public | 声明移除连接记录接口。 |
| `connection_manager_count` | public | 声明连接数量查询接口。 |
| `connection_manager_update_active` | public | 声明最后活跃时间更新接口。 |
| `connection_manager_set_auth` | public | 声明客户端认证信息设置接口。 |
| `connection_manager_set_status` | public | 声明客户端状态设置接口。 |
| `connection_manager_print_all` | public | 声明连接调试打印接口。 |
| `connection_manager_cleanup` | public | 声明连接管理器清理接口。 |
| `connection_manager_get_all` | public | 声明获取全部连接接口。 |
| `session_manager_authenticate` | public | 声明用户认证接口。 |
| `session_manager_logout` | public | 声明用户登出接口。 |
| `session_manager_is_authenticated` | public | 声明认证状态检查接口。 |
| `session_manager_get_user_id` | public | 声明当前用户 ID 查询接口。 |
| `session_manager_get_username` | public | 声明当前用户名查询接口。 |
| `session_manager_is_user_online` | public | 声明在线用户检查接口。 |
| `session_manager_get_online_users` | public | 声明在线用户列表获取接口。 |
| `route_message` | public | 声明当前消息路由入口。 |

### `src/core/message_router.c`
文件职责：根据消息类型和目标用户将消息转发给对应客户端。

| 函数 | 可见性 | 说明 |
| --- | --- | --- |
| `send_to_socket` | static | 向指定 socket 发送完整字符串消息。 |
| `route_private_message` | static | 将私聊消息路由给在线接收者。 |
| `route_broadcast_message` | static | 将广播消息发送给除发送者外的已认证客户端。 |
| `route_group_message` | static | 群组消息路由占位，当前返回未实现。 |
| `route_message` | public | 根据消息类型选择私聊、广播或群组路由。 |

### `src/core/session_manager.c`
文件职责：处理用户登录认证、登出和在线状态查询。

| 函数 | 可见性 | 说明 |
| --- | --- | --- |
| `session_manager_authenticate` | public | 验证用户名密码并把连接标记为已认证。 |
| `session_manager_logout` | public | 清除指定连接的认证信息并退回已连接状态。 |
| `session_manager_is_authenticated` | public | 检查指定 socket 是否已认证。 |
| `session_manager_get_user_id` | public | 获取已认证连接对应的用户 ID。 |
| `session_manager_get_username` | public | 获取已认证连接对应的用户名。 |
| `session_manager_is_user_online` | public | 判断指定用户名是否在线且已认证。 |
| `session_manager_get_online_users` | public | 返回当前所有在线用户名数组。 |

## models

### `src/models/client.h`
文件职责：当前为空文件，未定义函数。

当前无函数，仅作为待补充的客户端模型头文件占位。

### `src/models/message.h`
文件职责：当前为空文件，未定义函数。

当前无函数，仅作为待补充的消息模型头文件占位。

### `src/models/models.h`
文件职责：定义用户、客户端、消息、群组、服务器配置和响应等核心数据结构。

当前无函数，仅包含宏、枚举、结构体和全局配置声明。

### `src/models/user.h`
文件职责：当前为空文件，未定义函数。

当前无函数，仅作为待补充的用户模型头文件占位。

## network

### `src/network/client_handler.c`
文件职责：处理服务端单个客户端 socket 的读写、广播和关闭逻辑。

| 函数 | 可见性 | 说明 |
| --- | --- | --- |
| `client_handler_init` | public | 初始化客户端处理器。 |
| `client_handler_handle` | public | 读取客户端数据、解析协议并分发到命令处理器。 |
| `client_handler_send` | public | 向指定客户端 socket 发送字符串数据。 |
| `client_handler_broadcast` | public | 向所有符合条件的客户端广播字符串数据。 |
| `client_handler_close` | public | 关闭客户端 socket 并从事件循环移除。 |
| `get_client_ip` | public | 通过 socket 查询客户端 IP 字符串。 |
| `get_client_port` | public | 通过 socket 查询客户端端口。 |

### `src/network/event_handler.c`
文件职责：当前为空文件，未定义函数。

当前无函数，仅作为待补充的事件处理模块占位。

### `src/network/event_loop.c`
文件职责：实现基于 `select` 的服务端事件循环和客户端 socket 集合维护。

| 函数 | 可见性 | 说明 |
| --- | --- | --- |
| `event_loop_init` | public | 初始化客户端 fd 数组和循环状态。 |
| `add_client` | static | 将新客户端加入连接管理器和事件循环。 |
| `remove_client` | static | 从事件循环和连接管理器中移除客户端。 |
| `event_loop_remove_fd` | public | 供其他模块主动从事件循环移除指定 fd。 |
| `accept_connection` | static | 接受服务端监听 socket 上的新连接。 |
| `event_loop_run` | public | 运行 `select` 循环处理新连接和客户端数据。 |
| `event_loop_stop` | public | 停止事件循环并关闭所有客户端连接。 |

### `src/network/network.h`
文件职责：声明服务端网络、事件循环、客户端处理器和 TCP 客户端接口。

| 函数 | 可见性 | 说明 |
| --- | --- | --- |
| `tcp_server_init` | public | 声明 TCP 服务端初始化接口。 |
| `tcp_server_start` | public | 声明服务端监听启动接口。 |
| `tcp_server_stop` | public | 声明服务端停止接口。 |
| `tcp_server_get_fd` | public | 声明获取服务端监听 socket 接口。 |
| `tcp_server_is_running` | public | 声明服务端运行状态查询接口。 |
| `event_loop_init` | public | 声明事件循环初始化接口。 |
| `event_loop_run` | public | 声明事件循环运行接口。 |
| `event_loop_stop` | public | 声明事件循环停止接口。 |
| `event_loop_remove_fd` | public | 声明事件循环移除 fd 接口。 |
| `client_handler_init` | public | 声明客户端处理器初始化接口。 |
| `client_handler_handle` | public | 声明客户端数据处理接口。 |
| `client_handler_send` | public | 声明客户端发送接口。 |
| `client_handler_broadcast` | public | 声明客户端广播接口。 |
| `client_handler_close` | public | 声明客户端关闭接口。 |
| `tcp_connect` | public | 声明 TCP 客户端连接接口。 |
| `tcp_send` | public | 声明 TCP 发送接口。 |
| `tcp_receive` | public | 声明 TCP 接收接口。 |
| `tcp_close` | public | 声明 TCP 关闭接口。 |
| `set_socket_nonblocking` | public | 声明设置 socket 非阻塞接口。 |
| `get_client_ip` | public | 声明获取客户端 IP 接口。 |
| `get_client_port` | public | 声明获取客户端端口接口。 |

### `src/network/tcp_client.c`
文件职责：实现跨平台 TCP 客户端连接、发送、接收和关闭。

| 函数 | 可见性 | 说明 |
| --- | --- | --- |
| `create_tcp_socket` | static | 创建 TCP socket 并初始化平台 socket 层。 |
| `connect_to_server` | static | 将 socket 连接到指定服务器地址和端口。 |
| `tcp_connect` | public | 创建 socket、连接服务器并返回连接 fd。 |
| `tcp_send` | public | 循环发送指定长度的数据直到完成或出错。 |
| `tcp_receive` | public | 从 socket 接收数据并处理非阻塞状态。 |
| `tcp_close` | public | 关闭 TCP socket。 |

### `src/network/tcp_server.c`
文件职责：实现 TCP 服务端 socket 初始化、监听、信号处理和关闭。

| 函数 | 可见性 | 说明 |
| --- | --- | --- |
| `signal_handler` | static | 在 Unix 平台接收退出信号并标记服务端停止。 |
| `setup_signals` | static | 设置服务端信号处理，Windows 下为空实现。 |
| `tcp_server_init` | public | 创建、配置并绑定服务端监听 socket。 |
| `tcp_server_start` | public | 对监听 socket 调用 `listen` 并标记服务端运行。 |
| `tcp_server_stop` | public | 关闭监听 socket 并清理平台 socket 层。 |
| `tcp_server_get_fd` | public | 返回服务端监听 socket。 |
| `tcp_server_is_running` | public | 返回服务端运行标志。 |
| `set_socket_nonblocking` | public | 将指定 socket 设置为非阻塞模式。 |

## platform

### `src/platform/platform.h`
文件职责：提供 Windows 和 POSIX 的 socket、线程、锁、时间等跨平台内联适配函数。

| 函数 | 可见性 | 说明 |
| --- | --- | --- |
| `platform_socket_init` | static inline | 初始化平台 socket 子系统，POSIX 下为空操作。 |
| `platform_socket_cleanup` | static inline | 清理平台 socket 子系统，POSIX 下为空操作。 |
| `platform_socket_close` | static inline | 关闭平台 socket。 |
| `platform_socket_set_nonblocking` | static inline | 将 socket 设置为非阻塞模式。 |
| `platform_socket_last_error` | static inline | 返回最近一次平台 socket 错误码。 |
| `platform_socket_would_block` | static inline | 判断最近错误是否表示非阻塞暂无数据。 |
| `platform_socket_in_progress` | static inline | 判断最近错误是否表示连接仍在进行。 |
| `platform_socket_interrupted` | static inline | 判断最近错误是否表示系统调用被中断。 |
| `platform_socket_error_message` | static inline | 返回最近 socket 错误的人类可读字符串。 |
| `platform_socket_error_message_code` | static inline | 将指定错误码转换为人类可读字符串。 |
| `platform_socket_send` | static inline | 跨平台发送 socket 数据。 |
| `platform_socket_recv` | static inline | 跨平台接收 socket 数据。 |
| `platform_select_nfds` | static inline | 返回 `select` 需要的 nfds 参数，Windows 下忽略。 |
| `platform_sleep_ms` | static inline | 以毫秒为单位休眠当前线程。 |
| `platform_strdup` | static inline | 复制字符串并返回堆内存副本。 |
| `platform_localtime` | static inline | 跨平台安全转换本地时间结构。 |
| `platform_mutex_init` | static inline | 初始化平台互斥/锁对象。 |
| `platform_mutex_lock` | static inline | 加锁平台互斥/锁对象。 |
| `platform_mutex_unlock` | static inline | 解锁平台互斥/锁对象。 |
| `platform_mutex_destroy` | static inline | 销毁平台互斥/锁对象。 |
| `platform_thread_create` | static inline | 创建平台线程。 |
| `platform_thread_is_valid` | static inline | 判断线程句柄是否有效。 |
| `platform_thread_join` | static inline | 等待线程结束并释放线程句柄资源。 |
| `platform_strtok_r` | static inline | 提供可重入字符串分割的跨平台实现。 |

## protocol

### `src/protocol/builder.c`
文件职责：构建符合项目文本协议格式的请求、消息、响应和系统通知字符串。

| 函数 | 可见性 | 说明 |
| --- | --- | --- |
| `build_login_msg` | public | 构建登录请求消息。 |
| `build_logout_msg` | public | 构建登出请求消息。 |
| `build_text_msg` | public | 构建私聊文本消息并转义内容。 |
| `build_broadcast_msg` | public | 构建广播消息并转义内容。 |
| `build_group_msg` | public | 构建群组消息并生成 `group:` 接收者。 |
| `build_history_request` | public | 构建历史记录查询请求。 |
| `build_status_request` | public | 构建状态查询请求。 |
| `build_response_msg` | public | 构建 `OK` 或 `ERROR` 响应消息。 |
| `build_success_msg` | public | 构建成功响应消息。 |
| `build_error_msg` | public | 根据错误码构建错误响应消息。 |
| `build_response_from_struct` | public | 从 `Response` 结构体构建响应消息。 |
| `build_user_online_msg` | public | 构建用户上线广播通知。 |
| `build_user_offline_msg` | public | 构建用户下线广播通知。 |
| `build_system_notification` | public | 构建系统广播通知消息。 |

### `src/protocol/command_dandler.c`
文件职责：处理服务端收到的协议命令并调用认证、路由和存储相关模块。

| 函数 | 可见性 | 说明 |
| --- | --- | --- |
| `send_to_socket` | static | 向指定客户端 socket 发送字符串响应。 |
| `handle_login` | static | 处理登录消息、执行认证并发送登录结果。 |
| `handle_logout` | static | 处理登出消息并发送登出结果。 |
| `handle_send_message` | static | 校验私聊权限并调用消息路由发送私聊消息。 |
| `handle_broadcast` | static | 校验广播权限并调用消息路由广播消息。 |
| `handle_history_request` | static | 处理历史记录请求，当前返回未实现错误。 |
| `handle_status_request` | static | 构建并返回当前服务端状态信息。 |
| `handle_group_message` | static | 处理群组消息，当前返回未实现错误。 |
| `handle_command` | public | 根据消息类型分派到具体命令处理函数。 |
| `handle_raw_message` | public | 解析原始协议字符串并调用命令处理入口。 |

### `src/protocol/parser.c`
文件职责：解析、序列化、校验协议消息并提供协议辅助函数。

| 函数 | 可见性 | 说明 |
| --- | --- | --- |
| `parse_message` | public | 将原始协议字符串解析为 `Message` 结构体。 |
| `serialize_message` | public | 将 `Message` 结构体序列化为协议字符串。 |
| `validate_message` | public | 检查原始协议字符串是否满足基本字段格式。 |
| `get_command_type` | public | 将消息类型字符串转换为命令枚举。 |
| `get_command_str` | public | 将命令枚举转换为消息类型字符串。 |
| `is_valid_msg_type` | public | 判断消息类型是否是支持的协议类型。 |
| `is_valid_username` | public | 校验用户名长度和字符合法性。 |
| `escape_field` | public | 转义字段中的分隔符、反斜杠和换行。 |
| `unescape_field` | public | 还原字段中的协议转义序列。 |
| `get_current_timestamp` | public | 返回当前时间的协议时间戳字符串。 |
| `parse_group_id` | public | 从群组接收者字符串中解析群组 ID。 |
| `is_login_msg` | public | 判断消息是否为登录请求。 |
| `is_logout_msg` | public | 判断消息是否为登出请求。 |
| `is_private_msg` | public | 判断消息是否为私聊消息。 |
| `is_broadcast_msg` | public | 判断消息是否为广播消息。 |
| `is_group_msg` | public | 判断消息是否为群组消息。 |
| `is_history_request` | public | 判断消息是否为历史查询请求。 |
| `is_status_request` | public | 判断消息是否为状态查询请求。 |
| `free_message` | public | 释放解析得到的 `Message` 结构体。 |

### `src/protocol/protocol.h`
文件职责：声明协议解析、构建、校验、命令处理和消息类型判断接口。

| 函数 | 可见性 | 说明 |
| --- | --- | --- |
| `parse_message` | public | 声明协议解析接口。 |
| `serialize_message` | public | 声明协议序列化接口。 |
| `get_command_type` | public | 声明命令类型识别接口。 |
| `get_command_str` | public | 声明命令枚举转字符串接口。 |
| `build_login_msg` | public | 声明登录消息构建接口。 |
| `build_logout_msg` | public | 声明登出消息构建接口。 |
| `build_text_msg` | public | 声明私聊消息构建接口。 |
| `build_broadcast_msg` | public | 声明广播消息构建接口。 |
| `build_group_msg` | public | 声明群组消息构建接口。 |
| `build_history_request` | public | 声明历史请求构建接口。 |
| `build_status_request` | public | 声明状态请求构建接口。 |
| `build_response_from_struct` | public | 声明结构化响应构建接口。 |
| `build_user_online_msg` | public | 声明上线通知构建接口。 |
| `build_user_offline_msg` | public | 声明下线通知构建接口。 |
| `build_system_notification` | public | 声明系统通知构建接口。 |
| `build_response_msg` | public | 声明响应消息构建接口。 |
| `build_success_msg` | public | 声明成功响应构建接口。 |
| `build_error_msg` | public | 声明错误响应构建接口。 |
| `validate_message` | public | 声明协议校验接口。 |
| `is_valid_msg_type` | public | 声明消息类型校验接口。 |
| `is_valid_username` | public | 声明用户名校验接口。 |
| `escape_field` | public | 声明字段转义接口。 |
| `unescape_field` | public | 声明字段反转义接口。 |
| `get_current_timestamp` | public | 声明协议时间戳生成接口。 |
| `parse_group_id` | public | 声明群组 ID 解析接口。 |
| `free_message` | public | 声明消息结构释放接口。 |
| `is_login_msg` | public | 声明登录消息判断接口。 |
| `is_logout_msg` | public | 声明登出消息判断接口。 |
| `is_private_msg` | public | 声明私聊消息判断接口。 |
| `is_broadcast_msg` | public | 声明广播消息判断接口。 |
| `is_group_msg` | public | 声明群组消息判断接口。 |
| `is_history_request` | public | 声明历史请求判断接口。 |
| `is_status_request` | public | 声明状态请求判断接口。 |
| `handle_command` | public | 声明已解析消息命令处理接口。 |
| `handle_raw_message` | public | 声明原始消息命令处理接口。 |

## server

### `src/server/server.c`
文件职责：服务端主程序入口，负责初始化存储、网络和事件循环。

| 函数 | 可见性 | 说明 |
| --- | --- | --- |
| `print_server_info` | static | 打印服务端启动信息和运行配置。 |
| `main` | public | 解析端口参数、启动服务端并运行事件循环。 |

### `src/server/server.h`
文件职责：声明服务端共享配置。

当前无函数，仅声明全局服务端配置变量。

## storage

### `src/storage/history_manager.c`
文件职责：当前为空文件，未定义函数。

当前无函数，仅作为历史记录管理模块占位。

### `src/storage/storage.c`
文件职责：初始化和清理存储子模块。

| 函数 | 可见性 | 说明 |
| --- | --- | --- |
| `storage_init` | public | 初始化存储层并加载默认测试用户。 |
| `storage_cleanup` | public | 清理存储层资源，当前为占位实现。 |

### `src/storage/storage.h`
文件职责：声明用户存储和存储生命周期接口。

| 函数 | 可见性 | 说明 |
| --- | --- | --- |
| `user_store_find_by_username` | public | 声明按用户名查找用户接口。 |
| `user_store_find_by_id` | public | 声明按用户 ID 查找用户接口。 |
| `user_store_add` | public | 声明添加用户接口。 |
| `user_store_count` | public | 声明用户数量查询接口。 |
| `user_store_authenticate` | public | 声明用户认证接口。 |
| `user_store_print_all` | public | 声明打印用户列表接口。 |
| `user_store_init_defaults` | public | 声明初始化默认用户接口。 |
| `storage_init` | public | 声明存储初始化接口。 |
| `storage_cleanup` | public | 声明存储清理接口。 |

### `src/storage/user_store.c`
文件职责：使用内存链表实现用户创建、查询、添加和认证。

| 函数 | 可见性 | 说明 |
| --- | --- | --- |
| `create_user` | static | 创建并初始化新的用户结构体。 |
| `user_store_find_by_username` | public | 按用户名在用户链表中查找用户。 |
| `user_store_find_by_id` | public | 按用户 ID 在用户链表中查找用户。 |
| `user_store_add` | public | 校验并添加新用户到用户链表。 |
| `user_store_authenticate` | public | 验证用户是否存在、激活且密码匹配。 |
| `user_store_init_defaults` | public | 添加默认演示用户。 |
| `user_store_count` | public | 返回当前用户数量。 |
| `user_store_print_all` | public | 打印所有用户信息用于调试。 |

## tui

### `src/tui/tui.h`
文件职责：声明跨 ncurses/PDCurses 的统一 TUI 接口。

| 函数 | 可见性 | 说明 |
| --- | --- | --- |
| `tui_init` | public | 声明 TUI 初始化接口。 |
| `tui_shutdown` | public | 声明 TUI 关闭和终端恢复接口。 |
| `tui_clear` | public | 声明清空消息区接口。 |
| `tui_set_status` | public | 声明状态栏更新接口。 |
| `tui_draw_system` | public | 声明系统消息输出接口。 |
| `tui_draw_error` | public | 声明错误消息输出接口。 |
| `tui_draw_help` | public | 声明快捷帮助输出接口。 |
| `tui_draw_message` | public | 声明普通聊天消息输出接口。 |
| `tui_read_input` | public | 声明底部输入栏读取接口。 |

### `src/tui/tui_ncurses.c`
文件职责：Linux/Unix 下基于 ncurses 实现状态栏、消息区和输入栏。

| 函数 | 可见性 | 说明 |
| --- | --- | --- |
| `destroy_windows` | static | 销毁当前 TUI 窗口对象。 |
| `draw_status_locked` | static | 在已加锁状态下重绘顶部状态栏。 |
| `create_layout_locked` | static | 在已加锁状态下创建三段式 TUI 布局。 |
| `write_text_locked` | static | 在消息区写入可包含换行的文本。 |
| `draw_line_locked` | static | 在消息区写入带前缀和颜色的单条消息。 |
| `tui_init` | public | 初始化 locale、ncurses、颜色和窗口布局。 |
| `tui_shutdown` | public | 销毁窗口并恢复终端。 |
| `tui_clear` | public | 清空消息区。 |
| `tui_set_status` | public | 更新顶部状态栏文本。 |
| `tui_draw_system` | public | 向消息区写入系统消息。 |
| `tui_draw_error` | public | 向消息区写入错误消息。 |
| `tui_draw_help` | public | 输出 TUI 快捷命令提示。 |
| `tui_draw_message` | public | 向消息区写入带发送者的聊天消息。 |
| `tui_read_input` | public | 在底部输入栏读取一行用户输入。 |

### `src/tui/tui_pdcurses.c`
文件职责：Windows 下基于 PDCurses 实现与 ncurses 版本一致的 TUI 接口。

| 函数 | 可见性 | 说明 |
| --- | --- | --- |
| `destroy_windows` | static | 销毁当前 TUI 窗口对象。 |
| `draw_status_locked` | static | 在已加锁状态下重绘顶部状态栏。 |
| `create_layout_locked` | static | 在已加锁状态下创建三段式 TUI 布局。 |
| `write_text_locked` | static | 在消息区写入可包含换行的文本。 |
| `draw_line_locked` | static | 在消息区写入带前缀和颜色的单条消息。 |
| `tui_init` | public | 初始化 locale、PDCurses、颜色和窗口布局。 |
| `tui_shutdown` | public | 销毁窗口并恢复终端。 |
| `tui_clear` | public | 清空消息区。 |
| `tui_set_status` | public | 更新顶部状态栏文本。 |
| `tui_draw_system` | public | 向消息区写入系统消息。 |
| `tui_draw_error` | public | 向消息区写入错误消息。 |
| `tui_draw_help` | public | 输出 TUI 快捷命令提示。 |
| `tui_draw_message` | public | 向消息区写入带发送者的聊天消息。 |
| `tui_read_input` | public | 在底部输入栏读取一行用户输入。 |

## utils

### `src/utils/logger.c`
文件职责：实现日志级别、日志文件和格式化日志输出。

| 函数 | 可见性 | 说明 |
| --- | --- | --- |
| `set_log_level` | public | 设置最低输出日志级别。 |
| `set_log_file` | public | 设置日志输出目标文件或控制台。 |
| `log_message` | public | 按级别格式化并写入日志消息。 |
| `log_client_event` | public | 记录客户端相关事件日志。 |
| `log_message_event` | public | 记录消息相关事件日志。 |

### `src/utils/safe_utils.c`
文件职责：实现安全字符串、内存和网络参数校验工具。

| 函数 | 可见性 | 说明 |
| --- | --- | --- |
| `safe_strcpy` | public | 安全复制字符串并保证目标缓冲区以空字符结尾。 |
| `safe_strcat` | public | 安全拼接字符串并避免越界写入。 |
| `safe_strcmp` | public | 在指定长度内安全比较两个字符串。 |
| `safe_malloc` | public | 分配指定大小内存并在失败时记录日志。 |
| `safe_calloc` | public | 分配并清零数组内存，带溢出和失败检查。 |
| `safe_free` | public | 释放指针并将原指针置空。 |
| `is_valid_ip` | public | 校验 IPv4 字符串格式和每段范围。 |
| `is_valid_port` | public | 校验端口是否处于 1 到 65535。 |

### `src/utils/time_utils.c`
文件职责：实现当前时间、时间戳解析和时间格式化工具。

| 函数 | 可见性 | 说明 |
| --- | --- | --- |
| `get_current_time` | public | 获取当前本地时间并格式化为默认字符串。 |
| `parse_timestamp` | public | 将 `YYYY-MM-DD HH:MM:SS` 字符串解析为 `time_t`。 |
| `format_time` | public | 将 `time_t` 按指定格式转换为新分配的字符串。 |

### `src/utils/utils.h`
文件职责：声明日志、安全字符串、时间、内存和网络校验工具接口。

| 函数 | 可见性 | 说明 |
| --- | --- | --- |
| `log_message` | public | 声明日志写入接口。 |
| `set_log_level` | public | 声明日志级别设置接口。 |
| `set_log_file` | public | 声明日志文件设置接口。 |
| `safe_strcpy` | public | 声明安全字符串复制接口。 |
| `safe_strcat` | public | 声明安全字符串拼接接口。 |
| `safe_strcmp` | public | 声明安全字符串比较接口。 |
| `get_current_time` | public | 声明当前时间字符串获取接口。 |
| `parse_timestamp` | public | 声明时间戳解析接口。 |
| `format_time` | public | 声明时间格式化接口。 |
| `safe_malloc` | public | 声明安全内存分配接口。 |
| `safe_calloc` | public | 声明安全数组内存分配接口。 |
| `safe_free` | public | 声明安全释放接口。 |
| `is_valid_ip` | public | 声明 IP 校验接口。 |
| `is_valid_port` | public | 声明端口校验接口。 |

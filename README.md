# ITit 聊天系统

ITit 是一个用 C 语言实现的 TCP 聊天 Demo。项目包含服务端、命令行客户端、文本协议解析器、连接/会话管理、用户存储、基础测试，以及 Linux/Windows 平台兼容层。

这个项目适合用于学习：

- TCP Socket 编程
- `select` 多路复用
- 简单应用层协议设计
- 模块化 C 项目组织
- Linux 与 Windows 原生网络 API 的差异处理

当前版本以课程实践和演示为主。登录、私聊、广播、状态查询等基础流程已经具备；群组消息和历史查询保留了命令入口，但服务端处理仍是占位实现。

## 功能状态

已实现：

- TCP 服务端监听与多客户端接入，默认端口 `8080`
- 基于 `select` 的事件循环
- 命令行客户端连接、登录、发送消息、广播、退出
- 默认用户认证
- 私聊消息转发
- 广播消息转发
- 在线用户和连接状态查询
- 文本协议构建、解析、转义和反转义
- 日志输出到 `server.log`
- Linux/Windows 平台兼容封装
- 工具、协议、连接、会话相关测试程序
- vendored ncurses/PDCurses TUI 构建，不需要把 curses 库安装到系统目录

仍在完善：

- 群组消息：客户端命令和协议构建已存在，服务端当前返回未实现提示
- 历史查询：存储接口已定义，命令处理当前返回未实现提示
- 持久化用户文件：当前启动时初始化内存中的默认用户
- 客户端体验：接收线程仍会打印原始报文和解析调试信息
- TUI 客户端：当前是 ncurses/PDCurses 构建验证和输入输出演示，尚未接入完整聊天客户端逻辑

## 目录结构

```text
.
├── CMakeLists.txt          # CMake 构建入口，推荐 Windows 使用
├── Makefile                # make 构建入口，推荐 Linux/MSYS2 使用
├── README.md
├── docs/
│   └── module_status.md    # 模块状态说明
├── src/
│   ├── server/             # 服务端入口
│   ├── client/             # 命令行客户端和 TUI 演示入口
│   ├── core/               # 连接管理、会话管理、消息路由
│   ├── models/             # User、Client、Message 等数据结构
│   ├── network/            # TCP 服务端/客户端、事件循环、客户端 I/O
│   ├── platform/           # Linux/Windows 兼容层
│   ├── protocol/           # 协议解析、消息构建、命令处理
│   ├── storage/            # 用户存储、历史消息接口
│   ├── tui/                # ncurses/PDCurses 适配层
│   └── utils/              # 日志、时间、安全字符串/内存工具
├── third_party/
│   ├── ncurses/            # Linux/Unix TUI 库源码，项目内构建
│   └── PDCurses/           # Windows 控制台 TUI 库源码，项目内构建
└── tests/                  # 模块测试
```

编译产物默认生成到 `bin/` 或 `build/bin/`，这些目录可以删除，也会被 `.gitignore` 忽略。

## 环境要求

Linux/macOS：

- `gcc`
- `make`
- POSIX 线程库，Makefile 会自动使用 `-pthread`
- `third_party/ncurses` 源码目录，首次构建 TUI 时会自动编译到 `third_party/ncurses/_install`

Windows 原生：

- 推荐安装 MSYS2 MinGW 工具链
- 推荐使用 `cmake`
- 不需要 WSL
- `third_party/PDCurses` 源码目录，首次构建 TUI 时会自动生成 `third_party/PDCurses/wincon/pdcurses.a`

可选工具：

- `clang-format`：用于 `make format`
- `cppcheck`：用于 `make analyze`
- `doxygen`：Makefile 预留了 `make docs`，但当前没有提供 `Doxyfile`

## 快速开始

Linux 或类 Unix 环境：

```bash
make
make run
```

另开一个终端启动客户端：

```bash
make run_client
```

客户端启动后输入：

```text
connect 127.0.0.1 8080
login alice alice123
```

## Windows 原生构建

Windows 不需要 WSL。推荐在 MSYS2 MinGW 环境中使用 CMake：

```bash
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
```

生成的程序位于：

```text
build/bin/server.exe
build/bin/client_app.exe
build/bin/client_tui.exe
```

运行示例：

```powershell
.\build\bin\server.exe
.\build\bin\client_app.exe
.\build\bin\client_tui.exe
```

也可以在 MSYS2/MinGW shell 中使用 Makefile：

```bash
mingw32-make
```

Makefile 在 Windows 下会自动生成 `.exe`，并链接 Winsock 库 `ws2_32`。

## 常用构建命令

```bash
make                  # 编译服务端、客户端和主要测试程序
make server           # 只编译服务端
make client_app       # 只编译客户端
make client           # client_app 的别名
make client_tui       # 编译实验性 TUI 客户端
make run_client_tui   # 运行实验性 TUI 客户端
make clean            # 清理编译产物
```

## 项目内 curses 构建

本项目不要求把 ncurses 或 PDCurses 安装到系统目录。源码放在 `third_party/` 后，构建脚本会按平台自动选择：

| 平台 | 使用库 | 生成位置 |
| --- | --- | --- |
| Linux/macOS/MSYS2 类 Unix | `third_party/ncurses` | `third_party/ncurses/_install/lib/libncursesw.a` |
| Windows MinGW | `third_party/PDCurses` | `third_party/PDCurses/wincon/pdcurses.a` |

Makefile：

```bash
make client_tui
make run_client_tui
```

CMake：

```bash
cmake -S . -B build
cmake --build build --target client_tui
```

TUI 相关源码：

- `src/tui/tui.h`：统一接口
- `src/tui/tui_ncurses.c`：Linux/Unix ncurses 实现
- `src/tui/tui_pdcurses.c`：Windows PDCurses 实现
- `src/client/client_commands.c`：CLI/TUI 共用命令执行层
- `src/client/tui_main.c`：实验性 TUI 客户端入口

主要测试目标：

```bash
make test_utils
make test_protocol
make test_builder
make test_connection
make test_session
make run_all_tests
```

## 运行服务端

默认端口是 `8080`：

```bash
make run
```

或直接运行：

```bash
./bin/server
```

指定端口：

```bash
make run_port PORT=9000
```

等价于：

```bash
./bin/server 9000
```

服务端启动后会输出端口、最大连接数和日志文件路径。按 `Ctrl+C` 停止服务端。

## 运行客户端

启动客户端：

```bash
make run_client
```

或直接运行：

```bash
./bin/client_app
```

客户端命令：

```text
connect [ip] [port]       连接服务器，默认 127.0.0.1:8080，别名 c
disconnect                断开连接，别名 d
login <user> <pass>       登录，别名 l
logout                    登出
to <user>                 设置聊天对象，之后直接输入内容即可发送
send <user> <msg>         发送私聊消息，别名 s
broadcast <msg>           发送广播消息，别名 b
group <group> <msg>       发送群组消息，当前服务端未完整实现，别名 g
history <target>          查询历史记录，当前服务端未完整实现，别名 h
status                    查询状态，别名 st
help                      查看帮助，别名 ?
quit                      退出客户端，别名 q
```

默认测试用户：

| 用户名 | 密码 |
| --- | --- |
| `admin` | `admin123` |
| `alice` | `alice123` |
| `bob` | `bob123` |
| `charlie` | `charlie123` |

双客户端私聊示例：

```text
# 终端 1
./bin/server

# 终端 2
./bin/client_app
c
l alice alice123

# 终端 3
./bin/client_app
c
l bob bob123
to alice
hello
```

## 协议格式

应用层消息使用管道符分隔字段，并以换行结尾：

```text
type|sender|receiver|timestamp|content\n
```

字段说明：

| 字段 | 含义 |
| --- | --- |
| `type` | 消息类型，如 `LOGIN`、`MSG`、`BROADCAST`、`STATUS` |
| `sender` | 发送者用户名或 `server` |
| `receiver` | 接收者用户名、`server`、`*` 或 `group:<name>` |
| `timestamp` | 时间戳，格式为 `YYYY-MM-DD HH:MM:SS` |
| `content` | 消息内容或响应内容 |

支持的消息类型：

| 类型 | 说明 |
| --- | --- |
| `LOGIN` | 登录请求，`content` 为密码 |
| `LOGOUT` | 登出请求 |
| `MSG` | 私聊消息 |
| `BROADCAST` | 广播消息 |
| `GROUP` | 群组消息，当前服务端未完整实现 |
| `HISTORY` | 历史查询，当前服务端未完整实现 |
| `STATUS` | 状态查询 |
| `OK` | 成功响应 |
| `ERROR` | 错误响应 |

特殊字符转义：

| 原始字符 | 转义后 |
| --- | --- |
| `|` | `\|` |
| `\` | `\\` |
| 换行 | `\n` |

示例：

```text
LOGIN|alice|server|2026-04-18 12:00:00|alice123
MSG|alice|bob|2026-04-18 12:00:05|hello
BROADCAST|alice|*|2026-04-18 12:00:10|hi everyone
OK|server|client|2026-04-18 12:00:11|0|Login successful
ERROR|server|client|2026-04-18 12:00:12|1003|User is offline
```

解析器要求至少包含 5 个字段。第 5 个字段之后如果还有未转义的 `|`，会被并入 `content`，因此 `OK`/`ERROR` 响应中的 `code|message` 可以被正常解析。

## 模块说明

- `src/server/server.c`：服务端入口和全局配置
- `src/client/`：命令行客户端和用户输入处理
- `src/tui/`：ncurses/PDCurses TUI 适配层
- `src/network/`：socket 初始化、连接、收发、事件循环
- `src/core/`：连接管理、会话认证、消息路由
- `src/protocol/`：协议构建、解析、命令分发
- `src/storage/`：默认用户和历史消息接口
- `src/platform/platform.h`：Linux/Windows 平台兼容封装
- `src/utils/`：日志、时间、安全工具函数

## 测试状态

运行主要测试：

```bash
make run_all_tests
```

当前已知状态：`test_protocol` 中有一个用例期望 `TOO|MANY|FIELDS|EXTRA|EXTRA|EXTRA` 校验失败，但当前解析器允许额外分隔符进入 `content` 字段，所以 `make run_all_tests` 会在该断言处停止。

这不是构建或平台兼容问题，而是测试预期和解析器行为尚未统一。需要后续决定：

- 保持当前解析器行为，并更新测试预期
- 或收紧协议校验逻辑，让额外字段触发失败

## 注意事项

- 当前密码以明文形式保存在内存结构中，只适合 Demo 或教学场景
- 当前服务端没有实现注册命令，用户来自启动时初始化的默认用户
- 客户端默认配置为 `127.0.0.1:8080`，但仍需要输入 `connect <ip> <port>` 建立连接
- 客户端调试日志默认写入 `client.log`，避免和交互提示混在一起
- 如果广播时没有其他已认证用户在线，服务端可能返回发送失败
- Windows 原生版本通过 Winsock 运行，不依赖 WSL

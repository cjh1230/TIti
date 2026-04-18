# ITit项目Makefile
# 编译器设置
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g -O2

ifeq ($(OS),Windows_NT)
	EXEEXT = .exe
	CFLAGS += -D_CRT_SECURE_NO_WARNINGS
	LDFLAGS =
	LDLIBS = -lws2_32
else
	EXEEXT =
	CFLAGS += -pthread -D_POSIX_C_SOURCE=200809L -D_GNU_SOURCE
	LDFLAGS = -pthread
	LDLIBS =
endif

# 目录设置
SRCDIR = src
PLATFORMDIR = $(SRCDIR)/platform
COREDIR = $(SRCDIR)/core
SERVERDIR = $(SRCDIR)/server
MODELSDIR = $(SRCDIR)/models
NETWORKDIR = $(SRCDIR)/network
PROTOCOLDIR = $(SRCDIR)/protocol
STORAGEDIR = $(SRCDIR)/storage
UTILSDIR = $(SRCDIR)/utils
CLIENTDIR = $(SRCDIR)/client
TUIDIR = $(SRCDIR)/tui
TESTDIR = tests
BINDIR = bin
THIRDPARTYDIR = third_party
NCURSESDIR = $(THIRDPARTYDIR)/ncurses
NCURSES_PREFIX = $(CURDIR)/$(NCURSESDIR)/_install
PDCURSESDIR = $(THIRDPARTYDIR)/PDCurses
PDCURSES_LIB = $(PDCURSESDIR)/wincon/pdcurses.a

ifeq ($(OS),Windows_NT)
	TUI_SOURCE = $(TUIDIR)/tui_pdcurses.c
	TUI_CFLAGS = -I$(PDCURSESDIR)
	TUI_LIBS = $(PDCURSES_LIB)
	TUI_DEPS = $(PDCURSES_LIB)
else
	TUI_SOURCE = $(TUIDIR)/tui_ncurses.c
	TUI_CFLAGS = -I$(NCURSES_PREFIX)/include/ncursesw -I$(NCURSES_PREFIX)/include
	TUI_LIBS = $(NCURSES_PREFIX)/lib/libncursesw.a
	TUI_DEPS = $(NCURSES_PREFIX)/lib/libncursesw.a
endif

CFLAGS += $(TUI_CFLAGS)

# 目标文件
TARGET = $(BINDIR)/server$(EXEEXT)
CLIENT_TARGET = $(BINDIR)/client_app$(EXEEXT)
CLIENT_TUI_TARGET = $(BINDIR)/client_tui$(EXEEXT)
TEST_UTILS_TARGET = $(BINDIR)/test_utils$(EXEEXT)
TEST_PROTOCOL_TARGET = $(BINDIR)/test_protocol$(EXEEXT)
TEST_BUILDER_TARGET = $(BINDIR)/test_builder$(EXEEXT)
TEST_CONNECTION_TARGET = $(BINDIR)/test_connection$(EXEEXT)
TEST_SESSION_TARGET = $(BINDIR)/test_session$(EXEEXT)
TEST_TARGETS = $(TEST_UTILS_TARGET) $(TEST_PROTOCOL_TARGET) $(TEST_BUILDER_TARGET) $(TEST_CONNECTION_TARGET) $(TEST_SESSION_TARGET)

# 源文件
CORE_SOURCES = $(wildcard $(COREDIR)/*.c)
MODEL_SOURCES = $(wildcard $(MODELSDIR)/*.c)
NETWORK_SOURCES = $(wildcard $(NETWORKDIR)/*.c)
PROTOCOL_SOURCES = $(wildcard $(PROTOCOLDIR)/*.c)
STORAGE_SOURCES = $(wildcard $(STORAGEDIR)/*.c)
UTILS_SOURCES = $(wildcard $(UTILSDIR)/*.c)
CLIENT_SOURCES = $(CLIENTDIR)/client.c $(CLIENTDIR)/client_commands.c $(CLIENTDIR)/main.c $(CLIENTDIR)/ui.c
TEST_SOURCES = $(wildcard $(TESTDIR)/*.c)

# 目标文件
CORE_OBJECTS = $(CORE_SOURCES:.c=.o)
MODEL_OBJECTS = $(MODEL_SOURCES:.c=.o)
NETWORK_OBJECTS = $(NETWORK_SOURCES:.c=.o)
PROTOCOL_OBJECTS = $(PROTOCOL_SOURCES:.c=.o)
STORAGE_OBJECTS = $(STORAGE_SOURCES:.c=.o)
UTILS_OBJECTS = $(UTILS_SOURCES:.c=.o)
CLIENT_OBJECTS = $(CLIENT_SOURCES:.c=.o)
TUI_OBJECT = $(TUI_SOURCE:.c=.o)
TEST_OBJECTS = $(TEST_SOURCES:.c=.o)

# 所有对象文件
ALL_OBJECTS = $(CORE_OBJECTS) $(MODEL_OBJECTS) $(NETWORK_OBJECTS) $(PROTOCOL_OBJECTS) $(STORAGE_OBJECTS) $(UTILS_OBJECTS)

# 头文件依赖
DEPS = $(PLATFORMDIR)/platform.h $(COREDIR)/core.h $(MODELSDIR)/models.h $(NETWORKDIR)/network.h $(PROTOCOLDIR)/protocol.h $(STORAGEDIR)/storage.h $(UTILSDIR)/utils.h $(SERVERDIR)/server.h

# 默认目标
all: server client_app client_tui test_utils test_protocol test_builder test_connection test_session

$(BINDIR):
	mkdir -p $(BINDIR)

server: $(TARGET)

client: client_app

client_app: $(CLIENT_TARGET)

client_tui: $(CLIENT_TUI_TARGET)

# 主服务器程序
$(TARGET): $(SERVERDIR)/server.c $(NETWORK_OBJECTS) $(CORE_OBJECTS) $(STORAGE_OBJECTS) $(PROTOCOL_OBJECTS) $(UTILS_OBJECTS) | $(BINDIR)
	$(CC) $(CFLAGS) -o $@ $< $(NETWORK_OBJECTS) $(CORE_OBJECTS) $(STORAGE_OBJECTS) $(PROTOCOL_OBJECTS) $(UTILS_OBJECTS) $(LDFLAGS) $(LDLIBS)

# 客户端程序
$(CLIENT_TARGET): $(CLIENTDIR)/main.c $(CLIENT_OBJECTS) $(PROTOCOLDIR)/builder.o $(PROTOCOLDIR)/parser.o $(UTILS_OBJECTS) $(NETWORKDIR)/tcp_client.o | $(BINDIR)
	$(CC) $(CFLAGS) -o $@ $(CLIENT_OBJECTS) $(PROTOCOLDIR)/builder.o $(PROTOCOLDIR)/parser.o $(UTILS_OBJECTS) $(NETWORKDIR)/tcp_client.o $(LDFLAGS) $(LDLIBS)

$(CLIENT_TUI_TARGET): $(CLIENTDIR)/tui_main.c $(CLIENTDIR)/client.o $(CLIENTDIR)/client_commands.o $(PROTOCOLDIR)/builder.o $(PROTOCOLDIR)/parser.o $(UTILS_OBJECTS) $(NETWORKDIR)/tcp_client.o $(TUI_OBJECT) $(TUI_DEPS) | $(BINDIR)
	$(CC) $(CFLAGS) -o $@ $< $(CLIENTDIR)/client.o $(CLIENTDIR)/client_commands.o $(PROTOCOLDIR)/builder.o $(PROTOCOLDIR)/parser.o $(UTILS_OBJECTS) $(NETWORKDIR)/tcp_client.o $(TUI_OBJECT) $(LDFLAGS) $(LDLIBS) $(TUI_LIBS)

$(TUI_OBJECT): $(TUI_DEPS)

$(NCURSES_PREFIX)/lib/libncursesw.a:
	cd $(NCURSESDIR) && ./configure --prefix="$(NCURSES_PREFIX)" --with-shared=no --without-debug --without-tests --without-ada --without-cxx --without-cxx-binding --enable-widec --with-default-terminfo-dir="$(NCURSES_PREFIX)/share/terminfo" --with-terminfo-dirs="$(NCURSES_PREFIX)/share/terminfo:/etc/terminfo:/lib/terminfo:/usr/share/terminfo"
	$(MAKE) -C $(NCURSESDIR)
	$(MAKE) -C $(NCURSESDIR) install

$(PDCURSES_LIB):
	$(MAKE) -C $(PDCURSESDIR)/wincon -f Makefile

# 测试程序
test_utils: $(TEST_UTILS_TARGET)

$(TEST_UTILS_TARGET): $(TESTDIR)/test_utils.c $(UTILS_OBJECTS) | $(BINDIR)
	$(CC) $(CFLAGS) -o $@ $< $(UTILS_OBJECTS) $(LDFLAGS) $(LDLIBS)

test_protocol: $(TEST_PROTOCOL_TARGET)

$(TEST_PROTOCOL_TARGET): $(TESTDIR)/test_protocol.c $(PROTOCOL_OBJECTS) $(CORE_OBJECTS) $(STORAGE_OBJECTS) $(UTILS_OBJECTS) | $(BINDIR)
	$(CC) $(CFLAGS) -o $@ $< $(PROTOCOL_OBJECTS) $(CORE_OBJECTS) $(STORAGE_OBJECTS) $(UTILS_OBJECTS) $(LDFLAGS) $(LDLIBS)

test_builder: $(TEST_BUILDER_TARGET)

$(TEST_BUILDER_TARGET): $(TESTDIR)/test_builder.c $(PROTOCOL_OBJECTS) $(CORE_OBJECTS) $(STORAGE_OBJECTS) $(UTILS_OBJECTS) | $(BINDIR)
	$(CC) $(CFLAGS) -o $@ $< $(PROTOCOL_OBJECTS) $(CORE_OBJECTS) $(STORAGE_OBJECTS) $(UTILS_OBJECTS) $(LDFLAGS) $(LDLIBS)

test_connection: $(TEST_CONNECTION_TARGET)

$(TEST_CONNECTION_TARGET): $(TESTDIR)/test_connection.c $(COREDIR)/connection_manager.o $(UTILS_OBJECTS) | $(BINDIR)
	$(CC) $(CFLAGS) -o $@ $< $(COREDIR)/connection_manager.o $(UTILS_OBJECTS) $(LDFLAGS) $(LDLIBS)

test_session: $(TEST_SESSION_TARGET)

$(TEST_SESSION_TARGET): $(TESTDIR)/test_session.c $(CORE_OBJECTS) $(STORAGE_OBJECTS) $(PROTOCOL_OBJECTS) $(UTILS_OBJECTS) | $(BINDIR)
	$(CC) $(CFLAGS) -o $@ $< $(CORE_OBJECTS) $(STORAGE_OBJECTS) $(PROTOCOL_OBJECTS) $(UTILS_OBJECTS) $(LDFLAGS) $(LDLIBS)

# 编译规则
%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c $< -o $@

# 头文件依赖
$(COREDIR)/connection_manager.o: $(COREDIR)/core.h
$(COREDIR)/session_manager.o: $(COREDIR)/core.h $(STORAGEDIR)/storage.h $(PROTOCOLDIR)/protocol.h
$(COREDIR)/message_router.o: $(COREDIR)/core.h $(PROTOCOLDIR)/protocol.h

$(STORAGEDIR)/user_store.o: $(STORAGEDIR)/storage.h $(UTILSDIR)/utils.h
$(STORAGEDIR)/history_manager.o: $(STORAGEDIR)/storage.h $(UTILSDIR)/utils.h

$(NETWORKDIR)/tcp_server.o: $(NETWORKDIR)/network.h $(UTILSDIR)/utils.h
$(NETWORKDIR)/event_loop.o: $(NETWORKDIR)/network.h $(UTILSDIR)/utils.h
$(NETWORKDIR)/client_handler.o: $(NETWORKDIR)/network.h $(UTILSDIR)/utils.h
$(NETWORKDIR)/tcp_client.o: $(NETWORKDIR)/network.h $(UTILSDIR)/utils.h

$(PROTOCOLDIR)/parser.o: $(PROTOCOLDIR)/protocol.h $(UTILSDIR)/utils.h
$(PROTOCOLDIR)/builder.o: $(PROTOCOLDIR)/protocol.h $(UTILSDIR)/utils.h

$(UTILSDIR)/logger.o: $(UTILSDIR)/utils.h
$(UTILSDIR)/safe_utils.o: $(UTILSDIR)/utils.h
$(UTILSDIR)/time_utils.o: $(UTILSDIR)/utils.h

$(CLIENTDIR)/client.o: $(CLIENTDIR)/client.h $(PROTOCOLDIR)/protocol.h $(UTILSDIR)/utils.h
$(CLIENTDIR)/client_commands.o: $(CLIENTDIR)/client_commands.h $(CLIENTDIR)/client.h
$(CLIENTDIR)/main.o: $(CLIENTDIR)/client.h $(CLIENTDIR)/ui.h
$(CLIENTDIR)/ui.o: $(CLIENTDIR)/ui.h $(CLIENTDIR)/client.h

# 清理
clean:
	rm -f $(TARGET) $(CLIENT_TARGET) $(CLIENT_TUI_TARGET) $(TEST_TARGETS) client_app client_app.exe client_tui client_tui.exe test_utils test_utils.exe test_protocol test_protocol.exe test_builder test_builder.exe test_connection test_connection.exe test_session test_session.exe \
	      $(ALL_OBJECTS) $(CLIENT_OBJECTS) $(TUI_OBJECT)
	rmdir $(BINDIR) 2>/dev/null || true

# 运行目标
run: server
	./$(TARGET)

run_client: client_app
	./$(CLIENT_TARGET)

run_client_tui: client_tui
	./$(CLIENT_TUI_TARGET)

run_port: server
	./$(TARGET) $(PORT)

run_test_connection: test_connection
	./$(TEST_CONNECTION_TARGET)

run_test_session: test_session
	./$(TEST_SESSION_TARGET)

run_all_tests: test_utils test_protocol test_builder test_connection test_session
	@echo "=== Running all tests ==="
	@echo "1. Testing utils..."
	@./$(TEST_UTILS_TARGET)
	@echo "\n2. Testing protocol parser..."
	@./$(TEST_PROTOCOL_TARGET)
	@echo "\n3. Testing protocol builder..."
	@./$(TEST_BUILDER_TARGET)
	@echo "\n4. Testing connection manager..."
	@./$(TEST_CONNECTION_TARGET)
	@echo "\n5. Testing session manager..."
	@./$(TEST_SESSION_TARGET)
	@echo "\n=== All tests completed ==="

# 代码格式化
format:
	find $(SRCDIR) $(TESTDIR) \( -name '*.c' -o -name '*.h' \) -print | xargs clang-format -i

# 静态分析
analyze:
	cppcheck --enable=all --std=c99 $(SRCDIR) $(TESTDIR)

# 生成文档
docs:
	doxygen Doxyfile

# 打包
dist:
	tar -czvf ITit-$(shell date +%Y%m%d).tar.gz --exclude='*.o' --exclude='bin' *

# 帮助信息
help:
	@echo "可用目标："
	@echo "  all              - 编译所有目标（默认）"
	@echo "  server           - 编译主服务器"
	@echo "  client_app       - 编译客户端程序"
	@echo "  client           - client_app 的别名"
	@echo "  client_tui       - 编译实验性TUI客户端"
	@echo "  test_connection  - 编译连接管理器测试"
	@echo "  test_session     - 编译会话管理器测试"
	@echo "  test_utils       - 编译工具模块测试"
	@echo "  test_protocol    - 编译协议解析器测试"
	@echo "  test_builder     - 编译协议构建器测试"
	@echo "  clean            - 清理所有编译文件"
	@echo "  format           - 格式化代码"
	@echo "  analyze          - 静态代码分析"
	@echo "  docs             - 生成文档"
	@echo "  dist             - 打包源代码"
	@echo "  run              - 运行服务器（默认端口8080）"
	@echo "  run_client       - 运行客户端"
	@echo "  run_client_tui   - 运行实验性TUI客户端"
	@echo "  run_port PORT=X  - 在端口X上运行服务器"
	@echo "  run_all_tests    - 按顺序运行所有测试"

# 声明伪目标
.PHONY: all server client client_app client_tui test_utils test_protocol test_builder test_connection test_session clean run run_client run_client_tui run_port run_test_connection run_test_session run_all_tests format analyze docs dist help

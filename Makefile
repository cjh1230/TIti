# ITit项目Makefile
# 编译器设置
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g -O2 -pthread -D_POSIX_C_SOURCE=200809L -D_GNU_SOURCE
LDFLAGS = -pthread

# 目录设置
SRCDIR = .
COREDIR = core
MODELSDIR = models
NETWORKDIR = network
PROTOCOLDIR = protocol
STORAGEDIR = storage
UTILSDIR = utils
CLIENTDIR = client
TESTDIR = tests

# 目标文件
TARGET = server
CLIENT_TARGET = client_app

# 源文件
CORE_SOURCES = $(wildcard $(COREDIR)/*.c)
MODEL_SOURCES = $(wildcard $(MODELSDIR)/*.c)
NETWORK_SOURCES = $(wildcard $(NETWORKDIR)/*.c)
PROTOCOL_SOURCES = $(wildcard $(PROTOCOLDIR)/*.c)
STORAGE_SOURCES = $(wildcard $(STORAGEDIR)/*.c)
UTILS_SOURCES = $(wildcard $(UTILSDIR)/*.c)
CLIENT_SOURCES = $(wildcard $(CLIENTDIR)/*.c)
TEST_SOURCES = $(wildcard $(TESTDIR)/*.c)

# 目标文件
CORE_OBJECTS = $(CORE_SOURCES:.c=.o)
MODEL_OBJECTS = $(MODEL_SOURCES:.c=.o)
NETWORK_OBJECTS = $(NETWORK_SOURCES:.c=.o)
PROTOCOL_OBJECTS = $(PROTOCOL_SOURCES:.c=.o)
STORAGE_OBJECTS = $(STORAGE_SOURCES:.c=.o)
UTILS_OBJECTS = $(UTILS_SOURCES:.c=.o)
CLIENT_OBJECTS = $(CLIENT_SOURCES:.c=.o)
TEST_OBJECTS = $(TEST_SOURCES:.c=.o)

# 所有对象文件
ALL_OBJECTS = $(CORE_OBJECTS) $(MODEL_OBJECTS) $(NETWORK_OBJECTS) $(PROTOCOL_OBJECTS) $(STORAGE_OBJECTS) $(UTILS_OBJECTS)

# 头文件依赖
DEPS = $(COREDIR)/core.h $(MODELSDIR)/models.h $(NETWORKDIR)/network.h $(PROTOCOLDIR)/protocol.h $(STORAGEDIR)/storage.h $(UTILSDIR)/utils.h server.h

# 默认目标
all: $(TARGET) $(CLIENT_TARGET) test_utils test_protocol test_builder test_connection test_session

# 主服务器程序
$(TARGET): server.c $(NETWORK_OBJECTS) $(CORE_OBJECTS) $(STORAGE_OBJECTS) $(PROTOCOL_OBJECTS) $(UTILS_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $< $(NETWORK_OBJECTS) $(CORE_OBJECTS) $(STORAGE_OBJECTS) $(PROTOCOL_OBJECTS) $(UTILS_OBJECTS) $(LDFLAGS)

# 客户端程序
$(CLIENT_TARGET): $(CLIENTDIR)/main.c $(CLIENT_OBJECTS) $(UTILS_OBJECTS) $(NETWORKDIR)/tcp_client.o
	$(CC) $(CFLAGS) -o $@ $(CLIENT_OBJECTS) $(PROTOCOLDIR)/builder.o $(PROTOCOLDIR)/parser.o $(UTILS_OBJECTS) $(NETWORKDIR)/tcp_client.o $(LDFLAGS)

# 测试程序
test_utils: $(TESTDIR)/test_utils.c $(UTILS_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $< $(UTILS_OBJECTS) $(LDFLAGS)

test_protocol: $(TESTDIR)/test_protocol.c $(PROTOCOL_OBJECTS) $(UTILS_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $< $(PROTOCOL_OBJECTS) $(CORE_OBJECTS) $(STORAGE_OBJECTS) $(UTILS_OBJECTS) $(LDFLAGS)

test_builder: $(TESTDIR)/test_builder.c $(PROTOCOL_OBJECTS) $(UTILS_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $< $(PROTOCOL_OBJECTS) $(CORE_OBJECTS) $(STORAGE_OBJECTS) $(UTILS_OBJECTS) $(LDFLAGS)

test_connection: $(TESTDIR)/test_connection.c $(COREDIR)/connection_manager.o $(UTILS_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $< $(COREDIR)/connection_manager.o $(UTILS_OBJECTS) $(LDFLAGS)

test_session: $(TESTDIR)/test_session.c
	$(CC) $(CFLAGS) -o $@ $< $(CORE_OBJECTS) $(STORAGE_OBJECTS) $(PROTOCOL_OBJECTS) $(UTILS_OBJECTS) $(LDFLAGS)

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
$(CLIENTDIR)/ui.o: $(CLIENTDIR)/ui.h $(CLIENTDIR)/client.h

# 清理
clean:
	rm -f $(TARGET) $(CLIENT_TARGET) test_utils test_protocol test_builder test_connection test_session \
	      $(ALL_OBJECTS) $(CLIENT_OBJECTS)

# 运行目标
run: $(TARGET)
	./$(TARGET)

run_client: $(CLIENT_TARGET)
	./$(CLIENT_TARGET)

run_port: $(TARGET)
	./$(TARGET) $(PORT)

run_test_connection: test_connection
	./test_connection

run_test_session: test_session
	./test_session

run_all_tests: test_utils test_protocol test_builder test_connection test_session
	@echo "=== Running all tests ==="
	@echo "1. Testing utils..."
	@./test_utils
	@echo "\n2. Testing protocol parser..."
	@./test_protocol
	@echo "\n3. Testing protocol builder..."
	@./test_builder
	@echo "\n4. Testing connection manager..."
	@./test_connection
	@echo "\n5. Testing session manager..."
	@./test_session
	@echo "\n=== All tests completed ==="

# 代码格式化
format:
	clang-format -i */*.c *.c

# 静态分析
analyze:
	cppcheck --enable=all --std=c99 */*.c *.c

# 生成文档
docs:
	doxygen Doxyfile

# 打包
dist:
	tar -czvf ITit-$(shell date +%Y%m%d).tar.gz --exclude='*.o' --exclude='server' --exclude='test_*' *

# 帮助信息
help:
	@echo "可用目标："
	@echo "  all              - 编译所有目标（默认）"
	@echo "  server           - 编译主服务器"
	@echo "  client           - 编译客户端程序"
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
	@echo "  run_port PORT=X  - 在端口X上运行服务器"
	@echo "  run_all_tests    - 按顺序运行所有测试"

# 声明伪目标
.PHONY: all clean run run_client run_port run_test_connection run_test_session run_all_tests format analyze docs dist help

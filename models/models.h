/**
 * @file models.h
 * @brief 聊天系统的数据模型定义
 * 
 * 本文件定义了聊天系统中使用的所有数据结构，包括用户、客户端、消息、群组、会话等。
 * 同时定义了相关的常量、宏和枚举类型，用于支持聊天系统的各种功能。
 */

#ifndef MODELS_H
#define MODELS_H

#include <time.h>

/* ================ 字符串长度限制宏定义 ================ */
/** 用户名最大长度 */
#define MAX_USERNAME_LEN    32
/** 密码最大长度 */
#define MAX_PASSWORD_LEN    32
/** 群组名最大长度 */
#define MAX_GROUPNAME_LEN   32
/** 消息内容最大长度 */
#define MAX_CONTENT_LEN     256
/** 文件名最大长度 */
#define MAX_FILENAME_LEN    64

/* ================ 消息类型宏定义 ================ */
/** 登录消息类型 */
#define MSG_TYPE_LOGIN      "LOGIN"
/** 登出消息类型 */
#define MSG_TYPE_LOGOUT     "LOGOUT"
/** 普通消息类型 */
#define MSG_TYPE_MSG        "MSG"
/** 广播消息类型 */
#define MSG_TYPE_BROADCAST  "BROADCAST"
/** 群组消息类型 */
#define MSG_TYPE_GROUP      "GROUP"
/** 历史记录消息类型 */
#define MSG_TYPE_HISTORY    "HISTORY"
/** 状态查询消息类型 */
#define MSG_TYPE_STATUS     "STATUS"
/** 错误消息类型 */
#define MSG_TYPE_ERROR      "ERROR"
/** 确认消息类型 */
#define MSG_TYPE_OK         "OK"

/* ================ 接收者标识宏定义 ================ */
/** 广播消息的接收者标识 */
#define RECEIVER_BROADCAST  "*"
/** 群组消息接收者前缀 */
#define RECEIVER_GROUP_PREFIX "group:"

/* ================ 客户端状态宏定义 ================ */
/** 客户端离线状态 */
#define CLIENT_OFFLINE      0
/** 客户端已连接状态 */
#define CLIENT_CONNECTED    1
/** 客户端已认证状态 */
#define CLIENT_AUTHENTICATED 2


/* ================ 数据结构定义 ================ */

/**
 * @brief 用户信息结构体
 * 
 * 存储系统中注册用户的基本信息
 */
typedef struct 
{
	char username[MAX_USERNAME_LEN];     /**< 用户名 */
	char password[MAX_PASSWORD_LEN];     /**< 密码 */
	int user_id;                         /**< 用户ID，系统内唯一标识 */
	time_t register_time;                /**< 注册时间 */
	int is_active;                       /**< 账户激活状态：0-未激活，1-激活 */
} User;

/**
 * @brief 客户端连接信息结构体
 * 
 * 存储已连接客户端的详细信息，包括网络连接状态和用户信息
 */
typedef struct Client
{
	int socket_fd;                       /**< 客户端套接字文件描述符 */
	int client_id;                       /**< 客户端ID，系统内唯一标识 */
	int user_id;                         /**< 关联的用户ID */
	char username[MAX_USERNAME_LEN];     /**< 已认证的用户名 */
	int status;                          /**< 客户端状态：0-离线，1-已连接，2-已认证 */
	char remote_ip[16];                  /**< 客户端IP地址 */
	int remote_port;                     /**< 客户端端口号 */
	time_t connect_time;                 /**< 连接建立时间 */
	time_t last_active_time;             /**< 最后活动时间 */
	struct Client* next;                 /**< 链表指针，用于连接下一个客户端节点 */
} Client;

/**
 * @brief 消息结构体
 * 
 * 存储聊天消息的完整信息，包括发送者、接收者、内容和时间戳
 */
typedef struct Message
{
	char msg_type[16];                   /**< 消息类型 */
	char sender[MAX_USERNAME_LEN];       /**< 发送者用户名 */
	char receiver[MAX_USERNAME_LEN];     /**< 接收者用户名 */
	char timestamp[32];                  /**< 时间戳 */
	char content[MAX_CONTENT_LEN];       /**< 消息内容 */
	int is_delivered;                    /**< 送达状态：0-未送达，1-已送达 */
	struct Message* next;                /**< 链表指针，用于连接下一条消息 */
} Message;

/**
 * @brief 群组结构体
 * 
 * 存储聊天群组的基本信息和成员列表
 */
typedef struct
{
	char group_name[MAX_GROUPNAME_LEN];  /**< 群组名称 */
	int group_id;                        /**< 群组ID，系统内唯一标识 */
	int member_ids[50];                  /**< 成员用户ID数组，最多支持50个成员 */
	int member_count;                    /**< 当前成员数量 */
	char created_by[MAX_USERNAME_LEN];   /**< 创建者用户名 */
	time_t create_time;                  /**< 创建时间 */
} Group;

/**
 * @brief 会话结构体
 * 
 * 存储用户之间的会话信息，包括私聊和群聊会话
 */
typedef struct
{
	int session_id;                      /**< 会话ID，系统内唯一标识 */
	int user1_id;                        /**< 会话参与者1的用户ID */
	int user2_id;                        /**< 会话参与者2的用户ID（群聊时为群组ID） */
	int is_group_chat;                   /**< 会话类型：0-私聊，1-群聊 */
	Message* messages_history;           /**< 指向历史消息链表的指针 */
	time_t last_active_time;             /**< 会话最后活动时间 */
} Session;

/**
 * @brief 服务器配置结构体
 * 
 * 存储服务器运行时的各项配置参数
 */
typedef struct
{
	int server_port;                     /**< 服务器监听端口 */
	int max_client_count;                /**< 最大客户端连接数 */
	int max_history_messages;            /**< 最大历史消息保存数量 */
	int timeout_seconds;                 /**< 客户端超时时间（秒） */
	char log_file[MAX_FILENAME_LEN];     /**< 日志文件路径 */
	int enable_encryption;               /**< 加密开关：0-不启用，1-启用 */
	int require_authentication;          /**< 认证要求：0-不需要，1-需要 */
} ServerConfig;

/**
 * @brief 命令类型枚举
 * 
 * 定义客户端可以向服务器发送的命令类型
 */
typedef enum {
	CMD_UNKNOWN = 0,                    /**< 未知命令 */
	CMD_LOGIN,                          /**< 登录命令 */
	CMD_LOGOUT,                         /**< 登出命令 */
	CMD_SEND_MSG,                       /**< 发送消息命令 */
	CMD_BROADCAST,                      /**< 广播消息命令 */
	CMD_JOIN_GROUP,                     /**< 加入群组命令 */
	CMD_LEAVE_GROUP,                    /**< 离开群组命令 */
	CMD_GET_HISTORY,                    /**< 获取历史记录命令 */
	CMD_GET_STATUS                      /**< 获取状态命令 */
} CommandType;

/**
 * @brief 服务器响应结构体
 * 
 * 存储服务器对客户端请求的响应信息
 */
typedef struct {
	int  code;                           /**< 响应代码 */
	char type[16];                       /**< 响应类型 */
	char message[128];                   /**< 响应消息内容 */
	char timestamp[20];                  /**< 响应时间戳 */
} Response;

/* 全局服务器配置变量声明 */
extern ServerConfig server_config;

#endif /* MODELS_H */

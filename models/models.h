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
#define MAX_USERNAME_LEN 32	 /**< 用户名最大长度 */
#define MAX_PASSWORD_LEN 32	 /**< 密码最大长度 */
#define MAX_GROUPNAME_LEN 32 /**< 群组名最大长度 */
#define MAX_CONTENT_LEN 256	 /**< 消息内容最大长度 */
#define MAX_FILENAME_LEN 64	 /**< 文件名最大长度 */
#define MAX_IP_LEN 16		 /**< IP地址最大长度 */

/* ================ 消息类型宏定义 ================ */
#define MSG_TYPE_LOGIN "LOGIN"		   /**< 登录消息类型 */
#define MSG_TYPE_LOGOUT "LOGOUT"	   /**< 登出消息类型 */
#define MSG_TYPE_MSG "MSG"			   /**< 普通消息类型 */
#define MSG_TYPE_BROADCAST "BROADCAST" /**< 广播消息类型 */
#define MSG_TYPE_GROUP "GROUP"		   /**< 群组消息类型 */
#define MSG_TYPE_HISTORY "HISTORY"	   /**< 历史记录消息类型 */
#define MSG_TYPE_STATUS "STATUS"	   /**< 状态查询消息类型 */
#define MSG_TYPE_ERROR "ERROR"		   /**< 错误消息类型 */
#define MSG_TYPE_OK "OK"			   /**< 确认消息类型 */

/* ================ 接收者标识宏定义 ================ */
#define RECEIVER_BROADCAST "*"		   /**< 广播消息的接收者标识 */
#define RECEIVER_GROUP_PREFIX "group:" /**< 群组消息接收者前缀 */
#define RECEIVER_ALL_GROUP "group:all" /**< 所有群组的广播标识 */

/* ================ 客户端状态宏定义 ================ */
/* 使用更明确的宏名以避免与客户端本地枚举冲突 */
#define CLIENT_STATUS_OFFLINE 0	   /**< 客户端离线状态 */
#define CLIENT_STATUS_CONNECTED 1	   /**< 客户端已连接状态 */
#define CLIENT_STATUS_AUTHENTICATED 2 /**< 客户端已认证状态 */

/* ================ 响应代码宏定义 ================ */
#define RESPONSE_SUCCESS 0		  /**< 操作成功 */
#define ERROR_AUTH_FAILED 1001	  /**< 认证失败 */
#define ERROR_USER_NOT_FOUND 1002 /**< 用户不存在 */
#define ERROR_USER_OFFLINE 1003	  /**< 用户离线 */
#define ERROR_GROUP_FULL 1004	  /**< 群组已满 */
#define ERROR_SERVER_ERROR 5000	  /**< 服务器内部错误 */

/* ================ 数据结构定义 ================ */

/**
 * @brief 用户信息结构体
 */
typedef struct User
{
	char username[MAX_USERNAME_LEN]; /**< 用户名 */
	char password[MAX_PASSWORD_LEN]; /**< 密码（Demo中可存储明文） */
	int user_id;					 /**< 用户ID，系统内唯一标识 */
	time_t register_time;			 /**< 注册时间戳 */
	int is_active;					 /**< 账户激活状态：1-激活，0-禁用 */
	struct User *next;				 /**< 链表指针（用于用户列表） */
} User;

/**
 * @brief 客户端连接信息结构体
 */
typedef struct Client
{
	int sockfd;						 /**< 客户端套接字描述符 */
	int client_id;					 /**< 客户端连接ID */
	int user_id;					 /**< 关联的用户ID（认证后设置） */
	char username[MAX_USERNAME_LEN]; /**< 已认证的用户名 */
	int status;						 /**< 客户端状态 */
	char remote_ip[MAX_IP_LEN];		 /**< 客户端IP地址 */
	int remote_port;				 /**< 客户端端口号 */
	time_t connect_time;			 /**< 连接建立时间 */
	time_t last_active;				 /**< 最后活动时间 */
	struct Client *next;			 /**< 链表指针 */
} Client;

/**
 * @brief 消息结构体
 */
typedef struct Message
{
	char type[16];					 /**< 消息类型 */
	char sender[MAX_USERNAME_LEN];	 /**< 发送者用户名 */
	char receiver[MAX_USERNAME_LEN]; /**< 接收者标识 */
	char timestamp[32];				 /**< 时间戳 "YYYY-MM-DD HH:MM:SS" */
	char content[MAX_CONTENT_LEN];	 /**< 消息内容 */
	int message_id;					 /**< 消息唯一ID（用于历史记录） */
	int is_delivered;				 /**< 送达状态：1-已送达，0-未送达 */
	struct Message *next;			 /**< 链表指针 */
	struct Message *prev;			 /**< 双向链表反向指针（可选） */
} Message;

/**
 * @brief 群组结构体
 */
typedef struct
{
	char group_name[MAX_GROUPNAME_LEN]; /**< 群组名称 */
	int group_id;						/**< 群组ID */
	int member_ids[50];					/**< 成员用户ID数组 */
	int member_count;					/**< 当前成员数量 */
	char created_by[MAX_USERNAME_LEN];	/**< 创建者用户名 */
	time_t create_time;					/**< 创建时间 */
} Group;

/**
 * @brief 服务器配置结构体
 */
typedef struct
{
	int server_port;				 /**< 服务器监听端口 */
	int max_clients;				 /**< 最大客户端连接数 */
	int max_history;				 /**< 最大历史消息保存数量 */
	int timeout_seconds;			 /**< 客户端超时时间（秒） */
	char log_path[MAX_FILENAME_LEN]; /**< 日志文件路径 */
	int require_auth;				 /**< 认证要求：1-需要，0-不需要 */
	int enable_encryption;			 /**< 加密开关：1-启用，0-不启用 */
} ServerConfig;

/**
 * @brief 服务器响应结构体
 */
typedef struct
{
	int code;			/**< 响应代码：0-成功，其他-错误码 */
	char type[16];		/**< 响应类型 */
	char message[128];	/**< 响应消息内容 */
	char timestamp[20]; /**< 响应时间戳 */
} Response;

/**
 * @brief 命令类型枚举
 */
typedef enum
{
	CMD_UNKNOWN = 0,
	CMD_LOGIN,
	CMD_LOGOUT,
	CMD_SEND_MSG,
	CMD_BROADCAST,
	CMD_JOIN_GROUP,
	CMD_LEAVE_GROUP,
	CMD_GET_HISTORY,
	CMD_GET_STATUS
} CommandType;

/* 全局服务器配置变量声明 */
extern ServerConfig server_config;

#endif /* MODELS_H */

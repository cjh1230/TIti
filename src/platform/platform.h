#ifndef PLATFORM_H
#define PLATFORM_H

#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef FD_SETSIZE
#define FD_SETSIZE 128
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

typedef SOCKET socket_t;
typedef int socket_len_t;
typedef int socket_io_result_t;

#define SOCKET_INVALID INVALID_SOCKET
#define SOCKET_ID(sockfd) ((long long)(uintptr_t)(sockfd))
#define SOCKET_IS_VALID(sockfd) ((sockfd) != INVALID_SOCKET)
#define SOCKET_IS_INVALID(sockfd) ((sockfd) == INVALID_SOCKET)

static inline int platform_socket_init(void)
{
	WSADATA wsa_data;
	return WSAStartup(MAKEWORD(2, 2), &wsa_data) == 0 ? 0 : -1;
}

static inline void platform_socket_cleanup(void)
{
	WSACleanup();
}

static inline int platform_socket_close(socket_t sockfd)
{
	return closesocket(sockfd);
}

static inline int platform_socket_set_nonblocking(socket_t sockfd)
{
	u_long mode = 1;
	return ioctlsocket(sockfd, FIONBIO, &mode) == 0 ? 0 : -1;
}

static inline int platform_socket_last_error(void)
{
	return WSAGetLastError();
}

static inline int platform_socket_would_block(void)
{
	int err = WSAGetLastError();
	return err == WSAEWOULDBLOCK;
}

static inline int platform_socket_in_progress(void)
{
	int err = WSAGetLastError();
	return err == WSAEWOULDBLOCK || err == WSAEINPROGRESS;
}

static inline int platform_socket_interrupted(void)
{
	return WSAGetLastError() == WSAEINTR;
}

static inline const char *platform_socket_error_message(void)
{
	static char buffer[256];
	DWORD err = (DWORD)WSAGetLastError();
	DWORD len = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
							   NULL, err, 0, buffer, sizeof(buffer), NULL);
	if (len == 0)
	{
		snprintf(buffer, sizeof(buffer), "Winsock error %lu", (unsigned long)err);
	}
	return buffer;
}

static inline const char *platform_socket_error_message_code(int error_code)
{
	static char buffer[256];
	DWORD len = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
							   NULL, (DWORD)error_code, 0, buffer, sizeof(buffer), NULL);
	if (len == 0)
	{
		snprintf(buffer, sizeof(buffer), "Winsock error %d", error_code);
	}
	return buffer;
}

static inline socket_io_result_t platform_socket_send(socket_t sockfd, const char *data, size_t len)
{
	if (len > INT_MAX)
	{
		len = INT_MAX;
	}
	return send(sockfd, data, (int)len, 0);
}

static inline socket_io_result_t platform_socket_recv(socket_t sockfd, char *buffer, size_t len)
{
	if (len > INT_MAX)
	{
		len = INT_MAX;
	}
	return recv(sockfd, buffer, (int)len, 0);
}

static inline int platform_select_nfds(socket_t max_fd)
{
	(void)max_fd;
	return 0;
}

static inline void platform_sleep_ms(unsigned int milliseconds)
{
	Sleep(milliseconds);
}

static inline char *platform_strdup(const char *src)
{
	size_t len;
	char *copy;

	if (!src)
	{
		return NULL;
	}

	len = strlen(src) + 1;
	copy = (char *)malloc(len);
	if (copy)
	{
		memcpy(copy, src, len);
	}
	return copy;
}

static inline struct tm *platform_localtime(const time_t *timep, struct tm *result)
{
	return localtime_s(result, timep) == 0 ? result : NULL;
}

typedef HANDLE platform_thread_t;
typedef DWORD platform_thread_return_t;
typedef DWORD(WINAPI *platform_thread_func_t)(LPVOID);
typedef SRWLOCK platform_mutex_t;

#define PLATFORM_THREAD_CALL WINAPI
#define PLATFORM_THREAD_NULL NULL
#define PLATFORM_THREAD_RETURN_VALUE 0
#define PLATFORM_MUTEX_INITIALIZER SRWLOCK_INIT

static inline int platform_mutex_init(platform_mutex_t *mutex)
{
	InitializeSRWLock(mutex);
	return 0;
}

static inline void platform_mutex_lock(platform_mutex_t *mutex)
{
	AcquireSRWLockExclusive(mutex);
}

static inline void platform_mutex_unlock(platform_mutex_t *mutex)
{
	ReleaseSRWLockExclusive(mutex);
}

static inline void platform_mutex_destroy(platform_mutex_t *mutex)
{
	(void)mutex;
}

static inline int platform_thread_create(platform_thread_t *thread, platform_thread_func_t func, void *arg)
{
	*thread = CreateThread(NULL, 0, func, arg, 0, NULL);
	return *thread ? 0 : -1;
}

static inline int platform_thread_is_valid(platform_thread_t thread)
{
	return thread != NULL;
}

static inline void platform_thread_join(platform_thread_t thread)
{
	if (thread)
	{
		WaitForSingleObject(thread, INFINITE);
		CloseHandle(thread);
	}
}

#else

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

typedef int socket_t;
typedef socklen_t socket_len_t;
typedef ssize_t socket_io_result_t;

#define SOCKET_INVALID (-1)
#define SOCKET_ID(sockfd) ((long long)(sockfd))
#define SOCKET_IS_VALID(sockfd) ((sockfd) >= 0)
#define SOCKET_IS_INVALID(sockfd) ((sockfd) < 0)

static inline int platform_socket_init(void)
{
	return 0;
}

static inline void platform_socket_cleanup(void)
{
}

static inline int platform_socket_close(socket_t sockfd)
{
	return close(sockfd);
}

static inline int platform_socket_set_nonblocking(socket_t sockfd)
{
	int flags = fcntl(sockfd, F_GETFL, 0);
	if (flags == -1)
	{
		return -1;
	}
	return fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
}

static inline int platform_socket_last_error(void)
{
	return errno;
}

static inline int platform_socket_would_block(void)
{
	return errno == EAGAIN || errno == EWOULDBLOCK;
}

static inline int platform_socket_in_progress(void)
{
	return errno == EINPROGRESS;
}

static inline int platform_socket_interrupted(void)
{
	return errno == EINTR;
}

static inline const char *platform_socket_error_message(void)
{
	return strerror(errno);
}

static inline const char *platform_socket_error_message_code(int error_code)
{
	return strerror(error_code);
}

static inline socket_io_result_t platform_socket_send(socket_t sockfd, const char *data, size_t len)
{
	return send(sockfd, data, len, 0);
}

static inline socket_io_result_t platform_socket_recv(socket_t sockfd, char *buffer, size_t len)
{
	return recv(sockfd, buffer, len, 0);
}

static inline int platform_select_nfds(socket_t max_fd)
{
	return max_fd + 1;
}

static inline void platform_sleep_ms(unsigned int milliseconds)
{
	usleep(milliseconds * 1000);
}

static inline char *platform_strdup(const char *src)
{
	size_t len;
	char *copy;

	if (!src)
	{
		return NULL;
	}

	len = strlen(src) + 1;
	copy = (char *)malloc(len);
	if (copy)
	{
		memcpy(copy, src, len);
	}
	return copy;
}

static inline struct tm *platform_localtime(const time_t *timep, struct tm *result)
{
	return localtime_r(timep, result);
}

typedef pthread_t platform_thread_t;
typedef void *platform_thread_return_t;
typedef void *(*platform_thread_func_t)(void *);
typedef pthread_mutex_t platform_mutex_t;

#define PLATFORM_THREAD_CALL
#define PLATFORM_THREAD_NULL 0
#define PLATFORM_THREAD_RETURN_VALUE NULL
#define PLATFORM_MUTEX_INITIALIZER PTHREAD_MUTEX_INITIALIZER

static inline int platform_mutex_init(platform_mutex_t *mutex)
{
	return pthread_mutex_init(mutex, NULL);
}

static inline void platform_mutex_lock(platform_mutex_t *mutex)
{
	pthread_mutex_lock(mutex);
}

static inline void platform_mutex_unlock(platform_mutex_t *mutex)
{
	pthread_mutex_unlock(mutex);
}

static inline void platform_mutex_destroy(platform_mutex_t *mutex)
{
	pthread_mutex_destroy(mutex);
}

static inline int platform_thread_create(platform_thread_t *thread, platform_thread_func_t func, void *arg)
{
	return pthread_create(thread, NULL, func, arg);
}

static inline int platform_thread_is_valid(platform_thread_t thread)
{
	return thread != 0;
}

static inline void platform_thread_join(platform_thread_t thread)
{
	pthread_join(thread, NULL);
}

#endif

static inline char *platform_strtok_r(char *str, const char *delim, char **saveptr)
{
	char *token = str ? str : *saveptr;
	char *end;

	if (!token)
	{
		return NULL;
	}

	token += strspn(token, delim);
	if (*token == '\0')
	{
		*saveptr = NULL;
		return NULL;
	}

	end = token + strcspn(token, delim);
	if (*end == '\0')
	{
		*saveptr = NULL;
	}
	else
	{
		*end = '\0';
		*saveptr = end + 1;
	}

	return token;
}

#endif /* PLATFORM_H */

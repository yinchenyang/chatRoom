#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>

#define MSG_MAX_LEN 100
#define NAME_LEN 20
#define PSWOD_LEN 20
#define PORT 9010
#define PID_NUM 2
#define CMD_SIZE 10

enum{LOG = 1,REGISTER,UNKNOW_ORDER,CREAT_FAIL,PTH_CRET_ERR,REG_SUCCESS,REG_FAIL,LOG_SUCCESS,LOG_FAIL,NAME_PST,WHISPER,CHAT_ALL,SHOW_ONLINE,QUIT_SYSTEM,USER_ERROR,NO_USER};

typedef struct msg_tag
{
	int action;
	char name[NAME_LEN];
	char password[PSWOD_LEN];
	char target[NAME_LEN];
	char msg[MSG_MAX_LEN];
}msg_t;

typedef struct msg_ret_tag
{
	int action;
	char name[NAME_LEN];
	char target[NAME_LEN];
	char msg[MSG_MAX_LEN];
}msg_ret_t;

char present_user[NAME_LEN] = {0};

void log_or_register()
{
	printf("\n\n\n\n\n");
	printf("     \t\t   ********************************************* \n");
	printf("     \t\t                                                 \n");
	printf("     \t\t   *             欢迎使用聊天室系统            * \n");
	printf("     \t\t   *                                           * \n");
	printf("     \t\t   *   --------                     --------   * \n");
	printf("     \t\t   *   | 登录 |                     | 注册 |   * \n");
	printf("     \t\t   *   --------                     --------   * \n");
	printf("     \t\t                                                 \n");
	printf("     \t\t   ********************************************* \n");
	printf("\n\n\n\t\t  输入log进行登录，输入register进行注册:\n\n");
}

void after_log()
{
	printf("\n\n\n\n\n");
	printf("     \t\t ******************************************************\n");
	printf("     \t\t                                                        \n");
	printf("     \t\t *                  欢迎使用聊天室系统                * \n");
	printf("     \t\t *   显示在线用户 : show all online user              * \n");
	printf("     \t\t *   悄悄话:  whisper to user_name message            * \n");
	printf("     \t\t *            (etc: whisper to tom hello)             * \n");
	printf("     \t\t *   群聊:    say to all message                      * \n");
	printf("     \t\t *            (etc: say to all hello)                 * \n");
	printf("     \t\t *   退出:    quit chat room system                   * \n");
	printf("     \t\t *                                                    * \n");
	printf("     \t\t                                   当前用户: %s   \n",present_user);
	printf("     \t\t **************************************************** \n");
}

int block_read(int fd,msg_t *data,int len)
{
	int read_size = 0;
	int n = 0;
	while(read_size < len)
	{
		n = read(fd,data + read_size,len - read_size);
		if(n < 0)
		{
			perror("read fail!\n");
			break;
		}
		else if(0 == n)
		{
			perror("read 0\n");
			break;
		}
		read_size += n;
	}
	return 0;
}

int block_write(int sockfd,msg_t *data,int file_len)
{
	int write_size = 0;
	int n = 0;
	while(write_size < file_len)
	{     
		n = write(sockfd,data + write_size,file_len - write_size);	
		write_size += n;
	}
	return 0;
}

int order_check(char *order)
{
	if(strcmp(order,"log") == 0)
	{
		return LOG;
	}

	else if(strcmp(order,"register") == 0)
	{
		return REGISTER;
	}

	else if(strcmp(order,"whisper") == 0)
	{
		return WHISPER;
	}
	else if(strcmp(order,"say") == 0)
	{
		return CHAT_ALL;
	}

	else if(strcmp(order,"show") == 0)
	{
		return SHOW_ONLINE;
	}

	else if(strcmp(order,"quit") == 0)
	{
		return QUIT_SYSTEM;
	}

	else
	{
		return UNKNOW_ORDER;
	}
}

int log_register(int sockfd)
{
	msg_t msg;
	msg_ret_t msg_ret;
	char cmd[CMD_SIZE] = {0};
	char name[NAME_LEN] = {0};
	char password[PSWOD_LEN] = {0};
	char password_ag[PSWOD_LEN] = {0};
	int check_return = 0;
	while(check_return != LOG_SUCCESS)
	{
		system("reset");
		system("reset");
		log_or_register();
		scanf("%s",cmd);
		check_return = order_check(cmd);
		if(check_return == LOG)
		{
			system("reset");
			printf("请输入帐号:\n");
			scanf("%s",name);

			printf("请输入密码:\n");
			scanf("%s",password);

			msg.action = LOG;

			strcpy(msg.name,name);
			strcpy(msg.password,password);

			//block_write(sockfd,&msg,sizeof(msg));
			write(sockfd,&msg,sizeof(msg));
			//block_read(sockfd,&msg,sizeof(msg));
			int num = read(sockfd,&msg_ret,sizeof(msg_ret));
			//printf("num = %d\n",num);
			//printf("msg_ret.action = %d\n",msg_ret.action); 
			//printf("LOG_SUCCESS = %d\n",LOG_SUCCESS);
			if(msg_ret.action != LOG_SUCCESS)
			{
				printf("帐号与密码不符，请确认后再登录!\n");
				sleep(2);
				continue;
			}
			else
			{
				strcpy(present_user,name);
				check_return = LOG_SUCCESS;
			}
		}

		else if(check_return == REGISTER)
		{
			system("reset");
			printf("请输入帐号:\n");
			scanf("%s",name);
			printf("请输入密码:\n");
			scanf("%s",password);
			printf("请再次输入密码:\n");
			scanf("%s",password_ag);
			if(strcmp(password,password_ag) != 0)
			{
				printf("两次输入的密码不一致!\n");
				sleep(2);
				continue;
			}
			else if(strcmp(password,password_ag) == 0)
			{
				msg.action = REGISTER;
				strcpy(msg.name,name);
				strcpy(msg.password,password);
				//block_write(sockfd,&msg,sizeof(msg));
				write(sockfd,&msg,sizeof(msg));
				//block_read(sockfd,&msg,sizeof(msg));
				read(sockfd,&msg_ret,sizeof(msg_ret));
				printf("msg_ret.action = %d\n",msg_ret.action);
				if(msg_ret.action == REG_SUCCESS)
				{
					printf("注册成功!\n");
					sleep(2);
					continue;
				}
				else if(msg_ret.action == REG_FAIL)
				{
					printf("注册失败!未知原因!\n");
					sleep(2);
					continue;
				}
				else if(msg_ret.action == NAME_PST)
				{
					printf("注册失败！用户已存在！\n");
					sleep(2);
					continue;
				}
			}	
		}

		else if(check_return == UNKNOW_ORDER)
		{
			system("reset");
			printf("未知命令!  请输入log登录或register注册:\n");
			sleep(2);
			continue;
		}
	}
}

void *thread_read(void *arg)
{
	int sockfd = *((int *)arg);
	//msg_ret_t msg_ret = {0};
	msg_t msg_test;
	while(1)
	{
		msg_ret_t msg_ret = {0};
		block_read(sockfd,&msg_test,sizeof(msg_test));
		printf("msg_ret.msg = %d\n",msg_test.msg);
		if(msg_ret.action == WHISPER)
		{
		    printf("%s\n",msg_test.msg);
			printf("%s\n",msg_test.name);
			printf("%s悄悄对你说:%s\n",(msg_test.name),(msg_test.msg));	    
		    printf("%s\n",msg_test.msg);
			printf("%s\n",msg_test.name);
		}
		else if(msg_ret.action == CHAT_ALL)
		{
			printf("%s对大家说:%s\n",msg_ret.name,msg_ret.msg);	    
		}
		else if(msg_ret.action == SHOW_ONLINE)
		{
			printf("%s\n",msg_ret.msg);
		}
		else if(msg_ret.action == USER_ERROR)
		{
			printf("该用户不存在或不在线\n");
		}

		else if(msg_ret.action == NO_USER)
		{
			printf("当前无其他在线用户!\n");
		}
		else if(msg_ret.action == QUIT_SYSTEM)
		{
			return NULL;
		}
	}	
	return NULL;
}

void *thread_write(void *arg)
{
	int sockfd = *((int *)arg);
	char cmd[CMD_SIZE] = {0};
	char to[CMD_SIZE] = {"to"};
	char target[NAME_LEN] = {0};
	char message[MSG_MAX_LEN] = {0};
	msg_t msg;
	system("reset");
	after_log();
	printf("请输入操作命令:\n");
	while(1)
	{
		scanf("%s",cmd);
		int check_return = order_check(cmd);
		if(check_return == WHISPER)
		{
			printf("请输入接收的用户:\n");
			scanf("%s",target);
			printf("请输入消息内容:\n");
			scanf("%s",message);
			msg.action = WHISPER;
			strcpy(msg.name,present_user);
			strcpy(msg.target,target);
			printf("msg.target = %s\n",msg.target);
			strcpy(msg.msg,message);
			write(sockfd,&msg,sizeof(msg));
			printf("你悄悄对%s说:%s\n",msg.target,msg.msg);
		}

		if(check_return == CHAT_ALL)
		{
			msg.action = CHAT_ALL;
			strcpy(msg.name,present_user);
			strcpy(msg.msg,message);
			write(sockfd,&msg,sizeof(msg));
			printf("你对大家说:%s\n",msg.msg);
		}

		else if(check_return == SHOW_ONLINE)
		{
			msg.action = SHOW_ONLINE;
			strcpy(msg.name,present_user);
			write(sockfd,&msg,sizeof(msg));
			printf("在线用户有:\n");
		}

		else if(check_return == QUIT_SYSTEM)
		{
			msg.action = QUIT_SYSTEM;
			strcpy(msg.name,present_user);
			write(sockfd,&msg,sizeof(msg));
			return NULL;
		}

		else if(check_return == UNKNOW_ORDER)
		{
			printf("未知命令!请检查后再输入!\n");
		}
	}	
	return NULL;
}

int main(int argc,char *argv[])
{
	system("reset");

	struct sockaddr_in server;
	memset(&server,0,sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons(PORT);
	server.sin_addr.s_addr = inet_addr("127.0.0.1");

	char cmd[CMD_SIZE] = {0};
	int log_return = -1;
	int sockfd = socket(PF_INET,SOCK_STREAM,0);
	connect(sockfd,(struct sockaddr *)&server,sizeof(server));

	log_return = log_register(sockfd);

	pthread_t pid_write = 0;
	pthread_t pid_read = 1;

	int ret = pthread_create(&pid_write,NULL,(void *)thread_write,&(sockfd));
	if(0 != ret)
	{
		perror(strerror(ret));
		return CREAT_FAIL;
	}

	ret = pthread_create(&pid_read,NULL,(void *)thread_read,&(sockfd));
	if(0 != ret)
	{
		perror(strerror(ret));
		return CREAT_FAIL;
	}

	pthread_join(pid_write,NULL);
	pthread_join(pid_read,NULL);
	close(sockfd);

	system("reset");
	printf("感谢您的使用,再见!\n");
	sleep(2);
	return 0;
}

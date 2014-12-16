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

enum{LOG = 1,REGISTER,UNKNOW_ORDER,CREAT_FAIL,PTH_CRET_ERR,REG_SUCCESS,REG_FAIL,LOG_SUCCESS,LOG_FAIL,NAME_PST,WHISPER,CHAT_ALL,SHOW_ONLINE,GOON};

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
	printf("     \t\t   *                                           * \n");
	printf("     \t\t   *             欢迎使用聊天室系统            * \n");
	printf("     \t\t   *                                           * \n");
	printf("     \t\t   *   --------                     --------   * \n");
	printf("     \t\t   *   | 登录 |                     | 注册 |   * \n");
	printf("     \t\t   *   --------                     --------   * \n");
	printf("     \t\t   *                                           * \n");
	printf("     \t\t   ********************************************* \n");
	printf("\n\n\n\t\t  输入log进行登录，输入register进行注册:\n\n");
}

void after_log()
{
	printf("\n\n\n\n\n");
	printf("     \t\t ******************************************************\n");
	printf("     \t\t                                                        \n");
	printf("     \t\t *                  欢迎使用聊天室系统                * \n");
	printf("     \t\t *   显示在线用户 : show                              * \n");
	printf("     \t\t *   悄悄话:  whisper to user_name message            * \n");
	printf("     \t\t *            (etc: whisper to tom hello)             * \n");
	printf("     \t\t *   群聊:    say to all message                      * \n");
	printf("     \t\t *            (etc: say to all hello)                 * \n");
	printf("     \t\t *   退出:    quit                                    * \n");
	printf("     \t\t *                                                    * \n");
	printf("     \t\t                                   当前用户: %s   \n",present_user);
	printf("     \t\t **************************************************** \n");
	//printf("\n\n\n\t\t  请输入命令:\n\n");
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

int log_register(int sockfd,int cmd)
{
	msg_t msg;
	msg_ret_t msg_ret;
	char name[NAME_LEN] = {0};
	char password[PSWOD_LEN] = {0};
	char password_ag[PSWOD_LEN] = {0};

	if(cmd == LOG)
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
		printf("num = %d\n",num);
		printf("msg_ret.action = %d\n",msg_ret.action); 
		printf("LOG_SUCCESS = %d\n",LOG_SUCCESS);
		if(msg_ret.action != LOG_SUCCESS)
		{
			printf("帐号与密码不符，请确认后再登录!\n");
			sleep(3);
			return LOG_FAIL;
		}
		else
		{
			strcpy(present_user,name);
			return GOON;
		}
	}

	else if(cmd == REGISTER)
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
			sleep(3);
			return REG_FAIL;
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
				sleep(3);
				return REG_SUCCESS;
			}
			else if(msg_ret.action == REG_FAIL)
			{
				printf("注册失败!未知原因!\n");
				sleep(3);
				return REG_FAIL;
			}
			else if(msg_ret.action == NAME_PST)
			{
				printf("注册失败！用户已存在！\n");
				sleep(3);
				return REG_FAIL;
			}
		}	
	}
    
	else if(cmd == UNKNOW_ORDER)
	{
		system("reset");
		printf("未知命令!  请输入log登录或register注册:\n");
		sleep(2);
	}
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

	else
	{
		return UNKNOW_ORDER;
	}
}

void *thread_read(void *arg)
{
	int sockfd = *((int *)arg);
	msg_ret_t msg_ret;
	while(1)
	{
		read(sockfd,&msg_ret,sizeof(msg_ret));
		if(msg_ret.action == WHISPER)
		{
			printf("%s对你说:%s\n",msg_ret.name,msg_ret.msg);	    
		}
		else if(msg_ret.action == CHAT_ALL)
		{
			printf("%s对大家说:%s\n",msg_ret.name,msg_ret.msg);	    
		}
		else if(msg_ret.action == SHOW_ONLINE)
		{
			printf("%s\n",msg_ret.msg);
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
	while(1)
	{
		after_log();
		printf("请输入操作命令:\n");
		scanf("%s %s %s %s",cmd,to,target,message);
		int check_return = order_check(cmd);
		if(check_return == WHISPER)
		{
			msg.action = WHISPER;
			strcpy(msg.name,present_user);
			strcpy(msg.target,target);
			strcpy(msg.msg,message);
			write(sockfd,&msg,sizeof(msg));
		}

		if(check_return == CHAT_ALL)
		{
			msg.action = CHAT_ALL;
			strcpy(msg.name,present_user);
			strcpy(msg.msg,message);
			write(sockfd,&msg,sizeof(msg));
		}

		else if(check_return == SHOW_ONLINE)
		{
			msg.action = SHOW_ONLINE;
			write(sockfd,&msg,sizeof(msg));
			printf("在线用户有:\n");
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

	while(log_return != GOON)
	{
		system("reset");
		system("reset");
		log_or_register();
		//int sockfd = socket(PF_INET,SOCK_STREAM,0);
		//connect(sockfd,(struct sockaddr *)&server,sizeof(server));
		scanf("%s",cmd);
		int check_return = order_check(cmd);
		log_return = log_register(sockfd,check_return);
		close(sockfd);
	}

	pthread_t pid_write = 0;
	pthread_t pid_read = 1;

	//int sockfd_write = socket(PF_INET,SOCK_STREAM,0);
	//connect(sockfd_write,(struct sockaddr *)&server,sizeof(server));

	int ret = pthread_create(&pid_write,NULL,(void *)thread_write,&(sockfd));
	if(0 != ret)
	{
		perror(strerror(ret));
		return CREAT_FAIL;
	}
	//close(sockfd_write);

	//int sockfd_read = socket(PF_INET,SOCK_STREAM,0);
	//connect(sockfd_read,(struct sockaddr *)&server,sizeof(server));

	ret = pthread_create(&pid_read,NULL,(void *)thread_read,&(sockfd));
	if(0 != ret)
	{
		perror(strerror(ret));
		return CREAT_FAIL;
	}
	//close(sockfd_write);

	pthread_join(pid_write,NULL);
	pthread_join(pid_read,NULL);

	return 0;
}

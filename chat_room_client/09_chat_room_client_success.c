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

//enum{LOG = 1,REGISTER,UNKNOW_ORDER,CREAT_FAIL,PTH_CRET_ERR,REG_SUCCESS,REG_FAIL,LOG_SUCCESS,LOG_FAIL,NAME_PST,WHISPER,CHAT_ALL,SHOW_ONLINE,QUIT_SYSTEM,USER_ERROR,NO_USER};
#define LOG          1
#define LOG_SUCC     2
#define LOG_FAIL     3
#define PSWD_ERR     4
#define RE_LOG   	 5
#define REGISTER 	 6
#define REG_SUCC 	 7
#define REG_FAIL 	 8
#define RE_REG   	 9
#define WHISPER  	 10
#define SEND_FAIL 	 11
#define USER_ERROR 	 12
#define OFFLINE   	 13
#define CHAT      	 14
#define SHOW_ONLINE  15
#define SHOW_ERR     16
#define QUIT_SYSTEM  17
#define QUIT_FAIL    18
#define CREATE_FAIL  19
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

int is_log = 0;
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
	printf("     \t\t *       显示在线用户 : show                          * \n");
	printf("     \t\t *          悄悄话:     whsp                          * \n");
	printf("     \t\t *             群聊:    chat                          * \n");
	printf("     \t\t *                退出: quit                          * \n");
	printf("     \t\t *                                                    * \n");
	printf("     \t\t                                   当前用户: %s   \n",present_user);
	printf("     \t\t **************************************************** \n");
}

int block_read(int fd,msg_ret_t *data,int len)
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

int block_write(int sockfd,msg_ret_t *data,int file_len)
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

void *thread_read(void *arg)
{
	int sockfd = *((int *)arg);
	//msg_ret_t msg_ret = {0};
	msg_t msg_ret;
	msg_t msg;
	while(1)
	{
		memset(&msg_ret,0,sizeof(msg_ret));
		int n = read(sockfd,&msg_ret,sizeof(msg_ret));
		printf("n = %d\n",n);
		if(msg_ret.action == LOG_SUCC)
		{
			printf("登录成功!\n");
			is_log = 1;
			sleep(2);
			system("reset");
			after_log();
			continue;
		}
		else if(msg_ret.action == LOG_FAIL)
		{
			printf("登录失败!\n");
			continue;
		}
		else if(msg_ret.action == PSWD_ERR)
		{
			printf("密码错误!\n");
			continue;
		}
		else if(msg_ret.action == RE_LOG)
		{
			printf("该用户已登录,请勿重复登录!\n");
			continue;
		}
		else if(msg_ret.action == REG_SUCC)
		{
			printf("注册成功!\n");
			continue;
		}
		else if(msg_ret.action == REG_FAIL)
		{
			printf("注册失败!\n");
			continue;
		}
		else if(msg_ret.action == RE_REG)
		{
			printf("注册失败,用户已存在!\n");
			continue;
		}
		else if(msg_ret.action == WHISPER)
		{
			printf("%s悄悄对你说:%s\n",msg_ret.name,msg_ret.msg);
			continue;
		}
		else if(msg_ret.action == SEND_FAIL)
		{
			printf("消息发送失败!\n");
			continue;
		}	
		else if(msg_ret.action == USER_ERROR)
		{
			printf("该用户不存在!\n");
			continue;
		}
		else if(msg_ret.action == OFFLINE)
		{
			printf("该用户离线!\n");
			continue;
		}
		else if(msg_ret.action == CHAT)
		{
			printf("%s对大家说:%s\n",msg_ret.name,msg_ret.msg);	
			continue;
		}
		else if(msg_ret.action == SHOW_ONLINE)
		{
			printf("在线用户:");
			printf("\t%s\n",msg_ret.msg);
			continue;
		}
		else if(msg_ret.action == SHOW_ERR)
		{
			printf("show 失败!\n");
			continue;
		}

		else if(msg_ret.action == QUIT_SYSTEM)
		{
			close(sockfd);
			return NULL;
		}
		else if(msg_ret.action == QUIT_FAIL)
		{
		    printf("退出失败!\n");
			continue;
		}
		else
		{
			printf("未识别操作!\n");
			continue;
		}
	}	
	return NULL;
}

void *thread_write(void *arg)
{
	int sockfd = *((int *)arg);
	char cmd[CMD_SIZE] = {0};
	char target[NAME_LEN] = {0};
	char message[MSG_MAX_LEN] = {0};
	char password[PSWOD_LEN] = {0};
	msg_t msg;
	system("reset");
	log_or_register();
	while(1)
	{
		printf("请输入操作命令:\n");
		scanf("%s",cmd);
		//if(is_log == 0)
		//{	
			if(strcmp(cmd,"log") == 0)
			{
				printf("请输入帐号:\n");
				scanf("%s",msg.name);
				printf("请输入密码:\n");
				scanf("%s",msg.password);
				msg.action = LOG;
				write(sockfd,&msg,sizeof(msg));
				//continue;
			}

			else if(strcmp(cmd,"register") == 0)
			{
				printf("请输入帐号:\n");
				scanf("%s",msg.name);
				printf("请输入密码:\n");
				scanf("%s",msg.password);
				printf("请重新输入密码:\n");
				scanf("%s",password);
				if(strcmp(password,msg.password) != 0)
				{
					printf("两次输入的密码不一致!\n");
				//	sleep(1);
				//	system("reset");
					continue;
				}	
				msg.action = REGISTER;
				write(sockfd,&msg,sizeof(msg));
				//system("reset");
				//log_or_register();
				//continue;
			}
			/*else
			{
				printf("未知命令!请检查后再输入!\n");
				sleep(1);
				system("reset");
				log_or_register();
				//continue;
			}*/
		//}
		//else
		//{	
			else if(strcmp(cmd,"whsp") == 0)
			{
				printf("请输入接收的用户:\n");
				scanf("%s",msg.target);
				printf("请输入消息内容:\n");
				getchar();
				gets(msg.msg);

				msg.action = WHISPER;
				strcpy(msg.name,present_user);
				write(sockfd,&msg,sizeof(msg));
				printf("你悄悄对%s说:%s\n",msg.target,msg.msg);
				//continue;
			}

			else if(strcmp(cmd,"chat") == 0)
			{
				msg.action = CHAT;
				strcpy(msg.name,present_user);
				printf("请输入消息内容:\n");
				getchar();
				gets(msg.msg);
				write(sockfd,&msg,sizeof(msg));
				printf("你对大家说:%s\n",msg.msg);
				//continue;
			}

			else if(strcmp(cmd,"show") == 0)
			{
				msg.action = SHOW_ONLINE;
				write(sockfd,&msg,sizeof(msg));
				//continue;
			}

			else if(strcmp(cmd,"quit") == 0)
			{
				msg.action = QUIT_SYSTEM;
				strcpy(msg.name,present_user);
				write(sockfd,&msg,sizeof(msg));
				close(sockfd);
				return NULL;
			}

			else
			{
				printf("未知命令!请检查后再输入!\n");
				continue;
			}
		//}
		return NULL;
	}
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

	pthread_t pid_write = 0;
	pthread_t pid_read = 1;

	int ret = pthread_create(&pid_write,NULL,(void *)thread_write,&(sockfd));
	if(0 != ret)
	{
		perror(strerror(ret));
		return CREATE_FAIL;
	}

	ret = pthread_create(&pid_read,NULL,(void *)thread_read,&(sockfd));
	if(0 != ret)
	{
		perror(strerror(ret));
		return CREATE_FAIL;
	}

	pthread_join(pid_write,NULL);
	pthread_join(pid_read,NULL);

	system("reset");
	printf("感谢您的使用,再见!\n");
	close(sockfd);
	sleep(2);
	return 0;
}

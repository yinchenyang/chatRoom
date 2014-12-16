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
#include <time.h>
#include <termios.h>

#define MSG_MAX_LEN 100
#define NAME_LEN 20
#define PSWOD_LEN 20
#define PORT 9010
#define PID_NUM 2
#define CMD_SIZE 10

#define ECHOFLAGS (ECHO|ECHOE|ECHOK|ECHONL)

enum{
	LOG    =      1,
	LOG_SUCC  =   2,
	LOG_FAIL   =  3,
	PSWD_ERR    = 4,
	RE_LOG   	 =5,
	REGISTER 	= 6,
	REG_SUCC 	 =7,
	REG_FAIL =	 8,
	RE_REG   =	 9,
	WHISPER  =	 10,
	SEND_FAIL =	 11,
	USER_ERROR = 12,
	OFFLINE   	= 13,
	CHAT      =	 14,
	SHOW_ONLINE=  15,
	SHOW_ERR    = 16,
	QUIT_SYSTEM = 17,
	QUIT_FAIL   = 18,
	CREATE_FAIL  =19,
	OTHER       = 20,
	YES         = 21,
	NO          = 22,
	SLIENCE     = 23,
	SAY         = 24,
	GETOUT      = 25,
	SLIENCE_FAIL= 26,
	SAY_FAIL    = 27,
	REQUIRE     = 28,
	REQUIRE_FAIL= 29,
	GETOUT_FAIL = 30,
	WHISPER_SUCC= 31,
	CHAT_SUCC   = 32
};

typedef struct msg_tag
{
	int action;
	char name[NAME_LEN];
	char password[PSWOD_LEN];
	char target[NAME_LEN];
	char msg[MSG_MAX_LEN];
}msg_t;

int is_log = NO;
int is_slience = NO;
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
	printf("\n\n\n\t\t      登录:log,注册:register,退出:quit         \n\n");
}

void after_log()
{
	printf("\n\n\n\n\n");
	printf("     \t\t ******************************************************\n");
	printf("     \t\t                                                        \n");
	printf("     \t\t *                  欢迎使用聊天室系统                * \n");
	printf("     \t\t *        注册 :         register                     * \n");
	printf("     \t\t *        显示在线用户 : show                         * \n");
	printf("     \t\t *        悄悄话 :       whsp                         * \n");
	printf("     \t\t *        群聊 :         chat                         * \n");
	printf("     \t\t *        传输文件 :     tran                         * \n");
	printf("     \t\t *        退出 :         quit                         * \n");
	printf("     \t\t *        返回页面:      table                        * \n");
	printf("     \t\t *                                                    * \n");
	if(strcmp(present_user,"admin") == 0)
	{
		printf("     \t\t *  亲爱的管理员,你还可以对普通用户进行以下操作:      * \n");
		printf("     \t\t *         禁言:   slience                            * \n");
		printf("     \t\t *         解禁:   say                                * \n");
		printf("     \t\t *         踢人:   getout                             * \n");
	}	
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

int set_disp_mode(int fd,int option)
{
    int err;
    struct termios term;
	if(tcgetattr(fd,&term) == -1)
	{
	    perror("cannot get the attribution of thr terminal");
		return 1;
	}	
	if(option)
	{
	    term.c_lflag |= ECHOFLAGS;
	}	
	else
	{
		term.c_lflag &=~ ECHOFLAGS;	
	}
	err = tcsetattr(fd,TCSAFLUSH,&term);
	if(err = -1 && err == EINTR)
	{
	    perror("cannot get the attribution of thr terminal");
		return 1;
	}
	return 0;
}

int getpasswd(char *passwd,int size)
{
	int c;
	int n = 0;
	
	do
	{
		c = getchar();
		if(c != '\n' | c != '\r')
		{
			passwd[n++] = c;
		}	
	}
	while(c != '\n' && c != '\r' && n < (size - 1));
	passwd[n] = '\0';
	return n;
}

void *thread_read(void *arg)
{
	int sockfd = *((int *)arg);
	msg_t msg;
	time_t ptime;
	
	while(1)
	{
		memset(&msg,0,sizeof(msg));
		int n = read(sockfd,&msg,sizeof(msg));
		switch(msg.action)
		{	
			case LOG_SUCC:
			{
				printf("\n登录成功!\n");
				is_log = YES;
				sleep(1);
				strcpy(present_user,msg.name);
				system("reset");
				after_log();
				break;
			}
			case LOG_FAIL:
			{
				printf("\n登录失败!\n");
				break;
			}
			case PSWD_ERR:
			{
				printf("\n密码错误!\n");
				break;
			}
			case RE_LOG:
			{
				printf("\n该用户已登录,请勿重复登录!\n");
				break;
			}
			case REG_SUCC:
			{
				printf("\n注册成功!\n");
				break;
			}
			case REG_FAIL:
			{
				printf("\n注册失败!\n");
				break;
			}
			case RE_REG:
			{
				printf("\n注册失败,用户已存在!\n");
				break;
			}
			case SLIENCE:
			{
				printf("你倒霉了,你惹到管理员了,现在你被禁言,不能发言了!\n");
				printf("你可以输入require请求管理员解禁!\n\n");
				is_slience = YES;
				break;
			}
			case SLIENCE_FAIL:
			{
				printf("禁言失败!\n");
				break;
			}
			case SAY:
			{
				if(is_slience == YES)
				{
					printf("管理员大大原谅你了,你可以自由发言啦!\n");
					is_slience = NO;
				}
				break;
			}
			case SAY_FAIL:
			{
				printf("解禁失败!\n");
				break;
			}
			case REQUIRE:
			{
				printf("%s请求解禁!\n\n",msg.name);
				break;
			}
			case REQUIRE_FAIL:
			{
				printf("请耐心等待!\n\n");
				break;
			}
			case GETOUT:
			{
				printf("\n\n\n你已被管理员踢出聊天室!\n\n");
				sleep(2);
				system("reset");
				is_log = NO;
				log_or_register();
				break;
			}
			case GETOUT_FAIL:
			{
				printf("踢出失败,请重试!\n\n");
				break;
			}
			case WHISPER:
			{
				time(&ptime);
				printf("\n%s",ctime(&ptime));
				if(strcmp(msg.msg,"SaD") == 0)
				{
				    printf("%s对你做了一张哭脸\n",msg.name);	
				}	
				else if(strcmp(msg.msg,"SmIlE") == 0)
				{
				    printf("%s对你做了一张笑脸\n\n",msg.name);	
				}	
				
				else
				{	
					printf("%s悄悄对你说:%s\n\n",msg.name,msg.msg);
				}
				break;
			}
			case WHISPER_SUCC:
			{
				time(&ptime);
				printf("\n%s",ctime(&ptime));
				if(strcmp(msg.msg,"SaD") == 0)
				{
				    printf("你对%s做了一张哭脸\n",msg.target);	
				}	
				else if(strcmp(msg.msg,"SmIlE") == 0)
				{
				    printf("你对%s做了一张笑脸\n",msg.target);	
				}	
				else
				{	
					printf("你悄悄对%s说:%s\n\n",msg.target,msg.msg);
				}
				break;
			}
			case SEND_FAIL:
			{
				printf("消息发送失败!\n");
				break;
			}	
			case USER_ERROR:
			{
				printf("\n该用户不存在!\n");
				break;
			}
			case OFFLINE:
			{
				printf("该用户离线!\n");
				break;
			}
			case CHAT:
			{
				time(&ptime);
				printf("\n%s",ctime(&ptime));
				if(strcmp(msg.msg,"SaD") == 0)
				{
				    printf("%s对大家做了一张哭脸\n",msg.name);	
				}	
				else if(strcmp(msg.msg,"SmIlE") == 0)
				{
				    printf("%s对大家做了一张笑脸\n",msg.name);	
				}	
				else
				{	
					printf("%s对大家说:%s\n\n",msg.name,msg.msg);	
				}
				break;
			}
			case CHAT_SUCC:
			{
				time(&ptime);
				printf("\n%s",ctime(&ptime));
				if(strcmp(msg.msg,"SaD") == 0)
				{
				    printf("你对大家做了一张哭脸\n\n");	
				}	
				else if(strcmp(msg.msg,"SmIlE") == 0)
				{
				    printf("你对大家做了一张笑脸\n\n");	
				}	
				else
				{
					printf("你对大家说:%s\n\n",msg.msg);	
				}
				break;
			}
			case SHOW_ONLINE:
			{
				printf("在线用户:");
				printf("\t%s\n",msg.msg);
				break;
			}
			case SHOW_ERR:
			{
				printf("show 失败!\n");
				break;
			}

			case QUIT_SYSTEM:
			{
				close(sockfd);
				return NULL;
			}
			case QUIT_FAIL:
			{
				printf("退出失败!\n");
				break;
			}
			default:
			{
				printf("未识别操作!\n");
				break;
			}
		}
	}	
	return NULL;
}

void *thread_write(void *arg)
{
	char *p;
	int sockfd = *((int *)arg);
	char cmd[CMD_SIZE] = {0};
	char target[NAME_LEN] = {0};
	char message[MSG_MAX_LEN] = {0};
	char password[PSWOD_LEN] = {0};
	char passwd[PSWOD_LEN] = {0};
	msg_t msg;
	time_t ptime;
	
	system("reset");
	log_or_register();
	
	while(1)
	{
		sleep(1);
		scanf("%s",cmd);
		
		if(strcmp(cmd,"log") == 0)
		{
			if(is_log == NO)
			{	
				printf("请输入帐号:\n");
				scanf("%s",msg.name);
				getchar();
				printf("请输入密码:\n");
				set_disp_mode(STDIN_FILENO,0);
				getpasswd(password,sizeof(password));
				p = password;
				while(*p != '\n')
				{
				    p++;	
				}	
				*p = '\0';
				set_disp_mode(STDIN_FILENO,1);
				msg.action = LOG;
				strcpy(msg.password,password);
				write(sockfd,&msg,sizeof(msg));
				msg.action = OTHER;
		    }
			else
			{
			    printf("您已登录!\n");	
			}	
			continue;
		}

		else if(strcmp(cmd,"register") == 0)
		{
			printf("请输入帐号:\n");
			scanf("%s",msg.name);
			getchar();
			printf("请输入密码:\n");
			set_disp_mode(STDIN_FILENO,0);
			getpasswd(password,sizeof(password));
			p = password;
			while(*p != '\n')
			{
				p++;	
			}	
			*p = '\0';
			printf("\n请重新输入密码:\n");
			getpasswd(passwd,sizeof(passwd));
			p = passwd;
			while(*p != '\n')
			{
			    p++;	
			}	
			*p = '\0';
			set_disp_mode(STDIN_FILENO,1);
			if(strcmp(password,passwd) != 0)
			{
				printf("\n两次输入的密码不一致!\n");
				continue;
			}	
			msg.action = REGISTER;
			strcpy(msg.password,passwd);
			write(sockfd,&msg,sizeof(msg));
			msg.action = OTHER;
			continue;
		}

		else if(strcmp(cmd,"whsp") == 0)
		{
			if(is_log == YES && is_slience == NO)
			{	
				printf("请输入接收的用户:\n");
				scanf("%s",msg.target);
				printf("请输入消息内容:\n");
				getchar();
				gets(msg.msg);
				if(strcmp(msg.msg,":)") == 0)
				{
					strcpy(msg.msg,"SmIlE");	
				}	
				else if(strcmp(msg.msg,":(") == 0)
				{
					strcpy(msg.msg,"SaD");	
                }
				msg.action = WHISPER;
				strcpy(msg.name,present_user);
				write(sockfd,&msg,sizeof(msg));
			}
			
			else if(is_log == NO)
			{
			    printf("请先登录!\n");	
			}
			
			else if(is_log == YES && is_slience == YES)
			{
			    printf("你已被禁言!\n");	
				printf("你可以输入require请求管理员解禁!\n\n");
			}	
			continue;
		}

		else if(strcmp(cmd,"chat") == 0)
		{
			if(is_log == YES && is_slience == NO)
			{	
				strcpy(msg.name,present_user);
				printf("请输入消息内容:\n");
				getchar();
				gets(msg.msg);
				
				if(strcmp(msg.msg,":)") == 0)
				{
					strcpy(msg.msg,"SmIlE");	
				}	
				
				else if(strcmp(msg.msg,":(") == 0)
				{
					strcpy(msg.msg,"SaD");	
				}	
				
				msg.action = CHAT;
				write(sockfd,&msg,sizeof(msg));
			}
			
			else if(is_log == NO)
			{
				printf("请先登录!\n");	
			}
			else if(is_log == YES && is_slience == YES)
			{
			    printf("你已被禁言!\n");	
				printf("你可以输入require请求管理员解禁!\n\n");
			}	
			continue;
		}

		else if(strcmp(cmd,"show") == 0)
		{
			if(is_log == YES)
			{	
				msg.action = SHOW_ONLINE;
				write(sockfd,&msg,sizeof(msg));
				msg.action = OTHER;
	    	}
			else
			{
				printf("请先登录!\n");
			}
		}

		else if(strcmp(cmd,"quit") == 0)
		{
			msg.action = QUIT_SYSTEM;
			strcpy(msg.name,present_user);
			write(sockfd,&msg,sizeof(msg));
			close(sockfd);
			return NULL;
		}
		
		else if(strcmp(cmd,"table") == 0)
		{
			system("reset");
			after_log();
			continue;
		}
		
		else if(strcmp(cmd,"slience") == 0)
		{
			if(strcmp(present_user,"admin") == 0)
			{	
				msg.action = SLIENCE;
				printf("请输入想要禁言的用户:\n");
				scanf("%s",msg.target);
				if(strcmp(msg.target,"admin") == 0)
				{
				    printf("亲爱的,要对自己好一点啊!\n");	
				}	
				else
				{	
					write(sockfd,&msg,sizeof(msg));
			    }
			}
			else
			{
				printf("你不是管理员,没有此项操作权利!\n");
		    }
			continue;
		}

		else if(strcmp(cmd,"require") == 0)
		{
			if(is_slience == YES && is_log == YES)
			{	
				msg.action = REQUIRE;
				strcpy(msg.target,"admin");
				write(sockfd,&msg,sizeof(msg));
				printf("管理员大大我错了,放我出去...\n\n");
			}
			else if(is_slience == NO)
			{
				printf("莫非你干了坏事管理员大大没有发现,你还没被禁言纳!\n\n");
			}
			else if(is_log == NO)
			{
			    printf("亲,你还没有登录哦!\n");	
			}	
			continue;
		}
		
		else if(strcmp(cmd,"say") == 0)
		{
			if(strcmp(present_user,"admin") == 0)
			{	
				msg.action = SAY;
				printf("请输入要解禁的用户:\n");
				scanf("%s",msg.target);
				write(sockfd,&msg,sizeof(msg));
			}
			else
			{
				printf("你不是管理员,没有此项操作权利!\n");
		    }
			continue;
		}
		
		else if(strcmp(cmd,"getout") == 0)
		{
			if(strcmp(present_user,"admin") == 0)
			{	
				printf("请输入将要被踢出的用户:\n");
				scanf("%s",msg.target);
				msg.action = GETOUT;
				write(sockfd,&msg,sizeof(msg));
			}
			else
			{
				printf("你不是管理员,没有此项操作权利!\n");
		    }
			continue;
		}

		else
		{
			printf("未知命令!请检查后再输入!\n");
			continue;
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
	int n = connect(sockfd,(struct sockaddr *)&server,sizeof(server));
    if(n < 0)
	{
	    printf("未连接至服务器!\n");
		close(sockfd);
		return 0;
	}

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
	return 0;
}

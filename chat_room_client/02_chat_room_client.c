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

#define MSG_MAX_LEN 1000
#define NAME_LEN 20
#define PSWOD_LEN 20
#define PORT 9010
#define PID_NUM 2
#define CMD_SIZE 10

enum{LOG = 1,REGISTER,UNKNOW_ORDER,CREAT_FAIL,LOG_YES,REG_SUCCESS};

typedef struct msg_tag
{
    int action;
    char name[NAME_LEN];
    char password[PSWOD_LEN];
    char msg[MSG_MAX_LEN];
}msg_t;

int server_return = 0;

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

void *thread_write(void *arg)
{
    msg_t msg;
    char name[NAME_LEN] = {0};
    char password[PSWOD_LEN] = {0};
    char password_ag[PSWOD_LEN] = {0};
    char cmd[CMD_SIZE] = {0};
    int order_check_return = 0;
    int sockfd = *((int *)arg);
    while(1)
    {
        system("reset");
        system("reset");
        log_or_register();
        scanf("%s",cmd);
        order_check_return = order_check(cmd);
        if(order_check_return == LOG)
        {
	    system("reset");
	    printf("请输入帐号:\n");
	    scanf("%s",name);
	    
	    printf("请输入密码:\n");
	    scanf("%s",password);
	    
	    msg.action = LOG;
	    strcpy(msg.name,name);
	    strcpy(msg.password,password);
	    
	    block_write(sockfd,&msg,sizeof(&msg));
	    if(server_return != LOG_YES)
	    {
	        printf("帐号与密码不符，请确认后再登录!\n");
		sleep(2);
		continue;
	    }
	    else
	    {
	        break;
	    }	
        }

        else if(order_check_return == REGISTER)
        {
	    while(1)
	    {   
	        system("reset");
                printf("请输入帐号:\n");
	        scanf("%s",name);
	        printf("请输入密码:\n");
	    	scanf("%s",password);
	    	printf("请再次出入密码\n");
	    	scanf("%s",password_ag);
	    	if(strcmp(password,password_ag) != 0)
	    	{
	            printf("两次输入的密码不一致!\n");
		    continue;
	    	}
		else
		{
	    	    msg.action = REGISTER;
	            strcpy(msg.name,name);
	    	    strcpy(msg.password,password);
		    block_write(sockfd,&msg,sizeof(msg));
		    block_read(sockfd,&msg,sizeof(msg));
		    if(msg.action == REG_SUCCESS)
		    {
		        printf("注册成功!\n");
			sleep(2);
			break;
		    }
		    else
		    {
		        printf("注册失败!\n");
			sleep(2);
			continue;
		    }
		}
	    }	
        }

        else if(order_check_return == UNKNOW_ORDER)
        {
            system("reset");
	    printf("未知命令!  请输入log登录或register注册:\n");
	    sleep(2);
        }
    }
    printf("after log\n");
    return NULL;
}

void *thread_read(void *arg)
{
    int sockfd = *((int *)arg);
    printf("in read: %d\n",sockfd);
    return NULL;
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

    else
    {
        return UNKNOW_ORDER;
    }
}

int main(int argc,char *argv[])
{
    system("reset");
    
    int sockfd = socket(PF_INET,SOCK_STREAM,0);

    struct sockaddr_in server;
    memset(&server,0,sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    connect(sockfd,(struct sockaddr *)&server,sizeof(server));
    
    pthread_t pid[PID_NUM] = {0};
    pthread_t pid_write = 0;
    pthread_t pid_read = 1;
    
    int ret = pthread_create(&pid_write,NULL,thread_write,&(sockfd));
    if(0 != ret)
    {
        perror(strerror(ret));
	return CREAT_FAIL;
    }

    ret = pthread_create(&pid_read,NULL,thread_read,&(sockfd));
    if(0 != ret)
    {
        perror(strerror(ret));
	return CREAT_FAIL;
    }
    
    pthread_join(pid_write,NULL);
    pthread_join(pid_read,NULL);

    return 0;
}

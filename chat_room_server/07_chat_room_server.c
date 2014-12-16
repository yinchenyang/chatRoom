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
#include <sqlite3.h>

#define MSG_MAX_LEN 100
#define NAME_LEN 20
#define PSWOD_LEN 20
#define PORT 9010
#define PID_NUM 2
#define CMD_SIZE 10
#define QUEUE_LEN 10

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
#define PTH_CRET_ERR 19
#define OTHER        20

typedef struct msg_tag
{
	int action;
	char name[NAME_LEN];
	char password[PSWOD_LEN];
	char target[NAME_LEN];
	char msg[MSG_MAX_LEN];
}msg_t;

char str_return[100] = {0};

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

int block_write(int fd,msg_t *data,int len)
{
	int write_size = 0;
	int n = 0;
	while(write_size < len)
	{
		n = write(fd,data + write_size,len - write_size);
		if(n < 0)
		{
			perror("write fail!\n");
			break;
		}
		else if(0 == n)
		{
			perror("write 0\n");
			break;
		}
		write_size += n;
	}
	return 0;
}

static int call_back(void * not_used,int argc,char **argv,char **col_name)
{
    int i = 0;
	int count = 0;
	for(i = 0;i < argc;i++)
	{
	    count = strlen(str_return);
		sprintf(str_return + count,"%s\t",argv[i]?argv[i]:"NULL");
	}	
	return 0;
}

void *read_write_pthread(void *arg)
{
	printf("new listen pthread create\n");
	msg_t msg;
	msg_t msg_ret;
	int connectfd = *((int *)arg);
	sqlite3 *db = NULL;
	char *err_msg = NULL;
    int tgt_confd = 0;
	
	int rc = sqlite3_open("chat_room.db",&db);
	if(rc != SQLITE_OK)
	{
		fprintf(stderr,"open database failed %s\n",sqlite3_errmsg(db));
	}
	
	char sql_create_user_info[256] = {0};
	sprintf(sql_create_user_info,"create table user_info(name TEXT,password TEXT,primary key(name));");
	sqlite3_exec(db,sql_create_user_info,NULL,0,&err_msg);
	char sql_create_log_info[256] = {0};
	sprintf(sql_create_log_info,"create table log_info(name TEXT,connectfd INTEGER,primary key(name));");
	int n = sqlite3_exec(db,sql_create_log_info,NULL,0,&err_msg);
	
	while(1)
	{
		memset(&msg,0,sizeof(msg));
		int num = read(connectfd,&msg,sizeof(msg));
		if(num != sizeof(msg))
		{
			perror(strerror(errno));
		    continue;
		}

		if(LOG == msg.action)
		{
			printf("one user tring to log\n");
			char sql_log[256] = {0};

			sprintf(sql_log, "select password from user_info where name = '%s';", msg.name); 
			memset(str_return, 0, sizeof(str_return));
			int rc = sqlite3_exec(db, sql_log, call_back, 0, &err_msg);
			
			if(rc != SQLITE_OK)
			{
				msg_ret.action = LOG_FAIL;
				write(connectfd,&msg_ret,sizeof(msg_ret));
				printf("数据库异常!\n");
				continue;
			}
            
			if(0 == strlen(str_return))
			{
				msg_ret.action = USER_ERROR;
				write(connectfd,&msg_ret,sizeof(msg_ret));
				printf("用户不存在!\n");
				continue;
			}

			if(0 != strncmp(str_return, msg.password, strlen(msg.password)))
			{
				msg_ret.action = PSWD_ERR;
				printf("密码错误!\n");
				write(connectfd,&msg_ret,sizeof(msg_ret));
				continue;
			}

			sprintf(sql_log, "select * from log_info where name = '%s';", msg.name); 
			memset(str_return, 0, sizeof(str_return));
			rc = sqlite3_exec(db, sql_log, call_back, 0, &err_msg);
			if(rc != SQLITE_OK)
			{
				msg_ret.action = LOG_FAIL;
				write(connectfd,&msg_ret,sizeof(msg_ret));
				printf("数据库未打开!\n");
				continue;
			}

			if(0 != strlen(str_return))
			{
				msg_ret.action = RE_LOG;
				write(connectfd,&msg_ret,sizeof(msg_ret));
				printf("重复登录!\n");
				continue;
			}

			sprintf(sql_log,"insert into log_info(name,connectfd) values('%s',%d);",msg.name,connectfd); 
			rc = sqlite3_exec(db,sql_log,NULL,0,&err_msg);
		    //fprintf(stderr,"%s\n",err_msg);
			if(rc != SQLITE_OK)
			{
				msg_ret.action = LOG_FAIL;
				write(connectfd,&msg_ret,sizeof(msg_ret));
				printf("绑定connectfd失败!\n");
				continue;
			}

			msg_ret.action = LOG_SUCC;
			strcpy(msg_ret.name,msg.name);
			write(connectfd,&msg_ret,sizeof(msg_ret));
		}

		else if(REGISTER == msg.action)
		{
			char sql_register[256] = {0};

			sprintf(sql_register, "insert into user_info(name,password) values('%s','%s');",msg.name,msg.password); 

			int rc = sqlite3_exec(db, sql_register, NULL, 0, &err_msg);
			if(rc == SQLITE_CONSTRAINT)
			{
				msg_ret.action = RE_REG;
				write(connectfd,&msg_ret,sizeof(msg_ret));
				continue;
			}
			
			else if(rc != SQLITE_OK)
			{
				msg_ret.action = REG_FAIL;
				write(connectfd,&msg_ret,sizeof(msg_ret));
				continue;
			}
            
			msg_ret.action = REG_SUCC;
			write(connectfd,&msg_ret,sizeof(msg_ret));
			//continue;
		}	
		
		else if(SHOW_ONLINE == msg.action)
		{
			char * sql_show = "select name from log_info;";
			memset(str_return, 0, sizeof(str_return));
			int rc = sqlite3_exec(db, sql_show, call_back, 0, &err_msg);
			
			if(rc != SQLITE_OK)
			{
				msg.action = SHOW_ERR;
				write(connectfd,&msg_ret,sizeof(msg_ret));
				continue;
			}

			msg_ret.action = SHOW_ONLINE;
			strcpy(msg_ret.msg,str_return);
			write(connectfd,&msg_ret,sizeof(msg_ret));
			//continue;
		}	
		
		else if(WHISPER == msg.action)
		{
			char sql_whisper[100];
			sprintf(sql_whisper, "select name from user_info where name = '%s';", msg.target); 
			memset(str_return, 0, sizeof(str_return));
			int rc = sqlite3_exec(db, sql_whisper, call_back, 0, &err_msg);
			
			if(rc != SQLITE_OK)
			{
				msg_ret.action = SEND_FAIL;
				write(connectfd,&msg_ret,sizeof(msg_ret));
				continue;
			}

			if(0 == strlen(str_return))
			{
				msg_ret.action = USER_ERROR;
				write(connectfd,&msg_ret,sizeof(msg_ret));
				continue;
			}

			sprintf(sql_whisper, "select connectfd from log_info where name = '%s';", msg.target); 
			memset(str_return, 0, sizeof(str_return));
			rc = sqlite3_exec(db, sql_whisper, call_back, 0, &err_msg);
			
			if(rc != SQLITE_OK)
			{
				msg_ret.action = SEND_FAIL;
				write(connectfd,&msg_ret,sizeof(msg_ret));
				continue;
			}

			if(0 == strlen(str_return))
			{
				msg_ret.action = OFFLINE;
				write(connectfd,&msg_ret,sizeof(msg_ret));
				continue;
			}

			tgt_confd = atoi(str_return);
			//printf("msg_ret.name = %s\n",msg_ret.name);
            strcpy(msg.name,msg_ret.name);
			//printf("msg.name = %s\n",msg.name);
			
			//msg.action = WHISPER;
			write(tgt_confd,&msg,sizeof(msg));
            //continue;
		}	
		else if(CHAT == msg.action)
		{
			int i = 0;
			int count = 0;
			char temp[4];
			char * sql_chat = "select connectfd from log_info;";
			
			memset(str_return, 0, sizeof(str_return));
			int rc = sqlite3_exec(db, sql_chat, call_back, 0, &err_msg);
			
			if(rc != SQLITE_OK)
			{
				msg_ret.action = SEND_FAIL;
				write(connectfd,&msg_ret,sizeof(msg_ret));
				continue;
			}

			msg.action = CHAT;
			//strcpy(msg.name,msg_ret.name);
			
			while(0 != *(str_return + count))
			{
				if('\t' == *(str_return + count))
				{
					tgt_confd = atoi(temp);
					if(tgt_confd != connectfd)
					{
						write(tgt_confd,&msg,sizeof(msg));
						printf("chat\n");
					}
					memset(temp, 0, sizeof(temp));
					i = 0;
				}
				strncpy(temp + i, str_return + count, 1);
				i++;
				count++;
			}
            
			//msg.action = OTHER;
		}	
		else if(QUIT_SYSTEM == msg.action)
		{
			char sql_quit[100] = {0};
			sprintf(sql_quit, "delete from log_info where name = '%s';", msg.name);
			int rc = sqlite3_exec(db,sql_quit,NULL,0,&err_msg);
			
			if(rc != SQLITE_OK)
			{
				msg_ret.action = QUIT_FAIL;
				write(connectfd,&msg_ret,sizeof(msg_ret));
				continue;
			}
			
			else
			{	
				msg_ret.action = QUIT_SYSTEM;
				write(connectfd,&msg_ret,sizeof(msg_ret));
				printf("%s quit chat room\n",msg.name);
				return NULL;
			}
		}	
	}
}

void *listen_thread(void *arg)
{	
	int listenfd = *((int *)arg);
	int connectfd = 0;
	pthread_t pid_read_write = 2;
	
	struct sockaddr_in client;
	socklen_t addrlen = sizeof(client);
	
	while(1)
	{	
    	connectfd = accept(listenfd,(struct sockaddr*)&client,&addrlen);
		printf("accept one connect\n");
		pthread_create(&pid_read_write,NULL,(void *)read_write_pthread,&connectfd);
    }
}

int main(int argc,char *argv[])
{
	//1 creat socket
	int listenfd = socket(PF_INET,SOCK_STREAM,0);
	//set addr reuse
	int opt = SO_REUSEADDR;
	setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
	//2 bind
	struct sockaddr_in server;
	memset(&server,0,sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons(PORT);
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	bind(listenfd,(struct sockaddr *)&server,sizeof(server));
	//3 listen
	listen(listenfd,QUEUE_LEN);

	int connectfd = 0;
	pthread_t pid_listen = 1;

	printf("accepting...\n");

	pthread_create(&pid_listen,NULL,(void *)listen_thread,&listenfd);
	pthread_join(pid_listen,NULL);
	//printf("exit\n");
	close(listenfd);
	return 0;
}

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
#include <time.h>

#define MSG_MAX_LEN 100
#define NAME_LEN 20
#define PSWOD_LEN 20
#define PORT 9010
#define PID_NUM 2
#define CMD_SIZE 10
#define QUEUE_LEN 10

enum{ 
    LOG = 1,
	LOG_SUCC  =  2,
	LOG_FAIL  =  3,
	PSWD_ERR  =  4,
	RE_LOG    =	 5,
	REGISTER =	 6,
	REG_SUCC =	 7,
	REG_FAIL =	 8,
	RE_REG   =	 9,
	WHISPER  =	 10,
	SEND_FAIL =	 11,
	USER_ERROR = 12,
	OFFLINE   	= 13,
	CHAT      	 =14,
	SHOW_ONLINE = 15,
	SHOW_ERR    = 16,
	QUIT_SYSTEM = 17,
	QUIT_FAIL   = 18,
	PTH_CRET_ERR =19,
	OTHER        =20,
	YES          =21,
	NO           =22,
	SLIENCE      =23,
	SAY          =24,
	GETOUT       =25,
	SLIENCE_FAIL =26,
	SAY_FAIL     =27,
    REQUIRE      =28,
	REQUIRE_FAIL =29,
	GETOUT_FAIL  =30,
	WHISPER_SUCC =31,
	CHAT_SUCC    =32,
	TRAN         =33,
	TRAN_SUCC    =34,
	TRAN_FAIL    =35,
	SLIENCE_SUCC =36,
	SAY_SUCC     =37,
    GETOUT_SUCC  =38
};

typedef struct msg_tag
{
	int action;
	char name[NAME_LEN];
	char password[PSWOD_LEN];
	char target[NAME_LEN];
	char msg[MSG_MAX_LEN];
	char fname[NAME_LEN];
	int flen;
}msg_t;

char str_return[100] = {0};

int block_read(int fd,char *data,int len)
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

int block_write(int fd,char *data,int len)
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
	time_t ptime;
    char pestime[30] = {0};

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
	
	char sql_create_record[256] = {0};
	
	sprintf(sql_create_record,"create table record(name TEXT,size TEXT,target TEXT,msg TEXT,time TEXT,primary key(name,time));");
	sqlite3_exec(db,sql_create_record,NULL,0,&err_msg);
	
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
			char sql_log[256] = {0};

			sprintf(sql_log, "select password from user_info where name = '%s';", msg.name); 
			memset(str_return, 0, sizeof(str_return));
			int rc = sqlite3_exec(db, sql_log, call_back, 0, &err_msg);
			
			if(rc != SQLITE_OK)
			{
				msg.action = LOG_FAIL;
				write(connectfd,&msg,sizeof(msg));
				//printf("数据库异常!\n");
				continue;
			}
            
			if(0 == strlen(str_return))
			{
				msg.action = USER_ERROR;
				write(connectfd,&msg,sizeof(msg));
				//printf("用户不存在!\n");
				continue;
			}

			if((0 != strncmp(str_return, msg.password, strlen(str_return) - 1)) || (strlen(msg.password) != (strlen(str_return) - 1)))
			{
				msg.action = PSWD_ERR;
				//printf("密码错误!\n");
				write(connectfd,&msg,sizeof(msg));
				continue;
			}

			sprintf(sql_log, "select * from log_info where name = '%s';", msg.name); 
			memset(str_return, 0, sizeof(str_return));
			rc = sqlite3_exec(db, sql_log, call_back, 0, &err_msg);
			
			if(rc != SQLITE_OK)
			{
				msg.action = LOG_FAIL;
				write(connectfd,&msg,sizeof(msg));
				//printf("数据库未打开!\n");
				continue;
			}

			if(0 != strlen(str_return))
			{
				msg.action = RE_LOG;
				write(connectfd,&msg,sizeof(msg));
				//printf("重复登录!\n");
				continue;
			}

			sprintf(sql_log,"insert into log_info(name,connectfd) values('%s',%d);",msg.name,connectfd); 
			rc = sqlite3_exec(db,sql_log,NULL,0,&err_msg);
			
			if(rc != SQLITE_OK)
			{
				msg.action = LOG_FAIL;
				write(connectfd,&msg,sizeof(msg));
				//printf("绑定connectfd失败!\n");
				continue;
			}

			msg.action = LOG_SUCC;
			write(connectfd,&msg,sizeof(msg));
			printf("user %s log\n",msg.name);
			
			time(&ptime);
            strcpy(pestime,ctime(&ptime));
			char sql_log_record[256];
			sprintf(sql_log_record, "insert into record (name,size,time) values('%s','%s','%s');",msg.name,"登录",pestime); 
			
			rc = sqlite3_exec(db, sql_log_record, NULL, 0, &err_msg);
			continue;
		}

		else if(REGISTER == msg.action)
		{
			char sql_register[256] = {0};

			sprintf(sql_register, "insert into user_info(name,password) values('%s','%s');",msg.name,msg.password); 

			int rc = sqlite3_exec(db, sql_register, NULL, 0, &err_msg);
			if(rc == SQLITE_CONSTRAINT)
			{
				msg.action = RE_REG;
				write(connectfd,&msg,sizeof(msg));
				continue;
			}
			
			else if(rc != SQLITE_OK)
			{
				msg.action = REG_FAIL;
				write(connectfd,&msg,sizeof(msg));
				continue;
			}
            
			msg.action = REG_SUCC;
			write(connectfd,&msg,sizeof(msg));
			time(&ptime);
            strcpy(pestime,ctime(&ptime));
			char sql_register_record[256];
			sprintf(sql_register_record, "insert into record (name,size,time) values('%s','%s','%s');",msg.name,"注册",pestime); 
			
			rc = sqlite3_exec(db, sql_register_record, NULL, 0, &err_msg);
			continue;
		}	
		
		else if(SHOW_ONLINE == msg.action)
		{
			char * sql_show = "select name from log_info;";
			memset(str_return, 0, sizeof(str_return));
			int rc = sqlite3_exec(db, sql_show, call_back, 0, &err_msg);
			
			if(rc != SQLITE_OK)
			{
				msg.action = SHOW_ERR;
				write(connectfd,&msg,sizeof(msg));
				continue;
			}
            
			strcpy(msg.msg,str_return);
			msg.action = SHOW_ONLINE;
			write(connectfd,&msg,sizeof(msg));
			continue;
		}	
		
		else if(TRAN == msg.action)
		{
			char sql_tran[256];
			sprintf(sql_tran, "select name from user_info where name = '%s';", msg.target); 
			
			memset(str_return, 0, sizeof(str_return));
			int rc = sqlite3_exec(db, sql_tran, call_back, 0, &err_msg);
			
			if(rc != SQLITE_OK)
			{
				msg.action = TRAN_FAIL;
				write(connectfd,&msg,sizeof(msg));
				continue;
			}

			if(0 == strlen(str_return))
			{
				msg.action = USER_ERROR;
				write(connectfd,&msg,sizeof(msg));
				continue;
			}

			sprintf(sql_tran, "select connectfd from log_info where name = '%s';", msg.target); 
			memset(str_return, 0, sizeof(str_return));
			rc = sqlite3_exec(db,sql_tran,call_back,0,&err_msg);
			
			if(rc != SQLITE_OK)
			{
				msg.action = TRAN_FAIL;
				write(connectfd,&msg,sizeof(msg));
				continue;
			}

			if(0 == strlen(str_return))
			{
				msg.action = OFFLINE;
				write(connectfd,&msg,sizeof(msg));
				continue;
			}

			char *data = NULL;
			data = (char *)malloc(msg.flen + 1);
			memset(data,0,msg.flen + 1);

			block_read(connectfd,data,msg.flen);
			
			tgt_confd = atoi(str_return);
			msg.action = TRAN;
			write(tgt_confd,&msg,sizeof(msg));
			block_write(tgt_confd,data,msg.flen);
			
			free(data);
			
			msg.action = TRAN_SUCC;
			write(connectfd,&msg,sizeof(msg));
			time(&ptime);
            strcpy(pestime,ctime(&ptime));
			char sql_tran_record[256];
			sprintf(sql_tran_record, "insert into record (name,size,target,time) values('%s','%s','%s','%s');",msg.name,"发送文件",msg.target,pestime); 
			
			rc = sqlite3_exec(db, sql_tran_record, NULL, 0, &err_msg);
			continue;
		}	
		
		else if(WHISPER == msg.action)
		{
			char sql_whisper[256];
			
			sprintf(sql_whisper, "select name from user_info where name = '%s';", msg.target); 
			memset(str_return, 0, sizeof(str_return));
			int rc = sqlite3_exec(db, sql_whisper, call_back, 0, &err_msg);
			
			if(rc != SQLITE_OK)
			{
				msg.action = SEND_FAIL;
				write(connectfd,&msg,sizeof(msg));
				continue;
			}

			if(0 == strlen(str_return))
			{
				msg.action = USER_ERROR;
				write(connectfd,&msg,sizeof(msg));
				continue;
			}

			sprintf(sql_whisper, "select connectfd from log_info where name = '%s';", msg.target); 
			memset(str_return, 0, sizeof(str_return));
			rc = sqlite3_exec(db, sql_whisper, call_back, 0, &err_msg);
			
			if(rc != SQLITE_OK)
			{
				msg.action = SEND_FAIL;
				write(connectfd,&msg,sizeof(msg));
				continue;
			}

			if(0 == strlen(str_return))
			{
				msg.action = OFFLINE;
				write(connectfd,&msg,sizeof(msg));
				continue;
			}
            
			tgt_confd = atoi(str_return);
			write(tgt_confd,&msg,sizeof(msg));
			
			msg.action = WHISPER_SUCC;
			write(connectfd,&msg,sizeof(msg));
			
			time(&ptime);
            strcpy(pestime,ctime(&ptime));
			char sql_whsp_record[256];
			sprintf(sql_whsp_record, "insert into record (name,size,target,msg,time) values('%s','%s','%s','%s','%s');",msg.name,"私密",msg.target,msg.msg,pestime); 
			
			rc = sqlite3_exec(db, sql_whsp_record, NULL, 0, &err_msg);
			continue;
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
				msg.action = SEND_FAIL;
				write(connectfd,&msg,sizeof(msg));
				continue;
			}

			msg.action = CHAT;
			
			while(0 != *(str_return + count))
			{
				if('\t' == *(str_return + count))
				{
					tgt_confd = atoi(temp);
					if(tgt_confd != connectfd)
					{
						write(tgt_confd,&msg,sizeof(msg));
					}
					memset(temp, 0, sizeof(temp));
					i = 0;
				}
				strncpy(temp + i, str_return + count, 1);
				i++;
				count++;
			}
			msg.action = CHAT_SUCC;
			write(connectfd,&msg,sizeof(msg));
			
			time(&ptime);
            strcpy(pestime,ctime(&ptime));
			char sql_chat_record[256];
			sprintf(sql_chat_record, "insert into record (name,size,target,msg,time) values('%s','%s','%s','%s','%s');",msg.name,"群发","all user",msg.msg,pestime); 
			
			rc = sqlite3_exec(db, sql_chat_record, NULL, 0, &err_msg);
		}	
		
		else if(SLIENCE == msg.action)
		{
			char sql_slience[256];
			sprintf(sql_slience, "select name from user_info where name = '%s';", msg.target); 
			memset(str_return, 0, sizeof(str_return));
			int rc = sqlite3_exec(db, sql_slience, call_back, 0, &err_msg);
			
			if(rc != SQLITE_OK)
			{
				msg.action = SLIENCE_FAIL;
				write(connectfd,&msg,sizeof(msg));
				continue;
			}

			if(0 == strlen(str_return))
			{
				msg.action = USER_ERROR;
				write(connectfd,&msg,sizeof(msg));
				continue;
			}

			sprintf(sql_slience, "select connectfd from log_info where name = '%s';", msg.target); 
			memset(str_return, 0, sizeof(str_return));
			rc = sqlite3_exec(db, sql_slience, call_back, 0, &err_msg);
			
			if(rc != SQLITE_OK)
			{
				msg.action = SLIENCE_FAIL;
				write(connectfd,&msg,sizeof(msg));
				continue;
			}

			if(0 == strlen(str_return))
			{
				msg.action = OFFLINE;
				write(connectfd,&msg,sizeof(msg));
				continue;
			}

			tgt_confd = atoi(str_return);
			msg.action = SLIENCE;
			write(tgt_confd,&msg,sizeof(msg));
			msg.action = SLIENCE_SUCC;
			write(connectfd,&msg,sizeof(msg));
			
			time(&ptime);
            strcpy(pestime,ctime(&ptime));
			char sql_slience_record[256];
			sprintf(sql_slience_record, "insert into record (name,size,target,time) values('%s','%s','%s','%s');",msg.name,"禁言",msg.target,pestime); 
			
			rc = sqlite3_exec(db, sql_slience_record, NULL, 0, &err_msg);
		}	
		
		else if(REQUIRE == msg.action)
		{
			char sql_require[256];
			
			sprintf(sql_require, "select name from user_info where name = '%s';", msg.target); 
			memset(str_return, 0, sizeof(str_return));
			int rc = sqlite3_exec(db, sql_require, call_back, 0, &err_msg);
			
			if(rc != SQLITE_OK)
			{
				msg.action = REQUIRE_FAIL;
				write(connectfd,&msg,sizeof(msg));
				continue;
			}

			if(0 == strlen(str_return))
			{
				msg.action = USER_ERROR;
				write(connectfd,&msg,sizeof(msg));
				continue;
			}

			sprintf(sql_require, "select connectfd from log_info where name = '%s';", msg.target); 
			memset(str_return, 0, sizeof(str_return));
			rc = sqlite3_exec(db, sql_require, call_back, 0, &err_msg);
			
			if(rc != SQLITE_OK)
			{
				msg.action = REQUIRE_FAIL;
				write(connectfd,&msg,sizeof(msg));
				continue;
			}

			if(0 == strlen(str_return))
			{
				msg.action = OFFLINE;
				write(connectfd,&msg,sizeof(msg));
				continue;
			}

			tgt_confd = atoi(str_return);
			msg.action = REQUIRE;
			write(tgt_confd,&msg,sizeof(msg));
			time(&ptime);
            strcpy(pestime,ctime(&ptime));
			char sql_require_record[256];
			sprintf(sql_require_record, "insert into record (name,size,time) values('%s','%s','%s');",msg.name,"请求解禁",pestime); 
			
			rc = sqlite3_exec(db, sql_require_record, NULL, 0, &err_msg);
		}	
		
		else if(SAY== msg.action)
		{
			char sql_say[256];
			
			sprintf(sql_say, "select name from user_info where name = '%s';", msg.target); 
			memset(str_return, 0, sizeof(str_return));
			int rc = sqlite3_exec(db, sql_say, call_back, 0, &err_msg);
			
			if(rc != SQLITE_OK)
			{
				msg.action = SAY_FAIL;
				write(connectfd,&msg,sizeof(msg));
				continue;
			}

			if(0 == strlen(str_return))
			{
				msg.action = USER_ERROR;
				write(connectfd,&msg,sizeof(msg));
				continue;
			}

			sprintf(sql_say, "select connectfd from log_info where name = '%s';", msg.target); 
			memset(str_return, 0, sizeof(str_return));
			rc = sqlite3_exec(db, sql_say, call_back, 0, &err_msg);
			
			if(rc != SQLITE_OK)
			{
				msg.action = SAY_FAIL;
				write(connectfd,&msg,sizeof(msg));
				continue;
			}

			if(0 == strlen(str_return))
			{
				msg.action = OFFLINE;
				write(connectfd,&msg,sizeof(msg));
				continue;
			}

			tgt_confd = atoi(str_return);
			msg.action = SAY;
			write(tgt_confd,&msg,sizeof(msg));
			
			msg.action = SAY_SUCC;
			write(connectfd,&msg,sizeof(msg));
			time(&ptime);
            strcpy(pestime,ctime(&ptime));
			char sql_say_record[256];
			sprintf(sql_say_record, "insert into record (name,size,target,time) values('%s','%s','%s','%s');",msg.name,"解禁",msg.target,pestime); 
			
			rc = sqlite3_exec(db, sql_say_record, NULL, 0, &err_msg);
		}	
		
		else if(GETOUT == msg.action)
		{
			char sql_getout[256];
			sprintf(sql_getout, "select name from user_info where name = '%s';", msg.target); 
			memset(str_return, 0, sizeof(str_return));
			int rc = sqlite3_exec(db, sql_getout, call_back, 0, &err_msg);
			
			if(rc != SQLITE_OK)
			{
				msg.action = GETOUT_FAIL;
				write(connectfd,&msg,sizeof(msg));
				continue;
			}

			if(0 == strlen(str_return))
			{
				msg.action = USER_ERROR;
				write(connectfd,&msg,sizeof(msg));
				continue;
			}

			sprintf(sql_getout, "select connectfd from log_info where name = '%s';", msg.target); 
			memset(str_return, 0, sizeof(str_return));
			rc = sqlite3_exec(db, sql_getout, call_back, 0, &err_msg);
			
			if(rc != SQLITE_OK)
			{
				msg.action = GETOUT_FAIL;
				write(connectfd,&msg,sizeof(msg));
				continue;
			}

			if(0 == strlen(str_return))
			{
				msg.action = OFFLINE;
				write(connectfd,&msg,sizeof(msg));
				continue;
			}

			msg.action = GETOUT_SUCC;
			write(connectfd,&msg,sizeof(msg));
			
            char sql_quit[256] = {0};
			sprintf(sql_quit, "delete from log_info where name = '%s';", msg.target);
			rc = sqlite3_exec(db,sql_quit,NULL,0,&err_msg);
			
			if(rc != SQLITE_OK)
			{
				msg.action = GETOUT_FAIL;
				write(connectfd,&msg,sizeof(msg));
				continue;
			}
			
			tgt_confd = atoi(str_return);
			msg.action = GETOUT;
			write(tgt_confd,&msg,sizeof(msg));
			printf("user %s logoff\n",msg.target);

			time(&ptime);
            strcpy(pestime,ctime(&ptime));
			char sql_getout_record[256];
			sprintf(sql_getout_record, "insert into record (name,size,target,time) values('%s','%s','%s','%s');",msg.name,"踢出聊天室",msg.target,pestime); 
			rc = sqlite3_exec(db, sql_getout_record, NULL, 0, &err_msg);
			
			time(&ptime);
            strcpy(pestime,ctime(&ptime));
			rc = sqlite3_exec(db, sql_getout_record, NULL, 0, &err_msg);
			sprintf(sql_getout_record, "insert into record (name,size,time) values('%s','%s','%s');",msg.target,"下线",pestime); 
			
			rc = sqlite3_exec(db, sql_getout_record, NULL, 0, &err_msg);
		}	
		
		else if(QUIT_SYSTEM == msg.action)
		{
			char sql_quit[100] = {0};
			sprintf(sql_quit, "delete from log_info where name = '%s';", msg.name);
			int rc = sqlite3_exec(db,sql_quit,NULL,0,&err_msg);
			
			if(rc != SQLITE_OK)
			{
				msg.action = QUIT_FAIL;
				write(connectfd,&msg,sizeof(msg));
				continue;
			}
			
			else
			{	
				msg.action = QUIT_SYSTEM;
				write(connectfd,&msg,sizeof(msg));
				//printf("%s quit chat room\n",msg.name);
				printf("one client quit chat room\n",msg.name);
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
	
	close(listenfd);
	
	return 0;
}

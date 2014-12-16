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

enum{LOG = 1,REGISTER,UNKNOW_ORDER,CREAT_FAIL,PTH_CRET_ERR,REG_SUCCESS,REG_FAIL,LOG_SUCCESS,LOG_FAIL,NAME_PST,WHISPER,CHAT_ALL,SHOW_ONLINE,QUIT_SYSTEM,USER_ERROR,NO_USER};


typedef struct msg_tag
{
	int action;
	char name[NAME_LEN];
	char password[PSWOD_LEN];
	char target[NAME_LEN];
	char msg[MSG_MAX_LEN];
}msg_t;

typedef struct msg_return_tag
{
	int action;
	char name[NAME_LEN];
	char msg[MSG_MAX_LEN];
}msg_ret_t;

static pthread_mutex_t g_mutex;

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

void *thread_read(void *arg)
{
	printf("new thread create\n");
	int connectfd = *((int *)arg);
	msg_t msg;
	msg_ret_t msg_ret;
	int tag_connectfd = 0;
	int column = 0;
	int count = 0;
	char **se_result;
	sqlite3 *db = NULL;
	char *err_msg = NULL;
	int rc = sqlite3_open("chat_room.db",&db);
	if(rc != SQLITE_OK)
	{
		fprintf(stderr,"open database failed %s\n",sqlite3_errmsg(db));
	}
	while(1)
	{
		read(connectfd,&msg,sizeof(msg));
		if(msg.action == WHISPER)
		{
			printf("11 12 13 14msg.action = %d\n",msg.action);
			printf("msg.target = %s\n",msg.target);
			char sql_select_user[256] = {0};
			//char *sql_select_user = "select * from log_info;";
			sprintf(sql_select_user,"select connectfd from log_info where name = '%s';",msg.target);
			//int rc = sqlite3_get_table(db,sql_select_user,&se_result,&count,&column,&err_msg);
			int rc = sqlite3_get_table(db,sql_select_user,&se_result,&count,&column,&err_msg);
			tag_connectfd = atoi(se_result[1]);
			printf("tag_connectfd = %d\n",tag_connectfd);
			sqlite3_free_table(se_result);
			if(rc != SQLITE_OK)
			{
			    msg_ret.action = USER_ERROR;
				write(connectfd,&msg_ret,sizeof(msg_ret));
			}	
            else
			{	
				msg_ret.action = WHISPER;
				strcpy(msg_ret.name,msg.name);
				strcpy(msg_ret.msg,msg.msg);
				printf("msg.msg = %s\n",msg.msg);
				printf("msg_ret.msg = %s\n",msg_ret.msg);

				printf("whisper\n");
				write(tag_connectfd,&msg,sizeof(msg));	    
		    }
		}
		else if(msg.action == CHAT_ALL)
		{
			char *sql_select_user = "select connectfd from log_info;";
			int rc = sqlite3_get_table(db,sql_select_user,&se_result,&count,&column,&err_msg);
			if(rc == SQLITE_OK)
			{	
				int i = 0;
				for(i = 0;i < (count + 1) * column;)
				{
					tag_connectfd = atoi(se_result[i++]);
            		msg_ret.action = WHISPER;
					strcpy(msg_ret.name,msg.name);
					strcpy(msg_ret.msg,msg.msg);
					write(tag_connectfd,&msg_ret,sizeof(msg_ret));	    
		    	}
				sqlite3_free_table(se_result);
			}	
			
			else
			{
			    msg_ret.action = USER_ERROR;	
				write(connectfd,&msg_ret,sizeof(msg_ret));
			}
		}
		else if(msg_ret.action == SHOW_ONLINE)
		{
			char *sql_select_user = "select * from log_info;";
			int rc = sqlite3_get_table(db,sql_select_user,&se_result,&count,&column,&err_msg);
			if(rc == SQLITE_OK)
			{	
				//tag_connectfd = atoi(se_result[0]);
				int i = 0;
				for(i = 2;i < (count * column);)
				{
					msg_ret.action = SHOW_ONLINE;
					strcmp(se_result[i++],msg_ret.msg);
					tag_connectfd = atoi(se_result[i++]);
					write(tag_connectfd,&msg_ret,sizeof(msg));
				}
			}
			else
			{
			    msg_ret.action = NO_USER;	
				write(connectfd,&msg_ret,sizeof(msg_ret));
			}
			sqlite3_free_table(se_result);
		}
		else if(msg_ret.action == QUIT_SYSTEM)
		{
			char *sql_select_user = "select connectfd from log_info where name = 'msg.name';";
			int rc = sqlite3_get_table(db,sql_select_user,&se_result,&count,&column,&err_msg);
			tag_connectfd = atoi(se_result[1]);
			msg_ret.action = QUIT_SYSTEM;
			write(tag_connectfd,&msg_ret,sizeof(msg));
			char *sql_drop_user = "drop from log_info where name = 'msg.name';";
			rc = sqlite3_exec(db,sql_drop_user,NULL,0,&err_msg);
		}
	}	
	return NULL;
}

void *read_write_pthread(void *arg)
{
	printf("new listen pthread create\n");
	msg_t msg;
	msg_ret_t msg_ret;
	pthread_mutex_lock(&g_mutex);
	int connectfd = *((int *)arg);
	pthread_mutex_unlock(&g_mutex);
	sqlite3 *db = NULL;
	char *err_msg = NULL;

	int rc = sqlite3_open("chat_room.db",&db);
	if(rc != SQLITE_OK)
	{
		fprintf(stderr,"open database failed %s\n",sqlite3_errmsg(db));
	}
	char sql_create_user_info[256] = {0};
	sprintf(sql_create_user_info,"create user_info(name TEXT,password TEXT,primary key(name));");
	char sql_create_log_info[256] = {0};
	sprintf(sql_create_log_info,"create user_info(name TEXT,conenctfd INTEGER,primary key(name));");
	while(1)
	{
		memset(&msg,0,sizeof(msg));
		int num = read(connectfd,&msg,sizeof(msg));
		printf("num = %d\n",num);
		if(num != sizeof(msg))
		{
			perror(strerror(errno));
			close(connectfd);
			return NULL;
		}

		if(LOG == msg.action)
		{
			printf("one user tring to log\n");
			char *sql_select_user = "select * from user_info;";
	char *err_msg = NULL;
	char sql_log_user[256] = {0};

	sprintf(sql_log_user, "select password from user where name = '%s';", msg->name); 
	memset(str_return, 0, sizeof(str_return));
	int rc = sqlite3_exec(db, sql_log_user, call_back, 0, &err_msg);
	if(rc != SQLITE_OK)
	{
		msg->action = ERROR;
		sprintf(msg->msg, "%s", err_msg);
		return 0;
	}

	if(0 == strlen(str_return))
	{
		msg->action = ERROR;
		sprintf(msg->msg, "name %s inexistence", msg->name);
		return 0;
	}

	if(0 != strncmp(str_return, msg->password, strlen(msg->password)))
	{
		msg->action = ERROR;
		sprintf(msg->msg, "password error");
		return 0;
	}

	sprintf(sql_log_user, "select * from log where name = '%s';", msg->name); 
	memset(str_return, 0, sizeof(str_return));
	rc = sqlite3_exec(db, sql_log_user, call_back, 0, &err_msg);
	if(rc != SQLITE_OK)
	{
		msg->action = ERROR;
		sprintf(msg->msg, "%s", err_msg);
		return 0;
	}

	if(0 != strlen(str_return))
	{
		msg->action = ERROR;
		sprintf(msg->msg, "%s is logged", msg->name);
		return 0;
	}

	sprintf(sql_log_user, "insert into log(name, connectfd) values('%s', %d);", msg->name, connectfd); 
	rc = sqlite3_exec(db, sql_log_user, NULL, 0, &err_msg);
	if(rc != SQLITE_OK)
	{
		msg->action = ERROR;
		sprintf(msg->msg, "%s", err_msg);
		return 0;
	}

	msg->action = LOG;

	return 0;
}
			rc = sqlite3_get_table(db,sql_select_user,&se_result,&count,&column,&err_msg);
			if(rc == 1)
			{
				char *sql_create_user = "create table user_info(name TEXT,password TEXT,primary key(name));";
				rc = sqlite3_exec(db,sql_create_user,NULL,0,&err_msg);
				rc = sqlite3_get_table(db,sql_select_user,&se_result,&count,&column,&err_msg);
				if(rc != SQLITE_OK)
				{
					fprintf(stderr,"sql error: %s\n",err_msg);
					msg_ret.action = LOG_FAIL;
					num = write(connectfd,&msg_ret,sizeof(msg_ret));
					printf("LOG_FAIL write = %d",num);
					continue;
					//return NULL;
				}
			}

			int i = 0;
			for(i = 0;i < (count + 1) * column;)
			{
				if(strcmp(se_result[i++],msg.name) == 0 && strcmp(se_result[i],msg.password) == 0)
				{
					sqlite3_free_table(se_result);
					char sql_insert_log[256] = {0};
					sprintf(sql_insert_log,"insert into log_info(name,connectfd) values(%s,%d);",msg.name,connectfd);
					rc = sqlite3_exec(db,sql_insert_log,NULL,0,&err_msg);
					if(rc == 1)
					{
						char *sql_create_log = "create table log_info(name TEXT,connectfd INT,primary key(name));";
						rc = sqlite3_exec(db,sql_create_log,NULL,0,&err_msg);
						rc = sqlite3_exec(db,sql_insert_log,NULL,0,&err_msg);
					}    

					if(rc != SQLITE_OK)
					{
						fprintf(stderr,"sql error: %s\n",err_msg);
						msg_ret.action = LOG_FAIL;
						num = write(connectfd,&msg_ret,sizeof(msg_ret));
						printf("LOG_FAIL write num = %d\n",num);
						printf("connect failed\n");
						continue;
						//return NULL;
					}

					if(rc == SQLITE_OK)
					{
						printf("user log\n");
						msg_ret.action = LOG_SUCCESS;
						num = write(connectfd,&msg_ret,sizeof(msg_ret));
						pthread_t pid_read = 1;
						int ret = pthread_create(&pid_read,NULL,(void *)thread_read,(void *)&connectfd);
						if(0 != ret)
						{
							perror(strerror(errno));
						}
						printf("log success\n");
						//pthread_join(pid_read,NULL);
						sqlite3_close(db);
						pthread_detach(pid_read);
						return NULL;
					}
				}
			}
		}

		else if(REGISTER == msg.action)
		{
			char *sql_select_user = "select name from user_info where name = 'msg.name';";
			rc = sqlite3_get_table(db,sql_select_user,&se_result,&count,&column,&err_msg);
			char sql_insert_user[256] = {0};
			sprintf(sql_insert_user,"insert into user_info(name,password) values('%s','%s');",msg.name,msg.password);
			rc = sqlite3_exec(db,sql_insert_user,NULL,0,&err_msg);
			if(rc == 1)
			{
				char *sql_create_user = "create table user_info(name TEXT,password TEXT,primary key(name));";
				rc = sqlite3_exec(db,sql_create_user,NULL,0,&err_msg);
				rc = sqlite3_exec(db,sql_insert_user,NULL,0,&err_msg);
			}    

			if(rc != SQLITE_OK)
			{
				fprintf(stderr,"register failed,user present\n");
				msg_ret.action = NAME_PST;
				write(connectfd,&msg_ret,sizeof(msg_ret));
				continue;
				//return NULL;
			}
			else if(rc == SQLITE_OK)
			{
				printf("new user registered\n");
				msg_ret.action = REG_SUCCESS;
				write(connectfd,&msg_ret,sizeof(msg_ret));
				//return NULL;
				continue;
			}
		}

	}
	return NULL;
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
	//4 accept
	struct sockaddr_in client;
	socklen_t addrlen = sizeof(client);

	int connectfd = 0;
	pthread_t pid_read_write = 0;
	pthread_mutex_init(&g_mutex,NULL);

	printf("accepting...\n");

	while(1)
	{ 
		connectfd = accept(listenfd,(struct sockaddr*)&client,&addrlen);

		if(connectfd > 0)
		{
			printf("accept one connect\n");
			int ret = pthread_create(&pid_read_write,NULL,(void *)read_write_pthread,&connectfd);
			if(0 != ret)
			{
				perror(strerror(errno));
				return PTH_CRET_ERR;
			}
			//pthread_join(pid_listen,NULL);
		}
	}

	printf("exit\n");
	close(listenfd);
	pthread_mutex_destory(&g_mutex);
	return 0;
}


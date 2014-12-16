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

enum{LOG = 1,REGISTER,UNKNOW_ORDER,CREAT_FAIL,PTH_CRET_ERR,REG_SUCCESS,REG_FAIL,LOG_SUCCESS,LOG_FAIL,NAME_PST};
	    

typedef struct msg_tag
{
    int action;
    char name[NAME_LEN];
    char password[PSWOD_LEN];
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

void *listen_pthread(void *arg)
{
    msg_t msg;
    msg_ret_t msg_ret;
    int column = 0;
    int count = 0;
    char **se_result;
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
    while(1)
    {
    //int num = block_read(connectfd,&msg,sizeof(msg));
    int num = read(connectfd,&msg,sizeof(msg));
    printf("num = %d\n",num);
    //rc = sqlite3_exec(db,sql_select_user,NULL,0,&err_msg);
    if(num != sizeof(msg))
    {
        perror(strerror(errno));
        close(connectfd);
	return 0;
    }
    printf("msg.action = %d\n",msg.action); 
    if(LOG == msg.action)
    {
	//char sql_select_user[256] = {0};
	//sprintf(sql_select_user,"select * from user_info where name = %s AND password = %s;",msg.name,msg.password);
	printf("one user tring to log\n");
	char *sql_select_user = "select * from user_info;";
	//rc = sqlite3_exec(db,sql_select_user,&se_result,&column,&err_msg);
	rc = sqlite3_get_table(db,sql_select_user,&se_result,&count,&column,&err_msg);
	printf("rc = %d\n",rc);
	if(rc == 0)
	{
	    char *sql_create_user = "create table user_info(name TEXT,password TEXT,primary key(name));";
	    rc = sqlite3_exec(db,sql_create_user,NULL,0,&err_msg);
	    printf("rc = %d\n",rc); 
	    rc = sqlite3_get_table(db,sql_select_user,&se_result,&count,&column,&err_msg);
	    printf("rc = %d\n",rc); 
	    printf("SQLITE_OK = %d\n",SQLITE_OK);
	    if(rc != SQLITE_OK)
	    {
	        fprintf(stderr,"sql error: %s\n",err_msg);
		msg_ret.action = LOG_FAIL;
		//block_write(connectfd,&msg,sizeof(msg));
		num = write(connectfd,&msg_ret,sizeof(msg_ret));
		printf("LOG_FAIL write = %d",num);
		continue;
	    }
	}
	int i = 0;
	for(i = 0;i < (count + 1) * column;i++)
	{
	    printf("selecting...\n");
	    if(strcmp(se_result[i],msg.name) == 0 && strcmp(se_result[i+1],msg.password) == 0)
	    {
	    	char sql_insert_log[256] = {0};
		sprintf(sql_insert_log,"insert into log_info(name,connectfd) values(%s,%d);",msg.name,connectfd);
		rc = sqlite3_exec(db,sql_insert_log,NULL,0,&err_msg);
		printf("rc = %d\n",rc);
		if(rc == 1)
		{
	    	    char *sql_create_log = "create table log_info(name TEXT,connectfd INT,primary key(name));";
	    	    rc = sqlite3_exec(db,sql_create_log,NULL,0,&err_msg);
	    	    rc = sqlite3_exec(db,sql_insert_log,NULL,0,&err_msg);
	            printf("rc = %d\n",rc); 
	    	    if(rc != SQLITE_OK)
	    	    {
	        	fprintf(stderr,"sql error: %s\n",err_msg);
                	msg_ret.action = LOG_FAIL;
			//block_write(connectfd,&msg,sizeof(msg));
			num = write(connectfd,&msg_ret,sizeof(msg_ret));
			printf("LOG_FAIL write num = %d\n",num);
			printf("connect failed\n");
			continue;
	    	    }
	 
	    	    else if(rc == SQLITE_OK)
	    	    {
	    		printf("user log\n");
	    		msg_ret.action = LOG_SUCCESS;
	    		//block_write(connectfd,&msg,sizeof(msg));
	    		num = write(connectfd,&msg_ret,sizeof(msg_ret));
			printf("LOG SUCCESS  write num = %d",num);
			printf("log success\n");
	            }	
		}
	    }
	}
    }

    else if(REGISTER == msg.action)
    {
	char *sql_select_user = "select name from user_info where name = 'msg.name';";
	rc = sqlite3_get_table(db,sql_select_user,&se_result,&count,&column,&err_msg);
	int i = 0;
	for(i = 0;i < (count + 1) * column;i++)
	{
	    if(strcmp(se_result[i],msg.name) == 0)
	    {
	        msg_ret.action = NAME_PST;
		//block_write(connectfd,&msg_ret,sizeof(msg_ret));
		write(connectfd,&msg_ret,sizeof(msg_ret));
		continue;
	    } 
	}
	char sql_insert_user[256] = {0};
	sprintf(sql_insert_user,"insert into user_info(name,password) values(%s,%s);",msg.name,msg.password);
	rc = sqlite3_exec(db,sql_insert_user,NULL,0,&err_msg);
	if(rc == 1)
	{
	    char *sql_create_user = "create table user_info(name TEXT,password TEXT,primary key(name));";
	    rc = sqlite3_exec(db,sql_create_user,NULL,0,&err_msg);
	    rc = sqlite3_exec(db,sql_insert_user,NULL,0,&err_msg);
	    
	    if(rc != SQLITE_OK)
	    {
	        fprintf(stderr,"sql error: %s\n",err_msg);
                msg.action = REG_FAIL;
		//block_write(connectfd,&msg_ret,sizeof(msg_ret));
		write(connectfd,&msg_ret,sizeof(msg_ret));
		continue;
	    }
	    else if(rc == SQLITE_OK)
	    {
	    	printf("user registered\n");
	    	msg.action = REG_SUCCESS;
	    	//block_write(connectfd,&msg,sizeof(msg));
	    	write(connectfd,&msg,sizeof(msg));
	    }	
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
    pthread_t pid_msg = 1;
    pthread_t pid_listen = 0;
    pthread_mutex_init(&g_mutex,NULL);

    printf("accepting...\n");

    while(1)
    { 
        connectfd = accept(listenfd,(struct sockaddr*)&client,&addrlen);
	
	if(connectfd > 0)
	{
            printf("accept one connect\n");
	    int ret = pthread_create(&pid_listen,NULL,(void *)listen_pthread,&connectfd);
            if(0 != ret)
            {
	        perror(strerror(errno));
	        return PTH_CRET_ERR;
       	    }
	}
    }
    /*ret = pthread_create(&pid_msg,NULL,(void *)msg,NULL);
    if(0 != ret)
    {
	perror(strerror(errno));
	return PTH_CRET_ERR;
    }*/

    pthread_join(pid_listen,NULL);
    //pthread_join(pid_msg,NULL);
    printf("exit\n");
    close(listenfd);
    pthread_mutex_destory(&g_mutex);
    return 0;
}
   

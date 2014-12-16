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
#include <sqlite3.h>

#define MSG_MAX_LEN 100   //消息最大长度
#define NAME_LEN 20       //名字最大长度
#define PSWOD_LEN 20      //密码最大长度
#define PORT 9010         //端口号
#define PID_NUM 2         //线程id 
#define CMD_SIZE 10       //操作命令长度      
#define FILE_LEN 1000     //文件大小 

#define ECHOFLAGS (ECHO|ECHOE|ECHOK|ECHONL)

enum{
	LOG         = 1,
	LOG_SUCC    = 2,
	LOG_FAIL    = 3,
	PSWD_ERR    = 4,
	RE_LOG   	= 5,
	REGISTER 	= 6,
	REG_SUCC 	= 7,
	REG_FAIL    = 8,
	RE_REG      = 9,
	WHISPER     = 10,
	SEND_FAIL   = 11,
	USER_ERROR  = 12,
	OFFLINE   	= 13,
	CHAT        = 14,
	SHOW_ONLINE = 15,
	SHOW_ERR    = 16,
	QUIT_SYSTEM = 17,
	QUIT_FAIL   = 18,
	CREATE_FAIL = 19,
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
	CHAT_SUCC   = 32,
	TRAN        = 33,
	TRAN_SUCC   = 34,
	TRAN_FAIL   = 35,
	SLIENCE_SUCC= 36,
	SAY_SUCC    = 37,
    GETOUT_SUCC = 38
};

typedef struct msg_tag
{
	int action;                 //命令类型
    int id;                     //用户id
	char name[NAME_LEN];        //用户名
	char password[PSWOD_LEN];   //密码
	char target[NAME_LEN];      //接受方用户名
	char msg[MSG_MAX_LEN];      //消息内容
    char fname[NAME_LEN];       //文件名 
	int flen;                   //文件大小 
}msg_t;

int is_log = NO;                          //登录状态:未登录
int is_slience = NO;                      //是否被禁言:否
char present_user[NAME_LEN] = {0};        //当前用户名 
int id = 0;                               //当前id

void log_or_register()                    //登录界面
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

void after_log()             //登录后界面
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
	if(strcmp(present_user,"admin") == 0)          //是否是管理员
	{
		printf("     \t\t *  亲爱的管理员,你还可以对普通用户进行以下操作:      * \n");
		printf("     \t\t *         禁言:   slience                            * \n");
		printf("     \t\t *         解禁:   say                                * \n");
		printf("     \t\t *         踢人:   getout                             * \n");
	}	
	printf("     \t\t                                   当前用户: %s   \n",present_user);
	printf("     \t\t                                         id: %d  \n",id);
	printf("     \t\t **************************************************** \n");
}

int block_read(int fd,char *data,int len,char *fname)   //接收文件
{
	int read_size = 0;                                  //已读大小
	int n = 0;                                          //每次读取大小
	while(read_size < len)                                   
	{ 
		n = read(fd,data + read_size,len - read_size);                
		
		if(n < 0)
		{
			perror("read fail!\n");                     //读取错误
			break;
		}
		
		else if(0 == n)
		{
			perror("read 0\n");                         //无内容
			break;
		}
		
		read_size += n;
		printf("文件 %s 已接收 %d%,请耐心等待!\n",fname,(100 * read_size)/len);	
	}
	
	return 0;
}

int block_write(int sockfd,char *data,int file_len,char *fname)   //发送文件
{
	int write_size = 0;                                           //已发送大小
	int n = 0;                                                    //每次发送的大小
	
	while(write_size < file_len)
	{     
		n = write(sockfd,data + write_size,file_len - write_size);	
		write_size += n;
		printf("文件 %s 已传输 %d%,请耐心等待!\n",fname,(100 * write_size)/file_len);	
	}
	
	return 0;
}

int set_disp_mode(int fd,int option)        //取消密码回显
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

int getpasswd(char *passwd,int size)     //获取密码
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

void *thread_read(void *arg)         //接收服务器消息
{
	int sockfd = *((int *)arg);      //连接描述符
	msg_t msg;                       //结构体接收消息
	time_t ptime;                    //获取当前时间
	char pestime[50] = {0};          //当前时间

	sqlite3 *db = NULL;              //数据库
	char *err_msg = NULL;            //错误消息 

	int rc = sqlite3_open("chat_room.db",&db);           //打开数据库保存用户消息
	if(rc != SQLITE_OK)
	{
		printf("数据库异常,您的记录将无法保存!\n");	
	}

	while(1)
	{
		memset(&msg,0,sizeof(msg));                      //初始化
		
		int m = read(sockfd,&msg,sizeof(msg));           //读取服务器消息
		
        switch(msg.action)
		{	
			case LOG_SUCC:                               //登录成功
			{
				printf("\n登录成功!\n");
				is_log = YES;                            //登录状态: 已登录
                
                strcpy(present_user,msg.name);           //当前用户名
                id = msg.id;                             //当前id

                char sql_create_user_record[256] = {0};  //创建表保存消息
                sprintf(sql_create_user_record,"create table %s(id INTEGER,name TEXT,size TEXT,target TEXT,msg TEXT,time TEXT,primary key(size,time));",msg.name);
				int n = sqlite3_exec(db,sql_create_user_record,NULL,0,&err_msg);
                
                time(&ptime);
				strcpy(pestime,ctime(&ptime));          
 
  				char sql_log[256] = {0};                 //保存登录成功信息
				sprintf(sql_log,"insert into %s (id,name,size,time) values(%d,'%s','%s','%s');",present_user,msg.id,present_user,"登录成功",pestime);
                sqlite3_exec(db,sql_log,NULL,0,&err_msg);

				sleep(1);
				system("reset");
				after_log();                             //进入登录后页面
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
                if(msg.id == 666666)                            //管理员专用id
                {
                    printf("亲爱的管理员,该id为您专用id: 666666\n");    
                    break;
                }    
                    
                printf("您的id号为: %d,请您谨记!\n",msg.id);    //id号
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
				printf("\n你倒霉了,你惹到管理员了,现在你被禁言,不能发言了!\n");
				printf("你可以输入require请求管理员解禁!\n");

                time(&ptime);
				strcpy(pestime,ctime(&ptime));
 
				char sql_beslience[256] = {0};      //保存在数据库中
				sprintf(sql_beslience,"insert into %s(id,name,size,time) values(%d,'%s','%s','%s');",present_user,msg.id,present_user,"被管理员禁言",pestime);
                sqlite3_exec(db,sql_beslience,NULL,0,&err_msg);
				
				is_slience = YES;                   //禁言状态: 是
				break;
			}
			
			case SLIENCE_FAIL:                      //禁言用户失败    
			{
				printf("\n%s禁言失败!\n",msg.target);
                
				time(&ptime);
				strcpy(pestime,ctime(&ptime));

				char sql_slience[256] = {0};       
				sprintf(sql_slience,"insert into %s (id,name,size,target,time) values(%d,'%s','%s','%s','%s');",present_user,msg.id,present_user,"禁言失败",msg.target,pestime);
                sqlite3_exec(db,sql_slience,NULL,0,&err_msg);
				
				break;
			}
		    
			case SLIENCE_SUCC:                             //禁言用户成功
			{
			    printf("\n%s禁言成功!\n",msg.target);
				
				time(&ptime);
				strcpy(pestime,ctime(&ptime));               

				char sql_slience[256] = {0};
				sprintf(sql_slience,"insert into %s (id,name,size,target,time) values(%d,'%s','%s','%s','%s');",present_user,msg.id,present_user,"禁言成功",msg.target,pestime);
                sqlite3_exec(db,sql_slience,NULL,0,&err_msg);
				
				break;
			}
		    
			case SAY:                               //被解禁
			{
				if(is_slience == YES)               //未被禁言不显示 
				{
					printf("\n管理员大大原谅你了,你可以自由发言啦!\n");

					time(&ptime);
					strcpy(pestime,ctime(&ptime));

					char sql_say[256] = {0};
					sprintf(sql_say,"insert into %s (id,name,size,time) values(%d,'%s','%s','%s');",present_user,msg.id,present_user,"被解禁",pestime);
					sqlite3_exec(db,sql_say,NULL,0,&err_msg);

					is_slience = NO;
				}
				break;
			}
			
			case SAY_FAIL:                              //解禁用户失败
			{
				printf("\n%s解禁失败!\n",msg.target);
				
				time(&ptime);
				strcpy(pestime,ctime(&ptime));

				char sql_say[256] = {0};
				sprintf(sql_say,"insert into %s (id,name,size,target,time) values(%d,'%s','%s','%s','%s');",present_user,msg.id,present_user,"解禁失败",msg.target,pestime);
				sqlite3_exec(db,sql_say,NULL,0,&err_msg);
				break;
			}

			case SAY_SUCC:                              //解禁用户
			{
				printf("\n%s解禁成功!\n",msg.target);
				
				time(&ptime);
				strcpy(pestime,ctime(&ptime));

				char sql_say[256] = {0};
				sprintf(sql_say,"insert into %s (id,name,size,target,time) values(%d,'%s','%s','%s','%s');",present_user,msg.id,present_user,"解禁成功",msg.target,pestime);
				sqlite3_exec(db,sql_say,NULL,0,&err_msg);
				
				break;
			}
			
			case REQUIRE:                               //用户请求解禁
			{
				printf("\n%s请求解禁!\n",msg.name);
				
				time(&ptime);
				strcpy(pestime,ctime(&ptime));

				char sql_require[256] = {0};
				sprintf(sql_require,"insert into %s (id,name,size,time) values(%d,'%s','%s','%s');",present_user,msg.id,present_user,"请求解禁",pestime);
				sqlite3_exec(db,sql_require,NULL,0,&err_msg);
				
				break;
			}
			
			case REQUIRE_FAIL:
			{
				printf("\n请耐心等待!\n");
				break;
			}
			
			case GETOUT:                             //被踢出聊天室
			{
				printf("\n\n\n你已被管理员踢出聊天室!\n\n");
				
				time(&ptime);
				strcpy(pestime,ctime(&ptime));

				char sql_getout[256] = {0};
				sprintf(sql_getout,"insert into %s (id,name,size,time) values(%d,'%s','%s','%s');",present_user,msg.id,present_user,"被踢出聊天室",pestime);
				sqlite3_exec(db,sql_getout,NULL,0,&err_msg);
				
				char sql_logoff[256] = {0};
				sprintf(sql_logoff,"insert into %s (id,name,size,time) values(%d,'%s','%s','%s');",present_user,msg.id,present_user,"下线",pestime);
				sqlite3_exec(db,sql_logoff,NULL,0,&err_msg);
				
				sleep(1);
				system("reset");
				is_log = NO;                         //登录状态:未登录
				log_or_register();                   //登录页面
				break;
			}
			
			case GETOUT_FAIL:
			{
				printf("\n踢出失败,请重试!\n");
				
				time(&ptime);
				strcpy(pestime,ctime(&ptime));

				char sql_getout[256] = {0};
				sprintf(sql_getout,"insert into %s (id,name,size,time) values(%d,'%s','%s','%s');",present_user,msg.id,present_user,"踢出失败",pestime);
				sqlite3_exec(db,sql_getout,NULL,0,&err_msg);
				break;
			}
			
            case GETOUT_SUCC:                        //成功将用户踢出 
			{
				printf("\n您已将 %s 踢出聊天室!\n",msg.target);
				
				time(&ptime);
				strcpy(pestime,ctime(&ptime));

				char sql_getout[256] = {0};
				sprintf(sql_getout,"insert into %s (id,name,size,target,time) values(%d,'%s','%s','%s','%s');",present_user,msg.id,present_user,"踢出成功",msg.target,pestime);
				sqlite3_exec(db,sql_getout,NULL,0,&err_msg);
				break;
			}
			
            case WHISPER:                            //接收私聊消息
			{
				time(&ptime);
				printf("\n%s",ctime(&ptime));
				
				if(strcmp(msg.msg,"SaD") == 0)
				{
				    printf("%s对你做了一张哭脸\n",msg.name);	
				}	
				
				else if(strcmp(msg.msg,"SmIlE") == 0)
				{
				    printf("%s对你做了一张笑脸\n",msg.name);	
				}	
				
				else
				{	
					printf("%s悄悄对你说:%s\n",msg.name,msg.msg);
				}
				break;
			}
			
			case WHISPER_SUCC:                            //私聊消息发送成功
			{
				time(&ptime);
				printf("\n%s",ctime(&ptime));
				
				if(strcmp(msg.msg,"SaD") == 0)            //表情（苦脸）
				{
				    printf("你对%s做了一张哭脸\n",msg.target);	
					
					time(&ptime);
					strcpy(pestime,ctime(&ptime));

					char sql_getout[256] = {0};
					sprintf(sql_getout,"insert into %s (id,name,size,target,time) values(%d,'%s','%s','%s','%s');",present_user,msg.id,present_user,"做了一个哭脸",msg.target,pestime);
					sqlite3_exec(db,sql_getout,NULL,0,&err_msg);
				}	
				
				else if(strcmp(msg.msg,"SmIlE") == 0)     //表情（笑脸）
				{
				    printf("你对%s做了一张笑脸\n",msg.target);	
					
					time(&ptime);
					strcpy(pestime,ctime(&ptime));

					char sql_getout[256] = {0};
					sprintf(sql_getout,"insert into %s (id,name,size,target,time) values(%d,'%s','%s','%s','%s');",present_user,msg.id,present_user,"做了一个笑脸",msg.target,pestime);
					sqlite3_exec(db,sql_getout,NULL,0,&err_msg);
				}	
				
				else
				{	
					printf("你悄悄对%s说:%s\n",msg.target,msg.msg);     //普通消息
					
					time(&ptime);
					strcpy(pestime,ctime(&ptime));

					char sql_getout[256] = {0};
					sprintf(sql_getout,"insert into %s (id,name,size,target,msg,time) values(%d,'%s','%s','%s','%s');",present_user,msg.id,present_user,"私密",msg.target,msg.msg,pestime);
					sqlite3_exec(db,sql_getout,NULL,0,&err_msg);
				}
				
				break;
			}
			
			case SEND_FAIL:
			{
				printf("\n消息发送失败!\n");
				break;
			}	
			
			case USER_ERROR:                           
			{
				printf("\n该用户不存在!\n");
				break;
			}
			
			case OFFLINE:
			{
				printf("\n该用户离线!\n");
				break;
			}
			
			case CHAT:                                                 //接收群发消息
			{
				time(&ptime);
				printf("\n%s",ctime(&ptime));
				
				if(strcmp(msg.msg,"SaD") == 0)                         //表情（哭脸） 
				{
				    printf("%s对大家做了一张哭脸\n",msg.name);	
					
					time(&ptime);
					strcpy(pestime,ctime(&ptime));

					char sql_getout[256] = {0};
					sprintf(sql_getout,"insert into %s (id,name,size,target,time) values(%d,'%s','%s','%s','%s');",present_user,msg.id,present_user,"做了一个哭脸","所有在线用户",pestime);
					sqlite3_exec(db,sql_getout,NULL,0,&err_msg);
				}	
				
				else if(strcmp(msg.msg,"SmIlE") == 0)                  //表情（笑脸）
				{
				    printf("%s对大家做了一张笑脸\n",msg.name);	
					
					time(&ptime);
					strcpy(pestime,ctime(&ptime));

					char sql_getout[256] = {0};
					sprintf(sql_getout,"insert into %s (id,name,size,target,time) values(%d,'%s','%s','%s','%s');",present_user,msg.id,present_user,"做了一个笑脸","所有在线用户",pestime);
					sqlite3_exec(db,sql_getout,NULL,0,&err_msg);
				}	
				
				else
				{	
					printf("%s对大家说:%s\n",msg.name,msg.msg);	
					
					time(&ptime);
					strcpy(pestime,ctime(&ptime));

					char sql_getout[256] = {0};
					sprintf(sql_getout,"insert into %s (id,name,size,target,msg,time) values(%d,'%s','%s','%s','%s','%s');",present_user,msg.id,present_user,"群发","所有在线用户",msg.msg,pestime);
					sqlite3_exec(db,sql_getout,NULL,0,&err_msg);
				}
				
				break;
			}
			
			case CHAT_SUCC:                                    //群发消息发送成功
			{
				time(&ptime);
				printf("\n%s",ctime(&ptime));
				
				if(strcmp(msg.msg,"SaD") == 0)
				{
				    printf("你对大家做了一张哭脸\n");	       //表情（哭脸）
					
					time(&ptime);
					strcpy(pestime,ctime(&ptime));

					char sql_getout[256] = {0};
					sprintf(sql_getout,"insert into %s (id,name,size,target,time) values(%d,'%s','%s','%s','%s');",present_user,msg.id,present_user,"做了一个哭脸","所有在线用户",pestime);
					sqlite3_exec(db,sql_getout,NULL,0,&err_msg);
				}	
				
				else if(strcmp(msg.msg,"SmIlE") == 0)          //表情（笑脸）
				{
				    printf("你对大家做了一张笑脸\n");	
					
					time(&ptime);
					strcpy(pestime,ctime(&ptime));

					char sql_getout[256] = {0};
					sprintf(sql_getout,"insert into %s (id,name,size,target,time) values(%d,'%s','%s','%s','%s');",present_user,msg.id,present_user,"做了一个笑脸","所有在线用户",pestime);
					sqlite3_exec(db,sql_getout,NULL,0,&err_msg);
				}	
				
				else
				{
					printf("你对大家说:%s\n",msg.msg);
					
					time(&ptime);
					strcpy(pestime,ctime(&ptime));

					char sql_getout[256] = {0};
					sprintf(sql_getout,"insert into %s (id,name,size,target,msg,time) values(%d,'%s','%s','%s','%s','%s');",present_user,msg.id,present_user,"群发","所有在线用户",msg.msg,pestime);
					sqlite3_exec(db,sql_getout,NULL,0,&err_msg);
				}
				
				break;
			}
			
			case SHOW_ONLINE:                   //查询在线用户
			{
				printf("\n在线用户:");
				printf("  %s\n",msg.msg);
				break;
			}
			
			case SHOW_ERR:                      //查询失败
			{
				printf("\nshow 失败!\n");
				break;
			}
			
			case TRAN:                                                //接收文件 
      		{
    			char *data = NULL;                                    //开辟空间
				data = (char *)malloc(msg.flen + 1);
				memset(data,0,msg.flen + 1);
		        
				block_read(sockfd,data,msg.flen,msg.fname);           //接收文件
				
				int fd = open("rec",O_RDWR|O_CREAT|O_TRUNC,0666);     //保存在rec文件中
				
				if(fd < 0)
				{
				    printf("\n文件接收失败!\n");	
				}
				
				write(fd,data,msg.flen);                              //发送接收成功信息 
				printf("\n文件 %s 接收成功!\n",msg.fname);

				time(&ptime);
				strcpy(pestime,ctime(&ptime));

				char sql_getout[256] = {0};                           //存入数据库  
				sprintf(sql_getout,"insert into %s (id,name,size,target,time) values(%d,'%s','%s','%s','%s');",present_user,msg.id,present_user,"接收文件",msg.target,pestime);
				sqlite3_exec(db,sql_getout,NULL,0,&err_msg);

				close(fd);
				free(data);
			    break; 	
			}
			
			case TRAN_FAIL:                   //文件发送失败 
			{
				printf("\n文件传送失败!\n");
				break;
			}
			
			case TRAN_SUCC:                   //文件发送成功
			{
				printf("\n文件传送成功!\n");

				time(&ptime);
				strcpy(pestime,ctime(&ptime));

				char sql_getout[256] = {0};
				sprintf(sql_getout,"insert into %s (id,name,size,target,time) values(%d,'%s','%s','%s','%s');",present_user,msg.id,present_user,"发送文件",msg.target,pestime);
				sqlite3_exec(db,sql_getout,NULL,0,&err_msg);
				break;
			}

			case QUIT_SYSTEM:              //退出系统
			{
				close(sockfd);
				return NULL;
			}
			
			case QUIT_FAIL:                //退出系统失败
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

void *thread_write(void *arg)                  //像服务器发送命令 
{
	char *p;                               
	int sockfd = *((int *)arg);                //连接描述符
	char cmd[CMD_SIZE] = {0};                  // 操作命令
	char target[NAME_LEN] = {0};               //消息发送对象
	char message[MSG_MAX_LEN] = {0};           //消息内容
	char password[PSWOD_LEN] = {0};            //密码
	char passwd[PSWOD_LEN] = {0};              //确认密码
	msg_t msg;                                 //结构体
	time_t ptime;                              //获取时间 
	
	system("reset");
	log_or_register();                         //登录页面
	
	while(1)
	{
		sleep(1);
		scanf("%s",cmd);                       //操作命令
		
		if(strcmp(cmd,"log") == 0)             //登录
		{
			if(is_log == NO)                   //未登录
			{	
                int id = 0;
				printf("请输入id:");
                scanf("%d",&(msg.id));
				getchar();
				printf("请输入密码:");
				
				set_disp_mode(STDIN_FILENO,0);                 //取消密码回显
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
                printf("send log info\n");
		    }
			
			else
			{
			    printf("\n您已登录!\n");	
			}	
			
			continue;
		}

		else if(strcmp(cmd,"register") == 0)                 //注册
		{
			printf("请输入帐号:");
			scanf("%s",msg.name);
			getchar();
			printf("请输入密码:");
			set_disp_mode(STDIN_FILENO,0);                   //取消密码回显
			getpasswd(password,sizeof(password));
			p = password;
			
			while(*p != '\n')
			{
				p++;	
			}	
			*p = '\0';
			
			printf("\n请重新输入密码:");
			getpasswd(passwd,sizeof(passwd));
			p = passwd;
			while(*p != '\n')
			{
			    p++;	
			}	
			*p = '\0';
			
			set_disp_mode(STDIN_FILENO,1);
			
			if(strcmp(password,passwd) != 0)                   //密码不一致        
			{
				printf("\n两次输入的密码不一致!\n");
				continue;
			}	
			
			msg.action = REGISTER;
			strcpy(msg.password,passwd);
			write(sockfd,&msg,sizeof(msg));
			continue;
		}

		else if(strcmp(cmd,"whsp") == 0)                       //私密
		{
			if(is_log == YES && is_slience == NO)
			{	
				printf("请输入接收的用户:");
				scanf("%s",msg.target);
				printf("\n请输入消息内容:");
				getchar();
				gets(msg.msg);
				
				if(strcmp(msg.msg,":)") == 0)                  //表情符转换
				{
					strcpy(msg.msg,"SmIlE");	
				}	
				
				else if(strcmp(msg.msg,":(") == 0)
				{
					strcpy(msg.msg,"SaD");	
                }
				
				msg.id = id;
                msg.action = WHISPER;
				strcpy(msg.name,present_user);
				write(sockfd,&msg,sizeof(msg));
			}
			
			else if(is_log == NO)
			{
			    printf("\n请先登录!\n");	
			}
			
			else if(is_log == YES && is_slience == YES)              //被禁言无法发送
			{
			    printf("\n你已被禁言!\n");	
				printf("你可以输入require请求管理员解禁!\n");
			}	
			
			continue;
		}

		else if(strcmp(cmd,"chat") == 0)
		{
			if(is_log == YES && is_slience == NO)             //已登录且未被禁言
			{	
				strcpy(msg.name,present_user);
				printf("请输入消息内容:");
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
				
                msg.id =id;
				msg.action = CHAT;
				write(sockfd,&msg,sizeof(msg));
			}
			
			else if(is_log == NO)                          //未登录
			{
				printf("\n请先登录!\n");	
			}
			
			else if(is_log == YES && is_slience == YES)    //被禁言
			{
			    printf("\n你已被禁言!\n");	
				printf("你可以输入require请求管理员解禁!\n");
			}	
			continue;
		}

		else if(strcmp(cmd,"show") == 0)                   //查询在线用户
		{
			if(is_log == YES)
			{	
				msg.id = id;
                msg.action = SHOW_ONLINE;
				write(sockfd,&msg,sizeof(msg));
	    	}
			
			else
			{
				printf("\n请先登录!\n");                   //未登录
			}
		}

		else if(strcmp(cmd,"quit") == 0)                   //请求退出系统
		{
			msg.id = id;
            msg.action = QUIT_SYSTEM;
            strcpy(msg.name,present_user);
			write(sockfd,&msg,sizeof(msg));
			close(sockfd);
			return NULL;
		}
		
		else if(strcmp(cmd,"table") == 0)                  //返回页面 
		{
			system("reset");
			after_log();
			continue;
		}
		
		else if(strcmp(cmd,"slience") == 0)                //禁言
		{
			if(strcmp(present_user,"admin") == 0)
			{	
				msg.id = id;
                msg.action = SLIENCE;
				printf("请输入想要禁言的用户:");
				scanf("%s",msg.target);
				
				if(strcmp(msg.target,"admin") == 0)        //不能禁言自己
				{
				    printf("\n亲爱的,要对自己好一点啊!\n");	
				}	
				
				else 
				{	
					write(sockfd,&msg,sizeof(msg));
			    }
			}
			
			else                                           //非管理员 
			{
				printf("\n你不是管理员,没有此项操作权利!\n");
		    }
			continue;
		}

		else if(strcmp(cmd,"require") == 0)                //请求解禁
		{
			if(is_slience == YES && is_log == YES)
			{	
				msg.id = id;
                msg.action = REQUIRE;
				strcpy(msg.target,"admin");
				write(sockfd,&msg,sizeof(msg));
				printf("\n管理员大大我错了,放我出去...\n");
			}
			
			else if(is_slience == NO)                      //没有被禁言
			{
				printf("\n莫非你干了坏事管理员大大没有发现,你还没被禁言纳!\n");
			}
			
			else if(is_log == NO)                          //未登录
			{
			    printf("\n亲,你还没有登录哦!\n");	
			}	
			continue;
		}
		
		else if(strcmp(cmd,"say") == 0)                     //解禁
		{
			if(strcmp(present_user,"admin") == 0)
			{	
				msg.id = id;
                msg.action = SAY;
				printf("请输入要解禁的用户:");
				scanf("%s",msg.target);
				write(sockfd,&msg,sizeof(msg));
			}
			
			else                                            //非管理员
			{
				printf("\n你不是管理员,没有此项操作权利!\n");
		    }
			continue;
		}
		
		else if(strcmp(cmd,"getout") == 0)                  //踢出聊天室
		{
			if(strcmp(present_user,"admin") == 0)
			{	
				printf("请输入将要被踢出的用户:");
				scanf("%s",msg.target);
				msg.id = id;
                msg.action = GETOUT;
				write(sockfd,&msg,sizeof(msg));
			}
			
			else                                            //非管理员  
			{
				printf("\n你不是管理员,没有此项操作权利!\n");
		    }
			continue;
		}
		
		else if(strcmp(cmd,"tran") == 0)                    //发送文件
		{
			if(is_log == YES)
			{	
				printf("请输入文件传输的对象:");
				scanf("%s",msg.target);
				printf("\n请输入要传输的文件:");
				scanf("%s",msg.fname);
				int fd = open(msg.fname,O_RDONLY);
				
				if(fd < 0)
				{
					printf("\n文件打开失败,请检查文件是否正常!\n");	  //打开文件失败
				}	
                
				else
				{	
					msg.flen = lseek(fd,0,SEEK_END);
					lseek(fd,0,SEEK_SET);
					
                    msg.id = id;
					msg.action = TRAN;
					write(sockfd,&msg,sizeof(msg));               //向接收方发送文件的大小
					
					char *data = NULL;
					data = (char *)malloc(msg.flen + 1);
					memset(data,0,msg.flen + 1);
					
					int m = read(fd,data,msg.flen);
					
					if(m < msg.flen)
					{
						printf("\n文件未能读取完整,请重试!\n");   
						continue;
					}	
					
					block_write(sockfd,data,msg.flen,msg.fname);  //发送文件
					
					free(data);
					close(fd);
				}
			}
			
			else if(is_log == NO)
			{
				printf("\n你还没登录哦亲!\n");                    //未登录
		    }
			
			continue;
		}

		else
		{
			printf("\n未知命令!请检查后再输入!\n");               //未知命令
			continue;
		}
	}

	return NULL;
}

int main(int argc,char *argv[])
{
	system("reset");

	struct sockaddr_in server;                                        //设定ip与端口
	memset(&server,0,sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons(PORT);
	server.sin_addr.s_addr = inet_addr("127.0.0.1");

	char cmd[CMD_SIZE] = {0};
	int log_return = -1;
	int sockfd = socket(PF_INET,SOCK_STREAM,0);
	int n = connect(sockfd,(struct sockaddr *)&server,sizeof(server));     //连接服务器
    
	if(n < 0)                                                              //连接失败  无法运行
	{
	    printf("未连接至服务器!\n");
		close(sockfd);
		return 0;
	}

	pthread_t pid_write = 0;                  //发送指令线程id         
	pthread_t pid_read = 1;                   //接收消息线程id 

	int ret = pthread_create(&pid_write,NULL,(void *)thread_write,&(sockfd));   //创建发送指令线程
	if(0 != ret)
	{
		perror(strerror(ret));
		return CREATE_FAIL;
	}

	ret = pthread_create(&pid_read,NULL,(void *)thread_read,&(sockfd));         //创建接收消息线程  
	if(0 != ret)
	{
		perror(strerror(ret));
		return CREATE_FAIL;
	}

	pthread_join(pid_write,NULL);                                               //等待线程结束
	pthread_join(pid_read,NULL);                                                 

	system("reset");
	printf("感谢您的使用,再见!\n");
	close(sockfd);
	return 0;
}

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

#define MSG_MAX_LEN 100               //最大消息长度
#define NAME_LEN 20                   //最大用户名长度 
#define PSWOD_LEN 20                  //最大密码长度
#define PORT 9010                     //端口号  
#define PID_NUM 2                     //线程id
#define CMD_SIZE 10                   //操作命令  
#define QUEUE_LEN 10                  //等候队列最大长度  
#define MAX_LINE 80                   //

enum{ 
    LOG         = 1,
    LOG_SUCC    = 2,
    LOG_FAIL    = 3,
    PSWD_ERR    = 4,
    RE_LOG      = 5,
    REGISTER    = 6,
    REG_SUCC    = 7,
    REG_FAIL    = 8,
    RE_REG      = 9,
    WHISPER     = 10,
    SEND_FAIL   = 11,
    USER_ERROR  = 12,
    OFFLINE   	= 13,
    CHAT      	= 14,
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
    GETOUT_SUCC  =38,
    ERROR        =39,
    SUCC         =40  
};

typedef struct msg_tag
{
    int action;                      //指令类型
    int id;                          //id  
    char name[NAME_LEN];             //用户名
    char password[PSWOD_LEN];        //密码
    char target[NAME_LEN];           //接收方用户名
    char msg[MSG_MAX_LEN];           //消息内容
    char fname[NAME_LEN];            //文件名
    int flen;                        //文件大小
}msg_t;

char str_return[100] = {0};                          //callback返回值 

int block_read(int fd,char *data,int len)            //接收文件  
{
    int read_size = 0;                               //已接收大小
    int n = 0;                                       //每次接收的大小

    while(read_size < len)
    {
        n = read(fd,data + read_size,len - read_size);

        if(n < 0)
        {
            perror("read fail!\n");                  //接收失败
            break;
        }

        else if(0 == n)
        {
            perror("read 0\n");                      //无数据
            break;
        }

        read_size += n;
    }
    return 0;
}

int block_write(int fd,char *data,int len)                    //发送文件
{
    int write_size = 0;                                       //已发送大小 
    int n = 0;                                                //当前发送的大小   

    while(write_size < len)
    {
        n = write(fd,data + write_size,len - write_size);

        if(n < 0)
        {
            //perror("write fail!\n");                        //发送失败
            break;
        }

        else if(0 == n)
        {
            //perror("write 0\n");                            //无数据
            break;
        }

        write_size += n;
    }
    return 0;
}

static int call_back(void * not_used,int argc,char **argv,char **col_name)  //调用函数查找表内信息
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

int msg_handle(msg_t *msg,int connectfd)                     //处理客户端信息      
{
    time_t ptime;                                            //获取时间
    char pestime[30] = {0};                                  //当前时间 
    
    sqlite3 *db = NULL;                                      //数据库  
    char *err_msg = NULL;                                    //错误信息
    int tgt_confd = 0;                                       //接收方connectfd
    
    int id = 0;                                              //生成的id号 

    int rc = sqlite3_open("chat_room.db",&db);               //打开数据库
    if(rc != SQLITE_OK)
    {
        fprintf(stderr,"open database failed %s\n",sqlite3_errmsg(db));
    }

    if(LOG == msg->action)                                         //指令:登录
    {
        char sql_log[256] = {0};                                   //该用户是否存在  
        sprintf(sql_log, "select password from user_info where id = %d;", msg->id); 
        memset(str_return, 0, sizeof(str_return));
        int rc = sqlite3_exec(db, sql_log, call_back, 0, &err_msg);

        if(rc != SQLITE_OK)
        {
            msg->action = LOG_FAIL;
            write(connectfd,msg,sizeof(msg_t));                    //查找失败
            return ERROR;
        }

        if(0 == strlen(str_return))                                //用户不存在 
        {
            msg->action = USER_ERROR;
            write(connectfd,msg,sizeof(msg_t));
            return ERROR;
        }

        if((0 != strncmp(str_return, msg->password, strlen(str_return) - 1)) || (strlen(msg->password) != (strlen(str_return) - 1)))
        {
            msg->action = PSWD_ERR;                                //密码错误  
            write(connectfd,msg,sizeof(msg_t));
            return ERROR;
        }

        sprintf(sql_log, "select * from log_info where id = %d;", msg->id);   //是否已登录
        memset(str_return, 0, sizeof(str_return));
        rc = sqlite3_exec(db, sql_log, call_back, 0, &err_msg);

        if(rc != SQLITE_OK)                                                   //查找失败
        {
            msg->action = LOG_FAIL;
            write(connectfd,msg,sizeof(msg_t));
            return ERROR;
        }

        if(0 != strlen(str_return))
        {
            msg->action = RE_LOG;
            write(connectfd,msg,sizeof(msg_t));                               //用户已登录 
            return ERROR;
        }

        sprintf(sql_log, "select name from user_info where id = %d;", msg->id);   //该id的用户名 
        memset(str_return, 0, sizeof(str_return));
        rc = sqlite3_exec(db, sql_log, call_back, 0, &err_msg);
        
        memset(msg->name,0,sizeof(msg->name));
        strncpy(msg->name,str_return,strlen(str_return) - 1);                   
        
        sprintf(sql_log,"insert into log_info(id,name,connectfd) values(%d,'%s',%d);",msg->id,msg->name,connectfd); 
        rc = sqlite3_exec(db,sql_log,NULL,0,&err_msg);                            //插入已登录用户表

        if(rc != SQLITE_OK)
        {
            msg->action = LOG_FAIL;
            write(connectfd,msg,sizeof(msg_t));                                   //绑定connectfd失败
            return ERROR;
        }
        
        msg->action = LOG_SUCC;                                                   //返回登录成功信息
        write(connectfd,msg,sizeof(msg_t));

        time(&ptime);
        strcpy(pestime,ctime(&ptime));                                            //获取时间
        char sql_log_record[256];
        sprintf(sql_log_record, "insert into record (id,name,size,time) values(%d,'%s','%s','%s');",msg->id,msg->name,"登录",pestime); 

        rc = sqlite3_exec(db, sql_log_record, NULL, 0, &err_msg);                  //保存记录
        return SUCC;
    }

    else if(REGISTER == msg->action)                    //指令:注册
    {   
        strcpy(str_return,"hello");                     //随意赋值,使不为空 

        while(0 != strlen(str_return))                   
        {
            int i = 0;
            int num = 0;
            id = 0;                                     //id 
            srand((unsigned)time(NULL));                //随机生成数字
            for(i = 0;i < 8;i++)
            {
                num = (rand()%10);
                id = id * 10 + num;
            }

            char sql_id[256] = {0};                     //该id是否已存在
            sprintf(sql_id, "select name from user_info where id = %d;",id); 
            memset(str_return, 0, sizeof(str_return));
            rc = sqlite3_exec(db, sql_id, call_back, 0, &err_msg);
        }
        
        if(strcmp(msg->name,"admin") == 0)              //管理员专属id 
        {
            id = 666666;    
        }    
        
        char sql_register[256] = {0};                   //保存资料
        sprintf(sql_register, "insert into user_info(id,name,password) values(%d,'%s','%s');",id,msg->name,msg->password); 
        int rc = sqlite3_exec(db, sql_register, NULL, 0, &err_msg);
        
        if(rc == SQLITE_CONSTRAINT)                     //已被注册   
        {
            msg->action = RE_REG;
            write(connectfd,msg,sizeof(msg_t));
            return ERROR;
        }

        else if(rc != SQLITE_OK)                        //查找失败
        {
            msg->action = REG_FAIL;
            write(connectfd,msg,sizeof(msg_t));
            return ERROR;
        }

        msg->action = REG_SUCC;                         //返回注册成功与id号  
        msg->id = id;
        write(connectfd,msg,sizeof(msg_t));
        
        time(&ptime);
        strcpy(pestime,ctime(&ptime));
        
        char sql_register_record[256];                   //保存记录
        sprintf(sql_register_record, "insert into record (id,name,size,time) values(%d,'%s','%s','%s');",msg->id,msg->name,"注册",pestime); 
        rc = sqlite3_exec(db, sql_register_record, NULL, 0, &err_msg);
        return SUCC;
    }	

    else if(SHOW_ONLINE == msg->action)
    {
        char * sql_show = "select name from log_info;";                //查找所有在线用户  
        memset(str_return, 0, sizeof(str_return));
        int rc = sqlite3_exec(db, sql_show, call_back, 0, &err_msg);    

        if(rc != SQLITE_OK)                                            //查找失败
        {
            msg->action = SHOW_ERR;
            write(connectfd,msg,sizeof(msg_t));
            return ERROR;
        }

        strcpy(msg->msg,str_return);
        msg->action = SHOW_ONLINE;
        write(connectfd,msg,sizeof(msg_t));                             //返回在线用户
        return SUCC;
    }	

    else if(TRAN == msg->action)                                        //指令:发送文件
    {
        char sql_tran[256];                                             //接收用户是否存在
        sprintf(sql_tran, "select name from user_info where name = '%s';", msg->target); 

        memset(str_return, 0, sizeof(str_return));
        int rc = sqlite3_exec(db, sql_tran, call_back, 0, &err_msg);

        if(rc != SQLITE_OK)                                             //查找失败
        {
            msg->action = TRAN_FAIL;
            write(connectfd,msg,sizeof(msg_t));
            //continue;
            return ERROR;
        }

        if(0 == strlen(str_return))                                     //用户不存在
        {
            msg->action = USER_ERROR;
            write(connectfd,msg,sizeof(msg_t));
            return ERROR;
        }

        sprintf(sql_tran, "select connectfd from log_info where name = '%s';", msg->target); //查找id
        memset(str_return, 0, sizeof(str_return));
        rc = sqlite3_exec(db,sql_tran,call_back,0,&err_msg);

        if(rc != SQLITE_OK)                                                 //查找失败
        {
            msg->action = TRAN_FAIL;
            write(connectfd,msg,sizeof(msg_t));                    
            return ERROR;
        }

        if(0 == strlen(str_return))                                         //用户不在线 
        {
            msg->action = OFFLINE;
            write(connectfd,msg,sizeof(msg_t));
            return ERROR;
        }

        char *data = NULL;                                                  //分配空间接收文件
        data = (char *)malloc(msg->flen + 1);                          
        memset(data,0,msg->flen + 1);

        block_read(connectfd,data,msg->flen);                               //接收 

        tgt_confd = atoi(str_return);                                       //转发给接收方 
        msg->action = TRAN;
        write(tgt_confd,msg,sizeof(msg_t));
        block_write(tgt_confd,data,msg->flen);

        free(data);

        msg->action = TRAN_SUCC;                                            //返回发送成功 
        write(connectfd,msg,sizeof(msg_t));                                 
        
        time(&ptime);                                                       //保存记录
        strcpy(pestime,ctime(&ptime));
        char sql_tran_record[256];
        sprintf(sql_tran_record, "insert into record (name,size,target,time) values('%s','%s','%s','%s');",msg->name,"发送文件",msg->target,pestime); 

        rc = sqlite3_exec(db, sql_tran_record, NULL, 0, &err_msg);
        return SUCC;
    }	

    else if(WHISPER == msg->action)                                         //指令:私密
    {
        char sql_whisper[256];                                              //用户是否存在   
        sprintf(sql_whisper, "select name from user_info where name = '%s';", msg->target); 
        memset(str_return, 0, sizeof(str_return));
        int rc = sqlite3_exec(db, sql_whisper, call_back, 0, &err_msg);

        if(rc != SQLITE_OK)                                                 //查找失败
        {
            msg->action = SEND_FAIL;
            write(connectfd,msg,sizeof(msg_t));
            return ERROR;
        }

        if(0 == strlen(str_return))                                         //用户不存在
        {
            msg->action = USER_ERROR;
            write(connectfd,msg,sizeof(msg_t));
            return ERROR;
        }

        sprintf(sql_whisper, "select connectfd from log_info where name = '%s';", msg->target); //查找id 
        memset(str_return, 0, sizeof(str_return));
        rc = sqlite3_exec(db, sql_whisper, call_back, 0, &err_msg);

        if(rc != SQLITE_OK)                                //查找失败                     
        {
            msg->action = SEND_FAIL;
            write(connectfd,msg,sizeof(msg_t));
            return ERROR;
        }

        if(0 == strlen(str_return))                        //不在线 
        {
            msg->action = OFFLINE;
            write(connectfd,msg,sizeof(msg_t));
            return ERROR;
        }
        
        tgt_confd = atoi(str_return);                      //发送给接收方
        write(tgt_confd,msg,sizeof(msg_t));

        msg->action = WHISPER_SUCC;                        //返回发送成功
        write(connectfd,msg,sizeof(msg_t));

        time(&ptime);                                      //保存记录
        strcpy(pestime,ctime(&ptime));                      
        char sql_whsp_record[256];
        sprintf(sql_whsp_record, "insert into record (id,name,size,target,msg,time) values(%d,'%s','%s','%s','%s','%s');",msg->id,msg->name,"私密",msg->target,msg->msg,pestime); 

        rc = sqlite3_exec(db, sql_whsp_record, NULL, 0, &err_msg);
        return SUCC;
    }	

    else if(CHAT == msg->action)                                      //指令:群发
    {
        int i = 0;
        int count = 0;
        char temp[4];

        char * sql_chat = "select connectfd from log_info;";         //查找所有在线用户

        memset(str_return, 0, sizeof(str_return));
        int rc = sqlite3_exec(db, sql_chat, call_back, 0, &err_msg);

        if(rc != SQLITE_OK)                                          //查找失败
        {
            msg->action = SEND_FAIL;
            write(connectfd,msg,sizeof(msg_t));
            return ERROR;
        }

        msg->action = CHAT;

        while(0 != *(str_return + count))                           //读取每一个connectfd
        {
            if('\t' == *(str_return + count))
            {
                tgt_confd = atoi(temp);
                if(tgt_confd != connectfd)                          //不能给自己发送 
                {
                    write(tgt_confd,msg,sizeof(msg_t));
                }
                memset(temp, 0, sizeof(temp));                      //初始化   
                i = 0;
            }
            strncpy(temp + i, str_return + count, 1);
            i++;
            count++;
        }
        msg->action = CHAT_SUCC;                                    //返回发送成功    
        write(connectfd,msg,sizeof(msg_t));

        time(&ptime);                                               //保存记录 
        strcpy(pestime,ctime(&ptime)); 
        char sql_chat_record[256];
        sprintf(sql_chat_record, "insert into record (name,size,target,msg,time) values('%s','%s','%s','%s','%s');",msg->name,"群发","all user",msg->msg,pestime); 

        rc = sqlite3_exec(db, sql_chat_record, NULL, 0, &err_msg);
        return SUCC;
    }	

    else if(SLIENCE == msg->action)                                 //指令: 禁言
    {
        char sql_slience[256];                                      //用户是否存在      
        sprintf(sql_slience, "select name from user_info where name = '%s';", msg->target); 
        memset(str_return, 0, sizeof(str_return));
        int rc = sqlite3_exec(db, sql_slience, call_back, 0, &err_msg);

        if(rc != SQLITE_OK)                                         //查找失败
        {
            msg->action = SLIENCE_FAIL;
            write(connectfd,msg,sizeof(msg_t));
            return ERROR;
        }

        if(0 == strlen(str_return))                                 //用户不存在
        {
            msg->action = USER_ERROR;
            write(connectfd,msg,sizeof(msg_t));
            return ERROR;
        }

        sprintf(sql_slience, "select connectfd from log_info where name = '%s';", msg->target); //查找id
        memset(str_return, 0, sizeof(str_return));
        rc = sqlite3_exec(db, sql_slience, call_back, 0, &err_msg);

        if(rc != SQLITE_OK)                                         //查找失败
        {
            msg->action = SLIENCE_FAIL;
            write(connectfd,msg,sizeof(msg_t));
            return ERROR;
        }

        if(0 == strlen(str_return))                                 //不在线
        {
            msg->action = OFFLINE;
            write(connectfd,msg,sizeof(msg_t));
            return ERROR;
        }

        tgt_confd = atoi(str_return);                               //发送禁言指令
        msg->action = SLIENCE;
        write(tgt_confd,msg,sizeof(msg_t));
        
        msg->action = SLIENCE_SUCC;                                 //返回禁言成功
        write(connectfd,msg,sizeof(msg_t));

        time(&ptime);                                               //保存记录
        strcpy(pestime,ctime(&ptime));
        char sql_slience_record[256];
        sprintf(sql_slience_record, "insert into record (id,name,size,target,time) values(%d,'%s','%s','%s','%s');",msg->id,msg->name,"禁言",msg->target,pestime); 
        rc = sqlite3_exec(db, sql_slience_record, NULL, 0, &err_msg);
        return SUCC;
    }	

    else if(REQUIRE == msg->action)                                 //指令: 请求解禁
    {
        char sql_require[256];                                      //管理员是否存在
        sprintf(sql_require, "select name from user_info where name = '%s';", msg->target); 
        memset(str_return, 0, sizeof(str_return));
        int rc = sqlite3_exec(db, sql_require, call_back, 0, &err_msg);

        if(rc != SQLITE_OK)                                         //查找失败
        {
            msg->action = REQUIRE_FAIL;
            write(connectfd,msg,sizeof(msg_t));
            return ERROR;
        }

        if(0 == strlen(str_return))                                 //管理员不存在
        {
            msg->action = USER_ERROR;
            write(connectfd,msg,sizeof(msg_t));
            return ERROR;
        }

        sprintf(sql_require, "select connectfd from log_info where name = '%s';", msg->target); 
        memset(str_return, 0, sizeof(str_return));                  //查找connectfd 
        rc = sqlite3_exec(db, sql_require, call_back, 0, &err_msg);

        if(rc != SQLITE_OK)                                         //查找失败
        {
            msg->action = REQUIRE_FAIL;
            write(connectfd,msg,sizeof(msg_t));
            return ERROR;
        }

        if(0 == strlen(str_return))                                 //不在线
        {
            msg->action = OFFLINE;
            write(connectfd,msg,sizeof(msg_t));
            return ERROR;
        }

        tgt_confd = atoi(str_return);                               //向管理员发送提示
        msg->action = REQUIRE;
        write(tgt_confd,msg,sizeof(msg_t));                    

        time(&ptime);                                               //保存记录  
        strcpy(pestime,ctime(&ptime));
        char sql_require_record[256];
        sprintf(sql_require_record, "insert into record (id,name,size,time) values(%d,'%s','%s','%s');",msg->id,msg->name,"请求解禁",pestime); 
        rc = sqlite3_exec(db, sql_require_record, NULL, 0, &err_msg);
        return SUCC;
    }	

    else if(SAY== msg->action)                                      //指令: 解禁 
    {
        char sql_say[256];                                          //用户是否存在 
        sprintf(sql_say, "select name from user_info where name = '%s';", msg->target); 
        memset(str_return, 0, sizeof(str_return));
        int rc = sqlite3_exec(db, sql_say, call_back, 0, &err_msg);

        if(rc != SQLITE_OK)                                         //查找失败   
        {
            msg->action = SAY_FAIL;
            write(connectfd,msg,sizeof(msg_t));
            return ERROR;
        }

        if(0 == strlen(str_return))                                 //不在线 
        {
            msg->action = USER_ERROR;
            write(connectfd,msg,sizeof(msg_t));
            return ERROR;
        }

        sprintf(sql_say, "select connectfd from log_info where name = '%s';", msg->target); 
        memset(str_return, 0, sizeof(str_return));                  //查找connectfd 
        rc = sqlite3_exec(db, sql_say, call_back, 0, &err_msg);

        if(rc != SQLITE_OK)                                         //查找失败
        {
            msg->action = SAY_FAIL;
            write(connectfd,msg,sizeof(msg_t));
            return ERROR;
        }

        if(0 == strlen(str_return))                                 //不在线
        {
            msg->action = OFFLINE;
            write(connectfd,msg,sizeof(msg_t));
            return ERROR;
        }

        tgt_confd = atoi(str_return);                               //向接收发发送指令
        msg->action = SAY;
        write(tgt_confd,msg,sizeof(msg_t));

        msg->action = SAY_SUCC;                                     //返回解禁成功
        write(connectfd,msg,sizeof(msg_t));
        
        time(&ptime);
        strcpy(pestime,ctime(&ptime));

        char sql_say_record[256];                                   //保存记录
        sprintf(sql_say_record, "insert into record (id,name,size,target,time) values(%d,'%s','%s','%s','%s');",msg->id,msg->name,"解禁",msg->target,pestime); 
        rc = sqlite3_exec(db, sql_say_record, NULL, 0, &err_msg);
        return SUCC;
    }	

    else if(GETOUT == msg->action)                        //指令: 踢出聊天室
    {
        char sql_getout[256];                             //用户是否存在
        sprintf(sql_getout, "select name from user_info where name = '%s';", msg->target); 
        memset(str_return, 0, sizeof(str_return));
        int rc = sqlite3_exec(db, sql_getout, call_back, 0, &err_msg);

        if(rc != SQLITE_OK)                               //查找失败 
        {
            msg->action = GETOUT_FAIL;
            write(connectfd,msg,sizeof(msg_t));
            return ERROR;
        }

        if(0 == strlen(str_return))                       //不在线
        {
            msg->action = USER_ERROR;
            write(connectfd,msg,sizeof(msg_t));
            return ERROR;
        }

        sprintf(sql_getout, "select connectfd from log_info where name = '%s';", msg->target); 
        memset(str_return, 0, sizeof(str_return));                    //查找connectfd
        rc = sqlite3_exec(db, sql_getout, call_back, 0, &err_msg);

        if(rc != SQLITE_OK)                                           //查找失败
        {
            msg->action = GETOUT_FAIL;
            write(connectfd,msg,sizeof(msg_t));
            return ERROR;
        }

        if(0 == strlen(str_return))                                   //不在线
        {
            msg->action = OFFLINE;
            write(connectfd,msg,sizeof(msg_t));
            return ERROR;
        }

        char sql_quit[256] = {0};                                     //移处登录状态
        sprintf(sql_quit, "delete from log_info where connectfd = %d;", atoi(str_return));
        rc = sqlite3_exec(db,sql_quit,NULL,0,&err_msg);

        if(rc != SQLITE_OK)                                           //移除成功
        {
            msg->action = GETOUT_FAIL;
            write(connectfd,msg,sizeof(msg_t));
            return ERROR;
        }

        msg->action = GETOUT_SUCC;                                    //返回踢出成功
        write(connectfd,msg,sizeof(msg_t));

        tgt_confd = atoi(str_return);                                 //向接受方发送指令
        msg->action = GETOUT;
        write(tgt_confd,msg,sizeof(msg_t));

        time(&ptime);                                                 //保存记录 
        strcpy(pestime,ctime(&ptime));
        char sql_getout_record[256];
        sprintf(sql_getout_record, "insert into record (id,name,size,target,time) values(%d,'%s','%s','%s','%s');",msg->name,"踢出聊天室",msg->target,pestime); 
        rc = sqlite3_exec(db, sql_getout_record, NULL, 0, &err_msg);

        time(&ptime);
        strcpy(pestime,ctime(&ptime));

        sprintf(sql_getout_record, "insert into record (id,name,size,time) values(%d,'%s','%s','%s');",msg->id,msg->target,"下线",pestime); 
        memset(str_return, 0, sizeof(str_return));                   //保存下线记录  
        rc = sqlite3_exec(db, sql_getout_record, NULL, 0, &err_msg);
        return SUCC;
    }	

    else if(QUIT_SYSTEM == msg->action)                              //指令:退出系统
    {
        char sql_quit[100] = {0};                                    //删除登录状态
        sprintf(sql_quit, "delete from log_info where id = %d;", msg->id);
        rc = sqlite3_exec(db,sql_quit,NULL,0,&err_msg);

        if(rc != SQLITE_OK)                                          //操作失败 
        {
            msg->action = QUIT_FAIL;
            write(connectfd,msg,sizeof(msg_t));
            return ERROR;
        }

        else                                                         //返回退出成功
        {	
            msg->action = QUIT_SYSTEM;
            write(connectfd,msg,sizeof(msg_t));
            return SUCC;
        }
    }	

}

/* 宏定义端口号 */

int main(void)
{
    int  lfd;
    int cfd;
    int sfd;
    int rdy;

    struct sockaddr_in sin;
    struct sockaddr_in cin;

    int client[FD_SETSIZE];  /* 客户端连接的套接字描述符数组 */

    int maxi;
    int maxfd;                        /* 最大连接数 */

    fd_set rset;
    fd_set allset;

    socklen_t addr_len;         /* 地址结构长度 */

    int i;
    int n;
    int len;
    int opt = 1;   /* 套接字选项 */

    char addr_p[20];

    sqlite3 *db = NULL;
    char *err_msg = NULL;
    msg_t msg;
    time_t ptime;
    char pestime[100] = {0};

                                                   /* 对server_addr_in  结构进行赋值  */
    bzero(&sin,sizeof(struct sockaddr_in));        /* 先清零 */
    sin.sin_family=AF_INET;                 
    sin.sin_addr.s_addr=htonl(INADDR_ANY);         //表示接受任何ip地址   将ip地址转换成网络字节序
    sin.sin_port=htons(PORT);                      //将端口号转换成网络字节序

                                                   /* 调用socket函数创建一个TCP协议套接口 */
    if((lfd=socket(AF_INET,SOCK_STREAM,0))==-1)    // AF_INET:IPV4;SOCK_STREAM:TCP
    {
        fprintf(stderr,"Socket error:%s\n\a",strerror(errno));
        exit(1);
    }


    /*设置套接字选项 使用默认选项*/
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    /* 调用bind函数 将serer_addr结构绑定到sockfd上  */
    if(bind(lfd,(struct sockaddr *)(&sin),sizeof(struct sockaddr))==-1)
    {
        fprintf(stderr,"Bind error:%s\n\a",strerror(errno));
        exit(1);
    }


    /* 开始监听端口   等待客户的请求 */
    if(listen(lfd,20)==-1)
    {
        fprintf(stderr,"Listen error:%s\n\a",strerror(errno));
        exit(1);
    }

    printf("Accepting connections .......\n");

    maxfd = lfd;                                /*对最大文件描述符进行初始化*/
    maxi = -1;

    /*初始化客户端连接描述符集合*/
    for(i = 0;i < FD_SETSIZE;i++)
    {
        client[i] = -1;
    }

    FD_ZERO(&allset);                     /* 清空文件描述符集合 */
    FD_SET(lfd,&allset);                    /* 将监听字设置在集合内 */


    int rc = sqlite3_open("chat_room.db",&db);
    if(rc != SQLITE_OK)
    {
        fprintf(stderr,"open database failed %s\n",sqlite3_errmsg(db));
    }

    char sql_create_user_info[256] = {0};            //保存用户信息
    sprintf(sql_create_user_info,"create table user_info(id INTEGER,name TEXT,password TEXT,primary key(id));");
    sqlite3_exec(db,sql_create_user_info,NULL,0,&err_msg);

    char sql_create_log_info[256] = {0};             //保存已登录用户
    sprintf(sql_create_log_info,"create table log_info(id INTEGER,name TEXT,connectfd INTEGER,primary key(id));");
    sqlite3_exec(db,sql_create_log_info,NULL,0,&err_msg);

    char sql_create_record[256] = {0};               //保存服务器运行记录
    sprintf(sql_create_record,"create table record(id INTEGER,name TEXT,size TEXT,target TEXT,msg TEXT,time TEXT,primary key(id));");
    sqlite3_exec(db,sql_create_record,NULL,0,&err_msg);

    /* 开始服务程序的死循环 */
    while(1)
    {
        rset = allset;
        /*得到当前可以读的文件描述符数*/
        rdy = select(maxfd + 1, &rset, NULL, NULL, NULL);

            if(FD_ISSET(lfd, &rset))
            {
                addr_len = sizeof(sin);

                /* 接受客户端的请求 */
                if((cfd=accept(lfd,(struct sockaddr *)(&cin),&addr_len))==-1)
                {
                    fprintf(stderr,"Accept error:%s\n\a",strerror(errno));
                    exit(1);
                }

                /*查找一个空闲位置*/
                for(i = 0; i<FD_SETSIZE; i++)
                {       //printf("%d\t",client[i]);
                    if(client[i] <= 0)
                    {
                        client[i] = cfd;   /* 将处理该客户端的连接套接字设置到该位置 */
                        break;
                    }
                }

                /* 太多的客户端连接   服务器拒绝俄请求  跳出循环 */
                if(i == FD_SETSIZE)
                {
                    printf("too many clients");
                    exit(1);
                }

                FD_SET(cfd, &allset);     /* 设置连接集合 */

                if(cfd > maxfd)                  /* 新的连接描述符 */
                {
                    maxfd = cfd;
                }

                if(i > maxi)
                {
                    maxi = i;
                }

                if(--rdy <= 0)                /* 减少一个连接描述符 */
                {
                    continue;
                }

            }
        /* 对每一个连接描述符做处理 */
        for(i = 0;i< FD_SETSIZE;i++)
        {   
            if((sfd = client[i]) < 0)
            {
                continue;
            }

            if(FD_ISSET(sfd, &rset))
            {

                n = read(sfd,&msg,sizeof(msg_t));

                if(n == 0)                                //客户端异常退出
                {                                     
                    printf("the other side has been closed. \n");              
                    
                    char sql_quit[256];                   //删除登录状态
                    sprintf(sql_quit, "delete from log_info where id = %d;", msg.id);
                    rc = sqlite3_exec(db,sql_quit,NULL,0,&err_msg);

                    time(&ptime);                         //保存记录
                    strcpy(pestime,ctime(&ptime));
                    rc = sqlite3_exec(db, sql_quit, NULL, 0, &err_msg);
                    sprintf(sql_quit, "insert into record (id,name,size,time) values(%d,'%s','%s','%s');",msg.id,msg.target,"下线",pestime); 

                    rc = sqlite3_exec(db, sql_quit, NULL, 0, &err_msg);
                    
                    fflush(stdout);                                    /* 刷新 输出终端 */
                    close(sfd);

                    FD_CLR(sfd, &allset);                        /*清空连接描述符数组*/
                    client[i] = -1;
                }

                else
                {
                    msg_handle(&msg,sfd);
                    /* 谐函数出错 */
                    if(n == 1)
                    {
                        exit(1);
                    }
                }

                /*如果没有可以读的套接字   退出循环*/
                if(--rdy <= 0)
                {
                    break;
                }


            }
        }

    }

    close(lfd);       /* 关闭链接套接字 */
    return 0;
}

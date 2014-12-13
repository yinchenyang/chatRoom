#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/fcntl.h>

enum{LOG = 1,REGISTER};

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
}

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
    printf("\n\n\n输入log进行登录，输入register进行注册:\n\n");
}

int main(int argc,char *argv[])
{
    system("reset");
    char cmd[10] = {0};
    int order_check_return = 0;
    log_or_register();
    scanf("%s",cmd);
    order_check_return = order_check(cmd);
    switch(order_check_return)
    {
        case LOG:
	{
	    printf("log\n");
	    break;
	    //log();
	}
	case REGISTER:
	{
	    printf("register\n");
	    break;
	    //regist();
	}
    }

    return 0;
}

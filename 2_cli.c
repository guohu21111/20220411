#include <stdio.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <string.h>    
#include <stdlib.h>    
#include <netinet/in.h>
#include <netinet/ip.h>
#include <unistd.h>    
#include <arpa/inet.h> 
#include <signal.h>    
#include <sys/wait.h>  
#include <time.h>      
#include <errno.h>     

#define PORT 8888

typedef struct{
	int pay; 						//工资
	int type; 						//协议 0错误 1注册 2登录 3增加 4删除 5修改 6查找 
	int state; 						//区分普通用户(1)与超级用户(2)
	int age; 						//年龄
	int phone; 						//电话
	int idcard; 					//身份证
	char id[32]; 					//账号
	char password[32]; 				//密码
	char name[32]; 					//姓名
	char sex[10]; 					//性别
	char department[32]; 			//部门
	char massge[128]; 				//储存服务器发送的信息
}MSG;

//创建客户端并连接服务器   
int create_tcp_cli(void); 

//组注册包
void user_enroll(MSG* info);

//组登录包
void user_login(MSG* info);

//发送包 
void user_send(MSG* info, int fd);

//接受包
void user_recv(MSG* info, int fd);

//超级用户界面
void user_super(MSG* info, int fd);

//添加包
void user_add(MSG* info);

//删除包
void user_del(MSG* info);

//修改包
void user_mod(MSG* info);

//查找包
void user_check(MSG* info);

//普通用户界面
void user_ordinary(MSG* info,int fd);

int main(int argc, const char *argv[])
{
	//创建客户端并连接服务器
	int fd = create_tcp_cli();	

	//创建一级菜单
	int choose;
	MSG info;
	while(1)
	{
		system("clear");
		printf("------------请选择您要进行的操作-----------------\n");
		printf("-------------------1.注册------------------------\n");
		printf("-------------------2.登录------------------------\n");
		printf("-------------------3.退出------------------------\n");
		scanf("%d",&choose);
		switch(choose)
		{
		case 1:
			//组注册包
			user_enroll(&info);
			break;
		case 2:
			//组登录包
			user_login(&info);
			break;
		case 3:
			return 0;
		default:
			printf("输入错误，请重新输入\n");
		}

		//发送包到服务器
		user_send(&info, fd);

		//从服务器接受包
		user_recv(&info, fd);


		//判断包的信息
		printf("type = %d  id = %s  password = %s  state = %d massge = %s\n",info.type,info.id,info.password,info.state,info.massge);
		if(info.type == 2)
		{
			if(info.state == 2)
			{
				//进入管理员用户界面
				user_super(&info,fd);
			}
			else if(info.state == 1)
			{
				//进入普通用户界面
				user_ordinary(&info,fd);
			}
		}
		printf("输入任意字符清屏>>>>>>\n");
		while(getchar() != 10);

	}


	return 0;
}

//发送包
void user_send(MSG* info, int fd)
{
	if(send(fd,info,sizeof(MSG),0) < 0)
	{
		perror("send");
		exit(-1);
	}
	return ;
}

//接受包
void user_recv(MSG* info, int fd)
{
	if(recv(fd,info,sizeof(MSG),0) < 0)
	{
		perror("recv");
		exit(-1);
	}
	return ;
}

//创建客户端并连接服务器   
int create_tcp_cli(void)
{
	//创建套接字
	int fd = socket(AF_INET,SOCK_STREAM,0);
	if(fd == -1)
	{
		perror("socket");
		exit(-1);
	}
	//连接服务器
	struct sockaddr_in sin;
	sin.sin_addr.s_addr = inet_addr("10.102.133.168");
	sin.sin_port = htons(PORT);
	sin.sin_family = AF_INET;
	if(connect(fd, (struct sockaddr*)&sin,sizeof(sin)) == -1)
	{
		perror("connect");
		exit(-1);
	}

	return fd;
}

//组注册包
void user_enroll(MSG* info)
{
	printf("请输入账号：");
	scanf("%s",info->id);
	while(getchar() != 10);
	printf("请输入密码：");
	scanf("%s",info->password);
	while(getchar() != 10);
	printf("请输入您的用户权限(1为普通权限，2为管理员权限)：");
	scanf("%d",&(info->state));
	while(getchar() != 10);
	info->type = 1;
	return ;
}

//组登录包
void user_login(MSG* info)
{
	printf("请输入账号：");
	scanf("%s",info->id);
	while(getchar() != 10);
	printf("请输入密码：");
	scanf("%s",info->password);
	while(getchar() != 10);
	info->type = 2;
	return ;
}

//超级用户界面
void user_super(MSG* info, int fd)
{
	int choose = 0;
	while(1)
	{
		system("clear");
		printf("------------------1.添加员工信息-------------------\n");
		printf("------------------2.删除员工信息-------------------\n");
		printf("------------------3.修改员工信息-------------------\n");
		printf("------------------4.查找员工信息-------------------\n");
		printf("------------------5.退出---------------------------\n");
		printf("请选择您要进行的操作--------------->\n");
		scanf("%d",&choose);
		while(getchar() != 10);
		switch(choose)
		{
		case 1:
			//组添加信息包
			user_add(info);
			break;
		case 2:
			//组删除信息包
			user_del(info);
			break;
		case 3:
			//组修改信息包
			user_mod(info);
			break;
		case 4:
			//组查找信息包
			user_check(info);
			break;
		case 5:
			return ;
		default :
			printf("请重新输入\n");
		}

		if(choose<6 && choose>0 )
		{
			//发送包到服务器
			user_send(info,fd);

			//接受包
			user_recv(info,fd);


			if(info->type = 6)
			{
				printf("姓名：%s 性别：%s 年龄：%d 电话：%d 身份证：%d 工资：%d 部门：%s \n",info->name,info->sex,info->age,info->phone,info->idcard,info->pay,info->department);
			}
			printf("%s\n",info->massge);
		}
		printf("输入任意字符清屏>>>>>>\n");
		while(getchar() != 10);
	}
	return ;
}

//添加包
void user_add(MSG* info)
{
	printf("请输入您要添加的姓名:");
	scanf("%s",info->name);
	while(getchar()!=10);

	printf("请输入您要添加的性别:");
	scanf("%s",info->sex);
	while(getchar()!=10);

	printf("请输入您要添加的年龄:");
	scanf("%d",&info->age);
	while(getchar()!=10);

	printf("请输入您要添加的电话:");
	scanf("%d",&info->phone);
	while(getchar()!=10);

	printf("请输入您要添加的身份证:");
	scanf("%d",&info->idcard);
	while(getchar()!=10);

	printf("请输入您要添加的工资:");
	scanf("%d",&info->pay);
	while(getchar()!=10);

	printf("请输入您要添加的部门:");
	scanf("%s",info->department);
	while(getchar()!=10);

	info->type = 3;
	return ;
}

//删除包
void user_del(MSG* info)
{
	printf("请输入您要删除员工的姓名:");
	scanf("%s",info->name);
	while(getchar()!=10);
	info->type = 4;
	return ;
}

//修改包
void user_mod(MSG* info)
{
	printf("请输入您要修改的员工的姓名：");
	scanf("%s",info->name);
	while(getchar()!=10);
	
	printf("请输入您要修改的员工的性别：");
	scanf("%s",info->sex);
	while(getchar()!=10);
	
	printf("请输入您要修改的员工的年龄：");
	scanf("%d",&info->age);
	while(getchar()!=10);
	
	printf("请输入您要修改的员工的电话：");
	scanf("%d",&info->phone);
	while(getchar()!=10);
	
	printf("请输入您要修改的员工的身份证：");
	scanf("%d",&info->idcard);
	while(getchar()!=10);
	
	printf("请输入您要修改的员工的工资：");
	scanf("%d",&info->pay);
	while(getchar()!=10);
	
	printf("请输入您要修改的员工的部门：");
	scanf("%s",info->department);
	while(getchar()!=10);
	
	info->type = 5;
	
	return ;
}

//查找包
void user_check(MSG* info)
{
	printf("请输入您要查找人的姓名：");
	scanf("%s",info->name);
	while(getchar()!=10);

	info->type = 6;
	return ;
}

//普通用户界面
void user_ordinary(MSG* info,int fd)
{
	int choose;
	while(1)
	{
		system("clear");
		printf("------------------1.查找员工信息-------------------\n");
		printf("------------------2.退出---------------------------\n");
		printf("请选择您要进行的操作--------------->\n");
		scanf("%d",&choose);
		while(getchar() != 10);
		switch(choose)
		{
		case 1:
			//查找包
			user_check(info);
			//发送包
			user_send(info, fd);
			//接受包
			user_recv(info, fd);

			printf("姓名：%s 性别：%s 年龄：%d 电话：%d 部门：%s \n",info->name,info->sex,info->age,info->phone,info->department);
			printf("%s\n",info->massge);
			break;
		case 2:
			return;
		default:
			printf("请重新输入\n");
		}

		printf("输入任意字符清屏>>>>>>\n");
		while(getchar() != 10);
		
	}
	return ;
}














#include <stdio.h>
#include <sqlite3.h>
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

//创建并打开数据库
sqlite3* sq_open(void);

//创建服务器
int create_tcp_ser(void);

//创建用户账号密码表
void create_user_info(sqlite3* db);	

//子进程功能函数
void func(int fdnew,struct sockaddr_in cin,sqlite3* db);

//注册处理函数
void user_enroll(MSG* info,sqlite3* db);

//登录处理函数
void user_login(MSG* info,sqlite3* db);

//增加处理函数
void user_add(MSG* info,sqlite3* db);

//修改处理函数
void user_mod(MSG* info,sqlite3* db);

//查找处理函数
void user_check(MSG* info, sqlite3* db);

//删除处理函数
void user_del(MSG* info,sqlite3* db);

//用信号回收僵尸进程
void handler(int sig)
{
	while(waitpid(-1,NULL,WNOHANG) > 0);
	return ;
}

int main(int argc, const char *argv[])
{
	//创建并打开数据库
	sqlite3* db = sq_open();

	//创建用户账号密码表
	create_user_info(db);

	//创建服务器
	int fds = create_tcp_ser();

	//循环连接客户端
	struct sockaddr_in cin;
	socklen_t len = sizeof(cin);
	while(1)
	{
		int fdnew = accept(fds,(struct sockaddr*)&cin,&len);
		if(fdnew < 0)
		{
			perror("accept");
			return -1;
		}
		printf("%s  %d 连接成功\n",inet_ntoa(cin.sin_addr),ntohs(cin.sin_port));
		if(fork() == 0)
		{
			close(fds);
			//功能
			func(fdnew,cin,db);
			close(fdnew);
			exit(0);
		}
		close(fdnew);
	}

	//关闭数据库
	sqlite3_close(db);

	return 0;
}

//创建并打开数据库
sqlite3* sq_open(void)
{
	sqlite3* db;
	if(sqlite3_open("./sq.db",&db) != 0)
	{
		printf("%d sqlite3 open error:%s\n",__LINE__,sqlite3_errmsg(db));
		exit(-1);
	}
	return db;
}

//创建服务器
int create_tcp_ser(void)
{

	//捕获17号信号 SIGCHLD                    
	__sighandler_t s = signal(SIGCHLD, handler);
	if(SIG_ERR == s)                          
	{                                         
		perror("signal");                     
		exit(-1);                             
	}                                         

	//创建套接字
	int fds = socket(AF_INET,SOCK_STREAM,0);
	if(fds < 0)
	{
		perror("socket");
		exit(-1);
	}

	//允许端口快速重用                                                      
	int reuse = 1;                                                          
	if(setsockopt(fds, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
	{                                                                       
		perror("setsockopt");                                               
	}                                                                       

	//绑定地址信息结构体
	struct sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_port = htons(PORT);
	sin.sin_addr.s_addr = inet_addr("10.102.133.168");
	if(bind(fds,(struct sockaddr*)&sin,sizeof(sin)) == -1)
	{
		perror("bind");
		exit(-1);
	}

	//将套接字设置为监听
	if(listen(fds,50) == -1)
	{
		perror("listen");
		exit(-1);
	}

	return fds;
}

//创建两张表
void create_user_info(sqlite3* db)
{
	//创建用户信息表
	char sql[256] = "create table if not exists user_info \
					 (id char primary key, password char,state int);";
	char *errmsg = NULL;
	if(sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK)
	{
		printf("%d sqlite3 open error:%s\n",__LINE__,sqlite3_errmsg(db));
		exit(-1);	
	}
	//创建员工信息表
	strcpy(sql,"create table if not exists employees_info(name char primary key, \
		sex char,age int,phone int,idcard int,pay int,department char);");
	errmsg = NULL;
	if(sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK)
	{
		printf("%d sqlite3 open error:%s\n",__LINE__,sqlite3_errmsg(db));
		exit(-1);	
	}
	return ;
}



//子进程功能函数
void func(int fdnew,struct sockaddr_in cin,sqlite3* db)
{
	MSG info;
	ssize_t res = 0;
	//循环接受客户端发来的信息
	while(1)
	{
		bzero(&info,sizeof(MSG));
		res = recv(fdnew, &info, sizeof(MSG), 0);
		if(res < 0)
		{
			perror("recv");
			break;
		}
	//	printf("id = %s  password = %s  state = %d \n",info.id,info.password,info.state);

		switch(info.type)
		{
		case 1:
			//注册处理函数
			user_enroll(&info, db);
			break;
		case 2:
			//登录处理函数
			user_login(&info, db);
			break;
		case 3:
			//添加处理函数
			user_add(&info, db);
			break;
		case 4:
			//删除处理函数
			user_del(&info,db);
			break;
		case 5:
			//修改处理函数
			user_mod(&info,db);
			break;
		case 6:
			//查找处理函数
			user_check(&info,db);
			break;
		default:
			break;
		}
		//发送信息到客户端
		if(send(fdnew, &info, sizeof(MSG), 0) < 0)
		{
			perror("send");
			break;
		}
	}
	printf("%s  %d 断开连接\n",inet_ntoa(cin.sin_addr),ntohs(cin.sin_port));
	return ;
}

//注册处理函数
void user_enroll(MSG* info,sqlite3* db)
{
	char sql[256] = "";
	sprintf(sql,"select * from user_info where id = \"%s\";",info->id);
	char** pres = NULL;
	int row = 0,column = 0;
	char* errmsg = NULL;
	//判断id是否已经存在
	if(sqlite3_get_table(db, sql, &pres, &row, &column, &errmsg) != SQLITE_OK)
	{
		strcpy(info->massge,"注册失败");
		printf("__%d__  sqlite3_get_table:%s\n",__LINE__,errmsg);
		return;
	}
	int i = 0;
	for(i=3; i<(row+1)*column; i=i+3)
	{
		if(strcmp(pres[i],info->id) == 0)
		{
			info->type = 0;
			strcpy(info->massge,"用户已存在");
			sqlite3_free_table(pres);
			return ;
		}
	}

	sprintf(sql,"insert into user_info values(\"%s\",\"%s\",\"%d\");",\
			info->id,info->password,info->state);
	//将数据添加到数据库中
	if(sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK)
	{
		strcpy(info->massge,"注册失败");
		printf("%d,  sqlite3_exec:%s",__LINE__,errmsg);
		sqlite3_free_table(pres);
		return ;
	}
	strcpy(info->massge,"注册成功");
	sqlite3_free_table(pres);

	return ;
}

//登录处理函数
void user_login(MSG* info,sqlite3* db)
{

	char sql[256] = "";
	char** pres = NULL;
	int row = 0,column = 0;
	char* errmsg = NULL;
	sprintf(sql,"select * from user_info where id = \"%s\";",info->id);
	//判断id是否存在
	if(sqlite3_get_table(db, sql, &pres, &row, &column, &errmsg) != SQLITE_OK)
	{
		strcpy(info->massge,"登录失败");
		printf("__%d__  sqlite3_get_table:%s\n",__LINE__,errmsg);
		sqlite3_free_table(pres);
		return;
	}

	if((row * column) == 0)
	{
			info->type = 0;
			strcpy(info->massge,"用户不存在");
			return ;
	}

	int i = 0;
	//判断密码是否正确
	for(i=4; i<(row+1)*column; i=i+3)
	{
		if(strcmp(pres[i],info->password) != 0)
		{
			info->type = 0;
			strcpy(info->massge,"密码错误");
			sqlite3_free_table(pres);
			return ;
		}
	}
	
	info->state = atof(pres[5]);
	strcpy(info->massge,"登录成功");
	sqlite3_free_table(pres);

	return ;
}

//增加处理函数
void user_add(MSG* info,sqlite3* db)
{
	char *errmsg = NULL;
	char sql[256] = "";
	char** pres = NULL;
	int row = 0,column = 0;
	sprintf(sql,"select * from employees_info where name = \"%s\";",info->name);
	//判断id是否存在
	if(sqlite3_get_table(db, sql, &pres, &row, &column, &errmsg) != SQLITE_OK)
	{
		strcpy(info->massge,"添加失败");
		printf("__%d__  sqlite3_get_table:%s\n",__LINE__,errmsg);
		sqlite3_free_table(pres);
		return;
	}
	if((row * column) != 0)
	{
		info->type = 0;
		strcpy(info->massge,"用户已存在");
		sqlite3_free_table(pres);
		return ;
	}

	errmsg = NULL;
	bzero(sql,sizeof(sql));
	sprintf(sql,"insert into employees_info values(\"%s\",\"%s\",\"%d\",\"%d\",\"%d\",\"%d\",\"%s\");",info->name,info->sex,info->age,info->phone,info->idcard,info->pay,info->department);
	if(sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK)
	{
		info->type = 0;
		strcpy(info->massge,"添加失败");
		printf("__%d__  sqlite3_exec:%s\n",__LINE__,errmsg);
		return;
	}
	strcpy(info->massge,"添加成功");
	
	return ;
}

//删除处理函数
void user_del(MSG* info,sqlite3* db)
{
	char *errmsg = NULL;
	char sql[256] = "";
	char** pres = NULL;
	int row = 0,column = 0;
	sprintf(sql,"select * from employees_info where name = \"%s\";",info->name);
	//判断id是否存在
	if(sqlite3_get_table(db, sql, &pres, &row, &column, &errmsg) != SQLITE_OK)
	{
		strcpy(info->massge,"删除失败");
		printf("__%d__  sqlite3_get_table:%s\n",__LINE__,errmsg);
		sqlite3_free_table(pres);
		return;
	}
	if((row * column) == 0)
	{
		info->type = 0;
		strcpy(info->massge,"用户不存在");
		sqlite3_free_table(pres);
		return ;
	}

	bzero(sql,sizeof(sql));
	errmsg = NULL;
	sprintf(sql,"delete from employees_info where phone = \"%d\";",info->phone);
	if(sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK)
	{
		info->type = 0;
		strcpy(info->massge,"删除失败");
		printf("__%d__  sqlite3_exec:%s\n",__LINE__,errmsg);
		return;
	}

	strcpy(info->massge,"删除成功");
	sqlite3_free_table(pres);
	return ;
}

//修改处理函数
void user_mod(MSG* info,sqlite3* db)
{
	char *errmsg = NULL;
	char sql[256] = "";
	char** pres = NULL;
	int row = 0,column = 0;
	sprintf(sql,"select * from employees_info where name = \"%s\";",info->name);
	//判断id是否存在
	if(sqlite3_get_table(db, sql, &pres, &row, &column, &errmsg) != SQLITE_OK)
	{
		info->type = 0;
		strcpy(info->massge,"修改失败");
		printf("__%d__  sqlite3_get_table:%s\n",__LINE__,errmsg);
		sqlite3_free_table(pres);
		return;
	}
	if((row * column) == 0)
	{
		info->type = 0;
		strcpy(info->massge,"未检测到此人");
		sqlite3_free_table(pres);
		return ;
	}

	errmsg = NULL;
	bzero(sql,sizeof(sql));
	sprintf(sql,"update employees_info set sex = %s and age = %d and phone = %d and idcard = %d and pay = %d and department = %s where name = %s;",info->sex,info->age,info->phone,info->idcard,info->pay,info->department,info->name);
	if(sqlite3_exec(db,sql,NULL,NULL,&errmsg) != SQLITE_OK)
	{
		info->type = 0;
		strcpy(info->massge,"修改失败");
		printf("__%d__  sqlite3_exec:%s\n",__LINE__,errmsg);
		sqlite3_free_table(pres);
		return;
	}

	strcpy(info->massge,"修改成功");
	sqlite3_free_table(pres);

	return ;
}

//查找处理函数
void user_check(MSG* info, sqlite3* db)
{
	char *errmsg = NULL;
	char sql[256] = "";
	char** pres = NULL;
	int row = 0,column = 0;
	sprintf(sql,"select * from employees_info where name = \"%s\";",info->name);
	//判断id是否存在
	if(sqlite3_get_table(db, sql, &pres, &row, &column, &errmsg) != SQLITE_OK)
	{
		info->type = 0;
		strcpy(info->massge,"查找失败");
		printf("__%d__  sqlite3_get_table:%s\n",__LINE__,errmsg);
		sqlite3_free_table(pres);
		return;
	}
	if((row * column) == 0)
	{
		info->type = 0;
		strcpy(info->massge,"未检测到此人");
		sqlite3_free_table(pres);
		return ;
	}

	
	strcpy(info->sex,pres[8]);
	info->age = atof(pres[9]);	
	info->phone = atof(pres[10]);	
	info->idcard = atof(pres[11]);	
	info->pay = atof(pres[12]);	
	strcpy(info->department, pres[13]);	
	
	strcpy(info->massge, "查询成功");	
	
	sqlite3_free_table(pres);

	return ;
}






























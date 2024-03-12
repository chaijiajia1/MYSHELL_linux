#ifndef HEADER_H
# define HEADER_H
#define _XOPEN_SOURCE//不加这行信号类会报错
//头文件
# include <stdio.h>
# include <stdlib.h>
# include <signal.h>
# include <string.h>
# include <unistd.h>
# include <dirent.h>
# include <stddef.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <wait.h>
#include <time.h>
//宏定义
# define MAXLENGTH 100 //命令最大长度
# define MAXNAME 100 //进程名最大长度
# define MAXJOB 1024 //最大进程数
# define MAXPATH 100 //路径最长长度
# define MAXSECTION 30 //一行最多的部分数量
# define SHMSIZE (sizeof(JobList) * MAXJOB) //共享内存大小

//结构体
enum Type{FG=0/*前台*/, BG=1/*后台*/};
enum JobStatus{RUN=0/*运行*/, SUSPEND=1/*中止*/, DONE=2/*完成*/};
typedef struct job{ //进程结构体
    int pid;    //进程号
    char name[MAXLENGTH];//进程名
    enum Type type;
    enum JobStatus status;
}Job;
struct joblist{ //进程列表结构体
    int num;    //进程数
    struct job List[MAXJOB]; //进程数组
};
struct joblist *JobList; //进程列表指针
//命令
struct command{
    char SplitCMD[MAXSECTION][MAXLENGTH];
    int PipIn;//管道输入
    int PipOut;//管道输出
    int RedirectionIn;//重定向输入
    int RedirectionOut;//重定向覆盖输出
    int RedirectionAppend;//重定向追加输出
    char RedirectionInFile[MAXPATH];//重定向输入文件
    char RedirectionOutFile[MAXPATH];//重定向输出文件
    char num;//命令数量
};
typedef struct commandlist{//命令列表
    struct command cmd[MAXSECTION];
    int num;
    enum Type type;
}CommandList;

//信号
struct sigaction TSTP; //SIGTSTP信号结构，中断
struct sigaction TSTP_old; //SIGTSTP信号原值
struct sigaction CHLD; //SIGCHLD信号结构，停止
struct sigaction CHLD_old; //SIGCHLD信号原值
//函数声明
void JobInit(); //初始化进程列表
void SignalInit(); //初始化信号
void CHLD_handler(int sign); //SIGCHLD信号处理函数
void TSTP_handler(int sign); //SIGTSTP信号处理函数
CommandList* paser(char* cmd);
void execute(CommandList* CMDlist);
int FindJob(int pid);
void DeleteJob(pid_t pid);
int BuildIn(struct command cmd);

//内建函数
void bg();//找到最近暂停的后台进程并将其运行
void clr();//清屏
void cd(char* path);//改变工作目录
void dir(char* path);//列出目录
void echo(struct command cmd);//输出字符串
void exec(struct command cmd);//执行程序
void fg();//找到最近后台并将其前台运行
void help(char* str);//输出帮助信息
void jobs();//列出所有后台进程
void pwd();//输出当前工作目录
void set(char* str);//设置环境变量
int test(struct command cmd);//测试函数
void Time();//输出当前时间
void Umask(char* str);//设置文件权限掩码

//全局变量
char path[MAXPATH]; //工作目录
char CurrentPath[MAXLENGTH];//当前目录
char PipePath[MAXLENGTH]="pipe.txt";//管道文件路径
# endif
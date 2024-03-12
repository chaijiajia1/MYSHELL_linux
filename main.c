# include "header.h"
int main(int argc,char **argv)
{
    JobInit();//初始化进程列表
    SignalInit();//初始化信号处理
    getcwd(path, sizeof(path));
    strcpy(CurrentPath,path);//得到当前目录
    strcat(path,"/myshell");//得到工作路径
    // 调用setenv设置myshell环境变量
    setenv( "myshell"/*变量名*/, path/*变量值*/, 1/*覆写*/);
    setenv("pwd", path, 1);//设置工作目录环境变量(后面能被getcwd读取
    FILE *fp;
    for(int i=1;i<argc;i++){//处理脚本
        if((fp=fopen(argv[i],"r"))==NULL){//用只读模式打开脚本
            printf("error: failed to open file\n");
            break;
        };
        char ScripCMD[MAXLENGTH];//存放脚本中的命令
        while(!feof(fp))//处理脚本每一行命令
        {
            fgets(ScripCMD,MAXLENGTH,fp);//得到每一行
            printf("[root@myshell %s]$ %s\n",getcwd(NULL, 0),ScripCMD);
            if(strcmp(ScripCMD,"exit")==0) exit(0);//退出
            execute(paser(ScripCMD));   
        }
    }
    char CMD[MAXLENGTH];
    while(1){//处理用户指令
        printf("[root@myshell %s]$ ",getcwd(NULL, 0));
        gets(CMD);//得到用户输入
        if(strcmp(CMD,"exit")==0) exit(0);
        execute(paser(CMD));
        }
    return 0;
}

void JobInit(){
    int Ipc_ID;
    if((Ipc_ID=shmget( IPC_PRIVATE/*忽略键，建立一个新的共享内存，指定一个键值 ，然后返回这块共享内存IPC标识符ID。*/ , SHMSIZE, 0666/*全局读取和写入许可*/ | IPC_CREAT/*如果不存在就创建*/ ))==-1) {printf("failed to create shared memeory");exit(1);}
    // 挂接共享内存
    void *shm_addr=NULL;
    shm_addr = shmat( Ipc_ID, NULL/*共享内存会被attach到一个合适的虚拟地址空间*/, 0/*可读可写*/);
    //初始化共享内存
    memset(shm_addr, 0, SHMSIZE) ;
    // 将共享内存地址赋给进程表
    JobList = (Job*)shm_addr;
    //初始化进程表
    JobList->List[0].pid = getpid();//getpid()返回当前进程的进程号，即myshell的进程号
    strcpy( JobList->List[0].name, "myshell");
    JobList->List[0].type = BG;
    JobList->List[0].status = RUN;
    JobList->num = 1;
}
void SignalInit(){
    //中断信号
    memset(&TSTP,0,sizeof(TSTP));//清空TSTP
    TSTP.sa_handler = TSTP_handler;//设置TSTP的信号处理函数
    sigaction(SIGTSTP, &TSTP, &TSTP_old);//注册TSTP信号处理函数
    //退出信号
    memset(&CHLD, 0, sizeof(CHLD));//清空CHLD
    CHLD.sa_handler = CHLD_handler;//设置CHLD的信号处理函数
    sigaction(SIGCHLD, &CHLD, &CHLD_old);//注册CHLD信号处理函数
}
void TSTP_handler(int sign){
    for(int i=JobList->num-1;i>0;i--){//从后往前找，找到第一个RUN的前台进程，将其状态改为后台SUSPEND
        if(JobList->List[i].status==RUN&&JobList->List[i].type==FG){
            JobList->List[i].status = SUSPEND;
            JobList->List[i].type = BG;
            kill(JobList->List[i].pid,SIGSTOP);//用kill函数发送信号
            break;
        }
    }
}
void CHLD_handler(int sign){
    if(JobList->num==1) return;//如果进程表中只有myshell
    if(JobList->List[JobList->num-1].type=FG){//若是前台进程，直接删除
       JobList->num-=1;
    }
    else if(JobList->List[JobList->num-1].status==SUSPEND||JobList->List[JobList->num-1].status==RUN){//如果最后一个进程是后台挂起或正在执行的进程
        JobList->List[JobList->num-1].status = DONE;//将其状态改为DONE
    }
}
CommandList* paser(char* cmd){
    if(strcmp(cmd,"\0")==0) return;//为空
    char SplitCMD[MAXSECTION][MAXLENGTH];
    memset(SplitCMD,0,sizeof(SplitCMD));
    char* separator=" \n\t";//用空格换行和制表符分隔
    strcpy(SplitCMD[0],strtok(cmd,separator));
    int size;
    for(size=1;;size++){//循环分解
        char *temp=strtok(NULL/*strtok用this指针指向了分解符的下一位*/,separator);
        if(temp==NULL) break;//分解完毕
        strcpy(SplitCMD[size],temp);
    }
    CommandList* CMD =(CommandList*)malloc(sizeof(CommandList));//创建命令结构体
    memset(CMD,0,sizeof(CommandList));//初始化为0
    int cnt=0;
    for(int i=0;i<size;i++){//检查每个部分,命令预处理
        if(strcmp(SplitCMD[i],"<")==0){//输入重定向
            CMD->cmd[CMD->num].RedirectionIn=1; 
            strcpy(CMD->cmd[CMD->num].RedirectionInFile,SplitCMD[++i]);//下一部分一定是重定向的文件路径
            break;
        }
        else if(strcmp(SplitCMD[i],">")==0){//输出重定向
            CMD->cmd[CMD->num].RedirectionOut=1;
            strcpy(CMD->cmd[CMD->num].RedirectionOutFile,SplitCMD[++i]);
            break;
        }
        else if(strcmp(SplitCMD[i],">>")==0){//输出重定向,追加
            CMD->cmd[CMD->num].RedirectionAppend=1;
            strcpy(CMD->cmd[CMD->num].RedirectionOutFile,SplitCMD[++i]);
            break;
        }
        else if(strcmp(SplitCMD[i],"&")==0){//后台运行
            CMD->type=BG;
            break;
        }
        else if(strcmp(SplitCMD[i],"|")==0){//管道
            CMD->cmd[CMD->num].PipOut=1;//这条命令为管道输出
            CMD->cmd[++CMD->num].PipIn=1;//下一条命令为管道输入
        }
        else{//非关键词
            strcpy(CMD->cmd[CMD->num].SplitCMD[CMD->cmd[CMD->num].num++],SplitCMD[i]);//将命令存入命令结构体
        }
    }
    return CMD;
}
void execute(CommandList* CMDlist){
    int stdin_copy = dup(0);//备份标准输入
    int stdout_copy = dup(1);//备份标准输出
    int FI=-1,FO=-1;//重定向文件描述符
    //处理重定向
    if(CMDlist->cmd[0].RedirectionIn){
        close(0);//关闭标准输入
        int FI=open(CMDlist->cmd[0].RedirectionInFile,O_RDONLY/*只读*/);
    }
    if(CMDlist->cmd[0].RedirectionOut){
        close(1);//关闭标准输入
        int FO=open(CMDlist->cmd[0].RedirectionOutFile,O_WRONLY/*只写*/ | O_CREAT/*自动创建*/ | O_TRUNC/*打开时清空*/, 0666/*所有用户有读写权限*/);
    }
    if(CMDlist->cmd[0].RedirectionAppend){
        close(1);
        int FO=open(CMDlist->cmd[0].RedirectionOutFile, O_RDWR/*读写*/ | O_CREAT/*自动创建*/ | O_APPEND/*追加*/, 0666);
    }
    //无管道内建命令，直接执行
    if(CMDlist->cmd[0].PipIn==0&&CMDlist->cmd[0].PipOut==0&&BuildIn(CMDlist->cmd[0])){
        if(FI>=0)close(FI);//恢复标准输入
        if(FO>=0)close(FO);//恢复标准输出
        dup2(stdin_copy, 0);
        dup2(stdout_copy, 1);
        close(stdin_copy);
        close(stdout_copy);
        return;
    }
    pid_t pid = fork();//创建子进程
    if(pid==0){//在新进程中执行
        usleep(1);//睡眠1微秒，防止子进程先于父进程执行
        // FILE *fp=fopen("temp.txt","w");//打开临时文件
        // fprintf(fp,"pid is %d\nnum %d in job list",getpid(),FindJob(getpid()));
        // fclose(fp);
        while(1){
            int temp=FindJob(getpid());
            if(temp!=-1&&JobList->List[temp].status==RUN){//在进程表中已经有此项
                break;
            }
        }
    
        //无管道外部命令
        if(CMDlist->cmd[0].PipIn==0&&CMDlist->cmd[0].PipOut==0){//开头没有管道，后面也不会有管道
            //处理重定向
            if(CMDlist->cmd[0].RedirectionIn){
                close(0);//关闭标准输入
                int FI=open(CMDlist->cmd[0].RedirectionInFile,O_RDONLY/*只读*/);
            }
            if(CMDlist->cmd[0].RedirectionOut){
                close(1);//关闭标准输入
                int FO=open(CMDlist->cmd[0].RedirectionOutFile,O_WRONLY/*只写*/ | O_CREAT/*自动创建*/ | O_TRUNC/*打开时清空*/, 0666/*所有用户有读写权限*/);
            }
            if(CMDlist->cmd[0].RedirectionAppend){
                close(1);
                int FO=open(CMDlist->cmd[0].RedirectionOutFile, O_RDWR/*读写*/ | O_CREAT/*自动创建*/ | O_APPEND/*追加*/, 0666);
            }
            //查找命令并执行
            // char* argv[MAXSECTION];
            // for(int j=0;j<CMDlist->cmd[0].num;j++){//生成参数列表
            //     strcpy(argv[j],CMDlist->cmd[0].SplitCMD[j]);
            // }
            // argv[CMDlist->cmd[0].num]=NULL;//参数列表最后一位为NULL
            // execvp(CMDlist->cmd[0].SplitCMD[0]/*文件名*/,argv/*参数列表*/);//execvp()会从环境变量所指的目录中查找符合参数 file 的文件名, 找到后执行该文件
            exec(CMDlist->cmd[0]);
        }
        //有管道内部命令
        else{
            for(int i=0;i<=CMDlist->num;i++){
                pid_t pid_son=fork();
                if(pid_son==0){//子进程
                    //输入
                    if(i){//只要不是第一个文件，都要去共享文件读输入
                        close(0);
                        int FI=open(PipePath,O_RDONLY); //只读打开管道文件
                    }
                    else if(CMDlist->cmd[0].RedirectionIn){//第一个文件有重定向输入
                        close(0);//关闭标准输入
                        int FI=open(CMDlist->cmd[0].RedirectionInFile,O_RDONLY/*只读*/);
                    }
                    //输出
                    if(i!=CMDlist->num){//只要不是最后一个文件，都要去共享文件写输出
                        close(1);
                        int FO=open(PipePath,O_WRONLY/*只写*/ | O_CREAT/*自动创建*/ | O_TRUNC/*打开时清空*/, 0666/*所有用户有读写权限*/);
                    }
                    else if(CMDlist->cmd[CMDlist->num].RedirectionOut){//最后一个文件有重定向输出
                        close(1);
                        int FO=open(CMDlist->cmd[CMDlist->num].RedirectionOutFile,O_WRONLY/*只写*/ | O_CREAT/*自动创建*/ | O_TRUNC/*打开时清空*/, 0666/*所有用户有读写权限*/);
                    }
                    else if(CMDlist->cmd[CMDlist->num].RedirectionAppend){//最后一个文件有重定向输出,追加
                        close(1);
                        int FO=open(CMDlist->cmd[CMDlist->num].RedirectionOutFile, O_RDWR/*读写*/ | O_CREAT/*自动创建*/ | O_APPEND/*追加*/, 0666);
                    }
                    //查找命令并执行
                    BuildIn(CMDlist->cmd[i]);
                }
                else{//父进程
                    waitpid(pid_son,NULL,0);//等待子进程完成
                }
            }
        }
        if(FI>=0)close(FI);//恢复标准输入
        if(FO>=0)close(FO);//恢复标准输出
        dup2(stdin_copy, 0);
        dup2(stdout_copy, 1);
        close(stdin_copy);
        close(stdout_copy);
        exit(0);//退出子进程
    }
    else{//在父进程执行
        // FILE *fp=fopen("temp.txt","w");//打开临时文件
        // fprintf(fp,"111");
        // fclose(fp);
        JobList->List[JobList->num].pid=pid;//将子进程加入进程表
        strcpy(JobList->List[JobList->num].name,CMDlist->cmd[0].SplitCMD[0]);
        JobList->List[JobList->num].type=CMDlist->type;
        JobList->num+=1;
        getcwd(path,sizeof(path));//得到当前目录
        setenv("PARENT", path, 1);//设置父进程环境变量
        if(CMDlist->type==BG){//是后台进程
            JobList->List[FindJob(pid)].status=RUN;//状态为运行,可以开始运行
        }
        else{//是前台进程
            JobList->List[FindJob(pid)].status=RUN;//状态为运行,可以开始运行
            waitpid(pid,NULL,WUNTRACED);//等待子进程结束
            DeleteJob(pid);
        }
    }
    return;
}
int FindJob(int pid){
    for(int i=JobList->num-1;i>=0;i--){
        if(JobList->List[i].pid==pid) return i;
    }
    return -1;
}
void DeleteJob(pid_t pid){//删除进程表中的进程(前台进程)
    int i;
    for(i=JobList->num-1;i>=0;i--){//从后往前找
        if(JobList->List[i].pid==pid) break;
    }
    for(;i<JobList->num-1;i++){//将后面的进程向前移动
        JobList->List[i]=JobList->List[i+1];
    }
    JobList->num-=1;
}
int BuildIn(struct command cmd){//内建命令
    if(strcmp(cmd.SplitCMD[0],"bg")==0) {bg(); return 1;}
    else if(strcmp(cmd.SplitCMD[0],"clr")==0){clr();return 1;}
    else if(strcmp(cmd.SplitCMD[0],"cd")==0){cd(cmd.SplitCMD[1]);return 1;}
    else if(strcmp(cmd.SplitCMD[0],"dir")==0){dir(cmd.SplitCMD[1]);return 1;}
    else if(strcmp(cmd.SplitCMD[0],"echo")==0){echo(cmd);return 1;}
    else if(strcmp(cmd.SplitCMD[0],"exit")==0)exit(0);
    else if(strcmp(cmd.SplitCMD[0],"exec")==0){exec(cmd);return 1;}
    else if(strcmp(cmd.SplitCMD[0],"fg")==0){fg();return 1;}
    else if(strcmp(cmd.SplitCMD[0],"help")==0){help(cmd.SplitCMD[1]);return 1;}
    else if(strcmp(cmd.SplitCMD[0],"jobs")==0){jobs();return 1;}
    else if(strcmp(cmd.SplitCMD[0],"pwd")==0){pwd();return 1;}
    else if(strcmp(cmd.SplitCMD[0],"set")==0){set(cmd.SplitCMD[1]);return 1;}
    else if(strcmp(cmd.SplitCMD[0],"test")==0){if(test(cmd)) printf("TRUE\n");else printf("FALSE\n");return 1;}
    else if(strcmp(cmd.SplitCMD[0],"time")==0){Time();return 1;}
    else if(strcmp(cmd.SplitCMD[0],"umask")==0){Umask(cmd.SplitCMD[1]);return 1;}
    else return 0;
}

void bg(){
    for(int i=JobList->num-1;i>0;i--){//从后往前找，找到第一个SUSPEND的后台进程，将其状态改为前台RUN
        if(JobList->List[i].status==SUSPEND&&JobList->List[i].type==BG){
            JobList->List[i].status = RUN;
            kill(JobList->List[i].pid,SIGCONT);//用kill函数发送信号，重新激活进程
            printf("[%d] %d\t\t%s\n",i,JobList->List[i].pid,JobList->List[i].name);
            break;
        }
    }
}
void clr(){
    printf("\033[2J");//清屏
    printf("\033[0;0H");//光标移动到左上角
}
void cd(char* path){

    if(chdir(path)==-1) printf("error: failed to change directory\n");//改变工作目录
    else{
        setenv("PWD",getcwd(NULL,0), 1);//设置工作目录环境变量(后面能被getcwd读取)
    }
}
void dir(char* path){
    DIR *dir;//目录指针
    if(strcmp(path,"\0")==0) dir=opendir(".");//打开当前目录
    else dir=opendir(path);//打开指定目录
    struct dirent *ptr;//目录结构体指针
    if(dir==NULL) {printf("error: failed to open directory\n");return;}
    while((ptr=readdir(dir))!=NULL){
        if(ptr->d_name[0]!='.')//不显示隐藏文件
        printf("%s\n",ptr->d_name);
    }
    closedir(dir);
}
void echo(struct command cmd){
    if (cmd.num == 1) {//没有参数
        printf("\n");
        return;
    }
    else if(cmd.SplitCMD[1][0]=='$'){//环境变量
        char *temp=getenv(cmd.SplitCMD[1]+1);//得到环境变量的值
        if(temp==NULL) printf("\n");
        else printf("%s\n",temp);
        return;
    }
    else if(cmd.SplitCMD[1][0]=='\"'){//字符串
        for(int i=2;i<cmd.num&&cmd.SplitCMD[i][0]!='\"'/*最后一个是引号*/;i++){
            printf("%s ",cmd.SplitCMD[i]);
        }
    }
    else{//只有一个字符串
        printf("%s\n",cmd.SplitCMD[1]);
    }
    printf("\n");
}

void exec(struct command cmd){
    char *argv[MAXSECTION];
    for(int j=2;j<cmd.num;j++){//生成参数列表
        argv[j-2]=(char*)malloc(sizeof(char)*MAXLENGTH);
        strcpy(argv[j-2],cmd.SplitCMD[j]);
    }
    argv[cmd.num-2]=NULL;//参数列表最后一位为NULL
    execvp(cmd.SplitCMD[1]/*文件名*/,argv/*参数列表*/);//execvp()会从环境变量所指的目录中查找符合参数 file 的文件名, 找到后执行该文件
}
void fg(){
    for(int i=JobList->num-1;i>0;i--){//从后往前找，找到第一个SUSPEND的后台进程，将其状态改为前台RUN
        if(JobList->List[i].status==SUSPEND&&JobList->List[i].type==BG){
            JobList->List[i].status = RUN;
            JobList->List[i].type = FG;
            kill(JobList->List[i].pid,SIGCONT);//用kill函数发送信号，重新激活进程
            printf("[%d] %d\t\t%s\n",i,JobList->List[i].pid,JobList->List[i].name);
            waitpid(JobList->List[i].pid,NULL,WUNTRACED);//等待子进程结束
            DeleteJob(JobList->List[i].pid);
            break;
        }
    }
}
void help(char* str){
    FILE* fp=fopen("README.md","r");//打开用户手册
    char temp[MAXLENGTH];
    while(!feof(fp)){
        fgets(temp,MAXLENGTH,fp);//读取一行
        if(strncmp(temp,str,strlen(str)/*前几个字符匹配*/)==0) 
            {printf("%s\n",temp);break;}//如果包含关键词，输出
    }
}
void jobs(){
    for(int i=JobList->num-1;i>0;i--){
        if(JobList->List[i].type==BG){//显示后台进程
            printf("[%d] %d\t\t%s\n",i,JobList->List[i].pid,JobList->List[i].name);
        }
    }
}
void pwd(){
    printf("%s\n",getenv("PWD"));//输出工作目录
}
void set(char* str){
    char* temp=strtok(str,"=");//分解字符串
    char* temp2=strtok(NULL,"=");
    setenv(temp,temp2,1);//设置环境变量
}
int test(struct command cmd){
    if(strcmp(cmd.SplitCMD[2],"-eq")==0){
        return strcmp(cmd.SplitCMD[1],cmd.SplitCMD[3])==0;
    }
    else if(strcmp(cmd.SplitCMD[2],"-ne")==0){
        return strcmp(cmd.SplitCMD[1],cmd.SplitCMD[3])!=0;
    }
    else if(strcmp(cmd.SplitCMD[2],"-gt")==0){
        return strcmp(cmd.SplitCMD[1],cmd.SplitCMD[3])>0;
    }
    else if(strcmp(cmd.SplitCMD[2],"-lt")==0){
        return strcmp(cmd.SplitCMD[1],cmd.SplitCMD[3])<0;
    }
    else if(strcmp(cmd.SplitCMD[2],"-ge")==0){
        return strcmp(cmd.SplitCMD[1],cmd.SplitCMD[3])>=0;
    }
    else if(strcmp(cmd.SplitCMD[2],"-le")==0){
        return strcmp(cmd.SplitCMD[1],cmd.SplitCMD[3])<=0;
    }
}
void Time(){
    time_t t=time(0);
    char* date=(char*)ctime(&t);//将时间转换为字符串
    printf("%s\n",date);//输出当前时间
}
void Umask(char* str){
    mode_t mode;
    if(strcmp(str,"")==0) //如果没有参数，就将umask设置为最大权限
        {
            mode=umask(0);//umask(0)意思就是0取反再创建文件时权限相与，即0777&这个文件，得到这个文件的权限
            umask(mode);
            printf("%04o\n",mode);
        }
    else{
        mode=atoi(str);//将字符串转换为数字
        umask(mode);
    }
}
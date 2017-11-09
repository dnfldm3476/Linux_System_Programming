#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <ctype.h>

#define MAX_ARG 16
#define MAX_OPT 100
#define DIRECTORY_SIZE MAXNAMLEN

void Sel_option(int argc, char *argv[]);
void f_option(char *argv);
void t_option(char *argv);
void c_option(char *argv);
void n_option(char *argv);
void m_option(char *argv);
void w_option();
void e_option();
void l_option();
void v_option();
void s_sort();
void s_option(char *argv[]);
void s_opt(char *name,char *argv[]);
void proc_Make(char *val);
void option();

char (*c_parsing(char *argv))[100];
char *t_state(char *buf,char *buf2);
char (*m_parsing(char *argv))[1024];

void removeBSN(char str[]);

char **int_sort(int cnt,char *A[]);
char **str_sort(int cnt,char *A[]);

typedef enum{OPTION,NEUTRAL,FSTATE,MSTATE,CSTATE,NSTATE,TSTATE,KSTATE,SSTATE,OSTATE} STATUS;
typedef struct Option{
    bool f_opt,t_opt,c_opt,n_opt,m_opt,k_opt,w_opt,e_opt,l_opt,v_opt,r_opt,s_opt,o_opt;
    char *F_PID[MAX_OPT];
    char *M_PID[MAX_OPT];
    char *C_PID[MAX_OPT];
    char *N_PID[MAX_OPT];
    char *T_PID[MAX_OPT];
    char *K_KEY[MAX_OPT];
    char *S_NAM[MAX_OPT];
    char *O_NAM;
    int f_cnt;
    int m_cnt;
    int c_cnt;
    int n_cnt;
    int t_cnt;
    int k_cnt;
    int s_cnt;
}OPT;

typedef struct SOPT{
    bool F,C,I,S,E;
}S_opt;

OPT opt;
S_opt s_val;

int main(int argc, char* argv[])
{
    Sel_option(argc, argv);
    fprintf(stdout,">: ssu_lsproc start. :<\n");
    int fd;
    int std_out = dup(1);
    int std_err = dup(2);

    if(opt.o_opt == true){
        if((fd = creat(opt.O_NAM,0666)) !=-1)
            dup2(fd,1);
        dup2(fd,2);
        if(fd != -1)
            printf("!--Successfully Redirected : %s--!\n",opt.O_NAM);
        else
            printf("!--FAIL Redirected : %s--!\n",opt.O_NAM);
    }

    option();

    if(opt.o_opt == true){
        close(fd);
        dup2(std_out,1);
        dup2(std_err,2);
    }

    fprintf(stdout,">: ssu_lsproc terminated. :<\n");
    exit(0);
}

char **str_sort(int cnt, char *A[])
{
    char *tmp;
    int i,j;
    if(cnt > MAX_ARG)
        cnt = MAX_ARG;
    for(i=0;i<cnt;i++){
        for(j=0;j<cnt;j++){
            if(strcmp(A[i],A[j]) < 0)
            { 
                tmp = A[i];
                A[i] = A[j];
                A[j] = tmp;
            }
        }
    }
    return A;
}
char **int_sort(int cnt,char *A[])
{
    int B[cnt];
    char *tmp_s;
    int tmp;
    int i=0;
    if(cnt > MAX_ARG)
        cnt = MAX_ARG;
    // for(i=0 ;i<cnt; i++)
    //      printf("변환 전 A[%d] : %s\n",i,A[i]);
    for(i=0;i<cnt;i++)
    {
        B[i] = atoi(A[i]);
    }
    for(i=0 ;i<cnt;i++){
        for(int j=0;j<cnt;j++){
            if(B[i] < B[j])
            {
                tmp = B[i];
                B[i] = B[j];
                B[j] = tmp;
                tmp_s = A[i];
                A[i] = A[j];
                A[j] = tmp_s;
            }
        }
    }
    //   for(i=0 ;i<cnt; i++)
    //      printf("변환 후 A[%d] : %s\n",i,A[i]);
    return A;
}
void removeBSN(char str[])
{
    int len = strlen(str);
    str[len-1] = 0;
}
void option()
{
    int pid = getpid();
    char PID[MAX_OPT];
    sprintf(PID,"%d",pid);
    char *op;
    if(opt.r_opt == true)
    {
        int_sort(opt.f_cnt,opt.F_PID);
        int_sort(opt.t_cnt,opt.T_PID);
        int_sort(opt.c_cnt,opt.C_PID);
        int_sort(opt.n_cnt,opt.N_PID);
        int_sort(opt.m_cnt,opt.M_PID);
        str_sort(opt.k_cnt,opt.K_KEY);
    }
    if(opt.f_opt == true)
    {
        if(opt.f_cnt > MAX_ARG)
        {
            for(int i = MAX_ARG;i<opt.f_cnt;i++)
                printf("Maxinum Number of Argument Exceeded. :: %s\n",opt.F_PID[i]);
            opt.f_cnt = MAX_ARG;
        }
        else if(opt.f_cnt ==0)
        {
            opt.f_cnt =1;
            opt.F_PID[0] = PID;
        }
        op = "f";
        proc_Make(op);
    }
    if(opt.t_opt == true)
    {
        if(opt.t_cnt > MAX_ARG) 
        {
            for(int i = MAX_ARG;i<opt.t_cnt;i++)
                printf("Maxinum Number of Argument Exceeded. :: %s\n",opt.T_PID[i]);
            opt.t_cnt = MAX_ARG;
        }
        else if(opt.t_cnt ==0)
        {
            opt.t_cnt =1;
            opt.T_PID[0] = PID;
        }
        op = "t";
        proc_Make(op);
    }
    if(opt.c_opt == true)
    {
        if(opt.c_cnt > MAX_ARG )
        {
            for(int i = MAX_ARG;i<opt.c_cnt;i++)
                printf("Maxinum Number of Argument Exceeded. :: %s\n",opt.C_PID[i]);
            opt.c_cnt = MAX_ARG;
        }
        else if(opt.c_cnt ==0)
        {
            opt.c_cnt =1;
            opt.C_PID[0] = PID;
        }
        op = "c";
        proc_Make(op);
    }
    if(opt.n_opt == true)
    {
        if(opt.n_cnt > MAX_ARG)
        {
            for(int i = MAX_ARG;i<opt.n_cnt;i++)
                printf("Maxinum Number of Argument Exceeded. :: %s\n",opt.N_PID[i]);
            opt.n_cnt = MAX_ARG;
        }
        else if(opt.n_cnt ==0)
        {
            opt.n_cnt =1;
            opt.N_PID[0] = PID;
        }
        op = "n";
        proc_Make(op);
    }
    if(opt.m_opt == true)
    {
        if(opt.m_cnt > MAX_ARG)
        {
            for(int i = MAX_ARG;i<opt.m_cnt;i++)
                printf("Maxinum Number of Argument Exceeded. :: %s\n",opt.M_PID[i]);
            opt.m_cnt = MAX_ARG;
        }
        else if(opt.m_cnt ==0)
        {
            opt.m_cnt =1;
            opt.M_PID[0] = PID;
        }
        if(opt.k_cnt > MAX_ARG)
        {
            for(int i = MAX_ARG;i<opt.k_cnt;i++)
                printf("Maxinum Number of Argument Exceeded. :: %s\n",opt.K_KEY[i]);
            opt.k_cnt = MAX_ARG;
        }
        op = "m";
        proc_Make(op);
    }
    if(opt.w_opt == true)
    {
        op ="w";
        proc_Make(op);
    }
    if(opt.e_opt == true)
    {
        op ="e";
        proc_Make(op);
    }
    if(opt.l_opt == true)
    {
        op ="l";
        proc_Make(op);
    }
    if(opt.v_opt == true)
    {
        op ="v";
        proc_Make(op);
    }
    if(opt.s_opt == true)
    {
        op ="s";
        proc_Make(op);
    }
}
void proc_Make(char *val)
{
    int pid, status;

    pid = fork();
    if(pid < 0)
    {
        fprintf(stderr,"FORK ERROR\n");
        exit(0);
    }
    else if(pid ==0){ // 자식프로세스 시작
        switch(val[0]){
            case 'f' :
                for(int i=0 ;i < opt.f_cnt ;i++){
                    if(opt.f_cnt > 1)
                        printf("([/proc/%s/fd])\n",opt.F_PID[i]);
                    f_option(opt.F_PID[i]);
                }
                break;
            case 't' :
                for(int i =0;i < opt.t_cnt ;i++){
                    if(opt.t_cnt > 1)
                        printf("([/proc/%s/status])\n",opt.T_PID[i]);
                    t_option(opt.T_PID[i]);
                }
                break;
            case 'c' :
                for(int i =0;i < opt.c_cnt ;i++){
                    if(opt.c_cnt > 1)
                        printf("([/proc/%s/cmdline])\n",opt.C_PID[i]);
                    c_option(opt.C_PID[i]);
                }
                break;
            case 'n' :
                for(int i =0; i < opt.n_cnt ;i++){
                    if(opt.n_cnt > 1)
                        printf("([/proc/%s/io])\n",opt.N_PID[i]);
                    n_option(opt.N_PID[i]);
                }
                break;
            case 'm' : 
                for(int i =0;i < opt.m_cnt ;i++){
                    if(opt.m_cnt > 1)
                        printf("([/proc/%s/environ])\n",opt.M_PID[i]);
                    m_option(opt.M_PID[i]);
                }
                break;
            case 'w' : 
                w_option();
                break;
            case 'e' : 
                e_option();
                break;
            case 'l' : 
                l_option();
                break;
            case 'v' : 
                v_option();
                break;
            case 's' : 
                s_option(opt.S_NAM);  //배열 넣어야함
                break;
        }

        exit(1);
    }

    else{
        wait(&status);
    }

}
void s_option(char *argv[])
{
    DIR *dirp;
    FILE *fp;
    struct dirent *dentry;
    struct stat statbuf;
    int uid = getuid();
    char pathname[100];
    char buf[20];
    char filename[DIRECTORY_SIZE+1];

    chdir("/proc");
    getcwd(pathname,100);
    if((dirp =opendir(pathname)) == NULL){
        fprintf(stderr,"opendir error %s\n",pathname);
    }
    else {
        while((dentry = readdir(dirp)) != NULL){
            //       printf("name : %s\n",dentry->d_name);
            if(dentry->d_ino==0)
                continue;
            memcpy(filename,dentry->d_name,DIRECTORY_SIZE);

            if(!isdigit(filename[0])) // 숫자가 아니면 돌리기
                continue;

            stat(filename, &statbuf);
            //    fprintf(stderr,"stat error for %s\n",filename);
            if((statbuf.st_mode & S_IFMT) == S_IFDIR)
            {
                chdir(filename);
                if((fp = fopen("status","r")) ==NULL)
                    fprintf(stderr,"/proc/filename/status can't read\n");
                else{
                    while(1){
                        fscanf(fp,"%s",buf);
                        if(strcmp(buf,"Uid:") == 0){
                            fscanf(fp,"%s",buf);
                            break;
                        }
                    }   
                    int id = atoi(buf);

                    if(id==uid)
                        s_opt(filename,argv);
                }

                fclose(fp);
                chdir("..");
            }
        }
    }
}

void s_sort()
{
    s_val.F = false;
    s_val.C = false;
    s_val.I = false;
    s_val.S = false;
    s_val.E = false;
    int cnt = opt.s_cnt;

    if(opt.s_cnt >5)
        cnt = 5;
    for(int i=0;i<cnt;i++)
    {
        if(strcmp(opt.S_NAM[i],"FILEDES") == 0)
            s_val.F = true;
        else if(strcmp(opt.S_NAM[i],"CMDLINE") == 0)
            s_val.C = true;
        else if(strcmp(opt.S_NAM[i],"IO") == 0)
            s_val.I = true;
        else if(strcmp(opt.S_NAM[i],"STAT") == 0)
            s_val.S = true;
        else if(strcmp(opt.S_NAM[i],"ENVIRON") == 0)
            s_val.E = true;
    }

}

void s_opt(char *name,char *argv[])
{
    int tmp=0;
    s_sort();
    bool f =s_val.F;
    bool c =s_val.C;
    bool i =s_val.I;
    bool s =s_val.S;
    bool e =s_val.E;

    while(1){
        if(argv[tmp] == NULL)
            break;
        if(tmp == 5)
            break;
        if(f == true){
            printf("## Attribute : FILEDES, Target Process ID : %s ##\n",name);
            f_option(name);
            f= false;
        }
        else if(c == true){
            printf("## Attribute : CMDLINE, Target Process ID : %s ##\n",name);
            c_option(name);
            c = false;
        }
        else if(i == true){
            printf("## Attribute : IO, Target Process ID : %s ##\n",name);
            n_option(name);
            i = false;
        }
        else if(s == true){
            printf("## Attribute : STAT, Target Process ID : %s ##\n",name);
            t_option(name);
            s = false;
        }
        else if(e == true){
            printf("## Attribute : ENVIRON, Target Process ID : %s ##\n",name);
            m_option(name);
            e=false;
        }
        else
            break;
        tmp++;
    }

}

void v_option()
{
    FILE *fp;
    char buf[1024];
    printf("([/proc/version])\n");
    if((fp = fopen("/proc/version","r")) == NULL)
        fprintf(stderr,"open error version\n");

    else {
        fgets(buf,1024,fp);
        printf("%s",buf);
        fclose(fp); 
    }
}

void l_option()
{
    FILE *fp;
    char sec[50],sec2[50];

    printf("([/proc/uptime])\n");

    if((fp = fopen("/proc/uptime","r")) ==NULL)
        fprintf(stderr,"open error for uptime\n");

    else {
        fscanf(fp,"%s %s",sec,sec2);
        printf("Process worked time : %s(sec)\n",sec);
        printf("Process idle time : %s(sec)\n",sec2);

        fclose(fp);
    }
}

void e_option()
{
    FILE *fp;
    char buf[1024][50];
    int i=0;

    chdir("/proc");
    printf("<<Supported Filesystems>>\n");

    if((fp =fopen("filesystems","r")) ==NULL)
        fprintf(stderr,"open error for filesystems\n");

    else {
        while(1){

            if(fscanf(fp,"%s",buf[i]) == EOF)
                break;
            if(strcmp(buf[i],"nodev") == 0){
                fscanf(fp,"%s",buf[i]);
                continue;
            }
            if(i%5 == 0)
                printf("%s", buf[i]);
            else if(i%5 ==4 )
                printf(", %s\n", buf[i]);
            else
                printf(", %s",buf[i]);
            i++;
        }
        printf("\n");
        fclose(fp);
    }
}
void w_option()
{
    FILE *fp;
    char *buf = (char *) malloc(sizeof(buf) * 1024);
    char CPU[10][10];
    char name[5];
    float average=0;
    int cpu_num = 0;
    bzero(buf,sizeof(buf));
    bzero(CPU,sizeof(CPU));
    printf("([/proc/interrupts])\n");
    chdir("/proc");
    if((fp = fopen("interrupts","r"))==NULL){
        fprintf(stderr,"open error for interrupts");
    }
    else{
        fgets(buf,1024,fp);

        int oft=0;

        while(sscanf(buf,"%s%n",CPU[cpu_num],&oft) == 1)
        {
            buf +=oft;
            cpu_num++;
        }
        printf("      ---Number of CPUs : %d---\n",cpu_num);
        printf("                   [Average] : [Description]\n");
        oft = 0;
        int num[cpu_num+1];
        bzero(buf,sizeof(buf));

        while(fgets(buf,1024,fp) != NULL){
            bzero(num,sizeof(num));
            sscanf(buf,"%s%n",name,&oft);
            if(name[0] =='0'||name[0] == '1'||name[0] == '2'|| name[0] =='3'|| name[0]=='4'||name[0]=='5'||
                    name[0]=='6'||name[0] =='7'||name[0]=='8'||name[0]=='9')
                continue;
            for(int i=0;i<=cpu_num;i++){
                sscanf(buf,"%d%n",&num[i],&oft);
                buf += oft;
            }

            for(int i=1;i<=cpu_num;i++)
                average = average +num[i];
            average = average / (float)cpu_num;
            removeBSN(buf);
            printf("       %15.03f <%c%c%c> : %s\n",average, name[0],name[1],name[2], buf+3);
            bzero(buf,sizeof(buf));
            average = 0;
        }
        fclose(fp);
    }

}
char (*m_parsing(char *argv))[1024]
{
    static char cmd[100][1024];
    char buf[4096];
    int fd, ret;
    int a = 0, b = 0;

    bzero(cmd, sizeof(cmd));

    if((fd = open("environ",O_RDONLY)) <0)
        fprintf(stderr,"/proc/%s/environ can't be read\n",argv);

    else{
        ret = read(fd,buf, sizeof(buf));
        close(fd);

        for(int i=0;i<ret;i++){
            if(buf[i] == '\0'){
                a++;
                b=0;
                continue;
            }
            else{
                cmd[a][b] = buf[i];
                b++;
            }
        }
    }
    return cmd;
}

void m_option(char *argv){
    char (*buf)[1024];
    char environ[100][100];
    int k=0;

    chdir("/proc");

    if(chdir(argv) == -1)
        fprintf(stderr,"/proc/%s/environ doesn't exist\n",argv);

    else{
        buf = m_parsing(argv);
        bzero(environ,sizeof(environ));

        for(int i=0;buf[i][0] != '\0';i++) // k옵션 - =앞에까지 짜르기
        {
            k=0;
            while(buf[i][k]!= '='){
                environ[i][k] = buf[i][k];
                k++;
            }
            //    printf("environ %s\n",environ[i]);
        }
        if (opt.k_opt == true && opt.k_cnt != 0)
        {
            for(int i =0;i<opt.k_cnt;i++)
            {
                for(int j=0;buf[j][0] !='\0';j++)
                {
                    if(strcmp(opt.K_KEY[i],environ[j]) == 0)
                        printf("%s\n",buf[j]);
                }
            }
        }
        else {
            for(int i = 0;buf[i] !=NULL ;i++){
                if(buf[i][0] !='\0')
                    printf("%s\n",buf[i]);
                else 
                    break;
            }
        }
    }
}

void n_option(char *argv){
    char buf[100][100];
    FILE *fp;
    int i=0;

    chdir("/proc");

    if(chdir(argv) == -1)
        fprintf(stderr,"/proc/%s/io doesn't exist\n",argv);

    else{
        if((fp = fopen("io","r")) ==NULL)
            fprintf(stderr,"/proc/%s/io can't be read\n",argv);

        else{
            while(1){
                if((fscanf(fp,"%s",buf[i]))== EOF) 
                    break;
                if(strcmp(buf[i],"rchar:") ==0)
                    printf("Characters read : ");
                else if(strcmp(buf[i],"wchar:") ==0)
                    printf("Characters written : ");
                else if(strcmp(buf[i],"syscr:") ==0)
                    printf("Read syscalls : ");
                else if(strcmp(buf[i],"syscw:") ==0)
                    printf("Write syscalls : ");
                else if(strcmp(buf[i],"read_bytes:") ==0)
                    printf("Bytes read : ");
                else if(strcmp(buf[i],"write_bytes:") ==0)
                    printf("Bytes written : ");
                else if(strcmp(buf[i],"cancelled_write_bytes:") ==0)
                    printf("Cancelled write bytes : ");
                else
                    printf("%s\n",buf[i]);
                i++;
            }
        }

    }
}
char (*c_parsing(char *argv))[100]
{
    static char cmd[100][100];
    int cmdfd, ret;
    char buf[128];

    bzero(buf, sizeof(buf));
    bzero(cmd, sizeof(cmd));

    if ((cmdfd = open("cmdline", O_RDONLY)) == -1)
        fprintf(stdout,"/proc/%s/cmdline can't be read\n",argv);

    else{
        ret = read(cmdfd, buf, sizeof(buf));
        close(cmdfd);
        int k=0;
        int t=0;
        for(int i=0; i<ret; i++){
            if(buf[i] == '\0'){
                k++;
                t=0;
                continue;
            }
            else{
                cmd[k][t] = buf[i];
                t++;
            }
        }
    }
    return cmd;
}
void c_option(char *argv)
{
    char (*buf)[100];
    chdir("/proc");
    if(chdir(argv) == -1)
        fprintf(stderr,"/proc/%s/cmdline doesn't exist\n",argv);
    else{
        buf = c_parsing(argv);
        for(int i = 0;buf[i][0] != '\0'; i++)
            printf("argv[%d] :  %s\n",i,buf[i]);

    }

}
void t_option(char *argv)
{
    FILE *fp;
    int i=0;
    char buf[100][100];
    bzero(buf,sizeof(buf));
    chdir("/proc");
    if(chdir(argv) ==-1)
        fprintf(stderr,"/proc/%s/status doesn't exist\n",argv);
    else{

        if((fp = fopen("status","r")) == NULL){
            fprintf(stderr,"/proc/%s/status can't be read\n",argv);
        }
        else{

            for(int i=0;i<50;i++)
                fscanf(fp,"%s",buf[i]);

            while(1){
                if(strcmp(buf[i],"Name:") ==0)
                    printf("%s %s\n",buf[i], buf[i+1]);
                if(strcmp(buf[i],"State:") ==0)
                    printf("%s %s\n",buf[i],t_state(buf[i+1],buf[i+2]));
                if(strcmp(buf[i],"Tgid:") ==0)
                    printf("%s %s\n",buf[i], buf[i+1]);
                if(strcmp(buf[i],"Pid:") ==0)
                    printf("%s %s\n",buf[i], buf[i+1]);
                if(strcmp(buf[i],"PPid:") ==0)
                    printf("%s %s\n",buf[i], buf[i+1]);
                if(strcmp(buf[i],"TracerPid:") ==0)
                    printf("%s %s\n",buf[i], buf[i+1]);
                if(strcmp(buf[i],"Uid:") ==0)
                    printf("%s %s\n",buf[i], buf[i+1]);
                if(strcmp(buf[i],"Gid:") ==0){
                    printf("%s %s\n",buf[i], buf[i+1]);
                    break;
                }
                i++;
            }
        }
    }

}
char *t_state(char *buf,char *buf2)
{   
    char *state = NULL;
    //    bzero(state,sizeof(state));
    //  state= malloc(100);
    if(strcmp(buf,"R")== 0)
        state ="Running";
    if(strcmp(buf,"S")== 0)
        state="Sleeping in an interruptible wait";
    if(strcmp(buf,"D")== 0)
        state= "Waiting in uninterruptible disk sleep";
    if(strcmp(buf,"Z")== 0)
        state =  "Zombie";
    if(strcmp(buf,"T")== 0){
        if(strcmp(buf2,"(stopped)") == 0)
            state = "Stopped or trace stopped";
        else if(strcmp(buf2,"(tracing stop)") == 0)
            state ="Tracing stop";
    }
    if(strcmp(buf,"X")== 0)
        state ="Dead";
    //strcpy(state,"Dead");
    return state; 
}
void f_option(char *argv)
{
    struct stat statbuf;
    struct dirent *dentry;
    DIR *dirp;
    char filename[100];
    char *pathname;
    char buf[1024] = {};
    pathname = malloc(1024);

    //작업폴더 이동
    chdir("/proc");
    if(chdir(argv)==-1)
        fprintf(stderr,"/proc/%s/fd doesn't exist\n",argv);

    else {
        if(chdir("fd") < 0)
            fprintf(stderr,"/proc/%s/fd can't be read\n",argv);

        else{
            getcwd(pathname,1024);

            if((dirp = opendir(pathname)) ==NULL)
            {
                fprintf(stderr,"opendir error \n");
                exit(1);
            }

            while((dentry = readdir(dirp)) != NULL){
                if(dentry->d_ino ==0)
                    continue;
                strcpy(filename,dentry->d_name);
                if(lstat(filename,&statbuf) == -1){
                    fprintf(stderr,"stat error for %s\n",filename);
                    exit(1);
                }
                if(S_ISLNK(statbuf.st_mode)){
                    readlink(filename,buf,1024);
                    printf("File Descriptor number : %s Opened File: %s\n",filename,buf);    //정보받아오기
                }
            }
        }
    }
}


void Sel_option(int argc,char *argv[])
{
    int i=1;
    STATUS state;
    state = NEUTRAL;
    int f=0,m=0,c=0,n=0,t=0,k=0,s=0;
    char *F_PID[MAX_OPT];
    char *M_PID[MAX_OPT];
    char *C_PID[MAX_OPT];
    char *N_PID[MAX_OPT];
    char *T_PID[MAX_OPT];
    char *K_KEY[MAX_OPT];
    char *S_NAM[MAX_OPT];
    opt.f_opt = false;
    opt.t_opt = false;
    opt.c_opt = false;
    opt.n_opt = false;
    opt.m_opt = false;
    opt.k_opt = false;
    opt.w_opt = false;
    opt.e_opt = false;
    opt.l_opt = false;
    opt.v_opt = false;
    opt.r_opt = false;
    opt.s_opt = false;
    opt.o_opt = false;

    while(1)
    {

        if(argc == i){
            break;
        }
        if(argv[i][0] == '-'){
            state = NEUTRAL;
        }
        switch(state)
        {
            case NEUTRAL :
                switch(argv[i][1])
                {
                    case 'f' :
                        i++;
                        state =FSTATE;
                        opt.f_opt = true;
                        continue;
                    case 't' :
                        i++;
                        state =TSTATE;
                        opt.t_opt = true;
                        continue;
                    case 'c' :
                        i++;
                        state =CSTATE;
                        opt.c_opt = true;
                        continue;
                    case 'n' :
                        i++;
                        state =NSTATE;
                        opt.n_opt = true;
                        //printf("n 옵션 실행\n");
                        continue;
                    case 'm' :
                        i++;
                        state =MSTATE;
                        opt.m_opt = true;
                        // printf("m 옵션 실행\n");
                        continue;
                    case 'k' : 
                        i++;
                        state = KSTATE;
                        if(opt.k_opt == false){
                            for(int tmp = 1;tmp < i;tmp++)
                            {
                                if(argv[tmp][0] == '-'){
                                    if(argv[tmp][1] == 'm')
                                        opt.k_opt = true;
                                }
                            }
                        }
                        // printf("k 읽기\n");
                        continue;

                    case 'w' :
                        i++;
                        state =NEUTRAL;
                        opt.w_opt = true;
                        //  printf("w 옵션 실행\n");
                        continue;
                    case 'e' :
                        i++;
                        state =NEUTRAL;
                        opt.e_opt = true;
                        //  printf("e 옵션 실행\n");
                        continue;
                    case 'l' :
                        i++;
                        state =NEUTRAL;
                        opt.l_opt = true;
                        //  printf("l 옵션 실행\n");
                        continue;
                    case 'v' :
                        i++;
                        state =NEUTRAL;
                        opt.v_opt = true;
                        //  printf("v 옵션 실행\n");
                        continue;
                    case 'r' :
                        i++;
                        state =NEUTRAL;
                        opt.r_opt = true;
                        //  printf("r 옵션 실행\n");
                        continue;
                    case 's' :
                        i++;
                        state =SSTATE;
                        opt.s_opt = true;
                        //  printf("s 옵션 실행\n");
                        continue;
                    case 'o' :
                        i++;
                        state =OSTATE;
                        opt.o_opt = true;
                        // printf("o 옵션 실행\n");
                        continue;
                    default : 
                        printf("이런 옵션은 없습니다.\n");
                        break;
                }
            case FSTATE : 
                F_PID[f] = argv[i];
                //   printf("f : %s\n",argv[i]);
                f++;
                i++;
                opt.f_cnt = f;
                //    printf(" i : %d\n",i);
                continue;
            case MSTATE : 
                //     printf("m에 배열 넣음\n");
                M_PID[m] = argv[i];
                m++;
                i++;
                opt.m_cnt = m;
                continue;
            case CSTATE : 
                // printf("c에 배열 넣음\n");
                C_PID[c] = argv[i];
                c++;
                i++;
                opt.c_cnt = c;
                continue;
            case NSTATE : 
                // printf("n에 배열 넣음\n");
                N_PID[n] = argv[i];
                n++;
                i++;
                opt.n_cnt = n;
                continue;
            case TSTATE : 
                // printf("t에 배열 넣음\n");
                T_PID[t] = argv[i];
                t++;
                i++;
                opt.t_cnt = t;
                continue;
            case KSTATE : 
                // printf("k에 배열 넣음\n");
                K_KEY[k] = argv[i];
                k++;
                i++;
                opt.k_cnt = k;
                continue;
            case SSTATE : 
                //  printf("k에 배열 넣음\n");
                S_NAM[s] = argv[i];
                // printf("s : %s\n",argv[i]);
                s++;
                i++;
                opt.s_cnt = s;
                continue;
            case OSTATE : 
                opt.O_NAM = argv[i];
                i++;
            default : 
                break;
                // printf("끝\n");

        }

    }
    for(int cmt = 0; cmt < f; cmt++)
        opt.F_PID[cmt] = F_PID[cmt];
    for(int cmt = 0; cmt < m; cmt++)
        opt.M_PID[cmt] = M_PID[cmt];
    for(int cmt = 0; cmt < c; cmt++)
        opt.C_PID[cmt] = C_PID[cmt];
    for(int cmt = 0; cmt < n; cmt++)
        opt.N_PID[cmt] = N_PID[cmt];
    for(int cmt = 0; cmt < t; cmt++)
        opt.T_PID[cmt] = T_PID[cmt];
    for(int cmt = 0; cmt < k; cmt++)
        opt.K_KEY[cmt] = K_KEY[cmt];
    for(int cmt = 0; cmt < s; cmt++)
        opt.S_NAM[cmt] = S_NAM[cmt];

}

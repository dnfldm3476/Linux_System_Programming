#include<stdio.h>
#include<stdlib.h>
#include<fcntl.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<dirent.h>
#include<sys/time.h>
#include<string.h>
#include<stdbool.h>
#include<pwd.h>

#define MAX_ARG 10
#define SECOND_TO_MICRO 1000000
#define DIRECTORY_SIZE MAXNAMLEN

typedef struct option{
    bool b;
    bool u;
    bool i;
    bool e;
    bool d;
    bool p;
    bool s;
    bool P;
    char *PATHNAME;
    char *USERNAME;
    char *STRING;
    int DEPTH;
}option ;
option str_opt;

char *argv[MAX_ARG+1];
void ssu_sed(int argc, char *argv[]);
void ssu_shell();
typedef enum{OPTION,NEUTRAL,OPT,INWORD} STATUS;

void do_grep(char *pathname,char *src_str, char *dest_str,option A);
void do_modify(char *filename, char *src_str, char *dest_str,option A);

int myStrcmp(char *A, char *B);
int stringLen(char string[]);
void *myMemcpy(void *dst, void *src, size_t n);
void ssu_runtime(struct timeval *begin_t, struct timeval *end_t);
bool compare(char *filename, option str_opt);


void ssu_runtime(struct timeval *begin_t, struct timeval *end_t)
{
    end_t->tv_sec -= begin_t ->tv_sec;

    if(end_t -> tv_usec < begin_t ->tv_usec){
        end_t -> tv_sec--;
        end_t -> tv_usec += SECOND_TO_MICRO;
    }

    end_t -> tv_usec -= begin_t -> tv_usec;
    printf("time : %ld.%03ld\n",end_t->tv_sec,end_t->tv_usec);
}

void removeBSN(char str[])
{
    int len = strlen(str);
    str[len-1] = 0;
}
int stringLen(char string[])
{
    int i=0;
    while(string[i] !='\0')
    {
        i++;
    }
    return i;
}
int myStrcmp(char *A,char *B)
{
    for(int i=0;A[i] !='\0' || B[i]!='\0';++i) {
        if(A[i] !=B[i])
            return 0;
    }
    return 1;
}
void *myMemcpy(void *dst, void *src, size_t n)
{
    char *d = dst, *s =src;
    while(n--)
        *d++ = *s++;

    return dst;
}

void parsing(char word[]){
    int argc = 0;
    int length=0;
    int slash_count = 0;
    char *slash ="\\";
    char *ptr;
    char *word2[11];
    // word2에 파싱해서 집어넣기
    ptr = strtok(word," ");  
    while(ptr != NULL)
    {
        word2[argc] = ptr;
        ptr = strtok(NULL," ");
        argc++;
    }
    if(argc > 10)
    {
        printf("Too many argument!\n");
        exit(0);
    }
    

    // argc 초기화
    int cnt = 0;
    for(int i=0;i<argc;i++){

        if(slash_count ==0)
            argv[cnt] = word2[i];

        // 역슬래쉬이면 이어서 출력

        if(slash_count ==1)
        {
            strcat(argv[cnt-1],word2[i]);
            slash_count =0;
            cnt--;
        }
        else if(strstr(argv[cnt],slash)){
            strcpy(argv[cnt] + strlen(argv[cnt])-1 ," " );
            slash_count =1;
        }
        cnt++;
    }

    argc = cnt; // 매개변수 갯수에 대한 새로운 값 
   
    if(!strcmp(argv[0],"ssu_sed")){
        ssu_sed(argc,argv);
    }
    else
        system(word);

    //초기화
    for(int i=0;i<argc;i++)
        argv[argc] = NULL;
    argc = 0;
}

int main(void)
{
    struct timeval begin_t,end_t;
    char word[255];
    int argc = 0;
    char buf[255];
    while(1){
        printf("20132419 $ ");

        fgets(word,100,stdin);
        if(word[0] =='\n')
            continue;
        removeBSN(word);  

        gettimeofday(&begin_t,NULL);

        parsing(word);

        gettimeofday(&end_t,NULL);
        printf("\n");
        ssu_runtime(&begin_t,&end_t);

    }   

    return 0;


}
void do_grep(char *pathname, char *src_str,char *dest_str, option str_opt){
    struct dirent *dentry;
    struct stat statbuf;
    DIR *dirp;
    char filename[DIRECTORY_SIZE+1];
    char *cwd;
    int depth_cnt = 0;
    int check =1;
    char *username;
    struct passwd *User;
    if(str_opt.u ==true){
        username = str_opt.USERNAME;
        if((User= getpwnam(username))==NULL)
            check =0;
    }


    cwd = getcwd(NULL,1024);

    if(( dirp = opendir(pathname)) ==NULL || chdir(pathname) == -1){
        fprintf(stderr,"opendir error\n");
        exit(1);
    }

    while((dentry = readdir(dirp)) != NULL)
    {
        if(dentry->d_ino ==0)
            continue;

        myMemcpy(filename,dentry->d_name,DIRECTORY_SIZE);

        if(myStrcmp(filename,".") ==1 ||myStrcmp(filename,"..") == 1)
            continue;

        if(stat(filename,&statbuf) == -1)
        {
            fprintf(stderr,"stat error for %s\n",filename);
            exit(1);
        }

        if((statbuf.st_mode & S_IFMT) == S_IFREG){
            if(str_opt.u==true){
                if(check && User->pw_uid == statbuf.st_uid )
                    do_modify(filename, src_str,dest_str, str_opt);
            }
            else if(str_opt.e == true){
                if( compare(filename,str_opt) == false){
                    do_modify(filename, src_str,dest_str, str_opt);
                } }
            // i가 설정되었을때 비교 함수가 string이 있다고 출력하면 수정함.
            else if(str_opt.i == true){
                if( compare(filename,str_opt) == true){
                    do_modify(filename, src_str,dest_str, str_opt);
                }
            }
            else
                do_modify(filename, src_str,dest_str, str_opt);
        }
        else if(S_ISDIR(statbuf.st_mode)!=0 )
        {
            if(str_opt.d==true)
            {
                if(str_opt.DEPTH != depth_cnt)
                    do_grep(filename, src_str,dest_str, str_opt);
            }
            else
                do_grep(filename, src_str,dest_str, str_opt);
            depth_cnt ++;
            //옵션 카운트
        }

    }
    chdir(cwd);
    closedir(dirp);
}
bool compare(char *filename, option str_opt)
{
    FILE *fp;
    fp = fopen(filename,"r+");
    int i=0, j=0;
    char buf[1024];
    char *com_str;
    int com_leng;
    bool check = false;

    com_str = str_opt.STRING;
    com_leng= stringLen(com_str);
    while(fgets(buf,1024,fp) != '\0')
    {
        i=0;
        j=0;
        while(buf[i] !='\0')
        {
            if(buf[i] == com_str[j]){
                i++;
                j++;
            }
            else if(buf[i] == com_str[0]){
                i++;
                j=1;
            }
            else{
                j=0;
                i++;
            }

            if( j== com_leng)
                check = true;
        }
    }
    fclose(fp);
    return check;
}

void do_modify(char *filename,char *src_str,char *dest_str,option str_opt)
{
    int fd;
    FILE *fp;
    char buf[1024];
    int line[1024];

    int line_count = 0;
    int k=0;
    bool check = false;
    bool check2 =false;

    char *fname =  "asdf";
    char *path = getcwd(NULL,1024);

    int src_leng = stringLen(src_str);
    int dest_leng = stringLen(dest_str);
    int gab = dest_leng - src_leng;

    fp = fopen(filename,"r+");

    int i=0;
    int cnt =0;

    if((fd = creat(fname,0666))<0){
        exit(1);
    }

    while(fgets(buf,1024,fp) != '\0')
    {
        line_count ++;
        int count =0;
        int j=0;
        i=0;
        while(buf[i] != '\0'){
            count++;
            i++;
        }

        i=0;
        while(buf[i] != '\0')
        {
            if(buf[i] == src_str[j]){
                i++;
                j++;
            }
            else if(buf[i] == src_str[0]){
                i++;
                j=1;
            }
            else{
                j=0;
                i++;
            }

            if( j== src_leng)
            {
                check2 =true;
                check = true;

                cnt = i;

                if(gab >= 0){
                    for(int k=count ; k>=i;k--){
                        buf[k+gab] = buf[k];
                    }
                }

                else if(gab<0)
                {
                    for(int k = i;k<count;k++)
                        buf[k+gab] = buf[k];
                }
                cnt = i;
                for(int k=0; k<dest_leng;k++){
                    buf[cnt-src_leng] = dest_str[k];
                    cnt++;
                }
            }
        }




        int c=0;
        while(buf[c] !='\n')
        {
            c++;
        }
        c= c+1;
        write(fd,buf,c);
        if(str_opt.p == true){
            if(check == true){
                printf("%s %d\n",path,line_count);

                check =false;
            }
        }
    }

    fputs(path,stdout);
    fputs(" ",stdout);
    fputs(filename,stdout);
    fputs(" : ",stdout);

    if(check2 == true){
        fputs("success\n",stdout);
    }
    else
        fputs("failed\n", stdout);


    remove(filename);
    rename(fname,filename);
    fclose(fp);

}

void ssu_sed(int argc, char *argv[]){
    char *pathname;
    char *src_str;
    char *dest_str;
    char *num[] = {"0","1","2","3","4","5","6","7","8","9"};
    STATUS state;
    state = NEUTRAL;
    int i=0; 
    int argc2 = 1;
    int cmt = 0;

    str_opt.b = false;
    str_opt.u = false;
    str_opt.i = false;
    str_opt.e = false;
    str_opt.d = false;
    str_opt.p = false;
    str_opt.s = false;
    str_opt.P = false;
    str_opt.PATHNAME = NULL;
    str_opt.USERNAME = NULL;
    str_opt.STRING = NULL;
    str_opt.STRING = NULL;
    str_opt.DEPTH = 0;

    for(int j=0;j<argc;j++)
    {
        // printf("%d argv : %s\n", j, argv[j]);
        write(1,"argv",4);
        write(1,num[j],1);
        write(1," : ",3);
        write(1,argv[j],stringLen(argv[j]));
        write(1,"\n",1);

    }
    if(argc <= 10){
    while(1)
    {
        if(argc-1 == i){
            do_grep(pathname,src_str,dest_str, str_opt);
            break;
        }
        switch(state)
        {
            case NEUTRAL :
                switch(argv[i+1][0])
                {
                    case '-' :
                        state = OPTION;
                        continue;
                    default : 
                        state = INWORD;
                        continue;
                }
            case OPTION :
                switch(argv[i+1][1])
                {
                    case 'b' :
                        printf("b옵션 실행 : \n");
                        i++;
                        str_opt.b= true;
                        state =NEUTRAL;
                        continue;
                    case 'u' :
                        i++;
                        str_opt.u= true;
                        cmt =1;
                        state =OPT;
                        continue;
                    case 'i' :
                        i++;
                        str_opt.i= true;
                        cmt = 2;
                        state =OPT;
                        continue;
                    case 'e' :
                        i++;
                        str_opt.e= true;
                        state =OPT;
                        cmt = 3;
                        continue;
                    case 'd' :
                        i++;
                        str_opt.d= true;
                        state =OPT;
                        cmt =4;
                        continue;
                    case 'p' :
                        i++;
                        str_opt.p= true;
                        state =NEUTRAL;
                        continue;
                    case 's' :
                        printf("s옵션 실행 : \n");
                        i++;
                        str_opt.s= true;
                        state =NEUTRAL;
                        continue;
                    case 'P' :
                        printf("P옵션 실행 : \n");
                        i++;
                        str_opt.P= true;
                        state =OPT;
                        cmt =5;
                        continue;
                    default : 
                        printf("OPTION error\n");
                        // 옵션 헬프문 출력
                        break;
                }
            case OPT :
                switch(cmt){
                    case 1 :
                        str_opt.USERNAME =argv[i+1];
                        i++;
                        state = NEUTRAL;
                        continue;
                    case 2 :
                        str_opt.STRING =argv[i+1];
                        i++;
                        state = NEUTRAL;
                        continue;
                    case 3 :
                        str_opt.STRING =argv[i+1];
                        i++;
                        state = NEUTRAL;
                        continue;
                    case 4 :
                        str_opt.DEPTH = atoi(argv[i+1]);
                        i++;
                        state = NEUTRAL;
                        continue;
                    case 5 :
                        str_opt.PATHNAME =argv[i+1];
                        i++;
                        state = NEUTRAL;
                        continue;
                }
            case INWORD :
                switch(argc2)
                {
                    case 1 :
                        pathname = argv[i+1];
                        argc2 ++;
                        i++;
                        state =NEUTRAL;
                        continue;
                    case 2 :
                        src_str = argv[i+1];
                        argc2 ++;
                        i++;
                        state =NEUTRAL;
                        continue;
                    case 3 :
                        dest_str = argv[i+1];
                        argc2 ++;
                        i++;
                        state =NEUTRAL;
                        continue;
                    default : 
                        printf("INWORD error \n");
                        break;
                }
            default : 
                printf("경로 잘못입력하셨습니다.\n");
                break;
        }


    }
    }

}


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <utime.h>
#include <dirent.h>
#include <ctype.h>

#define MAX_LINE 4096
#define BUFFER_SIZE 1024
#define FILE_FIFO "/tmp/fifo"
#define DIRECTORY_SIZE MAXNAMLEN
#define RW_NO 0 // readwrite의 옵션 미설정
#define RW_OK 1 // readwrite의 읽고 쓰기하기
#define W_NO 2 // readwrite의 읽기만 실행

typedef struct Option // 파싱 후 그 값을 저장할 구조체
{
    bool readonly;
    bool writeonly;
    int readwrite;
    bool s_opt;
    bool d_opt;
    bool t_opt;
    char req_file[256];
    char filename[256];
    pid_t daemon_pid;

}OPT;
OPT command;

void parsing(int argc, char *argv[]); // 프로그램 실행 시 받아온 인자들을 검사하고 구조체에 초기화함.
void t_option(struct stat buf); // vim의 t옵션 수행에 필요한 문자열 출력
void s_option(); // s옵션 수행
void d_option(char *name); // d 옵션 수행
void opt_Read(); // 요청한 파일을 읽기전용으로 열어서 표준출력에 출력
void opt_Write(); // vim에디터 생성
int check_Mod(struct stat beforeBuf); // 수정된 시간을 비교하여 파일이 수정되었는지 확인
char *make_Tmpfile(char *name); // 수정전의 파일과 같은 내용의 임시파일 생성
void lock_file(int lock); // 파일을 쓰기전에 잠근다.
void usr1_handler(int signo, siginfo_t *info, void *uarg); // SIGUSR1에 대한 액션 핸들러
void init(char *tmpname); // 액션 핸들러 설정 및 readwrite옵션들에 따른 수행
void write_fifo(); // FIFO파일에 요청하는 파일 적기
bool find_ofm(); // /proc에서 ssu_ofm을 찾아서 시그널 보내기
void cur_time(); // 현재 시각을 출력

int main(int argc, char *argv[])
{
    int fd;
    struct stat statbuf;
    struct utimbuf time_buf;
    char name[PATH_MAX] = { 0 };
    parsing(argc, argv); // 인자 파싱
    
    if ((fd = open(command.filename, O_RDWR | O_CREAT, 0666)) < 0) { // 요청하는 파일을 생성 또는 열기
        fprintf(stderr, "filename creat error\n");
        exit(1);
    }
    else 
        close(fd);

    if (command.t_opt == true) 
        t_option(statbuf);

    init(name);
  
    stat(command.filename, &statbuf); // 옵션 수행 후 파일의 상태가져오기

    if (command.t_opt == true && command.readonly == false) { // 수정여부 출력
        printf("##[Modification Time]##\n");
        if (check_Mod(statbuf) == 1)
            printf("- There was modification\n");
        else
            printf("- There was no modification\n");
    }
 
    if(command.s_opt == true && check_Mod(statbuf) == 1) // s 옵션 내용 출력
        s_option(statbuf);

    if (command.d_opt == true) { // d옵션 내용 출력
        if (check_Mod(statbuf) == 1)
            d_option(name);
    }

    remove(name); // 임시파일 제거

    if (command.writeonly == true || command.readwrite == RW_OK) {
        kill(command.daemon_pid, SIGUSR2);
    }
    exit(0);
}

void init(char *tmpname) // 핸들러 설정 및 읽기쓰기 옵션에 따른 수행
{
    struct sigaction sig_act;
    char req_file[BUFFER_SIZE] = { 0 };
    char answer[10];
    sigset_t sig_set;
    sigset_t pendingset;

    sigemptyset(&sig_set);
    sigemptyset(&pendingset);
    sigemptyset(&sig_act.sa_mask);
    sigaddset(&sig_set, SIGUSR1);
    sigprocmask(SIG_SETMASK, &sig_set, NULL); // SIGUSR1을 마스크 설정한다.
    sig_act.sa_flags = SA_SIGINFO;
    sig_act.sa_sigaction = usr1_handler;

    if (sigaction(SIGUSR1, &sig_act, NULL) != 0) { // SIGUSR1에 대한 핸들러 설정
        fprintf(stderr, "sigaction error\n");
        exit(1);
    }
    if(command.readonly == true) // 읽기 옵션 실행 시
        opt_Read();
    
    else if (command.readwrite == RW_OK) {
        opt_Read();
        printf("Would you like to modify '%s'? (yes/no)\n", command.req_file);
        while(1) {
            memset(answer, 0, sizeof(answer));
            scanf("%s", answer);
            if (strcmp(answer, "yes") == 0) { // rw옵션 시 w도 실행
                command.readwrite = RW_OK;
                break;
            }
            else if (strcmp(answer, "no") == 0) { // rw옵션 시 r만을 실행
                command.readwrite = W_NO;
                break;
            }
            else
                printf("다시 입력하세요\n");
        }
    }

    if (command.writeonly == true || command.readwrite == RW_OK) {
        if (find_ofm() == false){ // 시그널이 보낸적 없으면 실행 = ofm이 존재하지 않음
            fprintf(stderr, "ssu_ofm이 없습니다.\n");
            exit(1);
        }

        write_fifo(); // fifo파일에 적기

        while(1) { // 시그널을 받을 때 까지 기다림.
            if (sigpending(&pendingset) == 0) { // 유보된 시그널에 SIGUSR1이 있으면 루프를 나온다.
                if (sigismember(&pendingset, SIGUSR1)) {
                    break;
                }
                printf("waiting token...%s  ", command.req_file);
                if (command.t_opt == true)
                    cur_time();
                printf("\n");
                sleep(1);
            }
        }

      make_Tmpfile(tmpname); // vim 실행 전에 임시파일 생성
      sigprocmask(SIG_UNBLOCK, &sig_set, NULL); // SIGUSR1에 대한 마스크 해제
    }
}

void cur_time(){ // 현재 시각을 날짜 형식으로 출력
    struct tm *ctime;
    time_t now;
    time(&now);
    ctime = localtime(&now);
    printf("[%d-%02d-%02d %02d:%02d:%02d]",
            ctime-> tm_year + 1900,
            ctime-> tm_mon +1,
            ctime-> tm_mday,
            ctime-> tm_hour,
            ctime-> tm_min,
            ctime-> tm_sec);
}
bool find_ofm() // ssu_ofm 찾아서 시그널 보내기
{
    DIR *dirp;
    struct dirent *dentry;
    struct stat statbuf;
    char pathname[DIRECTORY_SIZE +1] = { 0 };
    char initpath[DIRECTORY_SIZE +1] = { 0 };
    char buf[20];
    char filename[DIRECTORY_SIZE +1];
    bool killCheck = false;

    getcwd(initpath, sizeof(initpath));
    chdir("/proc");
    getcwd(pathname, sizeof(pathname));

    if ((dirp = opendir(pathname)) == NULL) {  // proc에 있는 폴더들을 들어가본다.
        fprintf(stderr, " opendir error %s\n", pathname);
    }
    else {
        while ((dentry = readdir(dirp)) != NULL) {
            if (dentry -> d_ino == 0)
                continue;
            memcpy(filename, dentry -> d_name, DIRECTORY_SIZE);

            if (!isdigit(filename[0])) // 프로세스의 폴더가 아니라면
                continue;

            stat(filename, &statbuf);

            if ((statbuf.st_mode & S_IFMT) == S_IFDIR)
            {
                chdir(filename);
                int fd;
                memset(buf, 0, sizeof(buf)); // cmdline에 ssu_ofm으로 실행한 프로세스가 있는지 확인
                if ((fd = open("cmdline", O_RDONLY)) < 0) {
                    fprintf(stderr, "cmdline cant read\n");
                    exit(1);
                }
                else {
                    read(fd, buf, sizeof(buf));
                    if (strcmp(buf,"./ssu_ofm") == 0) { // 있다면 시그널을 보낸다.
                        killCheck = true;
                        if (kill(atoi(filename), SIGUSR1) == -1) {
                            fprintf(stderr, "filename : %s kill error\n", filename);
                            exit(1);
                        }
                    }
                }
                close(fd);
                chdir("..");
            }
        }
    }
    chdir(initpath);
    return killCheck;
}

void usr1_handler(int signo, siginfo_t *info, void *uarg) // SIGUSR1에 대한 액션 핸들러
{
    command.daemon_pid = info->si_pid;
    lock_file(1); // 파일을 잠근다.
    opt_Write(); // vim에디터 생성
    lock_file(0); // 파일에 대한 잠금을 푼다.
}

void write_fifo() // FIFO파일에 요청하는 파일을 쓰기.
{
    int fd;
    char buf[BUFFER_SIZE] = { 0 };
    char name[BUFFER_SIZE] = { 0 };
    
    if ((fd = open(FILE_FIFO, O_WRONLY)) <0) { // FIFO파일 열기
        fprintf(stderr, "fopen fifo error\n");
        exit(1);
    }

    strcpy(name, getcwd(NULL, 0));
    strcat(name, "/");
    strcat(name, command.filename);
    strcpy(command.filename, name);
    sprintf(buf, "%s", command.filename);
    if(write(fd, buf, sizeof(buf)) < 0) {
        fprintf(stderr, "write error\n");
        exit(1);
    }
}

void lock_file(int num)
{
    int fd;
    struct flock lock;
    if ((fd = open(command.filename, O_RDWR)) < 0){
        fprintf(stderr, "open error %s\n", command.filename);
        exit(1);
    }
    if (num == 1) { // 파일 잠그기
        lock.l_type = F_WRLCK;
        lock.l_start = 0;
        lock.l_whence = SEEK_SET;
        lock.l_len = 0;
        if (fcntl(fd, F_SETLK, &lock) == -1) {
            fprintf(stderr, "fcntl error\n");
            exit(1);
        }
    }
    else if (num == 0) { // 파일 잠금 풀기
        lock.l_type = F_UNLCK;
        lock.l_start = 0;
        lock.l_whence = SEEK_SET;
        lock.l_len = 0;
        if (fcntl(fd, F_SETLK, &lock) == -1) {
            fprintf(stderr, "fcntl unlock error\n");
            exit(1);
        }
    }
}
void req_filename(char *filename) // 요청하는 파일의 상대경로를 가져오기
{
    int j=0;
    for (int i=0; filename[i] != '\0'; i++) {
        if (filename[i] == '/') {
            j = i;
            continue;
        }
    }
    filename = filename + j + 1;
}
void parsing(int argc, char *argv[]) // 파싱
{
    command.readonly = false;
    command.writeonly = false;
    command.readwrite = RW_NO;
    command.t_opt = false;
    command.s_opt = false;
    command.d_opt = false;
    memset(command.filename, 0, sizeof(command.filename));
    memset(command.req_file, 0, sizeof(command.req_file)); // 구조체 초기화

    if (argc < 3) {
        fprintf(stderr, "인자가 부족\n");
        exit(1);
    }
    strcpy(command.filename, argv[1]);
    strcpy(command.req_file, argv[1]);

    if( strcmp(argv[2], "-r") == 0) 
        command.readonly = true;

    else if( strcmp(argv[2], "-w") == 0)
        command.writeonly = true;

    else if( strcmp(argv[2], "-rw") == 0) 
        command.readwrite = RW_OK;

    for(int i = 3; i <argc; i++)
    {
        if(argv[i][0] == '-') {
            switch(argv[i][1])
            {
                case 't' :
                    command.t_opt = true;
                    continue;
                case 's' :
                    command.s_opt = true;
                    continue;
                case 'd' :
                    command.d_opt = true;
                    continue;
                default :
                        if (strcmp(argv[i], "-w") == 0 || strcmp(argv[i], "-rw") == 0 || strcmp(argv[i], "-r") == 0 ){
                            fprintf(stderr, "중복된 읽기 또는 쓰기 옵션이 입력되었습니다.\n");
                            exit(1);
                        }
                    continue;
            }
        }
    }
}

void t_option(struct stat buf) // t옵션의 문자열들 출력
{
    struct tm *mtime;

    mtime = localtime(&buf.st_mtime);
    printf("##[Modification Time]##\n");
    fprintf(stdout, "Last Modification time of '%s': ", command.req_file);
    printf("[%d-%02d-%02d %02d:%02d:%02d]\n",
            mtime -> tm_year + 1900,
            mtime-> tm_mon +1,
            mtime-> tm_mday,
            mtime-> tm_hour,
            mtime-> tm_min,
            mtime-> tm_sec);

    printf("Current time: ");
    cur_time();
    printf("\n");
    sleep(1);

    printf("Waiting for Token...%s  ", command.req_file);
    cur_time();
    printf("\n");
}

void s_option(struct stat beforeBuf) // s옵션의 문자열들 출력
{
    struct stat afterBuf;

    printf("##[File size]##\n");
    printf("-- Before modification : %ld(bytes)\n", beforeBuf.st_size);

    if (stat(command.filename, &afterBuf) < 0) {
        fprintf(stderr, "stat error\n");
        exit(1);
    }

    printf("-- After modification : %ld(bytes)\n", afterBuf.st_size);
}
void d_option(char *name) // d옵션의 문자열들 출력
{
    int status;
    pid_t pid;
    printf("##[Compare with Previous File]##\n");
    if ((pid = fork()) == 0) {
        if (execl("/usr/bin/diff", "diff", name, command.filename, NULL) < 0) {
            fprintf(stderr, "execl error\n");
            exit(1);
        }
    }

    else
        wait(&status);
}

char *make_Tmpfile(char *name) // d옵션이 설정 시 파일수정 전에 일어난다.
{
    struct utimbuf time_buf;
    struct stat statbuf;
    char buf[MAX_LINE];
    FILE *tmpfp, *fp;
    char pathname[PATH_MAX];
    getcwd(pathname, PATH_MAX);
    strcpy(name, tempnam(pathname, '\0'));

    stat(command.filename, &statbuf); // 파일의 접근시간과 수정시간을 저장한다.
    time_buf.actime = statbuf.st_atime;
    time_buf.modtime = statbuf.st_mtime;

    if ((fp = fopen(command.filename,"r")) == NULL) { 
        fprintf(stderr, "fopen error\n");
        exit(1);
    }

    if ((tmpfp = fopen(name, "w")) == NULL) { // 임시 파일을 쓰기로 열기
        fprintf(stderr, "tmpfile fopen error\n");
        exit(1);
    }

    while(fgets(buf, sizeof(buf), fp) != NULL) { // 임시파일에 쓰기
        fputs(buf, tmpfp);
    }
    fclose(fp);
    fclose(tmpfp);
 
    utime(command.filename, &time_buf); 

    return name;
}

void opt_Read() // 파일읽어서 표준출력에 출력
{
    FILE *fp;
    char buf[BUFFER_SIZE] = { 0 };

    if ((fp = fopen(command.filename,"r")) ==NULL){
        fprintf(stderr, "open error\n");
        exit(1);
    }

    while(fgets(buf, BUFFER_SIZE, fp) != NULL) 
        fputs(buf, stdout);

    fclose(fp);
}
void opt_Write() // vim에디터 생성
{
    pid_t pid;
    int status;
    if ((pid = fork()) == 0)
    {
        if (execl("/usr/bin/vim", "vim", command.filename, NULL) < 0) {
            fprintf(stderr, "execv error\n");
            exit(1);
        }
    }
    else    // 부모가 기다리는중임
        wait(&status);

}

int check_Mod(struct stat beforeBuf) // 수정된 시간을 가지고 수정여부를 확인함.
{
    struct stat afterBuf;
    stat(command.filename, &afterBuf);

    if(afterBuf.st_mtime != beforeBuf.st_mtime)  // 수정된 시간이 다르다면!
        return 1;
    return 0;
}


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <syslog.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdbool.h>
#include <sys/msg.h>
#include <ctype.h>
#include <dirent.h>
#include <pwd.h>
#include <time.h>

#define BUFFER_SIZE 1024
#define FILE_FIFO "/tmp/fifo"
#define DIRECTORY_SIZE MAXNAMLEN

typedef enum {NEUTRAL, NSTATE, PSTATE} STATUS; 

struct ssu_msgbuf {
    long msg_type; // pid
    char msg_text[BUFFER_SIZE]; // pid
};

struct option {
    bool l_opt, n_opt, t_opt, p_opt, id_opt;
    char DIRECTORY[DIRECTORY_SIZE + 1];
    unsigned int QUEUE_NUM;
    char string[BUFFER_SIZE]; //  log.txt에 적을 문자열들
    char string2[BUFFER_SIZE];
};

struct option command; // 인자로 받은 것을 파싱해서 관리하는 구조체
char filename[BUFFER_SIZE]; // ofm이 관리하는 파일의 절대 경로
char fifo_name[BUFFER_SIZE]; // FIFO파일의 이름
char req_string[BUFFER_SIZE]; // 두 번째 메시지 큐에 담을 내용
int msg_id; // 요청한 프로세스의 정보를 담을 메시지 큐 ID
int msg_stringID; // l옵션에 적을 내용을 담을 메시지 큐 ID
int fifofd; // FIFO파일의 파일디스크립터
int logfd; // log파일의 파일디스크립터

int ssu_daemon_init(void); // 프로세스를 데몬프로세스로 만듬
void send_usr1(); // SIGUSR1을 보낸다.
void ssu_usr1(); // SIGUSR1에 대한 액션을 설정
void ssu_usr2(); // SIGUSR2에 대한 액션을 설정
void usr1_handler(int signo, siginfo_t *info, void *uarg); // SIGUSR1의 액션에 대한 핸들러 
void usr2_handler(int signo, siginfo_t *info, void *uarg); // SIGUSR2의 액션에 대한 핸들러
void term_handler(int signo); // SIGTERM에 대한 핸들러
bool check_right(); // 공유파일에 대한 접근 권한 확인
void make_msg(); // 메시지 큐 생성, l 옵션시 메시지 큐를 하나 더 생성
void send_msg(pid_t pid, char *string); // 메시지 큐에 PID와 문자열을 저장, l 옵션의 메시지 큐에도 저장
void init(char *argv); // 공유파일을 만들고, -p옵션시 디렉토리에 파일 생성 및 로그파일 생성, 메시지 큐를 만드는 함수 및 시그널 설정 함수 호출
void write_log(pid_t pid, char *filename); // 로그파일에 옵션에 따른 내용을 적어준다.
void parsing(int argc, char *argv[]); // 프로그램실행시 받아온 가변인자를 파싱한다.
void parsing_fifo(char *fifo, char *path); // 절대경로의 파일 경로를 상대경로로 바꿔서 반환한다.
char *id_option(char *pid); // -id옵션을 위해 UID값을 가져온다.
char *cur_time(char *curtime); // 날짜 형식의 현재시간을 저장한 문자열 반환

int main(int argc, char *argv[])
{
    char name[BUFFER_SIZE];
    mkfifo(FILE_FIFO, 0666);

    printf("Daemon Process Initialization\n");

    strcpy(name, getcwd(NULL, 0));
    strcpy(filename, name);
    if (ssu_daemon_init() < 0) { // 데몬프로세스 생성
        fprintf(stderr, "ssu_daemon_init failed\n");
        exit(1);
    }

    parsing(argc, argv); // 인자 파싱
    init(argv[1]); // 핸들러 및 초기 설정

    while(1) {
        pause(); // 시그널을 기다림
    } 
    
    exit(0);
}
void init(char *argv) // 공유파일 만들기, 메시지 큐 만들기, 파일이름 저장하기,  시그널 핸들러 설정하기
{
    char tmpname[BUFFER_SIZE] = { 0 };
    char buf_pid[BUFFER_SIZE] = { 0 };
    char queue_num[BUFFER_SIZE] = { 0 };
    char curtime[BUFFER_SIZE] = { 0 };

    memset(command.string, 0, BUFFER_SIZE);

    strcpy(tmpname, filename);
    strcat(filename, "/");

    if (open(strcat(filename, argv), O_RDWR | O_CREAT, 0666) < 0) {
        fprintf(stderr, "creat error\n");
        exit(1);
    }
    if (command.p_opt == true) { // p 옵션 시 디렉토리 생성 및 파일 생성경로를 디렉토리로 변경
        strcat(tmpname, "/");
        strcat(tmpname, command.DIRECTORY);
        mkdir(tmpname, 0755);
    }

    if ((logfd = creat(strcat(tmpname,"/log.txt"), 0666)) <0) { // 경로에 log파일 생성
        fprintf(stderr, "log.txt creat error\n");
        exit(1);
    }
    if (command.t_opt == true) { // t옵션시 시간 값 쓰기
        cur_time(curtime);
        sprintf(command.string, "%s  ", curtime);
        write(logfd, command.string, strlen(command.string));
    }

    sprintf(command.string2, "<<Damon Process Initialized pid : %d>>\nInitialized with Default value : %d\n", getpid(), command.QUEUE_NUM);

    write(logfd, command.string2, strlen(command.string2)); // 로그파일에 문자열 쓰기

    make_msg(); // 메시지 큐 생성
    ssu_usr1(); // SIGUSR1 액션 설정
    ssu_usr2(); // SIGUSR2 액션 설정

    if (signal(SIGTERM, term_handler) == SIG_ERR) { // SIGTERM 핸들러 설정
        fprintf(stderr, "signal error\n");
        exit(1);
    }
}

void make_msg() // 메시지 큐 생성 함수
{
    key_t key;

    if ((key = ftok(filename, getpid()+10)) == -1) { // 메시지 큐를 위한 key값 생성
        fprintf(stderr, "ftok error\n");
        exit(1);
    }

    if ((msg_id = msgget(key, 0644 | IPC_CREAT)) == -1) { // 메시지 큐를 생성하고 ID를 저장함
        fprintf(stderr, "msgget error\n");
        exit(1);
    }
    else
        printf("success msg큐 만들기\n");

    if (command.l_opt == true) { // l 옵션을 위한 메시지 큐 생성
        if ((key = ftok(filename, getpid())) == -1) {
            fprintf(stderr, "ftok2 error\n");
            exit(1);
        }
        if ((msg_stringID = msgget(key, 0644 | IPC_CREAT)) == -1) {
            fprintf(stderr, "msgget 2 error\n");
            exit(1);
        }
    }
}

void send_msg(pid_t pid, char *string) // 메시지 저장하기
{
    struct ssu_msgbuf buf;
    struct ssu_msgbuf stringbuf;
    struct msqid_ds queue;
    char text[BUFFER_SIZE] = { 0 };

    buf.msg_type = pid;
    stringbuf.msg_type = pid;
    sprintf(text, "%d",pid);
    strcpy(buf.msg_text, text);
    strcpy(stringbuf.msg_text, string);

    if (msgctl(msg_id, IPC_STAT, &queue) == -1) { // 메시지 큐의 상태 가져오기
        fprintf(stderr, "main msgctl error\n");
        exit(1);
    }

    int length = strlen(buf.msg_text);
    int length2 = strlen(stringbuf.msg_text);
    // 메시지 큐의 크기 한계값 
    if (queue.msg_qnum < command.QUEUE_NUM){ // 메시지 큐의 크기가 현재 메시지 큐보다 커야 실행된다. 
        if (msgsnd(msg_id, &buf, length+1, 0) == -1) {
            fprintf(stderr, "sendmsg msgsnd error\n");
            exit(1);
        }
        if (command.l_opt == true) {
            printf("msg_id :%d msgstorngID ;%d\n ", msg_id, msg_stringID);
            if (msgsnd(msg_stringID, &stringbuf, length2 +1, 0) == -1) {
                fprintf(stderr, "sendmsg \n");
                exit(1);
            }
        }
    }
}
void send_usr1()  // SIGUSR1 보내기
{  
    struct ssu_msgbuf buf;
    if (msgrcv(msg_id, &buf, sizeof(buf.msg_text), 0, 0) == -1){ 
        fprintf(stderr, "msgrcv 큐에서 꺼내기 오류\n");
        exit(1);
    }
    printf("buf.msg_text : %s\n", buf.msg_text);
    kill(atoi(buf.msg_text), SIGUSR1);
}
void ssu_usr1() // SIGUSR1에 대한 액션 설정
{
    struct sigaction sig_act;

    sigemptyset(&sig_act.sa_mask);
    sig_act.sa_flags = SA_SIGINFO;
    sig_act.sa_sigaction = usr1_handler;

    if (sigaction(SIGUSR1, &sig_act, NULL) != 0) {
        fprintf(stderr, "sigaction1 error\n");
        exit(1);
    }
}
void ssu_usr2() // SIGUSR2에 대한 액션 설정
{
    struct sigaction sig_act;

    sigemptyset(&sig_act.sa_mask);
    sig_act.sa_flags = SA_SIGINFO;
    sig_act.sa_sigaction = usr2_handler;

    if(sigaction(SIGUSR2, &sig_act, NULL) != 0) {
        fprintf(stderr, "sigacton2 error\n");
        exit(1);
    }
}
void term_handler(int signo) // SIGTERM에 대한 액션 설정
{
    close(logfd); // 로그파일 닫기
    if (msgctl(msg_id, IPC_RMID, NULL) == -1) { // 메시지큐 삭제
        fprintf(stderr, "msgctl error\n");
        exit(1);
    }
    if (command.l_opt == true) {
        if (msgctl(msg_stringID, IPC_RMID, NULL) == -1) { // l 옵션 메시지 큐 삭제
            fprintf(stderr, "msgctl2 error\n");
            exit(1);
        }
    }
    printf("성공적으로 제거\n");
    exit(0);
}
void usr1_handler(int signo, siginfo_t *info, void *uarg)  // SIGUSR1에 대한 액션 핸들러
{
    struct msqid_ds queue;
    char buf[BUFFER_SIZE];
    memset(buf, 0, BUFFER_SIZE);
    memset(fifo_name, 0, BUFFER_SIZE);

    if ((fifofd = open(FILE_FIFO, O_RDWR)) < 0) { // FIFO파일 열기
        fprintf(stderr, "fifofile open error\n");
        exit(1);
    }
    printf("read 전\n");
    int tmp = 0;
    if ((tmp = read(fifofd, fifo_name, BUFFER_SIZE)) > 0) { // FIFO파일로 요청한 파일 읽어오기
        write_log(info -> si_pid, fifo_name);
        if (strcmp(fifo_name, filename) == 0) {
            printf("fifo와 file의 이름이 일치 큐에 넣기\n");
            send_msg(info->si_pid, req_string);
        }

        if (msgctl(msg_id, IPC_STAT, &queue) == -1) { // 메시지 큐의 상태 가져오기
            fprintf(stderr, "main msgctl error\n");
            exit(1);
        }
        if (queue.msg_qnum != 0) {  // 메시지 큐가 0이 아니면 시그널 보내기
            if(check_right() == true) // 접근 권한 확인
                send_usr1();
        }
    }
}
char *cur_time(char *tmp) // 현재 시간 값을 날짜 형식으로 문자열에 저장하고 반환
{
    struct tm *curtime;
    time_t now;

    time(&now);
    curtime = localtime(&now);
    sprintf(tmp, "[%d-%02d-%02d %02d:%02d:%02d]",
            curtime -> tm_year + 1900,
            curtime -> tm_mon +1,
            curtime -> tm_mday,
            curtime -> tm_hour,
            curtime -> tm_min,
            curtime -> tm_sec);
    return tmp;
}
void write_log(pid_t pid, char *filename)
{  
    char string[BUFFER_SIZE] = { 0 };
    char string2[BUFFER_SIZE] = { 0 };
    char string3[BUFFER_SIZE] = { 0 };
    char username[BUFFER_SIZE] = { 0 };
    char pid_buf[BUFFER_SIZE] = { 0 };
    char curtime[BUFFER_SIZE] = { 0 };
    char tmp_file[BUFFER_SIZE] = { 0 };
    char req_file[BUFFER_SIZE] = { 0 };
    char *buf;
    struct passwd *pw_passwd;

    memset(req_string, 0, BUFFER_SIZE);

    sprintf(pid_buf, "%d", pid);
    strcpy(tmp_file, filename);
    
    if (command.t_opt == true) { // t 옵션 시 앞에 시간값이 들어감.
        cur_time(curtime);
        strcat(string, curtime);
        strcat(string, "  ");
    }

    parsing_fifo(tmp_file, req_file); // 요청한 파일 상대경로 가져오기
    
    sprintf(string2, "Request Process PID : %d, Request Filename : %s\n", pid, req_file + 1);
    strcat(string, string2);
    if (command.id_opt == true) { // id 옵션 시 쓰는 문자열이 늘어난다.
        buf = id_option(pid_buf);
        if ((pw_passwd = getpwuid(atoi(buf))) == NULL) {
            fprintf(stderr, "getpwuid error\n");
            exit(1);
        }
        sprintf(string3, "User : %s, UID : %d, GID : %d\n",
                pw_passwd->pw_name,
                pw_passwd->pw_uid,
                pw_passwd->pw_gid);
        strcat(string, string3);
    }
    write(logfd, string, strlen(string));
    strcpy(req_string, string);

}
void parsing_fifo(char *fifo, char *path) // fifo에서 읽어온 요청한 파일이 절대경로이므로 상대경로로 짤라준다.
{
    int j=0;
    for (int i=0; fifo[i] != '\0'; i++) {
        if (fifo[i] == '/') {
            j=i;
            continue;
        }
    }
    strcpy(path, fifo + j);
    fifo[j+1] = '\0';
}
void usr2_handler(int signo, siginfo_t *info, void *uarg) // 완료되었다는 usr2를 받으면 메시지큐에서 하나를 빼고 보내준다
{
    printf("SIGUSR2 발생\n");
    int L_logfd;
    struct ssu_msgbuf msg;
    struct ssu_msgbuf buf;
    struct msqid_ds queue;
    char buf_id[BUFFER_SIZE] = { 0 };
    char l_filename[BUFFER_SIZE] = { 0 };
    char tmp_fifo[BUFFER_SIZE] = { 0 };
    char req_file[100] = { 0 };
    char time_string[BUFFER_SIZE] = { 0 };
    char curtime[BUFFER_SIZE] = { 0 };

    strcpy(tmp_fifo, fifo_name); // FIFO파일로 부터 읽어온 내용을 복사

    parsing_fifo(tmp_fifo, req_file); // req_file에는 상대경로 형식의 요청한 파일이 들어간다.
    printf("tmp_fifo ; %s\n", tmp_fifo);
    if (command.l_opt == true) {   // l 옵션을 요청할 때 
        cur_time(l_filename); // 파일의 이름을 형식에 맞는 현재시각으로 바꿔준다.
        if (command.p_opt== true) {
            strcat(tmp_fifo, command.DIRECTORY);
            strcat(tmp_fifo, "/");
        }
        if ((L_logfd = creat(strcat(tmp_fifo, l_filename), 0666)) < 0) {
            fprintf(stderr, "l_filename creat error\n");
            exit(1);
        }

        if (msgrcv(msg_stringID, &msg, sizeof(msg.msg_text), 0, 0) == -1){ 
            fprintf(stderr, "msgrcv 큐에서 꺼내기 오류\n");
            exit(1);
        }

        printf("msg stirng : %s\n", msg.msg_text);
        write(L_logfd, command.string, strlen(command.string)); // l 옵션으로 생성된 파일에 내용을 적는다.
        write(L_logfd, command.string2, strlen(command.string2));
        write(L_logfd, msg.msg_text, strlen(msg.msg_text));
    }

    if (command.t_opt == true) { // t 옵션일 시 시간값을 더 적어주어야 한다.
        cur_time(curtime); // curtime에 시간 형식으로 문자열을 저장한다.
        sprintf(time_string, "%s  ", curtime);
        write(logfd, time_string, strlen(time_string));
        if (command.l_opt == true)
            write(L_logfd, time_string, strlen(time_string));
    }
    if (msgctl(msg_id, IPC_STAT, &queue) == -1) { // 메시지 큐에 담긴 정보를 가져온다.
        fprintf(stderr, "msgctl error\n");
        exit(1);
    }
    if(queue.msg_qnum != 0) { // 메시지 큐의 크기가 0이 아니라면
        if (msgrcv(msg_id, &buf, sizeof(buf.msg_text), 0, 0) == -1){ 
            fprintf(stderr, "msgrcv 큐에서 꺼내기 오류\n");
            exit(1);
        }
        printf("buf.msg_text : %s\n", buf.msg_text);
        kill(atoi(buf.msg_text), SIGUSR1);
    }

    sprintf(buf_id, "%d", info -> si_pid);
    write(logfd, "Finished Process ID : ", 22); // log.txt파일에 내용을 적는다.
    write(logfd, buf_id, strlen(buf_id));
    write(logfd, "\n", 1);

    if (command.l_opt == true) { // l 옵션으로 생성된 파일에 내용을 적는다.
        write(L_logfd, "Finished Process ID : ", 22);
        write(L_logfd, buf_id, strlen(buf_id));
        write(L_logfd, "\n", 1);
    }

    printf("SIGUSR2 처리 완료\n");

}
char *id_option(char *pid) // id옵션
{
    FILE *fp;
    char pathname[DIRECTORY_SIZE];
    memset(pathname, 0, DIRECTORY_SIZE);

    sprintf(pathname, "/proc/%s/status", pid);

    if ((fp = fopen(pathname,"r")) == NULL) { // status에서 UID의 정보를 가져온다.
        fprintf(stderr, "pathname opeer %s\n", pathname);
        exit(1);
    }
    while(1) {
        fscanf(fp, "%s", pid);
        if (strcmp(pid, "Uid:") == 0) {
            fscanf(fp, "%s", pid);
            break;
        }
    }
    fclose(fp);
    return pid; // Uid 값을 반환해준다.
}
bool check_right()  // 파일이 잠겨있는지 확인을 한다. 다른프로세스에서 쓰기를 요청할 때 
{
    int fd;
    struct flock file_lock;
    file_lock.l_len = 0;
    file_lock.l_type = 0;
    file_lock.l_whence = SEEK_SET;
    file_lock.l_start = 0;

    if ((fd = open(filename, O_RDWR)) < 0) {
        fprintf(stderr, "filename open error\n");
        exit(1);
    }
    if (fcntl(fd, F_GETLK, &file_lock) == -1) { // 잠금 정보를 가져온다.
        fprintf(stderr, "fcntl error\n");
        exit(1);
    }
    close(fd);
    if (file_lock.l_type == 1) // 누군가 쓰고 잇음
        return false;
    return true;
}

int ssu_daemon_init(void) { // 프로세스를 데몬프로세스로 만든다.
    pid_t pid;
    int fd, maxfd;

    if ((pid = fork()) <0) { // 자식 프로세스 생성
        fprintf(stderr, "fork error\n");
        exit(1); 
    }
    else if (pid !=0)  // 부모프로세스 종료
        exit(0); 

    pid = getpid();
    printf("process %d running as daemon\n",pid);
    setsid();
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    maxfd = getdtablesize();

    for (fd = 0; fd < maxfd; fd++) // 필요없는 파일디스크립터 닫기
        close(fd);

    umask(0);
    chdir("/");
    fd = open("dev/null", O_RDWR); // 표준입출력과 오류출력을 리다이렉션한다.
    dup(0);
    dup(0);
    return 0;
} 
void parsing(int argc, char *argv[]) // 인자 파싱
{
    command.t_opt = false; // 구조체 값 초기화
    command.p_opt = false;
    command.n_opt = false;
    command.l_opt = false;
    command.id_opt = false;
    memset(command.DIRECTORY, 0 , sizeof(command.DIRECTORY));
    command.QUEUE_NUM = 16;
    STATUS state;
    state = NEUTRAL;
    int i = 2;
    while(1)
    {
        if (argc == i)
            break;
        if (argv[i][0] == '-') {
            state = NEUTRAL;
        }
        switch (state)
        {
            case NEUTRAL :
                switch (argv[i][1])
                {
                    case 'p' :
                        i++;
                        state = PSTATE;
                        command.p_opt = true;
                        continue;
                    case 'n' :
                        i++;
                        state = NSTATE;
                        command.n_opt = true;
                        continue;
                    case 'l' :
                        i++;
                        command.l_opt = true;
                        printf("l option true\n");
                        continue;
                    case 't' :
                        i++;
                        command.t_opt = true;
                        continue;
                    case 'i' :
                        if (strcmp("-id", argv[i]) == 0)
                            command.id_opt = true;
                        i++;
                        printf("id option true\n");
                        continue;
                }
            case PSTATE :
                strcpy(command.DIRECTORY, argv[i]);
                state = NEUTRAL;
                i++;
                continue;
            case NSTATE :
                command.QUEUE_NUM = atoi(argv[i]);
                state = NEUTRAL;
                i++;
                continue;
        }

    }

}

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <utime.h>
#include <ctype.h>
#include <time.h>

#define BUF_SIZE 1024

//동기화가 필요한 파일에 대한 정보를 담고있음
typedef struct _List
{
	char fname[BUF_SIZE];
	long size;
	struct _List *child;
	struct _List *next;
}List;

//사용법을 출력하는 함수
//usage: ./ssu_rsync [OPTION] <src> <dst>
void print_usage();
//동기화를 수행하기 위한 작업을 진행함
//일반파일을 동기화하는 경우 normal_sync호출 하고 끝
//디렉토리의 경우 동기화가 필요한 파일을 리스트화 시키고 r옵션이 있으면 서브디렉토리도 동기화함
void Sync(char *src, char *dst, char *sub_dir, int argc, char **argv, int depth);
//t옵션을 수행함
//tar로 묶어서 동기화를 진행 일반파일, 디렉토리 모두 가능
void do_tOption(char *src, char *dst);
//m옵션을 수행함
//src에 존재하지 않는파일을 dst에서도 삭제함
void do_mOption(char *src, char *dst);
//실질적으로 동기화를 진행함
//파일을 열고 복사하는 과정
void normal_sync(char *src, char *dst, char *fname);

//동기화 정보를 리스트화 시키는 함수들
List *make_node(char *fname);
List *make_list(char *path);
List *print_list(List *root);

//디렉토리 삭제함수
void rmdirs(char *path);
//최종적으로 로그에 시각과 파일정보를 작성해주는 함수
void print_log(char *src, char *dst);

//SIGINT 발생시 동기화 이전으로 돌리는 기능을 가지고 있음
void signal_handler();

//수행시간측정함수
void ssu_runtime(struct timeval *begin_t, struct timeval *end_t);

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <string.h>
#include <sys/stat.h>
#include <signal.h>

#define BUF_SIZE 1024

//프롬프트에서 보여줄 ssu_crontab_file의 정보를 링크드 리스트 형태로
//표현하기 위한 구조체
typedef struct crontab_file
{
	char command[BUF_SIZE];
	struct crontab_file *next;
}Crontab_file;

//ssu_crontab_file에서 한줄 읽어 노드를 생성하는 함수
Crontab_file *create_node(char *contents);
//프롬프트에서 입력받은 명령어들을 분리시켜주는 함수
int command_separation(char *command, char (*argv)[BUF_SIZE]);

//수행시간 측정함수
void ssu_runtime(struct timeval *begin_t, struct timeval *end_t);

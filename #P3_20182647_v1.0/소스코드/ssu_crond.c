#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>
#include <string.h>

#define BUF_SIZE 1024

//리스트를 구성할 구조체
typedef struct _Command
{
	char cmd[BUF_SIZE];
	time_t last_exec;
	pthread_t tid;
	struct _Command *next;

}Command;

//파일 수정여부를 확인하고 업데이트

//실행주기를 토큰화 시켜서 각 요소 별로 배열을 생성해
//실행될 시각에 대해서 flag를 켜서 현재시간과 총 5개의 요소 flag가 1이면 명령어를 수행한다
void check_time(char (*tok_buf)[BUF_SIZE], char (*tokens)[BUF_SIZE], int *type_array, int type);
//명령어별로 각 thread가 수행할 명령어를 인자로서 넘겨받고
//주기적으로 명령어를 수행한다.
void *thread_fun(void *arg);
//ssu_crontab_file이 수정되는지 확인한다
//파일이 수정되었을 경우 삭제된 명령어를 수행하는 thread를 종료시키고 리스트 정보를 업데이트
void *check_file_modify(void *arg);
//리스트 정보를 이용해 각 명령어별로 thread를 생성해주고 id를 리스트에 저장한다
void *check_cmd(void *arg);
//리스트에 필요한 노드를 생성하고 리턴시키는 함수
Command *make_node(char *cmd);
//명령어에 대한 리스트와 그 명령어를 수행하는 thread_id를 저장할 리스트를 제작한다
Command *make_list();

Command *root = NULL;
pthread_mutex_t mutex_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_lock1 = PTHREAD_MUTEX_INITIALIZER;

int main()
{
	FILE *fp;
	char *fname = "ssu_crontab_file";

	pid_t pid;
	if((pid = fork()) < 0)
		exit(0);
	else if(pid != 0)
		exit(0);

	signal(SIGHUP, SIG_IGN);
	close(0); close(1); close(2);
	setsid();


	while(1)
	{
		//ssu_crontab_file이 존재할때 까지 대기
		if(access(fname, F_OK) < 0)
		{
			continue;
		}
		else
		{
			root = make_list();
			break;
		}
	}
	//2개의 스레드를 생성하여 하나는 파일 수정체크용 스레드
	//하나는 명령어별 스레드 생성 여부 판단을 위한 스레드 생성
	pthread_t tid, tid2;
	if(pthread_create(&tid, NULL, check_file_modify, NULL) != 0)
	{
		fprintf(stderr, "pthread_create error\n");
		exit(1);
	}
	//일정간격으로 리스트에서 아직 출발하지 않은 명령어에 대한 스레드를 생성해주는 스레드
	if(pthread_create(&tid2, NULL, check_cmd, NULL) != 0)
	{
		fprintf(stderr, "pthread_create error\n");
		exit(1);
	}

	//각 스레드가 끝날때 까지 대기
	int status;
	pthread_join(tid, (void *)&status);
	pthread_join(tid2, (void *)&status);
}

//ssu_crontab_file이 수정되는지 감시하는 스레드가 할 함수
void *check_file_modify(void *arg)
{
	FILE *fp;
	char *fname = "ssu_crontab_file";
	struct stat statbuf;
	long size;
	time_t mtime;
	Command *new_root = NULL;

	//초기 파일의 크기와 수정시각을 저장
	stat(fname, &statbuf);
	mtime = statbuf.st_mtime;
	size = statbuf.st_size;
	//혹시 모르는 상황을 대비해 mutex_lock을 설정
	pthread_mutex_lock(&mutex_lock1);
	while(1)
	{
		stat(fname, &statbuf);

		//만약 크기나 최종수정일이 변경되었을 경우
		if(size != statbuf.st_size || mtime != statbuf.st_mtime)
		{
			//최신 정보를 저장한뒤
			size = statbuf.st_size;
			mtime = statbuf.st_mtime;
			//ssu_crontab_file의 명령어 정보를 리스트화 시킴
			new_root = make_list();

			//기존의 리스트와 비교를 진행
			//move1 -> 기존 리스트
			//move2 -> 변경된파일내용을 담은 리스트
			Command *move1 = root, *move2 = new_root;
			while(move1 != NULL)
			{
				int flag = 0;
				move2 = new_root;
				while(move2 != NULL)
				{
					//같은 내용이 존재할경우 패스
					if(!strcmp(move1->cmd, move2->cmd))
					{
						move2->tid = move1->tid;
						flag = 1;
						break;
					}
					move2 = move2->next;
				}
				//같은 내용의 명령어를 찾지 못 했을 경우
				//기존 리스트에 저장된 스레드 id를 통해 thread를 강제 종료시킴
				if(flag == 0)
				{
					pthread_cancel(move1->tid);
				}
				move1 = move1->next;
			}
			//리스트 갱신
			root = new_root;
		}
		//1초단위로 검사
		sleep(1);
	}
	pthread_mutex_unlock(&mutex_lock1);
}

//각 스레드가 수행할 함수
void *thread_fun(void *arg)
{
	time_t cur_time;
	struct tm *t;

	//실행할 주기를 설정할 배열들
	int minute[60];
	int hour[24];
	int day[32];
	int month[13];
	int week[7];

	//배열 초기화
	for(int i=0; i<60; i++) minute[i] = 0;
	for(int i=0; i<24; i++) hour[i] = 0;
	for(int i=0; i<32; i++) day[i] = 0;
	for(int i=0; i<13; i++) month[i] = 0;
	for(int i=0; i<7; i++) week[i] = 0;

	//문자열 정보 배열 초기화
	char tok_buf[500][BUF_SIZE];
	char arg_data[BUF_SIZE];
	char buf[BUF_SIZE];
	char real_cmd[BUF_SIZE];
	char tokens[5][BUF_SIZE];
	memset(arg_data, 0, BUF_SIZE);
	memset(buf, 0, BUF_SIZE);
	memset(real_cmd, 0, BUF_SIZE);
	for(int i=0; i<5; i++) memset(tokens[i], 0, BUF_SIZE);

	//인자로 넘겨받은 데이터를 따로 보관
	strcpy(arg_data, (char *)arg);
	strcpy(buf, arg_data);
	//실행주기를 공백 기준으로 토큰화 시킴
	char *p;
	p = strtok(buf, " ");
	strcpy(tokens[0], p);
	for(int i=1; i<5; i++)
	{
		p = strtok(NULL, " ");
		strcpy(tokens[i], p);
	}
	strcpy(real_cmd, strtok(NULL, ""));
	
	//토큰화 시킨 주기를 숫자와 기호에 맞게 다시 토큰화 시켜서 3번째인자 배열에다가
	//해당하는 주기를 1로 변화시킴
	check_time(tok_buf, tokens, minute, 0);
	check_time(tok_buf, tokens, hour, 1);
	check_time(tok_buf, tokens, day, 2);
	check_time(tok_buf, tokens, month, 3);
	check_time(tok_buf, tokens, week, 4);

	//??분 00초를 기준으로 명령어를 실행하기위해 00초까지 대기
	while(1)
	{
		cur_time = time(NULL);
		t = localtime(&cur_time);
		if(t->tm_sec == 0)
			break;
		sleep(1);
	}

	while(1)
	{
		//현재시간을 가져와서 저장
		cur_time = time(NULL);
		t = localtime(&cur_time);
		char time_buf[BUF_SIZE];
		strcpy(time_buf, ctime(&cur_time));
		time_buf[strlen(time_buf) - 1] = 0;

		//현재시간과 각 배열의 값이 모두 1일 경우에만 명령어를 실행
		//하나라도 1이 아닐경우 실행할 주기가 아님을 판단
		if(minute[t->tm_min] == 1 && hour[t->tm_hour] == 1 && day[t->tm_mday] == 1 &&
				month[t->tm_mon] == 1 && week[t->tm_wday] == 1)
		{
			FILE *fp;
			char *fname = "ssu_crontab_log";
			char new_buf[BUF_SIZE];
			memset(new_buf, 0, BUF_SIZE);
		
			//로그파일을 오픈하여 실행했다는 로그를 작성함
			if((fp = fopen(fname, "a")) == NULL)
			{
				fprintf(stderr, "fopen error for %s\n", fname);
				exit(1);
			}
			sprintf(new_buf, "[%s] run %s\n", time_buf, (char *)arg);
			fwrite(new_buf, strlen(new_buf), 1, fp);
			fclose(fp);
			//실제 명령어 실행
			system(real_cmd);
		}
		//60초 간격으로 주기를 확인
		sleep(60);
	}
}

//실행 주기를 분석하여 type_array에 플래그를 설정시켜줌
void check_time(char (*tok_buf)[BUF_SIZE], char (*tokens)[BUF_SIZE], int *type_array, int type)
{
	//버퍼 초기화
	for(int i=0; i<500; i++) memset(tok_buf[i], 0, BUF_SIZE);

	int index = 0;
	//실행주기들을 -,/,*,숫자에 따라 토큰화 시킴
	for(int i=0; i<strlen(tokens[type]); i++)
	{
		//주기와 범위 그리고 모든값을 의미하는 기호의 경우 혼자서 토큰이 됨
		if(tokens[type][i] == '/' || tokens[type][i] == '-' || tokens[type][i] == '*')
			strncat(tok_buf[index++], &tokens[type][i], 1);
		//앞에 토큰이 숫자고 현재 문자도 숫자면 앞에다가 연결시켜줌
		else if(i>0 && isdigit(tokens[type][i]) && isdigit(tok_buf[index-1][strlen(tok_buf[index-1]) - 1]))
			strncat(tok_buf[index-1], &tokens[type][i] , 1);
		//이외의 모든경우 그냥 토큰화시켜줌
		else
			strncat(tok_buf[index++], &tokens[type][i], 1);
	}
	//단 한개의 정보만 있을 경우
	if(index == 1)
	{
		//정보가 하나밖에 없는데 *일경우
		//모든 주기에 대해 플레그를 설정
		if(tok_buf[0][0] == '*')
		{
			int max;
			//0 -> 분, 1 -> 시, 2 -> 일, 3 -> 월, 4-> 요일
			if(type == 0) max = 60;
			else if(type == 1) max = 24;
			else if(type == 2) max = 32;
			else if(type == 3) max = 13;
			else if(type == 4) max = 7;
			//플래그 설정
			for(int i=0; i<max; i++) type_array[i] = 1;
		}
		else
			type_array[atoi(tok_buf[0])] = 1;
	}
	else
	{
		//범위값 초기화
		int r_start = -1,  r_end = -1;
		for(int i=0; i<index; i++)
		{
			// '-' 기호를 만났을 경우 앞 뒤 숫자가 범위가 됨
			if(!strcmp(tok_buf[i] , "-"))
			{
				//범위값을 정수화 시켜서 저장
				r_start = atoi(tok_buf[i-1]);
				r_end = atoi(tok_buf[i+1]);
				i++;

				int tmp = i + 1;
				if(tmp>=index || (tmp < i && !strcmp(tok_buf[tmp], ",")))
				{
					for(int z=r_start; z<=r_end; z++)
						type_array[z] = 1;
				}
			}
			// "/"의 경우 앞에 주기를 뒤에 숫자만큼 점프시킴
			else if(!strcmp(tok_buf[i], "/"))
			{
				//만약 앞에 주기가 *일 경우 type에 따라 최대범위 설정
				if(!strcmp(tok_buf[i-1] , "*"))
				{
					if(type == 0)
					{
						r_start = 0, r_end = 59;
					}
					else if(type == 1)
					{
						r_start = 0, r_end = 23;
					}
					else if(type == 2)
					{
						r_start = 1, r_end = 31;
					}
					else if(type == 3)
					{
						r_start = 1, r_end = 12;
					}
					else if(type == 4)
					{
						r_start = 0, r_end = 6;
					}
				}
				//점프할 주기는 "/" 뒤에 숫자
				int jump = atoi(tok_buf[i+1]);
				//만약 범위가 있다면 범위에 해당하는 숫자와 점프를 생각하여 플래그 설정
				if(r_start != -1 && r_end != -1)
				{
					int starting_point = r_start +jump - 1;
					for(int j=starting_point; j<=r_end; j=j+jump)
						type_array[j] = 1;
				}
				i++;
			}
			//숫자일 경우
			else if(isdigit(tok_buf[i][0]))
			{
				//다음 토큰이 ","의 경우 현재 숫자는 단순 그 숫자 주기에 실행하라는 의미
				//ex) 0,12일경우 0,12의 플래그만 설정
				if(!strcmp(tok_buf[i+1], ",") || i+1 == index)
				{
					type_array[atoi(tok_buf[i])] = 1;
				}
			}
			else
				continue;
		}
	}
}

//명령어에 대해 스레드를 생성할지 말지 결정함
void *check_cmd(void *arg) 
{
	Command *old = NULL;
	time_t cur_time;
	struct tm *t;
	char buf[BUF_SIZE];
	char real_cmd[BUF_SIZE];
	memset(real_cmd, 0, BUF_SIZE);
	memset(buf, 0, BUF_SIZE);
	//혹시 모르는 상황을 대비해 mutex_lock을 설정
	pthread_mutex_lock(&mutex_lock);
	while(1)
	{
		//2초전 리스트와 최신 리스트를 비교하기 위해 변수선언
		Command *move = root, *move2 = old;
		move = root; move2 = old;
		
		while(move != NULL)
		{
			int flag = 0;
			move2 = old;
			//만약 이전리스트와 현재리스트에 동일한 명령어가 존재할시
			//그 명령어에 대한 스레드가 존재함으로 추가적으로 생성할 필요없음
			while(move2 != NULL)
			{
				//같은 명령어인지 아닌지 비교
				if(!strcmp(move2->cmd, move->cmd))
				{
					flag = 1;
					break;
				}
				move2 = move2 -> next;
			}
			//존재하는 명령어 대한 필터
			if(flag == 1)
			{
				move = move->next;
				continue;
			}
			//명령어에 대해 스레드가 없으면 스레드를 새로 생성해주고 새로 생성한 스레드의 id를 저장
			char *new_buf = (char *)calloc(BUF_SIZE, sizeof(char));
			strcpy(new_buf, move->cmd);
			new_buf[strlen(new_buf) - 1] = 0;
			pthread_t tid;
			//스레드 생성부분
			if(pthread_create(&tid, NULL, thread_fun, new_buf) != 0)
			{
				fprintf(stderr, "pthread_create error\n");
				exit(1);
			}
			//id 저장부분
			move->tid = tid;
			move = move->next;
		}
		//현재리스트를 이전 리스트로 설정
		old = root;
		sleep(2);
	}

	pthread_mutex_unlock(&mutex_lock);
}

//넘겨 받은 인자로 노드를 생성해서 리턴
Command *make_node(char *cmd)
{
	Command *new_node = (Command *)calloc(1, sizeof(Command));
	memset(new_node->cmd, 0, BUF_SIZE);
	//인자 저장
	strcpy(new_node->cmd, cmd);	
	new_node->last_exec = 0;
	new_node->next = NULL;

	return new_node;
}

//리스트 생성함수
Command *make_list()
{
	Command *root = NULL;
	FILE *fp;
	char *fname = "ssu_crontab_file";
	char buf[BUF_SIZE];
	memset(buf, 0, BUF_SIZE);
	//ssu_crontab_file을 읽기 모드로 오픈
	if((fp = fopen(fname, "r")) == NULL)
	{
		fprintf(stderr, "fopen error for %s\n", fname);
		exit(1);
	}
	//ssu_crontab_file에서 읽을 정보가 없을때까지
	while(!feof(fp))
	{
		memset(buf, 0, BUF_SIZE);
		//한줄 씩 읽기
		fgets(buf, BUF_SIZE, fp);
		//읽은 길이가 0일 경우 종료
		if(strlen(buf) == 0) break;
		
		//읽은 내용을 인자로 노드를 생성
		Command *new_node = make_node(buf);

		//노드를 항상 맨 앞에 연결
		if(root == NULL)
		{
			root = new_node;
		}
		else
		{
			new_node->next = root;
			root = new_node;
		}
	}
	fclose(fp);
	return root;
}

#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "ssu_crontab.h"

#define SECOND_TO_MICRO 1000000

//ssu_crontab.c의 메인함수
//프롬프트 출력 및 add, remove 명령어 수행
//ssu_crontab_file에서 한줄 씩 읽어 리스트 형성
//add시 리스트에 추가하고 ssu_crontab_file에 반영
//remove시 리스트에서 삭제하고 처음부터 ssu_crontab_file에 작성
int main(void)
{
	struct timeval begin_t, end_t;
	FILE *crontab_file;
	FILE *crond_log;
	int ssu_crontab_file_command_cnt = 0;
	char *crontab_fname = "ssu_crontab_file";
	char *crond_log_fname = "ssu_crontab_log";
	char buf[BUF_SIZE];
	memset(buf, 0, BUF_SIZE);

	Crontab_file *head = NULL;
	Crontab_file *tail = NULL;

	gettimeofday(&begin_t, NULL);

	//ssu_crontab_file이 존재하지 않을 경우 생성함
	if(access(crontab_fname, F_OK) < 0)
	{
		int fd;
		creat(crontab_fname, 0644);
	}

	//get ssu_crontab_file info
	if((crontab_file = fopen(crontab_fname, "r+")) == NULL)
	{
		fprintf(stderr, "open error for %s\n", crontab_fname);
		exit(1);
	}

	//ssu_crontab_file에서 한줄 씩 읽어서 리스트화 시킴
	while(!feof(crontab_file))
	{
		memset(buf, 0, BUF_SIZE);
		fgets(buf, sizeof(buf), crontab_file);

		//읽은 길이가 0일 경우 파일의 끝임
		if(strlen(buf) == 0) break;

		//ssu_crontab_file에 존재하는 명령어의 갯수를 나타내는 변수
		//명령어의 갯수를 증가시켜줌
		ssu_crontab_file_command_cnt++;
		//새로운 노드를 생성하고
		Crontab_file *new_node = create_node(buf);
		//항상 리스트 맨뒤에 연결 시켜준다
		//순서를 맞추기 위함
		if(head == NULL)
		{
			head = new_node;
			tail = new_node;
		}
		else if(tail != NULL)
		{
			tail->next = new_node;
			tail = new_node;
		}
	}
	fclose(crontab_file);

	while(1)
	{
		char command[BUF_SIZE];
		char argv[200][BUF_SIZE];
		for(int i=0; i<200; i++) memset(argv[i], 0, BUF_SIZE);

		memset(command, 0, BUF_SIZE);

		int count = 0;
		//프롬프트 출력
		//리스트 정보와 학번을 출력함
		Crontab_file *move = head;
		while(move != NULL)
		{
			printf("%d. %s", count++, move->command);
			move = move->next;
		}
		//prompt
		write(1, "\n20182647>", strlen("\n20182647>"));
		//stdin으로 부터 명령어를 입력받음
		read(0, command, BUF_SIZE);

		//입력받은 명령어를 공백을 기준으로 분리하여 argv에 저장함
		int argc = command_separation(command, argv);

		//단순 엔터만 입력했을 경우 프롬프트 재출력
		if(command[0] == '\n')
			continue;

		//명령어 시작이 add일경우
		if(!strcmp(argv[0], "add"))
		{
			//check cycles
			//실행 주기 분석
			int flag = 0;
			//총 5개를 분석함
			for(int i=1; i<6; i++)
			{
				//각자의 길이 만큼 검사를 진행
				for(int j=0; j<strlen(argv[i]); j++)
				{
					if(j == 0)
					{
						//맨처음이 숫자나 * 이 아닐 경우 무조건 에러처리
						if(argv[i][j] != '*' && !isdigit(argv[i][j]))
						{
							flag = 1;
							break;
						}
					}
					else
					{
						// / ,  - 숫자 이외의 다른 기호가 들어올 경우 flag를 세워 에러처리
						if(argv[i][j] != '/' && argv[i][j] != ',' && argv[i][j] != '-'
								&& !isdigit(argv[i][j]))
						{
							flag = 1;
							break;
						}
					}
				}
				//플래그 확인
				if(flag == 1)
					break;
			}
			//플래기가 존재할 경우 에러처리
			if(flag == 1)
			{
				fprintf(stderr, "execute cycle error\n");
				continue;
			}

			char exec_cmd[BUF_SIZE];
			char exec_cycle[BUF_SIZE];
			memset(exec_cycle, 0, BUF_SIZE);
			memset(exec_cmd, 0 ,BUF_SIZE);

			//실행주기를 하나로 합쳐줌
			for(int i=1; i<6; i++)
			{
				strcat(exec_cycle, argv[i]);
				strncat(exec_cycle, " ", 1);
			}
			//공백으로 분리되었던 실행명령어를 다시 하나로 합쳐줌
			for(int i=6; i<argc; i++)
			{
				strcat(exec_cmd, argv[i]);
				strncat(exec_cmd , " ", 1);
			}
			//crontab_file 작성에 필요한 문자열 제작
			memset(buf, 0, BUF_SIZE);
			sprintf(buf, "%s%s\n", exec_cycle, exec_cmd);
			//ssu_crontab_file 오픈
			if((crontab_file = fopen(crontab_fname, "a+")) == NULL)
			{
				fprintf(stderr, "open error for %s\n", crontab_fname);
				continue;
			}
			//위에서 제작한 <실행주기> <실행할 명령어> 를 파일에 작성하고
			fwrite(buf, strlen(buf), 1, crontab_file);
			//그 string을 리스트로 관리하기 위해 노드를 생성후 리스트에 연결시켜줌
			Crontab_file *new_node = create_node(buf);
			//리스트 맨끝에 연결
			if(head == NULL)
			{
				head = new_node;
				tail = new_node;
			}
			else if(tail != NULL)
			{
				tail->next = new_node;
				tail = new_node;
			}
			fclose(crontab_file);

			//로그작성을 위한 로그파일 오픈
			if((crond_log = fopen(crond_log_fname, "a+")) == NULL)
			{
				fprintf(stderr, "open error for %s\n", crond_log_fname);
				continue;
			}
			//현재시각 추출
			time_t cur_time = time(NULL);
			struct tm *t = localtime(&cur_time);
			char added_cmd[BUF_SIZE];
			char buf2[BUF_SIZE];
			memset(buf2, 0, BUF_SIZE);
			memset(added_cmd, 0, BUF_SIZE);
			strcpy(added_cmd, buf);
			memset(buf, 0, BUF_SIZE);
			
			//현재시각을 문자열로 변경하고 맨뒤에 개행문자 삭제
			strcpy(buf2, ctime(&cur_time));
			buf2[strlen(buf2) - 1] = 0;

			//형식에 맞게 문자열 제작 [현재시각] add [추가한 명령어]
			sprintf(buf, "[%s] %s %s", buf2, "add", added_cmd);

			//로그파일에 작성함
			fwrite(buf, strlen(buf), 1, crond_log);
			fclose(crond_log);
			//ssu_crontab_file에 존재하는 명령어 갯수를 나타내는 변수 1 증가
			//-> 나중에 삭제할때 사용함
			ssu_crontab_file_command_cnt++;
		}
		//명령어 시작이 remove일 경우
		else if(!strcmp(argv[0], "remove"))
		{
			//명령어 인자의 갯수를 체크하여 에러처리
			if(argc > 2)
			{
				fprintf(stderr, "too many arguments\n");
				continue;
			}
			if(argc == 1)
			{
				fprintf(stderr, "input command_number\n");
				continue;
			}
			//만약 2번째인자로 숫자가 아닌 다른정보가 있다면 에러처리함
			int flag = 0;
			for(int i=0; i<strlen(argv[1]); i++)
			{
				//숫자 인지 판별
				if(!isdigit(argv[1][i]))
				{
					flag = 1;
					break;
				}
			}
			//에러처리
			if(flag == 1)
			{
				fprintf(stderr, "command number error\n");
				continue;
			}
			//에러가 없을 경우
			else
			{
				//두번째 인자를 정수로 변환
				int command_number = atoi(argv[1]);

				//만약 입력한 숫자가 리스트에 존재하는 정보의 갯수보다 많을 경우 에러
				if(command_number >= ssu_crontab_file_command_cnt)
				{
					fprintf(stderr, "%d doesn't exist command number\n", command_number);
					continue;
				}
				//범위내의 숫자를 입력했을 경우
				else
				{
					Crontab_file *move = head;
					Crontab_file *pre = NULL;

					//로그 작성에 필요한 삭제된 명령어 정보를 저장하는 배열
					char will_deleted_command[BUF_SIZE];
					memset(will_deleted_command, 0, BUF_SIZE);
					//입력한 숫자만큼  리스트에서 이동
					for(int i=0; i<command_number; i++)
					{
						pre = move;
						move = move->next;
					}
					//지울 데이터에 대한 정보를 저장
					strcpy(will_deleted_command, move->command);
					//start
					if(move == head)
					{
						head = move->next;
						free(move);
					}
					//end
					else if(move == tail)
					{
						pre->next = NULL;
						tail = pre;
						free(move);
					}
					//middle
					else 
					{
						pre->next = move->next;
						free(move);
					}

					//ssu_crontab_file을 쓰기 모드로 open
					if((crontab_file = fopen(crontab_fname, "w")) == NULL)
					{
						fprintf(stderr, "open error for %s\n", crontab_fname);
						continue;
					}
					//리스트 정보를 ssu_crontab_file에 갱신시켜줌
					move = head;
					while(move != NULL)
					{
						fwrite(move->command, strlen(move->command), 1, crontab_file);
						move = move->next;
					}
					fclose(crontab_file);
					//log파일을 append 모드로 open 하여 작성 준비를 함
					if((crond_log = fopen(crond_log_fname, "a+")) == NULL)
					{
						fprintf(stderr, "open error for %s\n", crond_log_fname);
						continue;
					}

					//로그파일에 현재시각과 사라진 명령어 정보를 작성
					time_t cur_time = time(NULL);
					struct tm *t = localtime(&cur_time);
					char buf2[BUF_SIZE];
					memset(buf2, 0, BUF_SIZE);
					memset(buf, 0, BUF_SIZE);

					//현재시각을 문자열화 시킴
					strcpy(buf2, ctime(&cur_time));
					buf2[strlen(buf2) - 1] = 0;
					sprintf(buf, "[%s] %s %s", buf2, "remove", will_deleted_command);
					fwrite(buf, strlen(buf), 1, crond_log);
					fclose(crond_log);
				}
			}
		}
		//exit명령어를 입력받았을 시
		else if(!strcmp(argv[0], "exit"))
		{
			gettimeofday(&end_t, NULL);
			ssu_runtime(&begin_t, &end_t);
			exit(0);
		}
		//이외의 모든 명령어는 에러처리
		else
		{
			fprintf(stderr, "wrong command!\n");
			continue;
		}
	}
}

//프롬프트에서 입력받은 문자열을 공백단위로 분리시켜서 tokens배열에 하나씩 저장
int command_separation(char *command, char (*tokens)[BUF_SIZE])
{
	int argc = 0;
	for(int i=0; i< strlen(command); i++)
	{
		for(int j=0; ; j++,i++)
		{
			//공백이나 엔터를 만날 경우 count를 하나 증가시키고 배열에 널문자를 저장
			if(command[i] == ' ' || command[i] == '\n')
			{
				argc++;
				tokens[argc][j] = '\0';
				break;
			}
			
			//엔터를 입력받았을 경우 중지
			if(command[i] == '\n')
				break;

			//일반문자들일 경우 하나씩 저장
			tokens[argc][j] = command[i];
		}
	}
	return argc;
}

//ssu_crontab_file의 리스트를 조직할 노드 생성함수
Crontab_file *create_node(char *contents)
{
	Crontab_file *new_node = (Crontab_file *)calloc(1, sizeof(Crontab_file));
	strcpy(new_node->command, contents);
	new_node->next = NULL;
	
	return new_node;
}

//수행시간 측정함수
void ssu_runtime(struct timeval *begin_t, struct timeval *end_t)
{
	//끝난 시각에서 시작한 시각을 뺌
	end_t->tv_sec -= begin_t->tv_sec;

	//만약 끝난시각의 밀리초가 더 작으면
	//끝난시각의 초를 하나 줄이고 마이크로초를 증가시킴
	if(end_t->tv_usec < begin_t->tv_usec)
	{
		end_t->tv_sec--;
		end_t->tv_usec += SECOND_TO_MICRO;
	}

	end_t->tv_usec -= begin_t->tv_usec;

	printf("Runtime: %ld:%06ld(sec:usec)\n", end_t->tv_sec, end_t->tv_usec);
}

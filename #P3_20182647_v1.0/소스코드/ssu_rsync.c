#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "ssu_rsync.h"
#include <sys/time.h>

#define SECOND_TO_MICRO 1000000
#define TRUE 1
#define FALSE 0

//옵션 체크용 변수
int tOption = FALSE;
int rOption = FALSE;
int mOption = FALSE;
int nonOption = FALSE;

//-t옵션 적용시 tar파일 생성을 위한 문자열
char *tar_command = "tar -cf";
char *tar_fname = "temp.tar";
char cur_file_name[BUF_SIZE];

//로그파일 작성시 로그 작성시각과 명령어 문장의 출력을 판단하기 위한 변수
int first_flag = 0;

//SIGINT 발생시 이전 dst로 되돌리기 위한 리스트
List *old_dst = NULL;

//동기화할 파일들의 리스트
List *sync_list = NULL;

//임시변수
char glob_dst[BUF_SIZE];
char glob_src[BUF_SIZE];
struct timeval begin_t, end_t;

int main(int argc, char *argv[])
{
	//SIGINT 발생시 시그널 핸들러 등록
	signal(SIGINT, signal_handler);
	int log_file;
	char *log_fname = "ssu_rsync_log";
	char option[BUF_SIZE];
	char src[BUF_SIZE];
	char dst[BUF_SIZE];
	memset(src, 0, BUF_SIZE);
	memset(dst, 0, BUF_SIZE);
	memset(option, 0, BUF_SIZE);
	
	gettimeofday(&begin_t, NULL);
	//command-line으로 들어온 인자의 갯수 체크
	if(argc > 4 || argc < 3)
	{
		fprintf(stderr, "usage: ./ssu_rsync [option] <src> <dst>\n");
		gettimeofday(&end_t, NULL);
		ssu_runtime(&begin_t, &end_t);
		exit(1);
	}

	//각 옵션별로 flag 설정
	//option check
	if(!strcmp(argv[1], "-t")) tOption = TRUE;
	else if(!strcmp(argv[1], "-r")) rOption = TRUE;
	else if(!strcmp(argv[1], "-m")) mOption = TRUE;
	else nonOption = TRUE;

	//옵션이 없을 경우 두번째 세번째 인자가 src와 dst
	if(nonOption == TRUE)
	{
		strcpy(src, argv[1]);
		strcpy(dst, argv[2]);
	}
	//옵션이 무엇이든 존재하면 세번째 네번째 인자가 src와 dst
	else
	{
		strcpy(option, argv[1]);
		strcpy(src, argv[2]);
		strcpy(dst, argv[3]);
	}
	//signal handler에서 src와dst 정보를 얻기 위해 데이터 복사
	strcpy(glob_dst, dst);
	strcpy(glob_src, src);
	
	//src가 존재하는지, src에 접근권한 rwx가 모두 존재하는지 확인
	if(access(src, F_OK) < 0)
		print_usage();
	if(access(src, R_OK) < 0 || access(src, W_OK) < 0 || access(src, X_OK) < 0)
		print_usage();
	//dst가 존재하는지 확인
	if(access(dst, F_OK) < 0)
		print_usage();

	struct stat statbuf;
	if(stat(dst, &statbuf) < 0)
	{
		fprintf(stderr, "stat error\n");
		exit(1);
	}
	//dst가 디렉토리가 아니거나, rwx접근권한이 없을 경우 사용법출력
	if(S_ISDIR(statbuf.st_mode) == 0)
		print_usage();
	if(access(dst, R_OK) < 0 || access(dst, W_OK) < 0 || access(dst, X_OK) < 0)
		print_usage();

	//로그파일이 없으면 새로생성됨
	FILE *log_file_fp;
	if((log_file_fp = fopen(log_fname, "a+")) == NULL)
	{
		fprintf(stderr ,"fopen error for %s\n", log_fname);
		exit(1);
	}
	fclose(log_file_fp);

	//SIGINT 발생시 되돌리기 위해 동기화전 데이터 저장
	old_dst = make_list(dst);

	//m옵션이 있을 경우 src에 없는 파일은 dst에서도 삭제를 수행 하고 동기화진행
	if(mOption == TRUE) do_mOption(src,dst);
	//t옵션이 있을 경우 따로 tar를 만들어서 동기화를 시켜줌
	if(tOption == TRUE) 
	{
		do_tOption(src,dst);

		gettimeofday(&end_t, NULL);
		ssu_runtime(&begin_t, &end_t);
		
		exit(1);
	}
	//그냥 동기화거나 r옵션, m옵션 수행 후 동기화 진행
	//아무작업을 진행하지 않더라도 로그파일에 command-line에서 넘어온 명령어를 출력하라는
	//게시판글을 보고 추가한 구문이라 이 뒤에 존재하는 if(first_flag == 0) {} 은 실행되지 않음
	Sync(src, dst, NULL, argc, argv, 0);
	if(first_flag == 0)
	{
		first_flag = 1;
		//로그파일 오픈
		if((log_file_fp = fopen(log_fname, "a+")) == NULL)
		{
			fprintf(stderr, "fopen error for %s\n", log_fname);
			exit(1);
		}
		//현재시각 추출
		time_t cur_time = time(NULL);
		char tmp_buf[BUF_SIZE];
		memset(tmp_buf, 0, BUF_SIZE);

		//현재시각을 문자열로 변경하고 맨뒤에 개행문자를 널문자로 변경함
		strcpy(tmp_buf, ctime(&cur_time));
		tmp_buf[strlen(tmp_buf) - 1] = 0;

		//로그파일에 command-line에서 입력받은 명령어를 출력시켜줌
		fprintf(log_file_fp, "[%s] ssu_rsync", tmp_buf);
		for(int i=1; i<argc; i++)
		{
			fprintf(log_file_fp, " %s", argv[i]);
		}
		fprintf(log_file_fp, "\n");
		fclose(log_file_fp);
	}
	//로그 작성
	print_log(src,dst);
	gettimeofday(&end_t, NULL);
	ssu_runtime(&begin_t, &end_t);
}

//SIGINT 발생시 처리함수
void signal_handler()
{
	//현재 dst에 존재하는 파일들을 리스트화
	List *new_list = make_list(glob_dst);
	List *move1 = old_dst, *move2 = new_list;
	struct stat statbuf;

	while(move2 != NULL)
	{
		int flag = 0;
		//리스트 맨 처음으로 되돌아감
		move1 = old_dst;
		while(move1 != NULL)
		{
			//같은 이름의 파일이 존재할 경우 pass
			if(!strcmp(move1->fname, move2->fname))
			{
				flag = 1;
				break;
			}
			move1 = move1->next;
		}
		//현재 dst디렉토리에 처음에 저장했던 dst 정보에 파일이 존재하지않으면 삭제를 진행함으로써 원래대로 복구시킴
		if(flag == 0)
		{
			char *tmp = (char *)calloc(BUF_SIZE, sizeof(char));
			memset(tmp, 0, BUF_SIZE);
			sprintf(tmp, "%s/%s", glob_dst, move2->fname);
			stat(tmp, &statbuf);

			//디렉토리일경우 따로 삭제함수를 호출
			if(S_ISDIR(statbuf.st_mode))
				rmdirs(tmp);
			else
				remove(tmp);
		}
		move2 = move2->next;
	}

	exit(1);
}

//t 옵션 수행 함수
void do_tOption(char *src, char *dst)
{
	struct stat src_statbuf, dst_statbuf;
	struct stat statbuf;
	struct dirent *src_dentry, *dst_dentry;
	DIR *src_dp, *dst_dp;
	int index = 0;
	char buf[BUF_SIZE];
	char src_buf[BUF_SIZE];
	char dst_buf[BUF_SIZE];
	char need_sync[1000][BUF_SIZE];

	stat(src, &statbuf);
	//일반파일의 경우
	if(!S_ISDIR(statbuf.st_mode))
	{
		strcpy(need_sync[index++], src);
	}
	//디렉토리를 동기화 하는경우
	else
	{
		//src와 dst 디렉토리 각각 opendir 해줌
		if((src_dp = opendir(src)) == NULL)
		{
			fprintf(stderr, "opendir error for %s\n", src);
			return;
		}
		if((dst_dp = opendir(dst)) == NULL)
		{
			fprintf(stderr, "opendir error for %s\n", dst);
			return;
		}

		//src의 한파일과 dst 전부를 비교하여 동기화가 필요한지 판단함
		while((src_dentry = readdir(src_dp)) != NULL)
		{
			//"." 와 ".."은 무시함
			if(!strcmp(src_dentry->d_name, ".") || !strcmp(src_dentry->d_name, "..")) continue;
			memset(src_buf, 0, BUF_SIZE);
			sprintf(src_buf, "%s/%s", src, src_dentry->d_name);
			stat(src_buf, &src_statbuf);
			//디렉토리는 t옵션에서 동기화 하지 않음
			if(S_ISDIR(src_statbuf.st_mode)) continue;
			//dst 디렉토리 포인터 처음으로 되돌림
			rewinddir(dst_dp);
			int flag = 0;
			while((dst_dentry = readdir(dst_dp)) != NULL)
			{
				//"." 와 ".."은 무시함
				if(!strcmp(dst_dentry->d_name, ".") || !strcmp(dst_dentry->d_name, "..")) continue;
				memset(dst_buf, 0, BUF_SIZE);
				sprintf(dst_buf, "%s/%s", dst, dst_dentry->d_name);
				stat(dst_buf, &dst_statbuf);

				//파일이름, 크기 , 최종수정일이 같을 경우 같은 파일 동기화x
				if(!strcmp(src_dentry->d_name, dst_dentry->d_name) && src_statbuf.st_size == dst_statbuf.st_size && src_statbuf.st_mtime == dst_statbuf.st_mtime)
				{
					flag = 1; 
					break;
				}
				//파일이름은 같지만 크기 , 최종수정일이 다를 경우 다른파일로 인식
				else if(!strcmp(src_dentry->d_name, dst_dentry->d_name) && (src_statbuf.st_size != dst_statbuf.st_size || src_statbuf.st_mtime != dst_statbuf.st_mtime))
				{
					flag = 2;
					break;
				}
			}
			//동기화가 필요하면 배열에 저장하여 이름을 저장해둠
			if(flag == 0 || flag == 2)
			{
				strcpy(need_sync[index++], src_dentry->d_name);
			}
		}
	}

	if(index == 0)
		exit(1);

	//doing tar prcoess
	struct stat tar_statbuf;
	char command[BUF_SIZE * 2];
	char process_dir[BUF_SIZE];
	//현재 작업디렉토리 경로를 미리저장
	getcwd(process_dir, BUF_SIZE);

	//src 디렉토리로 이동함
	chdir(src);

	memset(command, 0 , sizeof(command));
	strcpy(command, tar_command);
	strcat(command, " ");

	memset(buf, 0, BUF_SIZE);
	//tar파일이 어디 생성될지 결정하는 문자열
	//현재디렉토리/dst디렉토리/tar파일 이름
	sprintf(buf, "%s/%s/%s", process_dir, dst, tar_fname);
	strcat(command, buf);
	strcat(command, " ");
	//명령어 압축할 파일들을 나열해줌
	//ex tar -cf ./1.txt ./2.txt ./3.txt .....
	for(int i=0; i<index; i++)
	{
		memset(buf, 0, BUF_SIZE);
		sprintf(buf, "./%s", need_sync[i]);
		strcat(command, buf);
		strcat(command, " ");
	}
	//압축 진행
	system(command);

	chdir(process_dir);
	chdir(dst);
	//tar파일의 크기를 얻기위함
	stat("temp.tar", &tar_statbuf);
	memset(command, 0, sizeof(command));
	//압축해제 명령어 제작
	sprintf(command, "tar -xf %s",  tar_fname);
	//압축 해제시킴
	system(command);
	//tar파일은 삭제함
	remove("temp.tar");

	//다시 현재 작업디렉토리로 돌아와서 로그 작성을 진행함
	chdir(process_dir);
	FILE *log_file;
	char *log_fname = "ssu_rsync_log";
	time_t cur_time = time(NULL);
	memset(buf, 0, BUF_SIZE);
	//현재시각을 문자열화 시킴
	strcpy(buf, ctime(&cur_time));
	buf[strlen(buf) - 1] = 0;
	//로그파일을 append모드로 열기
	if((log_file = fopen(log_fname , "a")) == NULL)
	{
		fprintf(stderr, "fopen error for %s\n", log_fname);
		exit(1);
	}
	//현재시각과 명령어 정보 출력
	fprintf(log_file, "[%s] ssu_rsync -t %s %s\n", buf, src, dst);
	//tar파일의 크기도 출력
	fprintf(log_file, "\ttotalSize %ldbytes\n", tar_statbuf.st_size);
	//동기화한 파일들의 이름과 크기도 로그에 작성
	for(int i=0; i<index; i++)
	{
		//디렉토리를 t옵션으로 동기화하는경우
		//파일 크기를 얻기 위한 작업
		if(S_ISDIR(statbuf.st_mode))
		{
			struct stat statbuf2;
			memset(buf, 0, BUF_SIZE);
			sprintf(buf, "%s/%s", src, need_sync[i]);
			stat(buf, &statbuf2);
			fprintf(log_file, "\t%s %ldbytes\n", need_sync[i], statbuf2.st_size);
		}
		//일반파일을 동기화하는경우 그냥 로그 출력
		else
		{
			fprintf(log_file, "\t%s %ldbytes\n", need_sync[i], statbuf.st_size);
		}
	}

	fclose(log_file);
}

//-m 옵션 수행 함수
void do_mOption(char *src, char *dst)
{
	struct stat src_statbuf, dst_statbuf;
	struct dirent *src_dentry, *dst_dentry;
	DIR *src_dp, *dst_dp;

	char buf[BUF_SIZE];
	char src_buf[BUF_SIZE];
	char dst_buf[BUF_SIZE];
	memset(src_buf, 0, BUF_SIZE);
	memset(dst_buf, 0, BUF_SIZE);
	memset(buf, 0, BUF_SIZE);

	//src , dst 디렉토리를 open
	if((src_dp = opendir(src)) == NULL)
	{
		fprintf(stderr, "opendir error for %s\n", src);
		exit(1);
	}

	if((dst_dp = opendir(dst)) == NULL)
	{
		fprintf(stderr, "opendir error for %s\n", dst);
		exit(1);
	}

	//dst파일 하나와 src디렉토리를 전부 순회하면서 비교
	while((dst_dentry = readdir(dst_dp)) != NULL)
	{
		//"." 과 ".."은 무시
		if(!strcmp(dst_dentry->d_name, "..") || !strcmp(dst_dentry->d_name, ".")) continue;
		//파일의 존재여부 판단 flag
		int flag = 0;
		memset(dst_buf, 0, BUF_SIZE);
		sprintf(dst_buf, "%s/%s", dst, dst_dentry->d_name);
		stat(dst_buf, &dst_statbuf);
		//디렉토리 포인터 맨 처음으로 되돌려서 각 파일 별로 src디렉토리를 전부 순회하게 만듬
		rewinddir(src_dp);
		while((src_dentry = readdir(src_dp)) != NULL)
		{
			//"." 과 ".."은 무시
			if(!strcmp(src_dentry->d_name, "..") || !strcmp(src_dentry->d_name, ".")) continue;
			memset(src_buf, 0, BUF_SIZE);
			sprintf(src_buf, "%s/%s", src, src_dentry->d_name);
			stat(src_buf, &src_statbuf);
			//파일의 이름, 크기, 최종수정일이 같으면 같은 파일이라고 판단
			if(!strcmp(dst_dentry->d_name, src_dentry->d_name) && src_statbuf.st_size == dst_statbuf.st_size && src_statbuf.st_mtime == dst_statbuf.st_mtime)
			{
				flag = 1;
				break;
			}
		}
		//dst의 하나의 파일이 src디렉토리에 존재하지 않을 경우
		if(flag == 0)
		{
			//로그파일을 open하여 로그 작성
			FILE *log_file;
			char *log_fname = "ssu_rsync_log";
			if((log_file = fopen(log_fname, "a+")) == NULL)
			{
				fprintf(stderr, "fopen error for %s\n", log_fname);
				exit(1);
			}
			//현재시각과 명령어 정보를 처음 출력하는 경우에만 출력해줌
			if(first_flag == 0)
			{
				//현재 시간 정보를 얻어옴
				time_t cur_time = time(NULL);
				char buf2[BUF_SIZE];
				memset(buf2, 0, BUF_SIZE);
				//문자열 형태로 바꾸어서 buf2에 저장
				strcpy(buf2, ctime(&cur_time));
				//맨 뒤에 개행 문자 삭제
				buf2[strlen(buf2) - 1] = 0;
				//로그 작성시각 및 명령어 정보 로그파일에 작성
				fprintf(log_file, "[%s] ssu_rsync -m %s %s\n", buf2, src, dst);
				first_flag = 1;
			}
			//삭제된 파일 이름과 크기를 출력
			fprintf(log_file, "\t%s delete\n", dst_dentry->d_name);
			fclose(log_file);

			//디렉토리일경우 따로 작성한 디렉토리 삭제함수 호출
			if(S_ISDIR(dst_statbuf.st_mode))
				rmdirs(dst_buf);
			else
				remove(dst_buf);
		}
	}
}
//디렉토리 삭제함수
void rmdirs(char *path)
{
	struct dirent *dirp;
	struct stat statbuf;
	DIR *dp;
	char *tmp = (char *)calloc(BUF_SIZE, sizeof(char));

	//인자로 받은 디렉토리 오픈
	if((dp = opendir(path)) == NULL)
		return;

	while((dirp = readdir(dp)) != NULL)
	{
		//"." 과 ".."은 무시
		if(!strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, "..")) continue;

		sprintf(tmp, "%s/%s", path, dirp->d_name);

		//파일 정보를 불러움. 디렉토리인지 아닌지 판별할때 사용
		if(lstat(tmp, &statbuf) == -1)
			continue;

		//서브 디렉토리가 존재한다면 서브디렉토리로 들어가 내부 파일 삭제
		if(S_ISDIR(statbuf.st_mode))
			rmdirs(tmp);
		//일반파일의 경우 그냥 unlink
		else
			unlink(tmp);
	}
	//내부 파일 삭제가 끝나면 지금 디렉토리도 삭제시킴
	closedir(dp);
	rmdir(path);
}

//사용법 출력
void print_usage()
{
	fprintf(stderr, "usage: ./ssu_rsync [option] <src> <dst>\n");
	gettimeofday(&end_t, NULL);
	ssu_runtime(&begin_t, &end_t);
	exit(1);
}

//동기화 시켜주는 함수
void Sync(char *src, char* dst, char *sub_dir, int argc, char **argv, int depth)
{
	DIR *dp;
	struct stat statbuf;
	struct stat statbuf2;
	struct flock lock;
	struct dirent *dentry;
	char buf[BUF_SIZE];
	char file_name[BUF_SIZE];
	memset(buf, 0, BUF_SIZE);
	memset(file_name, 0, BUF_SIZE);

	strcpy(buf, src);

	//src에서 파일이름만을 추출함
	char *p = strtok(buf, "/");
	char *pre;
	while(p != NULL)
	{
		pre = p;
		p = strtok(NULL, "/");
	}
	strcpy(file_name, pre);

	//src파일의 정보를 저장함 -> 디렉토리인지 파일인지 판별하기 위함
	if(stat(src, &statbuf) < 0)
	{
		fprintf(stderr, "stat error\n");
		exit(1);
	}
	//dst 디렉토리를 오픈함
	if((dp = opendir(dst)) == NULL)
	{
		fprintf(stderr, "opendir error for %s\n", dst);
		exit(1);
	}
	
	//일반파일을 동기화 하는경우
	if(!S_ISDIR(statbuf.st_mode))
	{
		//파일이 존재하는지 여부 판단용 flag
		int flag = 0;
		while((dentry = readdir(dp)) != NULL)
		{
			//"." 와 ".."은 무시
			if(!strcmp(dentry->d_name, "..") || !strcmp(dentry->d_name, ".")) continue;
			sprintf(buf, "%s/%s", dst, dentry->d_name);

			stat(buf, &statbuf2);

			//파일의 이름, 크기, 최종수정시각이 같으면 같은 파일임으로 동기화할 필요없음
			if(!strcmp(dentry->d_name, file_name) && statbuf.st_size == statbuf2.st_size && statbuf.st_mtime == statbuf2.st_mtime)
			{
				flag = 1;
				break;
			}
			//이름은같은데 크기, 최종수정시각이 다를경우 동기화 필요
			else if (!strcmp(dentry->d_name, file_name) && (statbuf.st_size != statbuf2.st_size || statbuf.st_mtime != statbuf2.st_mtime))
			{
				flag = 2;
				break;
			}
		}
		//0 의경우 파일이 아예 존재하지 않는 경우
		//2 의경우 파일이 존재하지만 크기나 수정시각이 다를 경우
		if(flag == 0 || flag == 2)
		{
			//동기화 진행
			normal_sync(NULL, dst, file_name);

			//로그파일을 오픈하여 작성하는 작업 진행
			FILE *log_file;
			char *log_fname = "ssu_rsync_log";

			if((log_file = fopen(log_fname, "a+")) == NULL)
			{
				fprintf(stderr, "fopen error for %s\n", log_fname);
				exit(1);
			} 
			//로그의 시작부분을 아직 작성안했다면 작성하고 flag를 설정하여 다른곳에서 첫줄을 다시 작성하지 않도록 유도함
			if(first_flag == 0)
			{
				time_t cur_time = time(NULL);
				char buf2[BUF_SIZE];
				memset(buf2, 0, BUF_SIZE);
				strcpy(buf2, ctime(&cur_time));
				buf2[strlen(buf2) - 1] = 0;
				fprintf(log_file, "[%s] ssu_rsync %s %s\n", buf2, src, dst);
				first_flag = 1;
			}
			//파일의 이름과 크기 작성
			fprintf(log_file, "\t%s %ldbytes\n", file_name, statbuf.st_size);
			fclose(log_file);
		}
	}
	//디렉토리를 동기화 하는경우
	else
	{
		DIR *src_dp, *dst_dp;
		struct dirent *src_dentry, *dst_dentry;
		struct stat src_statbuf, dst_statbuf;
		char src_buf[BUF_SIZE], dst_buf[BUF_SIZE];
		
		//src와 dst 모두 디렉토리임으로 open 함
		if((src_dp = opendir(src)) == NULL)
		{
			fprintf(stderr, "opendir error for %s\n", src);
			exit(1);
		}
		if((dst_dp = opendir(dst)) == NULL)
		{
			fprintf(stderr, "opendir error for %s\n", dst);
			exit(1);
		}

		//src의 한 파일과 dst디렉토리 내부 모든 파일과 비교하는 과정을 거침
		while((src_dentry = readdir(src_dp)) != NULL)
		{
			//"."와 ".."은 무시
			if(!strcmp(src_dentry->d_name, ".") || !strcmp(src_dentry->d_name, "..")) continue;
			//파일의 존재여부를 판단하는 flag
			int flag=0;
			memset(src_buf, 0, BUF_SIZE);
			sprintf(src_buf, "%s/%s", src, src_dentry->d_name);
			stat(src_buf, &src_statbuf);
			//디렉토리 포인터 처음으로 돌려서 src의 각 파일이 dst의 모든 파일과 비교하도록 설정
			rewinddir(dst_dp);
			while((dst_dentry = readdir(dst_dp)) != NULL)
			{	
				//"."와 ".."은 무시
				if(!strcmp(dst_dentry->d_name, ".") || !strcmp(dst_dentry->d_name, "..")) continue;
				memset(dst_buf, 0, BUF_SIZE);
				sprintf(dst_buf, "%s/%s", dst, dst_dentry->d_name);
				stat(dst_buf, &dst_statbuf);
				
				//파일의 이름과 크기,최종수정일이 모두 같을때 
				if(!strcmp(src_dentry->d_name, dst_dentry->d_name) && src_statbuf.st_size == dst_statbuf.st_size && src_statbuf.st_mtime == dst_statbuf.st_mtime)
				{
					flag = 1;
					break;
				}
				//파일의 이름은 같지만 크기, 최종수정일이 다를 경우 동기화가 필요함
				else if(!strcmp(src_dentry->d_name, dst_dentry->d_name) && (src_statbuf.st_size != dst_statbuf.st_size || src_statbuf.st_mtime != dst_statbuf.st_mtime))
				{
					flag = 2;
					break;
				}
			}
			//0 의경우 파일이 아예 존재하지 않는 경우
			//2 의경우 파일이 존재하지만 크기나 수정시각이 다를 경우
			if(flag == 0 || flag == 2)
			{
				//src의 한 파일이 디렉토리가 아닐경우
				if(!S_ISDIR(src_statbuf.st_mode))
				{
					//동기화 진행
					normal_sync(src, dst, src_dentry->d_name);

					char *temp = (char *)calloc(BUF_SIZE, sizeof(char));
					//src 바로 밑에 있을 경우
					if(depth == 0)
					{
						strcpy(temp, src_dentry->d_name);
					}
					//src디렉토리안의 서브디렉토리에 있을 경우 경로를 추가
					else
					{
						sprintf(temp, "%s/%s", sub_dir, src_dentry->d_name);
					}
					//새로운 노드를 생성하여 동기화가 필요한 파일의 정보를 담고 있는 sync_list에 추가해줌
					List *new_node = make_node(temp);
					new_node->size = src_statbuf.st_size;
					new_node->next = NULL;
					new_node->child = NULL;

					if(sync_list == NULL)
						sync_list = new_node;
					else
					{
						new_node->next = sync_list;
						sync_list = new_node;
					}
				}
				//디렉토리의 경우
				else
				{
					//r 옵션이 존재하지않을 경우 동기화 x
					if(rOption == FALSE)
						continue;
					//r 옵션이 존재할 경우
					else
					{
						struct utimbuf time_buf;
						time_buf.actime = src_statbuf.st_atime;
						time_buf.modtime = src_statbuf.st_mtime;
						memset(buf, 0, BUF_SIZE);
						sprintf(buf, "%s/%s", dst, src_dentry->d_name);
						//dst내에 서브디렉토리를 생성, 접근권한은 원본파일과 동일하게 설정
						mkdir(buf, src_statbuf.st_mode);

						char new_src[BUF_SIZE];
						char new_dst[BUF_SIZE];
						char new_sub_dir[BUF_SIZE];
						memset(new_src, 0, BUF_SIZE);
						memset(new_dst, 0, BUF_SIZE);
						memset(new_sub_dir, 0, BUF_SIZE);

						//src내의 서브디렉토리안의 디렉토리일 경우 경로 추가
						if(sub_dir != NULL)
						{
							sprintf(new_sub_dir, "%s/%s", sub_dir, src_dentry->d_name);
						}
						else
						{
							sprintf(new_sub_dir, "%s", src_dentry->d_name);
						}
						sprintf(new_src, "%s/%s", src, src_dentry->d_name);
						sprintf(new_dst, "%s/%s", dst, src_dentry->d_name);
						//서브 디렉토리의 동기화 진행을 위해 Sync함수 재호출
						Sync(new_src, new_dst, new_sub_dir, argc, argv, depth+1);
						//atime과 mtime을 원본파일과 동일 시 시켜줌
						utime(buf, &time_buf);
					}
				}
			}
		}
	}
}

//실질적인 파일 복사가 이루어지는 동기화 함수
void normal_sync(char *src, char *dst, char *fname)
{
	struct flock lock;
	struct stat src_statbuf;
	struct utimbuf time_buf;
	int src_fd, dst_fd;
	char dst_path[BUF_SIZE];
	char src_path[BUF_SIZE];
	memset(dst_path, 0, BUF_SIZE);
	memset(src_path, 0, BUF_SIZE);

	if (src == NULL) strcpy(src_path, fname);
	else sprintf(src_path, "%s/%s", src, fname);
	sprintf(dst_path, "%s/%s", dst, fname);
	
	//원본 파일을 읽기 전용으로 오픈함
	if((src_fd = open(src_path, O_RDONLY)) < 0)
	{
		fprintf(stderr, "open error for %s\n", src);
		exit(1);
	}
	//원본 파일의 atime과 mtime을 얻어옴
	stat(src_path, &src_statbuf);
	time_buf.actime = src_statbuf.st_atime;
	time_buf.modtime = src_statbuf.st_mtime;

	//dst에 같은 이름의 파일을 새로 오픈함, 접근권한은 원본파일과 동일하게 설정
	if((dst_fd = open(dst_path, O_RDWR|O_CREAT|O_TRUNC, src_statbuf.st_mode)) < 0)
	{
		fprintf(stderr, "open error for %s\n", dst_path);
		exit(1);
	}
	//원본파일에 쓰기 lock을 걸어서 수정이 안되도록 설정
	lock.l_type = F_WRLCK;
	lock.l_whence = 0;
	lock.l_start = 0l;
	lock.l_len = 0l;

	fcntl(src_fd, F_SETLK, &lock);
	
	//src의 원본파일 내용을 -> dst의 새로생성한 파일에 복사
	char buf[BUF_SIZE];
	memset(buf, 0, BUF_SIZE);
	while(read(src_fd, buf,  BUF_SIZE) > 0)
	{
		write(dst_fd, buf, strlen(buf));
		memset(buf, 0, BUF_SIZE);
	}

	//락을 해제 함
	lock.l_type = F_UNLCK;
	fcntl(src_fd, F_SETLK, &lock);
	close(src_fd);
	close(dst_fd);
	//새로생성한 파일의 atime과 mtime을 원본파일의것으로 변경함
	utime(dst_path, &time_buf);
}

//넘겨 받은 인자로 노드를 생성하여 리턴시켜주는 함수
List *make_node(char *fname)
{
	List *new_node = (List *)calloc(1, sizeof(List));
	strcpy(new_node->fname, fname);
	new_node->next = NULL;
	new_node->child = NULL;

	return new_node;
}

//인자로 넘겨받은 디렉토리의 파일정보를 리스트화 시킴
List *make_list(char *path)
{
	List *root = NULL;
	DIR *dp;
	struct dirent *dentry;
	struct stat statbuf;
	char buf[BUF_SIZE];
	memset(buf, 0, BUF_SIZE);
	
	//디렉토리를 오픈
	if((dp = opendir(path)) == NULL)
	{
		fprintf(stderr, "open error for %s\n", path);
		return NULL;
	}

	while((dentry = readdir(dp)) != NULL)
	{
		//"." 와 ".."은 무시
		if(!strcmp(dentry->d_name , ".") || !strcmp(dentry->d_name, "..")) continue;
		//파일의 이름으로 새로운 노드를 생성함
		List *new_node = make_node(dentry->d_name);
		char *temp = (char *)calloc(BUF_SIZE, sizeof(char));
		sprintf(temp, "%s/%s", path, dentry->d_name);
		stat(temp, &statbuf);

		//만약 디렉토리일 경우 child를 루트로 하여 서브디렉토리의 리스트를 생성하여 연결
		if(S_ISDIR(statbuf.st_mode))
		{
			new_node->child = make_list(temp);
		}

		//항상 리스트 맨 앞에 연결
		if(root == NULL)
			root = new_node;
		else
		{
			new_node->next = root;
			root = new_node;
		}
	}
	return root;
}

/*
List *print_list(List *root)
{
	while(root != NULL)
	{
		printf("%s\n", root->fname);
		if(root->child != NULL)
			print_list(root->child);
		root = root->next;
	}
}
*/

//로그 출력 함수
void print_log(char *src, char *dst)
{
	//동기화 시킬 파일이 하나도 없을 경우 로그작성 x
	if(sync_list == NULL) return;
	FILE *fp;
	//현재시각을 불러와 저장
	time_t cur_time;
	cur_time = time(NULL);
	char *fname = "ssu_rsync_log";
	char buf[BUF_SIZE];
	memset(buf, 0, BUF_SIZE);
	//현재시각을 문자열로 변경해수 맨 뒤에 개행문자 삭제
	strcpy(buf, ctime(&cur_time));
	buf[strlen(buf) - 1] = 0;

	//append모드로 로그 파일 오픈
	if((fp = fopen(fname, "a+")) == NULL)
	{
		fprintf(stderr, "fopen error for %s\n", fname);
		return;
	}
	//현재시각과 명령어 정보를 처음 출력하는 경우 
	if(first_flag == 0)
		fprintf(fp, "[%s] ssu_rsync %s %s\n", buf, src, dst);

	//동기화가 필요한 파일의 이름과 크기 정보를 담고 있는 리스트를 순회하면서 로그파일에 작성
	List *move = sync_list;
	while(move != NULL)
	{
		fprintf(fp, "\t%s %ldbytes\n", move->fname, move->size);
		move = move->next;
	}
	fclose(fp);
	return;
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

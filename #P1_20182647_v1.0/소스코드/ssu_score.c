#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "ssu_score.h"
#include "blank.h"

extern struct ssu_scoreTable score_table[QNUM];
extern char id_table[SNUM][10];

//score table 선언
struct ssu_scoreTable score_table[QNUM];
//학생들 id 저장하는 배열
char id_table[SNUM][10];

//학생 디렉토리 경로 저장 배열
char stuDir[BUFLEN];
//정답 디렉토리 경로 저장 배열
char ansDir[BUFLEN];
//e 옵션의 경우 에러디렉토리
char errorDir[BUFLEN];
//스레드파일의 정보를 저장하는 배열
char threadFiles[ARGNUM][FILELEN];
//틀린문제 정보 출력할 학번들 저장하는 배열
char cIDs[ARGNUM][FILELEN];

//옵션 여부 flag
int iOption = false;
int eOption = false;
int mOption = false;
int tOption = false;
//디렉토리없이 i옵션만 있는지 확인
int first_is_i = false;


void ssu_score(int argc, char *argv[])
{
	//기존 경로 저장하기 위한 버퍼
	char saved_path[BUFLEN];
	int i;

	//맨처음이 i옵션인지 확인
	if(!strcmp(argv[1], "-i"))
		first_is_i = true;

	//-h옵션이 존재하는지 확인
	for(i = 0; i < argc; i++){
		if(!strcmp(argv[i], "-h")){
			print_usage();
			return;
		}
	}

	//saved_path 초기화
	memset(saved_path, 0, BUFLEN);

	//argv[1] 가 -i옵션이 아닐경우 학생디렉토리와 정답디렉토리를 저장
	//-i옵션이면 score.csv파일만 읽어서 출력할 예정
	if(argc >= 3 && strcmp(argv[1], "-i") != 0){
		strcpy(stuDir, argv[1]);
		strcpy(ansDir, argv[2]);
	}

	//옵션들의 flag설정
	if(!check_option(argc, argv))
		exit(1);

	//학생,정답 디렉토리없이 i옵션만 있을때 score.csv참고해서 정보 출력
	if(first_is_i && iOption)
		do_iOption(cIDs);



	//현재 작업디렉토리의 경로를 임시 버퍼에 저장
	getcwd(saved_path, BUFLEN);

	//학생 디렉토리가 존재하지않을 경우 에러처리 존재하면 이동
	if(chdir(stuDir) < 0){
		fprintf(stderr, "%s doesn't exist\n", stuDir);
		return;
	}
	//학생디렉토리의 경로를 저장
	getcwd(stuDir, BUFLEN);
	
	//원래 위치로 다시 이동
	chdir(saved_path);
	//정답 디렉토리가 존재하지 않을 경우 에러처리 존재하면 이동
	if(chdir(ansDir) < 0){
		fprintf(stderr, "%s doesn't exist\n", ansDir);
		return;
	}
	//정답 디렉토리의 경로를 저장한다
	getcwd(ansDir, BUFLEN);

	//원래 위치로 다시 돌아온다
	chdir(saved_path);


	//scoreTable 세팅해준다
	set_scoreTable(ansDir);

	//채점 시작전에 m옵션 수행
	if(mOption) do_mOption(ansDir);

	//학번테이블 세팅
	set_idTable(stuDir);

	printf("grading student's test papers..\n");
	//학생들 채점시작
	score_students();

	//채점이 끝난 후 score.csv파일을 이용해 i옵션 수행
	if(iOption)
		do_iOption(cIDs);

	return;
}

//옵션들 점검
int check_option(int argc, char *argv[])
{
	int i, j;
	int c;

	//e의 경우 추가인자가 발생
	//t와 i도 발생하기 하지만 case에서 따로 처리해줌
	while((c = getopt(argc, argv, "e:thmi")) != -1)
	{
		switch(c){
			//e 옵션의 경우 errorDir에 다음 인자를 저장함(에러디렉토리)
			case 'e':
				//e옵션 플레그 세우기
				eOption = true;
				strcpy(errorDir, optarg);

				//파일이 존재하지않으면 새로 만듬
				if(access(errorDir, F_OK) < 0)
					mkdir(errorDir, 0755);
				//존재하면 지우고 만듬
				else{
					rmdirs(errorDir);
					mkdir(errorDir, 0755);
				}
				break;
				//t옵션일때
			case 't':
				tOption = true;
				//다음 인자의 인덱스를 가리킴
				i = optind;
				j = 0;
				//argc보다 작고 옵션을 나타내는 -가 아닐때까지
				while(i < argc && argv[i][0] != '-'){

					//최대인자 갯수를 넘어가면 적용되지않는 문제 출력
					if(j >= ARGNUM)
						printf("Maximum Number of Argument Exceeded.  :: %s\n", argv[i]);
					//lpthread를 적용한 문제들 threadFiles에 저장
					else
						strcpy(threadFiles[j], argv[i]);
					i++; 
					j++;
				}
				break;
				//i옵션의 경우
			case 'i':
				//flag를 세워주고
				iOption = true;
				//i가 다음인자의인덱스를 저장
				i = optind;
				j = 0;
				//argc보다 작고 옵션을 나타내는 -가 아닐때까지
				while(i < argc && argv[i][0] != '-'){
					//인자 최대갯수 5개를 넘어가면 넘어간거 출력
					if(j >= ARGNUM)
						printf("Maximum Number of Argument Exceeded.  :: %s\n", argv[i]);
					//아니면 cIDS에 저장
					else
						strcpy(cIDs[j], argv[i]);
					i++;
					j++;
				}
				break;
				//m옵션의경우 flag만 세워줌
			case 'm':
				mOption = true;
				break;
				//지정한 옵션이 아닐경우 알수없다고 띄움
			case '?':
				printf("Unkown option %c\n", optopt);
				return false;
		}
	}

	return true;
}
//m옵션 수행
void do_mOption(char *path)
{
	int fd;
	char tmp[BUFLEN];
	char buf[BUFLEN];
	char buf2[BUFLEN];
	char file_path[BUFLEN];
	int i;
	int find=false;
	double new_score;
	
	//score_table.csv의 파일 경로를 저장
	memset(file_path, 0, BUFLEN);
	sprintf(file_path, "%s/%s", path, "score_table.csv");

	//문제별 배점이 저장된 score_table.csv파일 RDONLY로 염
	if ((fd = open(file_path, O_RDWR)) < 0)
	{
		fprintf(stderr, "file open error for score_table.csv\n");
		return;
	}

	while(1)
	{
		printf("Input question's number to modify >> ");
		scanf("%s",tmp);

		//no를 입력받으면 break;
		if(!strcmp(tmp, "no") || !strcmp(tmp, "NO") || !strcmp(tmp, "No")) break;

		find = false;
		//score_table[i]에서 입력받은 문제를 search
		for(i=0; i<QNUM; i++)
		{
			//읽을게 없으면 끝
			if(!strcmp(score_table[i].qname, ""))
				break;

			//확장자표시 .을 가리키고
			char *p = strchr(score_table[i].qname, '.');

			//txt나 c파일 일경우에만 문제번호를 비교해서 같으면 find 플래그를 true로 설정
			if(!strcmp(p, ".txt") || !strcmp(p, ".c"))
			{
				memset(buf, 0, BUFLEN);
				strncpy(buf, score_table[i].qname, strlen(score_table[i].qname) - strlen(p));
				
				if(!strcmp(buf,tmp))
				{
					find = true; break;
				}
			}
		}

		//존재하지 않는 문제번호면 다시 입력하게 유도
		if(!find)
		{
			printf("Input correct question's number\n");
			continue;
		}
		//존재하는 문제라면
		else
		{
			//현재 배점을 보여주고
			printf("Current score : %.1f\n", score_table[i].score);
			//새로운 배점을 저장해줌
			printf("New score : "); scanf("%lf",&new_score);
			score_table[i].score = new_score;

			//새롭게 업데이트된 정보들을 다시 써줌
			lseek(fd, 0, SEEK_SET);
			//score_Table 크기 계산
			int size = sizeof(score_table) / sizeof(score_table[0]);
			//파일에 변경된 내용 처음부터 다시쓰기
			for (int i = 0; i < size; i++)
			{
				//읽을게 없으면 스탑
				if (!strcmp(score_table[i].qname, ""))
					break;
				memset(buf2, 0, BUFLEN);
				sprintf(buf2, "%s,%.2f\n", score_table[i].qname, score_table[i].score);
				write(fd, buf2, strlen(buf2));
			}
		}
	}
}

//i옵션 실행
int do_iOption(char (*ids)[FILELEN])
{


	FILE *fp;
	char tmp[BUFLEN];
	int i = 0;
	char *p, *saved;
	//score.csv에 접근이 불가능하면 리턴 1
	if(access("score.csv", F_OK) < 0)
		return 1;

	//score.csv파일 열기를 시도
	if((fp = fopen("score.csv", "r")) == NULL){
		fprintf(stderr, "file open error for score.csv\n");
		return 0;
	}
	//score.csv파일 맨 첫줄을 읽어드림
	fscanf(fp, "%s\n", tmp);
	//첫번째 문제이름 저장
	p = strtok(tmp, ",");
	strcpy(score_table[0].qname, p);
	//','을 토큰으로 하여 문제이름들 score_table[i]에 하나씩 저장함
	i=1;
	while((p=strtok(NULL, ","))!=NULL)
	{
		//sum일경우 스탑
		if(!strcmp(p, "sum")) break;
		strcpy(score_table[i].qname, p);
		i++;
	}
	int index=i;

	//score.csv파일에서 읽을게 없을때 까지
	while(fscanf(fp, "%s\n", tmp) != EOF)
	{
		//','을토큰으로 학번의 시작을 가리킴
		p = strtok(tmp, ",");

		//score.csv에서 읽은 학번이 cIDs에 존재하는 확인
		//없으면 continue
		if(!is_exist(ids, tmp))
			continue;

		printf("%s's wrong answer : \n", tmp);

		i=0;
		//','을 토큰으로 점수를 하나씩읽음
		while((p = strtok(NULL, ",")) != NULL)
		{
			//점수가 0점일 경우 문제이름 출력
			if(atof(p) == 0.0)
					printf("%s, ",score_table[i].qname);	
			
			i++;
		}
		//맨마지막에 ,생기는거 없애줌
		printf("\b\b \n");
	}
	fclose(fp);
	return 0;
}

//target이 cIDs배열에 존재하는지 확인
int is_exist(char (*src)[FILELEN], char *target)
{
	int i = 0;

	//cIDS에 target이 존재하는지 확인
	while(1)
	{
		//최대갯수 5개보다 넘어가면 
		if(i >= ARGNUM)
			return false;
		//더 이상 읽을게 없으면 false
		else if(!strcmp(src[i], ""))
			return false;

		//같으면 true 리턴
		else if(!strcmp(src[i++], target))
			return true;
	}
	//없으면 false 리턴
	return false;
}

//score_table 세팅하는 함수
void set_scoreTable(char *ansDir)
{
	char filename[FILELEN];

	//filename에다가 "정답디렉토리 경로/score_table.csv" 문자열을 저장한다
	sprintf(filename, "%s/%s", ansDir, "score_table.csv");

	//파일이 이미 존재하면 scoreTable 읽기
	if(access(filename, F_OK) == 0)
		read_scoreTable(filename);
	//파일이 존재하지 않으면 scoreTable을 만들고 쓴다.
	else{
		make_scoreTable(ansDir);
		write_scoreTable(filename);
	}
}

//score_table이 존재하면 읽는 함수
void read_scoreTable(char *path)
{
	FILE *fp;
	//확장자를 제외한 이름 만 저장하는 배열
	char qname[FILELEN];
	//점수 배점을 저장
	char score[BUFLEN];
	int idx = 0;

	//"r" 모드로 넘겨받은 path에 있는 파일을 열기 실패하면 에러처리
	if((fp = fopen(path, "r")) == NULL){
		fprintf(stderr, "file open error for %s\n", path);
		return ;
	}

	//파일이 끝까지 읽는데 ','전까지 qname에 저장 나머지는 score에 저장
	while(fscanf(fp, "%[^,],%s\n", qname, score) != EOF){
		//score_table.qname에 하나씩 문제이름 저장
		strcpy(score_table[idx].qname, qname);
		//sore_table.score에 하니씩 문제이름 저장
		score_table[idx++].score = atof(score);
	}

	//열었던 파일 닫아줌
	fclose(fp);
}

//score_table 제작
void make_scoreTable(char *ansDir)
{
	int type, num;
	double score, bscore, pscore;
	struct dirent *dirp, *c_dirp;
	DIR *dp, *c_dp;
	char tmp[BUFLEN];
	int idx = 0;
	int i;

	//어떤 형식으로 score_table을 만들껀지 결정
	//1이면 한번에 형식 지정, 2면 문제하나씩 지정
	num = get_create_type();

	//1이면 빈칸문제와 프로그램 문제 점수 입력
	if(num == 1)
	{
		printf("Input value of blank question : ");
		scanf("%lf", &bscore);
		printf("Input value of program question : ");
		scanf("%lf", &pscore);
	}

	//정답 디렉토리를 열기를 시도, 실패하면 에러처리
	if((dp = opendir(ansDir)) == NULL){
		fprintf(stderr, "open dir error for %s\n", ansDir);
		return;
	}	

	//dp를 통해 디렉토리에서 파일들을 하나씩 읽어들임
	while((dirp = readdir(dp)) != NULL)
	{
		//. 이나 .. 일 경우 무시한
		if(!strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, ".."))
			continue;

		//tmp 배열에다가 "정답 디렉토리 경로/읽은 파일 이름"을 저장
		sprintf(tmp, "%s/%s", ansDir, dirp->d_name);
		//score_table.qname에 문제이름을 추가함
		strcpy(score_table[idx++].qname, dirp->d_name);
	}

	closedir(dp);
	//score_table을 정렬해준다
	sort_scoreTable(idx);

	for(i = 0; i < idx; i++)
	{
		//파일의 타입을 읽어옴
		type = get_file_type(score_table[i].qname);
		
		//한번에 결정하는 경우 였을경우
		if(num == 1)
		{
			//.txt 파일과 .c 파일 각각에 점수 배점
			if(type == TEXTFILE)
				score = bscore;
			else if(type == CFILE)
				score = pscore;
		}
		//일일이 점수를 배점하는 경우
		else if(num == 2)
		{
			//입력받아서 배점
			printf("Input of %s: ", score_table[i].qname);
			scanf("%lf", &score);
		}
		//scoreTable.score부분에 점수배점 등록
		score_table[i].score = score;
	}
}
//score_table 작성
void write_scoreTable(char *filename)
{
	int fd;
	char tmp[BUFLEN];
	int i;
	//score_table의 크기 계산
	int num = sizeof(score_table) / sizeof(score_table[0]);

	//filname의 이름으로 파일을 생성 실패시 에러처리
	if((fd = creat(filename, 0666)) < 0){
		fprintf(stderr, "creat error for %s\n", filename);
		return;
	}

	for(i = 0; i < num; i++)
	{
		//0점을 만나면 break
		if(score_table[i].score == 0)
			break;

		//문제이름과 점수 배점을 합쳐서 tmp에 저장
		sprintf(tmp, "%s,%.2f\n", score_table[i].qname, score_table[i].score);
		//fd를 통해 tmp 배열을 작성
		write(fd, tmp, strlen(tmp));
	}

	close(fd);
}

//학번테이블 세팅
void set_idTable(char *stuDir)
{
	struct stat statbuf;
	struct dirent *dirp;
	DIR *dp;
	char tmp[BUFLEN];
	int num = 0;

	//학생 디렉토리를 열기를 시도 실패시 에러처리
	if((dp = opendir(stuDir)) == NULL){
		fprintf(stderr, "opendir error for %s\n", stuDir);
		exit(1);
	}

	//학생디렉토리를 열었을때
	while((dirp = readdir(dp)) != NULL){
		//'.', '..'은 무시
		if(!strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, ".."))
			continue;

		//학번 디렉토리의 경로를 저장
		sprintf(tmp, "%s/%s", stuDir, dirp->d_name);
		//디렉토리 정보 추출
		stat(tmp, &statbuf);

		//디렉토리이면 id_table의 학번추가
		if(S_ISDIR(statbuf.st_mode))
			strcpy(id_table[num++], dirp->d_name);
		//그렇지 않으면 continue
		else
			continue;
	}
	//학번 테이블 오름차순 정렬
	sort_idTable(num);
}

//학번 테이블 정렬
void sort_idTable(int size)
{
	int i, j;
	char tmp[10];

	//버블 정렬을 이용
	//학번을 오름차순으로 정렬
	for(i = 0; i < size - 1; i++){
		for(j = 0; j < size - 1 -i; j++){
			if(strcmp(id_table[j], id_table[j+1]) > 0){
				strcpy(tmp, id_table[j]);
				strcpy(id_table[j], id_table[j+1]);
				strcpy(id_table[j+1], tmp);
			}
		}
	}
}

//score_table 정렬시켜줌
void sort_scoreTable(int size)
{
	int i, j;
	struct ssu_scoreTable tmp;
	int num1_1, num1_2;
	int num2_1, num2_2;

	//버블정렬을 이용
	for(i = 0; i < size - 1; i++){
		for(j = 0; j < size - 1 - i; j++){

			//비교할 두개의 이름에서 번호를 추출함
			//1-2.txt의 경우 num1_1에 1 num1_2에 2가 들어감
			get_qname_number(score_table[j].qname, &num1_1, &num1_2);
			get_qname_number(score_table[j+1].qname, &num2_1, &num2_2);

			//앞번호가 크면 뒤로, 앞번호가 같으면 뒷번호를비교해서 뒷번호가 큰걸 뒤로 보냄
			if((num1_1 > num2_1) || ((num1_1 == num2_1) && (num1_2 > num2_2))){

				//서로 교환
				memcpy(&tmp, &score_table[j], sizeof(score_table[0]));
				memcpy(&score_table[j], &score_table[j+1], sizeof(score_table[0]));
				memcpy(&score_table[j+1], &tmp, sizeof(score_table[0]));
			}
		}
	}
}

//문제해서 번호를 추출
void get_qname_number(char *qname, int *num1, int *num2)
{
	char *p;
	char dup[FILELEN];

	//dup에다가 문제이름을 복사해줌
	strncpy(dup, qname, strlen(qname));
	//num1에다가 "-."전까지 끝어서 저장해줌
	*num1 = atoi(strtok(dup, "-."));
	
	//p에다가 "-."로 다시한번 토큰화
	p = strtok(NULL, "-.");
	//NULL이면 0
	if(p == NULL)
		*num2 = 0;
	//존재하면 정수로 변환후 저장
	else
		*num2 = atoi(p);
}


//어떤 형태로 score_table을 만들지 판별
int get_create_type()
{
	int num;

	//1이나 2를 입력받을때까지 반복
	while(1)
	{
		printf("score_table.csv file doesn't exist\n");
		printf("1. input blank question and program question's score. ex) 0.5 1\n");
		printf("2. input all question's score. ex) Input value of 1-1: 0.1\n");
		printf("select type >> ");
		scanf("%d", &num);

		//1이나 2가 아니면 다시
		if(num != 1 && num != 2)
			printf("not correct number!\n");
		else
			break;
	}
	//1이나 2를 리턴
	return num;
}

//학생들 채점함수
void score_students()
{
	double score = 0;
	int num;
	int fd;
	char tmp[BUFLEN];
	//학번테이블의 크기 계산
	int size = sizeof(id_table) / sizeof(id_table[0]);

	//score.csv파일을 생성하고 에러처리
	if((fd = creat("score.csv", 0666)) < 0){
		fprintf(stderr, "creat error for score.csv");
		return;
	}
	//score.csv의 첫줄을 작성하는 함수
	write_first_row(fd);


	for(num = 0; num < size; num++)
	{
		//학번테이블을 읽었을 때 빈거면 break
		if(!strcmp(id_table[num], ""))
			break;

		//그렇지 않다면 학번, 을 fd를 통해 작성
		sprintf(tmp, "%s,", id_table[num]);
		write(fd, tmp, strlen(tmp)); 

		//학생의 점수를 구한다
		score += score_student(fd, id_table[num]);
	}
	//전체평균 출력
	printf("Total average : %.2f\n", score / num);

	close(fd);
}
//학생 개인 채점 함수
double score_student(int fd, char *id)
{
	int type;
	double result;
	double score = 0;
	int i;
	char tmp[BUFLEN];
	//score_table의 크기를 계산
	int size = sizeof(score_table) / sizeof(score_table[0]);

	for(i = 0; i < size ; i++)
	{
		if(score_table[i].score == 0)
			break;

		//경로 "학생디렉토리/학번/문제이름"을 tmp 에 저장
		sprintf(tmp, "%s/%s/%s", stuDir, id, score_table[i].qname);

		//파일에 접근 가능한지 판별 없으면 틀린거임
		if(access(tmp, F_OK) < 0)
			result = false;
		//접근 가능할때
		else
		{
			//파일 타입을 가져온다, .txt나 .c이외의 것을 경우 continue
			if((type = get_file_type(score_table[i].qname)) < 0)
				continue;
			
			//.txt파일이면 빈칸문제 채점
			if(type == TEXTFILE)
				result = score_blank(id, score_table[i].qname);
			//.c파일이면 프로그램 채점
			else if(type == CFILE)
				result = score_program(id, score_table[i].qname);
		}

		//결과가 false일때 ( 파일이 없거나, 0점이거나 )
		if(result == false)
			write(fd, "0,", 2);
		//결과가 뭐든지 간에 존재할때
		else{
			if(result == true){
				//다 맞았으면 score에 추가시켜줌
				score += score_table[i].score;
				sprintf(tmp, "%.2f,", score_table[i].score);
			}
			else if(result < 0){
				//warning이 있을 경우 점수를 감점 
				score = score + score_table[i].score + result;
				sprintf(tmp, "%.2f,", score_table[i].score + result);
			}
			write(fd, tmp, strlen(tmp));
		}
	}
	//학번과 채점 결과 출력
	printf("%s is finished. score : %.2f\n", id, score); 

	sprintf(tmp, "%.2f\n", score);
	//결과파일에 점수 합계 작성
	write(fd, tmp, strlen(tmp));

	return score;
}

//score.csv의 첫줄
void write_first_row(int fd)
{
	int i;
	//임시버퍼
	char tmp[BUFLEN];
	//scoretable의 크기 구하기
	int size = sizeof(score_table) / sizeof(score_table[0]);

	//맨처음은 공백
	write(fd, ",", 1);

	for(i = 0; i < size; i++){
		//점수 배점이 없는걸 만나면 break
		if(score_table[i].score == 0)
			break;
		//문제이름을 tmp에 저장한후
		sprintf(tmp, "%s,", score_table[i].qname);
		//fd를 통해 파일에 작성
		write(fd, tmp, strlen(tmp));
	}
	//마지막에sum 출력
	write(fd, "sum\n", 4);
}
//파일에서 :단위로 문자열 읽어줌
char *get_answer(int fd, char *result)
{
	char c;
	int idx = 0;

	//버퍼 초기화
	memset(result, 0, BUFLEN);
	//한글자 씩 읽는데 :를 기준으로 종료
	while(read(fd, &c, 1) > 0)
	{
		if(c == ':')
			break;
		
		//한글자씩저장
		result[idx++] = c;
	}
	//끝에다가 개행이면 널문자로 대체
	if(result[strlen(result) - 1] == '\n')
		result[strlen(result) - 1] = '\0';

	return result;
}
//빈칸문제 채점
int score_blank(char *id, char *filename)
{
	char tokens[TOKEN_CNT][MINLEN];
	node *std_root = NULL, *ans_root = NULL;
	int idx, start;
	char tmp[BUFLEN];
	char s_answer[BUFLEN], a_answer[BUFLEN];
	char qname[FILELEN];
	int fd_std, fd_ans;
	int result = true;
	int has_semicolon = false;

	//qname 초기화
	//qname에 확장자를 제외한 이름만 저장
	memset(qname, 0, sizeof(qname));
	memcpy(qname, filename, strlen(filename) - strlen(strrchr(filename, '.')));

	//tmp의 "학생디렉토리/학번/문제" 저장
	sprintf(tmp, "%s/%s/%s", stuDir, id, filename);
	//문제파일 열기
	fd_std = open(tmp, O_RDONLY);
	//파일에서 :단위로 적은답을 가져와서 s_answer에 저장
	strcpy(s_answer, get_answer(fd_std, s_answer));

	//적은 답이 없으면 return false
	if(!strcmp(s_answer, "")){
		close(fd_std);
		return false;
	}
	//괄호의 갯수가 맞지 않으면 return false
	if(!check_brackets(s_answer)){
		close(fd_std);
		return false;
	}
	//가져온 답의 앞뒤 공백을 없애준다
	strcpy(s_answer, ltrim(rtrim(s_answer)));

	//맨뒤가 세미콜론이면 세미콜론이 있다는 flag를 세워주고 널문자로 대체
	if(s_answer[strlen(s_answer) - 1] == ';'){
		has_semicolon = true;
		s_answer[strlen(s_answer) - 1] = '\0';
	}
	//답을 토큰화 시켰는데 문제가 있으면 틀림
	if(!make_tokens(s_answer, tokens)){
		close(fd_std);
		return false;
	}

	idx = 0;
	//토큰화 시킨걸로 트리를 제작함
	std_root = make_tree(std_root, tokens, &idx, 0);

	//sprintf(tmp, "%s/%s/%s", ansDir, qname, filename);
	//정답디렉토리/문제 저장함
	sprintf(tmp, "%s/%s", ansDir, filename);
	//문제파일을 열어줌
	fd_ans = open(tmp, O_RDONLY);

	while(1)
	{
		ans_root = NULL;
		result = true;

		//토큰이 들어있던 배열 초기화 후 재사용
		for(idx = 0; idx < TOKEN_CNT; idx++)
			memset(tokens[idx], 0, sizeof(tokens[idx]));

		//정답파일에서 : 단위로 답을 가져와 버퍼에 저장
		strcpy(a_answer, get_answer(fd_ans, a_answer));

		//공백이면 break
		if(!strcmp(a_answer, ""))
			break;
		//답의 앞뒤 공백을 제거한다
		strcpy(a_answer, ltrim(rtrim(a_answer)));

		//학생답에 세미콜론 없었는데 정답에는 있으면 continue
		if(has_semicolon == false){
			if(a_answer[strlen(a_answer) -1] == ';')
				continue;
		}
		//학생답에 세미콜론이 있었는데
		else if(has_semicolon == true)
		{
			//정답에도 있으면 continue
			if(a_answer[strlen(a_answer) - 1] != ';')
				continue;
			//그렇지 않으면 맨 마지막에 널문자로 대체
			else
				a_answer[strlen(a_answer) - 1] = '\0';
		}
		//정답을토큰화 시킨다
		if(!make_tokens(a_answer, tokens))
			continue;

		idx = 0;
		//토큰화 시킨걸로 트리를 제작
		ans_root = make_tree(ans_root, tokens, &idx, 0);
		//트리 비교
		compare_tree(std_root, ans_root, &result);

		//결과가 true면 트리들 다 free 시켜줌
		if(result == true){
			close(fd_std);
			close(fd_ans);

			if(std_root != NULL)
				free_node(std_root);
			if(ans_root != NULL)
				free_node(ans_root);
			return true;
		}
	}
	
	//트리들 다 free시켜줌
	close(fd_std);
	close(fd_ans);

	if(std_root != NULL)
		free_node(std_root);
	if(ans_root != NULL)
		free_node(ans_root);
	//틀림
	return false;
}

//프로그램 문제 채점
double score_program(char *id, char *filename)
{
	double compile;
	int result;

	//학번과 파일이름을 넘겨주면서 컴파일을 시켜봄
	compile = compile_program(id, filename);

	//ERROR이거나 false가뜨면 실패
	if(compile == ERROR || compile == false)
		return false;
	
	//학번과 파일이름을 넘겨주면서 실행시킴
	result = execute_program(id, filename);

	//결과가 다르거나 실행에 문제가 있을시 return false
	if(!result)
		return false;

	//컴파일했을떄 error는 없고 warning이 있을 때 그 점수 리턴
	if(compile < 0)
		return compile;

	//아무 문제없으면 true 리턴
	return true;
}

//스레드파일인지 확인
int is_thread(char *qname)
{
	int i;
	//threadFiles 배열의 크기를 구함
	int size = sizeof(threadFiles) / sizeof(threadFiles[0]);

	//같은게 있으면 return true
	for(i = 0; i < size; i++){
		if(!strcmp(threadFiles[i], qname))
			return true;
	}
	//없으면 false
	return false;
}

//프로그램 컴파일시킴
double compile_program(char *id, char *filename)
{
	int fd;
	char tmp_f[BUFLEN], tmp_e[BUFLEN];
	char command[BUFLEN];
	char qname[FILELEN];
	int isthread;
	off_t size;
	double result;

	//qname버퍼 초기화
	//qname에다가 확장자를 제외한 이름만 저장
	memset(qname, 0, sizeof(qname));
	memcpy(qname, filename, strlen(filename) - strlen(strrchr(filename, '.')));
	
	//쓰레드파일인지 확인
	isthread = is_thread(qname);

	//sprintf(tmp_f, "%s/%s/%s", ansDir, qname, filename);
	//"정답디렉토리/문제이름"을 경로 저장
	sprintf(tmp_f, "%s/%s", ansDir, filename);
	//sprintf(tmp_e, "%s/%s/%s.exe", ansDir, qname, qname);
	//실행파일의 경로를 저장
	sprintf(tmp_e, "%s/%s.exe", ansDir, qname);

	if(tOption && isthread)
		sprintf(command, "gcc -o %s %s -lpthread", tmp_e, tmp_f);
	else
		sprintf(command, "gcc -o %s %s", tmp_e, tmp_f);

	//sprintf(tmp_e, "%s/%s/%s_error.txt", ansDir, qname, qname);
	//컴파일 에러나 warning이 떳을때 문구들을 저장할 파일의 경로를 저장
	sprintf(tmp_e, "%s/%s_error.txt", ansDir, qname);
	//컴파일 에러확인용 파일 생성
	fd = creat(tmp_e, 0666);

	//STDERR를 fd로 재설정하고 명령어 실행
	redirection(command, fd, STDERR);
	//파일포인터를 맨끝으로 보냄
	size = lseek(fd, 0, SEEK_END);
	close(fd);
	//에러파일 삭제
	unlink(tmp_e);

	//파일의 크기가 0보다 크면 return false
	if(size > 0)
		return false;

	//"학생디렉토리/학번/문제이름"을 저장
	sprintf(tmp_f, "%s/%s/%s", stuDir, id, filename);
	//실행파일 경로를 배열에 저장
	sprintf(tmp_e, "%s/%s/%s.stdexe", stuDir, id, qname);

	if(tOption && isthread)
		sprintf(command, "gcc -o %s %s -lpthread", tmp_e, tmp_f);
	else
		sprintf(command, "gcc -o %s %s", tmp_e, tmp_f);

	//에러가 났을때 출력할 에러파일 경로 저장
	sprintf(tmp_f, "%s/%s/%s_error.txt", stuDir, id, qname);
	//에러출력할 파일 생성
	fd = creat(tmp_f, 0666);

	//STDERR를 fd로 재지정하고 명령어 실행
	redirection(command, fd, STDERR);
	//파일포인터를 파일의 끝으로 이동
	size = lseek(fd, 0, SEEK_END);
	close(fd);

	//에러나 warning이 있었을 경우
	if(size > 0){
		//e 옵션이 존재하는경우
		if(eOption)
		{
			//에러디렉토리/학번 저장
			sprintf(tmp_e, "%s/%s", errorDir, id);
			//만약 그 디렉토리가 존재하지 않으면 디렉토리생성
			if(access(tmp_e, F_OK) < 0)
				mkdir(tmp_e, 0755);
			//에러를 출력할 파일 경로저장
			sprintf(tmp_e, "%s/%s/%s_error.txt", errorDir, id, qname);
			//디렉토리에서 파일로 tmp_f 변경
			rename(tmp_f, tmp_e);

			//error나 warning의 갯수를 셈
			result = check_error_warning(tmp_e);
		}
		//e 옵션 없을경우
		else{ 
			//error나 warning의 갯수를 세고
			result = check_error_warning(tmp_f);
			//error출력했던 파일 unlink
			unlink(tmp_f);
		}
		//결과 값 리턴
		return result;
	}
	//error나 warning출력했던 파일 unlink후 결과 리턴
	unlink(tmp_f);
	return true;
}

//컴파일 결과중 error 와 warning 체크
double check_error_warning(char *filename)
{
	FILE *fp;
	char tmp[BUFLEN];
	double warning = 0;

	//파일이름을 넘겨 받아서 열기를 시도 열기 실패시 에러처리
	if((fp = fopen(filename, "r")) == NULL){
		fprintf(stderr, "fopen error for %s\n", filename);
		return false;
	}

	//파일의 내용중 error:가 있으면 return ERROR warning:이 있으면 갯수를 셈
	while(fscanf(fp, "%s", tmp) > 0){
		if(!strcmp(tmp, "error:"))
			return ERROR;
		else if(!strcmp(tmp, "warning:"))
			warning += WARNING;
	}
	//에러는 안났고 warning만 있는경우 점수 합산해서 넘겨줌
	return warning;
}

//프로그램 실행
int execute_program(char *id, char *filename)
{
	char std_fname[BUFLEN], ans_fname[BUFLEN];
	char tmp[BUFLEN];
	char qname[FILELEN];
	time_t start, end;
	pid_t pid;
	int fd;

	//qname 초기화
	//qname에다가 확장를 뺸 이름만 저장
	memset(qname, 0, sizeof(qname));
	memcpy(qname, filename, strlen(filename) - strlen(strrchr(filename, '.')));

	//sprintf(ans_fname, "%s/%s/%s.stdout", ansDir, qname, qname);
	//결과 저장 파일 경로를 ans_fname에 저장
	sprintf(ans_fname, "%s/%s.stdout", ansDir, qname);
	//결과를 저장할 파일 생성
	fd = creat(ans_fname, 0666);

	//sprintf(tmp, "%s/%s/%s.exe", ansDir, qname, qname);
	//실행 파일 경로를 tmp에 저장
	sprintf(tmp, "%s/%s.exe", ansDir, qname);

	//STDOUT과 STDERR를 fd로 변경시킨후 복구
	int out = dup(STDOUT);
	int err = dup(STDERR);
	//STDOUT,STDERR를 fd로 바꿈
	dup2(fd, STDOUT);  dup2(fd, STDERR);
	system(tmp);
	//원래대로 복구
	dup2(out, STDOUT); dup2(err, STDERR);
	close(out); close(err);
	//redirection(tmp, fd, STDOUT);
	close(fd);

	//학생파일의 결과를 저장할 파일의 경로를 std_fname에 저장
	sprintf(std_fname, "%s/%s/%s.stdout", stuDir, id, qname);
	//결과를 저장할 파일 생성
	fd = creat(std_fname, 0666);

	//실행파일의 경로를 tmp에 저장하는데 백그라운드로서 실행하기 위해 &를 붙혀줌
	sprintf(tmp, "%s/%s/%s.stdexe &", stuDir, id, qname);

	//시작시간 측정
	start = time(NULL);
	//STDOUT과 STDERR를 fd를 바꿔주고 복구
	out = dup(STDOUT);
	err = dup(STDERR);
	//STDOUT STDERR를 fd로 변경
	dup2(fd, STDOUT); dup2(fd, STDERR);
	system(tmp);
	//원래대로 복구
	dup2(out, STDOUT); dup2(err, STDERR);
	close(out); close(err);
	//redirection(tmp, fd, STDOUT);

	//실행파일의 이름을 tmp에 저장
	sprintf(tmp, "%s.stdexe", qname);
	//백그라운드에 존재할경우 pid를 받음
	while((pid = inBackground(tmp)) > 0){
		end = time(NULL);

		//시간을 측정해서 5초보다 길어지면 강제롤 kill신호를 보내서 죽임
		if(difftime(end, start) > OVER){
			kill(pid, SIGKILL);
			close(fd);
			//5초이상 걸렸기 떄문에 틀림
			return false;
		}
	}

	close(fd);
	//결과를 비교
	return compare_resultfile(std_fname, ans_fname);
}

//백그라운드 실행
pid_t inBackground(char *name)
{
	pid_t pid;
	char command[64];
	char tmp[64];
	int fd;
	off_t size;
	
	//임시 버퍼 초기화
	memset(tmp, 0, sizeof(tmp));
	//현재 실행중인 목록을 저장할 파일 생성
	fd = open("background.txt", O_RDWR | O_CREAT | O_TRUNC, 0666);

	//ps -> 현재 실행 중인 프로세스를 출력을 표준입력으로 하여 grep %s를 실행하는 명령어를 구성
	sprintf(command, "ps | grep %s", name);
	//명령어를 실행
	redirection(command, fd, STDOUT);

	//파일 포인터를 처음으로 보내고
	lseek(fd, 0, SEEK_SET);
	//tmp만큼 읽는다
	read(fd, tmp, sizeof(tmp));

	//비어있으면 "background.txt"파일 unlink
	if(!strcmp(tmp, "")){
		unlink("background.txt");
		close(fd);
		return 0;
	}
	//뭔가 있으면 " "을 토큰으로 pid번호를 저장
	pid = atoi(strtok(tmp, " "));
	close(fd);
	//그 후 unlink
	unlink("background.txt");
	//pid를 넘겨줌
	return pid;
}

//결과 파일을 비교한다
//프로그램 문제의 경우
int compare_resultfile(char *file1, char *file2)
{
	int fd1, fd2;
	char c1, c2;
	int len1, len2;

	//파일들의 경로를 받고 읽기모드로 열어준다
	fd1 = open(file1, O_RDONLY);
	fd2 = open(file2, O_RDONLY);

	while(1)
	{
		//fd1에서 문자하나를 읽는다
		while((len1 = read(fd1, &c1, 1)) > 0){
			//공백이면 continue
			if(c1 == ' ') 
				continue;
			//그렇지 않으면 스탑
			else 
				break;
		}
		//fd2에서 문자하나를 읽는다
		while((len2 = read(fd2, &c2, 1)) > 0){
			//공백은 무시
			if(c2 == ' ') 
				continue;
			//그렇지 않으면 스탑
			else 
				break;
		}
		//둘다 읽은 문자가 널 문자면 스탑
		if(len1 == 0 && len2 == 0)
			break;

		//가져온 문자 2개를 둘다 소문자로 변경
		to_lower_case(&c1);
		to_lower_case(&c2);

		//서로다르면 틀린거임
		if(c1 != c2){
			close(fd1);
			close(fd2);
			return false;
		}
	}
	//다 같았으면 정답
	close(fd1);
	close(fd2);
	return true;
}

//파일디스크립터 변경
void redirection(char *command, int new, int old)
{
	//기존 파일디스크립터를 저장하는 변수
	int saved;

	//old의 파일디스크립터를 saved에 저장
	saved = dup(old);
	//old를 new로 복사
	dup2(new, old);

	//명령어 실행
	system(command);

	//old에다가 saved를 저장
	//결국 기존껄로 돌아옴
	dup2(saved, old);
	close(saved);
}

//파일을 타입을 읽어드림
int get_file_type(char *filename)
{
	//filname 에서 '.'을 찾아서 위치 리턴받음
	char *extension = strrchr(filename, '.');

	//빈칸문제인경우
	if(!strcmp(extension, ".txt"))
		return TEXTFILE;
	//프로그램문제인경우
	else if (!strcmp(extension, ".c"))
		return CFILE;
	//예외
	else
		return -1;
}

//디렉토리 삭제 함수
void rmdirs(const char *path)
{
	struct dirent *dirp;
	struct stat statbuf;
	DIR *dp;
	char tmp[BUFLEN];

	//열기 실패시 리턴
	if((dp = opendir(path)) == NULL)
		return;

	//디렉토리에서 하나씩 읽음
	while((dirp = readdir(dp)) != NULL)
	{
		//. , .. 은 무시한다
		if(!strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, ".."))
			continue;

		//"경로/파일이름" 을 tmp에 저장
		sprintf(tmp, "%s/%s", path, dirp->d_name);

		//stat을 통해 정보를 얻어 올 수 없으면 무시함
		if(lstat(tmp, &statbuf) == -1)
			continue;

		//만약 그 파일이 디렉토리라면 그 디렉토리안에 있는 것들을 삭제하기 위해 rmdirs 실행
		if(S_ISDIR(statbuf.st_mode))
			rmdirs(tmp);
		//디렉토리가 아니면 그냥 unlink
		else
			unlink(tmp);
	}

	closedir(dp);
	//마지막으로 이 디렉토리도 삭제
	rmdir(path);
}

//영어소문자 일경우 대문자로 변경
void to_lower_case(char *c)
{
	//영어 소문자 'a'가 97이기 때문이 'A' = 65 에서 32를 더하면 소문자가 됨
	if(*c >= 'A' && *c <= 'Z')
		*c = *c + 32;
}

//프로그램사용방법 출력
void print_usage()
{
	printf("Usage : ssu_score <STD_DIR> <ANS_DIR> [OPTION]\n");
	printf("Option : \n");
	//m 옵션은 특정 문제의 배점 수정
	printf(" -m                modify question's score\n");
	//e 옵션은 디렉토리 이름을 건네주면 error정보를 학번별로 저장해줌
	printf(" -e <DIRNAME>      print error on 'DIRNAME/ID/qname_error.txt' file \n");
	//-t 옵션은 문제에 lpthread기능 추가
	printf(" -t <QNAMES>       compile QNAME.C with -lpthread option\n");
	//특정 학번의 틀린 문제 정보 출력
	printf(" -i <IDS>          print ID's wrong answer\n");
	//사용법 출력
	printf(" -h                print usage\n");
}

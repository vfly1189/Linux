#ifndef MAIN_H_
#define MAIN_H_

#ifndef true
	#define true 1
#endif
#ifndef false
	#define false 0
#endif
#ifndef STDOUT
	#define STDOUT 1
#endif
#ifndef STDERR
	#define STDERR 2
#endif
#ifndef TEXTFILE
	#define TEXTFILE 3
#endif
#ifndef CFILE
	#define CFILE 4
#endif
#ifndef OVER
	#define OVER 5
#endif
#ifndef WARNING
	#define WARNING -0.1
#endif
#ifndef ERROR
	#define ERROR 0
#endif

#define FILELEN 64
#define BUFLEN 1024
#define SNUM 100
#define QNUM 100
#define ARGNUM 5

//문제별 배점을 저장할 구조체
struct ssu_scoreTable{
	char qname[FILELEN];
	double score;
};

//채점프로그램 시작 함수
void ssu_score(int argc, char *argv[]);
//옵션들 체크
int check_option(int argc, char *argv[]);
//사용법 출력
void print_usage();

//학생들 채점하기
void score_students();
//학생 개인별 채점
double score_student(int fd, char *id);
//score.csv파일의 첫번째 줄 작성
void write_first_row(int fd);

//파일로부터 정답을 불러옴
char *get_answer(int fd, char *result);
//빈칸문제 채점함수
int score_blank(char *id, char *filename);
//프로그램문제 채점함수
double score_program(char *id, char *filename);
//프로그램문제 파일 컴파일 함수
double compile_program(char *id, char *filename);
//프로그램문제 파일 실행 함수
int execute_program(char *id, char *filname);
//프로그램문제 파일 백그라운드로 실행시키는함수
pid_t inBackground(char *name);
//컴파일에 에러나 warning이 있는지 확인
double check_error_warning(char *filename);
//결과파일 비교
int compare_resultfile(char *file1, char *file2);

//-m옵션 수행
void do_mOption(char *path);
//-i옵션 수행
int do_iOption(char (*ids)[FILELEN]);
//-i옵션으로 받은 학번이 존재하는지 확인
int is_exist(char (*src)[FILELEN], char *target);

//스레드파일인지 확인
int is_thread(char *qname);
//출력 재지정 및 명령어 실행
void redirection(char *command, int newfd, int oldfd);
//파일이 txt인지 c인이 확인
int get_file_type(char *filename);
//디렉토리삭제함수
void rmdirs(const char *path);
//대문자를 소문자로 변경
void to_lower_case(char *c);

//점수배점 테이블 세팅
void set_scoreTable(char *ansDir);
//점수배점 테이블이 존재하면 읽어드림
void read_scoreTable(char *path);
//점수배점 테이블을 만드는 함수
void make_scoreTable(char *ansDir);
//score_table.csv파일에 데이터 작성
void write_scoreTable(char *filename);
//학번 저장테이블 세팅
void set_idTable(char *stuDir);
//score_table.csv를 어떤 타입으로 만들건지 물어보는 함수
int get_create_type();

//학번 테이블 정렬
void sort_idTable(int size);
//score_table 정렬
void sort_scoreTable(int size);
//문제 번호 추출함수
void get_qname_number(char *qname, int *num1, int *num2);

#endif

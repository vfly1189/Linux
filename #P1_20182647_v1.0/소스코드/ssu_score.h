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

//������ ������ ������ ����ü
struct ssu_scoreTable{
	char qname[FILELEN];
	double score;
};

//ä�����α׷� ���� �Լ�
void ssu_score(int argc, char *argv[]);
//�ɼǵ� üũ
int check_option(int argc, char *argv[]);
//���� ���
void print_usage();

//�л��� ä���ϱ�
void score_students();
//�л� ���κ� ä��
double score_student(int fd, char *id);
//score.csv������ ù��° �� �ۼ�
void write_first_row(int fd);

//���Ϸκ��� ������ �ҷ���
char *get_answer(int fd, char *result);
//��ĭ���� ä���Լ�
int score_blank(char *id, char *filename);
//���α׷����� ä���Լ�
double score_program(char *id, char *filename);
//���α׷����� ���� ������ �Լ�
double compile_program(char *id, char *filename);
//���α׷����� ���� ���� �Լ�
int execute_program(char *id, char *filname);
//���α׷����� ���� ��׶���� �����Ű���Լ�
pid_t inBackground(char *name);
//�����Ͽ� ������ warning�� �ִ��� Ȯ��
double check_error_warning(char *filename);
//������� ��
int compare_resultfile(char *file1, char *file2);

//-m�ɼ� ����
void do_mOption(char *path);
//-i�ɼ� ����
int do_iOption(char (*ids)[FILELEN]);
//-i�ɼ����� ���� �й��� �����ϴ��� Ȯ��
int is_exist(char (*src)[FILELEN], char *target);

//�������������� Ȯ��
int is_thread(char *qname);
//��� ������ �� ��ɾ� ����
void redirection(char *command, int newfd, int oldfd);
//������ txt���� c���� Ȯ��
int get_file_type(char *filename);
//���丮�����Լ�
void rmdirs(const char *path);
//�빮�ڸ� �ҹ��ڷ� ����
void to_lower_case(char *c);

//�������� ���̺� ����
void set_scoreTable(char *ansDir);
//�������� ���̺��� �����ϸ� �о�帲
void read_scoreTable(char *path);
//�������� ���̺��� ����� �Լ�
void make_scoreTable(char *ansDir);
//score_table.csv���Ͽ� ������ �ۼ�
void write_scoreTable(char *filename);
//�й� �������̺� ����
void set_idTable(char *stuDir);
//score_table.csv�� � Ÿ������ ������� ����� �Լ�
int get_create_type();

//�й� ���̺� ����
void sort_idTable(int size);
//score_table ����
void sort_scoreTable(int size);
//���� ��ȣ �����Լ�
void get_qname_number(char *qname, int *num1, int *num2);

#endif

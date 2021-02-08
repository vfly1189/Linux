#ifndef BLANK_H_
#define BLANK_H_

#ifndef true
	#define true 1
#endif
#ifndef false
	#define false 0
#endif
#ifndef BUFLEN
	#define BUFLEN 1024
#endif

#define OPERATOR_CNT 24
#define DATATYPE_SIZE 35
#define MINLEN 64
#define TOKEN_CNT 50

//Ʈ���� ������ ��� ����ü
typedef struct node{
	int parentheses;
	char *name;
	struct node *parent;
	struct node *child_head;
	struct node *prev;
	struct node *next;
}node;

//������ �켱���� ����
typedef struct operator_precedence{
	char *operator;
	int precedence;
}operator_precedence;

//Ʈ�� �� �Լ�
void compare_tree(node *root1,  node *root2, int *result);
//��ū���� �̿��� Ʈ�� �����Լ�
node *make_tree(node *root, char (*tokens)[MINLEN], int *idx, int parentheses);
//������ �ٲ� �� �ִ� �����ڿ� ���� �ڽ� ���� �Լ�
node *change_sibling(node *parent);
//���ο� ��带 �����ϴ� �Լ�
node *create_node(char *name, int parentheses);
//���ڷ� ���� ������ �켱������ ���Ͻ����ִ� �Լ�
int get_precedence(char *op);
//���������� Ȯ���ϴ� �Լ�
int is_operator(char *op);
void print(node *cur);
//Ʈ������ �����ڸ� ã���Լ�
node *get_operator(node *cur);
//��Ʈ�� ��ȯ�ϴ� �Լ�
node *get_root(node *cur);
//�켱������ ���� ��带 ����
node *get_high_precedence_node(node *cur, node *new);
//���� �ֻ��� �켱���� ��带 ����
node *get_most_high_precedence_node(node *cur, node *new);
//���ο� ��带 ���Խ�Ű�� �Լ�
node *insert_node(node *old, node *new);
//���� ������ �ڽ��� ã�� �Լ�
node *get_last_child(node *cur);
//�޸������Լ�
void free_node(node *cur);
//�ڽ��� ������ ���Ͻ�Ű�� �Լ�
int get_sibling_cnt(node *cur);

//�Է¹��� str�� ������Ģ�� ���� ��ūȭ ��Ų��
int make_tokens(char *str, char tokens[TOKEN_CNT][MINLEN]);
//gcc���� �ڷ������� Ȯ��
int is_typeStatement(char* str);
//Ÿ�������� ã��
int find_typeSpecifier(char tokens[TOKEN_CNT][MINLEN]);
//Ÿ�������� ã�� 2
int find_typeSpecifier2(char tokens[TOKEN_CNT][MINLEN]);
//�������� Ȯ���ϴ� �Լ�
int is_character(char c);
//*�θ� �̷���� �ִ��� Ȯ���ϴ� �Լ�
int all_star(char *str);
//�� �������� Ȯ��
int all_character(char *str);
//struct�� unsigned ��� ó��ȭ ������ ��ū�� �ʱ�ȭ ��Ŵ
int reset_tokens(int start, char tokens[TOKEN_CNT][MINLEN]);
//��ū�� ����� �迭 �ʱ�ȭ
void clear_tokens(char tokens[TOKEN_CNT][MINLEN]);
int get_token_cnt(char tokens[TOKEN_CNT][MINLEN]);
//�������� ������ ���Ž�Ű�� �Լ�
char *rtrim(char *_str);
//������ ������ ���Ž�Ű�� �Լ�
char *ltrim(char *_str);
//���ڿ� ���� ���� ����
void remove_space(char *str);
//��ȣ�� ������ �´��� ���ִ� �Լ�
int check_brackets(char *str);
//�߰����� ������� ������Ű�� �Լ�
char* remove_extraspace(char *str);

#endif

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

//트리를 구성할 노드 구조체
typedef struct node{
	int parentheses;
	char *name;
	struct node *parent;
	struct node *child_head;
	struct node *prev;
	struct node *next;
}node;

//연산자 우선순위 저장
typedef struct operator_precedence{
	char *operator;
	int precedence;
}operator_precedence;

//트리 비교 함수
void compare_tree(node *root1,  node *root2, int *result);
//토큰들을 이용해 트리 제작함수
node *make_tree(node *root, char (*tokens)[MINLEN], int *idx, int parentheses);
//순서가 바뀔 수 있는 연산자에 대한 자식 교한 함수
node *change_sibling(node *parent);
//새로운 노드를 제작하는 함수
node *create_node(char *name, int parentheses);
//인자로 받은 문자의 우선순위를 리턴시켜주는 함수
int get_precedence(char *op);
//연산자인지 확인하는 함수
int is_operator(char *op);
void print(node *cur);
//트리에서 연산자를 찾는함수
node *get_operator(node *cur);
//루트를 반환하는 함수
node *get_root(node *cur);
//우선순위가 높은 노드를 리턴
node *get_high_precedence_node(node *cur, node *new);
//가장 최상위 우선순위 노드를 리턴
node *get_most_high_precedence_node(node *cur, node *new);
//새로운 노드를 삽입시키는 함수
node *insert_node(node *old, node *new);
//가장 마지막 자식을 찾는 함수
node *get_last_child(node *cur);
//메모리해제함수
void free_node(node *cur);
//자식의 갯수를 리턴시키는 함수
int get_sibling_cnt(node *cur);

//입력받은 str을 여러규칙에 따라 토큰화 시킨다
int make_tokens(char *str, char tokens[TOKEN_CNT][MINLEN]);
//gcc인지 자료형인지 확인
int is_typeStatement(char* str);
//타입지정자 찾기
int find_typeSpecifier(char tokens[TOKEN_CNT][MINLEN]);
//타입지정자 찾기 2
int find_typeSpecifier2(char tokens[TOKEN_CNT][MINLEN]);
//문자인지 확인하는 함수
int is_character(char c);
//*로만 이루어져 있는지 확인하는 함수
int all_star(char *str);
//다 문자인지 확인
int all_character(char *str);
//struct와 unsigned 등등 처리화 나머지 토큰들 초기화 시킴
int reset_tokens(int start, char tokens[TOKEN_CNT][MINLEN]);
//토큰이 저장된 배열 초기화
void clear_tokens(char tokens[TOKEN_CNT][MINLEN]);
int get_token_cnt(char tokens[TOKEN_CNT][MINLEN]);
//오른쪽의 공백을 제거시키는 함수
char *rtrim(char *_str);
//왼쪽의 공백을 제거시키는 함수
char *ltrim(char *_str);
//문자열 사이 공백 제거
void remove_space(char *str);
//괄호의 갯수가 맞는지 세주는 함수
int check_brackets(char *str);
//추가적인 공백들을 삭제시키는 함수
char* remove_extraspace(char *str);

#endif

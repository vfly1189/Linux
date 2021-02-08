#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include "blank.h"

//데이터타입들을 미리 저장해둔 배열
char datatype[DATATYPE_SIZE][MINLEN] = { "int", "char", "double", "float", "long"
			, "short", "ushort", "FILE", "DIR","pid"
			,"key_t", "ssize_t", "mode_t", "ino_t", "dev_t"
			, "nlink_t", "uid_t", "gid_t", "time_t", "blksize_t"
			, "blkcnt_t", "pid_t", "pthread_mutex_t", "pthread_cond_t", "pthread_t"
			, "void", "size_t", "unsigned", "sigset_t", "sigjmp_buf"
			, "rlim_t", "jmp_buf", "sig_atomic_t", "clock_t", "struct" };

//우선순위 저장 값이 작을수록 높은 우선순위
operator_precedence operators[OPERATOR_CNT] = {
	{"(", 0}, {")", 0}
	,{"->", 1}
	,{"*", 4}	,{"/", 3}	,{"%", 2}
	,{"+", 6}	,{"-", 5}
	,{"<", 7}	,{"<=", 7}	,{">", 7}	,{">=", 7}
	,{"==", 8}	,{"!=", 8}
	,{"&", 9}
	,{"^", 10}
	,{"|", 11}
	,{"&&", 12}
	,{"||", 13}
	,{"=", 14}	,{"+=", 14}	,{"-=", 14}	,{"&=", 14}	,{"|=", 14}
};

//트리 비교함수
void compare_tree(node* root1, node* root2, int* result)
{
	node* tmp;
	int cnt1, cnt2;


	//둘 중하나가 null일경우 return에다가 false 저장 후 종료
	if (root1 == NULL || root2 == NULL) {
		*result = false;
		return;
	}
	// 비교연산자의 경우
	//한쪽을 변경해서 자식들의 순서를 바꿔줌
	if (!strcmp(root1->name, "<") || !strcmp(root1->name, ">") || !strcmp(root1->name, "<=") || !strcmp(root1->name, ">=")) {
		if (strcmp(root1->name, root2->name) != 0) {

			if (!strncmp(root2->name, "<", 1))
				strncpy(root2->name, ">", 1);

			else if (!strncmp(root2->name, ">", 1))
				strncpy(root2->name, "<", 1);

			else if (!strncmp(root2->name, "<=", 2))
				strncpy(root2->name, ">=", 2);

			else if (!strncmp(root2->name, ">=", 2))
				strncpy(root2->name, "<=", 2);
			//자식 순서를 바꿔줌
			root2 = change_sibling(root2);
		}
	}
	//현재 노드의 이름이 다르면 return false
	if (strcmp(root1->name, root2->name) != 0) {
		*result = false;
		return;
	}
	//한쪽은 자식이 있는데 한쪽은 자식이없으면 false
	if ((root1->child_head != NULL && root2->child_head == NULL)
		|| (root1->child_head == NULL && root2->child_head != NULL)) {
		*result = false;
		return;
	}

	//root1의 자식이 존재하면
	else if (root1->child_head != NULL) {
		//root1과 root2의 자식갯수를 세서 다르면 false
		if (get_sibling_cnt(root1->child_head) != get_sibling_cnt(root2->child_head)) {
			*result = false;
			return;
		}
		//같은지 연산하는 연산자의 경우
		if (!strcmp(root1->name, "==") || !strcmp(root1->name, "!="))
		{
			//자식으로 이동후 트리비교함수 다시 호출
			compare_tree(root1->child_head, root2->child_head, result);

			//자식트리를 비교했는데 false일 경우
			if (*result == false)
			{
				//결과를 true로 다시 설정 후
				*result = true;
				//root2는 자식의 순서를 변경
				root2 = change_sibling(root2);
				//다시 비교
				compare_tree(root1->child_head, root2->child_head, result);
			}
		}
		//조건식의 연산자 일경우
		else if (!strcmp(root1->name, "+") || !strcmp(root1->name, "*")
			|| !strcmp(root1->name, "|") || !strcmp(root1->name, "&")
			|| !strcmp(root1->name, "||") || !strcmp(root1->name, "&&"))
		{
			//자식의 갯수가 다르면 false
			if (get_sibling_cnt(root1->child_head) != get_sibling_cnt(root2->child_head)) {
				*result = false;
				return;
			}
			//tmp의 roo2의 자식노드를 저장하고
			tmp = root2->child_head;
			//이전이 null이 아닌동안 이전으로 이동
			while (tmp->prev != NULL)
				tmp = tmp->prev;

			//root1과 root2를 따라가는 임시 tmp를 이용해 자식들을 비교한다
			while (tmp != NULL)
			{
				compare_tree(root1->child_head, tmp, result);

				//자식이 같으면 break;
				if (*result == true)
					break;
				//다르면 tmp를 next로 이동시킨후 다시 비교
				else {
					if (tmp->next != NULL)
						*result = true;
					tmp = tmp->next;
				}
			}
		}
		//다른 연산자들의 경우 자식으로가서 비교함수 다시 호출
		else {
			compare_tree(root1->child_head, root2->child_head, result);
		}
	}

	//root1의 다음이 존재한다면
	if (root1->next != NULL) {

		//자식의 갯수가 같은지 확인하고 다르면 false
		if (get_sibling_cnt(root1) != get_sibling_cnt(root2)) {
			*result = false;
			return;
		}
		//자식 비교해서 다른게 없었다면
		if (*result == true)
		{
			tmp = get_operator(root1);

			//순서가 바뀌어도 상관없는 연산자의 경우
			if (!strcmp(tmp->name, "+") || !strcmp(tmp->name, "*")
				|| !strcmp(tmp->name, "|") || !strcmp(tmp->name, "&")
				|| !strcmp(tmp->name, "||") || !strcmp(tmp->name, "&&"))
			{
				//임시로 tmp에 저장 후
				tmp = root2;

				//tmp에 이전이 없을 때까지 tmp을 이전으로 이동시킴
				while (tmp->prev != NULL)
					tmp = tmp->prev;

				//tmp가 뭔가를 가리키고 있다면
				while (tmp != NULL)
				{
					//트리 비교
					compare_tree(root1->next, tmp, result);
					//결과가 똑같다면 break
					if (*result == true)
						break;
					//결과가 다르다면
					else {
						//tmp에 next가 존재하면 결과를 true로 설정
						if (tmp->next != NULL)
							*result = true;
						//tmp를 다음으로 이동시킴
						tmp = tmp->next;
					}
				}
			}
			//순서가 중요한 연산자일 경우 트리 둘다 next로 이동 후 비교
			else
				compare_tree(root1->next, root2->next, result);
		}
	}
}

int make_tokens(char* str, char tokens[TOKEN_CNT][MINLEN])
{
	char* start, * end;
	char tmp[BUFLEN];
	char str2[BUFLEN];
	char* op = "(),;><=!|&^/+-*\"";
	int row = 0;
	int i;
	int isPointer;
	int lcount, rcount;
	int p_str;

	//tokens배열 초기화
	clear_tokens(tokens);

	start = str;
	//자료형이나 gcc확인후 에러 뜨면 false 리턴
	if (is_typeStatement(str) == 0)
		return false;

	while (1)
	{
		//op중 하나라도 걸리는 문자가 있으면 거기가 end 없으면 종료
		if ((end = strpbrk(start, op)) == NULL)
			break;

		//start와 end가 가리키는게 op중 하나라면
		if (start == end) {

			//증감연산자의 경우
			if (!strncmp(start, "--", 2) || !strncmp(start, "++", 2)) {
				//4개씩 있는 경우 return false
				if (!strncmp(start, "++++", 4) || !strncmp(start, "----", 4))
					return false;
				//증감연산자를 건너뛰고 다음이 문자라면 전위연산자
				if (is_character(*ltrim(start + 2))) {
					//앞에 토큰이 하나존재하고 이전이 문자라면 에러
					if (row > 0 && is_character(tokens[row - 1][strlen(tokens[row - 1]) - 1]))
						return false;

					//증감연산자 건너뛰고 다음 op에 걸리는 것을 end가 가리키게함
					end = strpbrk(start + 2, op);
					//없으면 문자열 맨끝으로 이동시킴
					if (end == NULL)
						end = &str[strlen(str)];
					//그 사이에 있는 데이터들 토큰에 하나씩추가
					while (start < end) {
						//변수명 사이에 공백이 존재하면 에러
						if (*(start - 1) == ' ' && is_character(tokens[row][strlen(tokens[row]) - 1]))
							return false;
						//공백이 아니면 추가
						else if (*start != ' ')
							strncat(tokens[row], start, 1);
						start++;
					}
				}

				//앞에 토큰이 하나 존재하고 row-1토큰의 마지막 문자가 character라면 ++은 후위 연산자
				else if (row > 0 && is_character(tokens[row - 1][strlen(tokens[row - 1]) - 1])) {
					//row-1토큰이 ++이면 에러
					if (strstr(tokens[row - 1], "++") != NULL || strstr(tokens[row - 1], "--") != NULL)
						return false;
					//이전토큰에다가 증감연산자 붙여줌
					memset(tmp, 0, sizeof(tmp));
					strncpy(tmp, start, 2);
					strcat(tokens[row - 1], tmp);
					start += 2;
					row--;
				}
				//나머지 경우  토큰에 추가시켜줌
				else {
					memset(tmp, 0, sizeof(tmp));
					strncpy(tmp, start, 2);
					strcat(tokens[row], tmp);
					start += 2;
				}
			}

			//비교연산자, 대입연산자를 축소시킨 것들인 경우 그냥 토큰배열에 추가
			else if (!strncmp(start, "==", 2) || !strncmp(start, "!=", 2) || !strncmp(start, "<=", 2)
				|| !strncmp(start, ">=", 2) || !strncmp(start, "||", 2) || !strncmp(start, "&&", 2)
				|| !strncmp(start, "&=", 2) || !strncmp(start, "^=", 2) || !strncmp(start, "!=", 2)
				|| !strncmp(start, "|=", 2) || !strncmp(start, "+=", 2) || !strncmp(start, "-=", 2)
				|| !strncmp(start, "*=", 2) || !strncmp(start, "/=", 2)) {
				//토큰배열에 2개 복사
				strncpy(tokens[row], start, 2);
				//포인터 2칸 이동
				start += 2;
			}
			//화살표 연산자의 경우
			else if (!strncmp(start, "->", 2))
			{
				//화살표연산자를 건너뛰고 그 다음 op에 걸리는 무언가를 찾음
				end = strpbrk(start + 2, op);

				//그 다음이 없으면 끝 부분을 이 함수에서 인자로 받은 문자열의 끝으로 이동시킴
				if (end == NULL)
					end = &str[strlen(str)];
				//공백을 제외한 문자열들을 이전토큰에 넣어줌 [row-1]에 ->와 ->다음 문자열들을 추가
				while (start < end) {
					if (*start != ' ')
						strncat(tokens[row - 1], start, 1);
					start++;
				}
				//토큰 한줄 뒤로
				row--;
			}
			//주소추출 연산자, 비트연산일경우
			else if (*end == '&')
			{
				//첫번째 토큰이거나 이전 토큰이 연산자일 경우 - 주소연산자
				if (row == 0 || (strpbrk(tokens[row - 1], op) != NULL)) {
					//주소연산자를 제외하고 다음 op에 걸리는 연산자를 가리킴
					end = strpbrk(start + 1, op);
					//없으면 문자열 맨뒤를 가리키게 함
					if (end == NULL)
						end = &str[strlen(str)];
					//먼저 주소연산자를 토큰에 넣어줌
					strncat(tokens[row], start, 1);
					start++;

					//그 사이의 내용들을
					while (start < end) {
						//주소연산자와 변수명사이에 공백이 아니라 문자열간에 공백이 있으면 에러
						if (*(start - 1) == ' ' && tokens[row][strlen(tokens[row]) - 1] != '&')
							return false;
						//공백이 아니면 토큰에 추가시켜줌
						else if (*start != ' ')
							strncat(tokens[row], start, 1);
						start++;
					}
				}
				//아니면 비트연산자일경우 그냥 토큰에 추가시킴
				else {
					strncpy(tokens[row], start, 1);
					start += 1;
				}

			}
			//포인터거나 곱셈의 연산자 '*'의 경우
			else if (*end == '*')
			{
				//포인터인지 확인용 flag
				isPointer = 0;

				//앞에 최소 토큰이 1개 있을때
				if (row > 0)
				{
					//앞 토큰이 자료형인지 확인
					for (i = 0; i < DATATYPE_SIZE; i++) {
						//자료형일경우 *을 앞토큰에 *을 붙혀줌
						if (strstr(tokens[row - 1], datatype[i]) != NULL) {
							strcat(tokens[row - 1], "*");
							start += 1;
							//포인터 플래그 활성화
							isPointer = 1;
							break;
						}
					}
					//만약 포인터라면 다음 토큰작업으로 이동
					if (isPointer == 1)
						continue;
					if (*(start + 1) != 0)
						end = start + 1;

					//앞에 토큰이 2개 존재하고 row-2토큰이 *이고 row-1이 다 '*'토큰이면
					//row-1에다가 *추가해주고 row값 유지
					if (row > 1 && !strcmp(tokens[row - 2], "*") && (all_star(tokens[row - 1]) == 1)) {
						strncat(tokens[row - 1], start, end - start);
						row--;
					}

					//row-1토큰이 문자들로만 이루어진 경우 새로운 토큰 *을 추가
					else if (is_character(tokens[row - 1][strlen(tokens[row - 1]) - 1]) == 1) {
						strncat(tokens[row], start, end - start);
					}

					//row-1토큰이 연산자로 이루어진 경우 포인터를 나타내는 *토큰 새로 추가
					else if (strpbrk(tokens[row - 1], op) != NULL) {
						strncat(tokens[row], start, end - start);

					}
					//나머지의 경우들은 그냥 *을 하나로 토큰화 시킴
					else
						strncat(tokens[row], start, end - start);

					start += (end - start);
				}
				//맨 처음 토큰일 경우
				else if (row == 0)
				{
					//start 위치를 하나 증가시킨상태에서 op에 걸리는 다음 연산자를 찾아서 end가 가리키게함

					//end가 null일 경우 start가 가리키는 '*' 토큰화
					if ((end = strpbrk(start + 1, op)) == NULL) {
						strncat(tokens[row], start, 1);
						start += 1;
					}
					//걸리는 연산자가 있을 경우
					else {
						//start와 end사이 문자들을 토큰화 시켜줌
						while (start < end) {
							//문자간 공백이 있을 경우 return false
							if (*(start - 1) == ' ' && is_character(tokens[row][strlen(tokens[row]) - 1]))
								return false;
							//공백이 아니면 토큰에 넣어줌
							else if (*start != ' ')
								strncat(tokens[row], start, 1);
							start++;
						}
						//만약 집어넣은 문자들이 다 *이면 row값 유지
						if (all_star(tokens[row]))
							row--;

					}
				}
			}
			//여는 괄호일경우
			else if (*end == '(')
			{
				lcount = 0;
				rcount = 0;
				//이전이 포인터,주소관련 연산자 였을경우
				if (row > 0 && (strcmp(tokens[row - 1], "&") == 0 || strcmp(tokens[row - 1], "*") == 0)) {
					//마지막 여는 괄호를 만날때까지 이동시키고 갯수를 셈
					while (*(end + lcount + 1) == '(')
						lcount++;
					start += lcount;

					//그 다음 닫는 괄호를 찾아 )이동
					end = strpbrk(start + 1, ")");

					//없으면 false
					if (end == NULL)
						return false;
					else {
						//마지막 닫는 괄호를 만날때까지 이동시키고 갯수를 셈
						while (*(end + rcount + 1) == ')')
							rcount++;
						end += rcount;

						//만약 여는 괄호랑 닫는괄호 갯수가 맞이 않으면 false
						if (lcount != rcount)
							return false;

						//앞에 토큰이 2개존재하고 2번째전 토큰의 마지막 문자가 character가 아닌경우 와 row==1일때
						//&,*일경우
						if ((row > 1 && !is_character(tokens[row - 2][strlen(tokens[row - 2]) - 1])) || row == 1) {
							//앞토큰에다가 괄호를 제외한 정보를 추가시켜줌
							strncat(tokens[row - 1], start + 1, end - start - rcount - 1);
							row--;
							start = end + 1;
						}
						//나머지면 그냥 토큰에 추가
						else {
							strncat(tokens[row], start, 1);
							start += 1;
						}
					}

				}
				//포인터관련 연산자가 아니면 그냥 토큰에 추가
				else {
					strncat(tokens[row], start, 1);
					start += 1;
				}

			}
			//큰 따움표의 경우
			else if (*end == '\"')
			{
				//처음 찾은 따움표를 건너뛰고 닫는 따움표를 찾아서 end가 가리키게 함
				end = strpbrk(start + 1, "\"");

				//없으면 return false -> 틀린거임
				if (end == NULL)
					return false;
				//찾았으면
				else {
					//따움표와 따움표사이의 내용을 토큰에 추가
					strncat(tokens[row], start, end - start + 1);
					start = end + 1;
				}

			}
			//이외의 연산자의 경우
			else {

				//앞 토큰이 ++이면 에러
				if (row > 0 && !strcmp(tokens[row - 1], "++"))
					return false;

				//앞 토큰이 --이면 에러
				if (row > 0 && !strcmp(tokens[row - 1], "--"))
					return false;

				//일단 연산자 추가후
				strncat(tokens[row], start, 1);
				start += 1;

				//조건문에 연산자일 경우
				if (!strcmp(tokens[row], "-") || !strcmp(tokens[row], "+") || !strcmp(tokens[row], "--") || !strcmp(tokens[row], "++")) {
					//맨처음인경우
					//부호나 전위증감연산자일경우
					if (row == 0)
						row--;
					//이전 토큰이character가 아니라면
					else if (!is_character(tokens[row - 1][strlen(tokens[row - 1]) - 1])) {

						//이전토큰에서 ++나 --을 찾아서없으면 row를 증가시키지 않고 그대로 둠
						if (strstr(tokens[row - 1], "++") == NULL && strstr(tokens[row - 1], "--") == NULL)
							row--;
					}
				}
			}
		}
		//start와 end 사이에는 문자열이 존재 ex)변수명
		else {
			//앞 토큰이 다 '*'이 포인터를 나타내는 것이면 row 증가x
			if (all_star(tokens[row - 1]) && row > 1 && !is_character(tokens[row - 2][strlen(tokens[row - 2]) - 1]))
				row--;
			//row ==1 이고 앞이 '*'를 나타내면 row 증가 x
			if (all_star(tokens[row - 1]) && row == 1)
				row--;

			//start와 end사이에 문자들을 토큰에 넣어줌
			for (i = 0; i < end - start; i++) {
				//.으로 연결되는 문자열도 하나의 토큰에 집어넣어줌
				if (i > 0 && *(start + i) == '.') {
					strncat(tokens[row], start + i, 1);
					//공백은무시하고 진행
					while (*(start + i + 1) == ' ' && i < end - start)
						i++;
				}
				//공백은 무시하고 진행
				else if (start[i] == ' ') {
					while (start[i] == ' ')
						i++;
					break;
				}
				//일반적인 경우면 토큰에 하나 넣어줌
				else
					strncat(tokens[row], start + i, 1);
			}
			//공백을 만나면 start와 end사이에 문자열 길이만큼 start 이동시킴
			if (start[0] == ' ') {
				start += i;
				continue;
			}
			//start와 end사이에 문자열 길이만큼 start 이동시킴
			start += i;
		}

		//토큰의 왼쪽 오른쪽 공백을 지워버림
		strcpy(tokens[row], ltrim(rtrim(tokens[row])));

		//(앞에 1개토큰이 존재하고 현재토큰의 끝이 문자이고 (자료형이거나 row-1의끝이 문자거나 '.'))일때		
		if (row > 0 && is_character(tokens[row][strlen(tokens[row]) - 1])
			&& (is_typeStatement(tokens[row - 1]) == 2
				|| is_character(tokens[row - 1][strlen(tokens[row - 1]) - 1])
				|| tokens[row - 1][strlen(tokens[row - 1]) - 1] == '.')) {

			//앞에 토큰이 2개있고 row-2 가 여는 괄호인데
			if (row > 1 && strcmp(tokens[row - 2], "(") == 0)
			{
				//row-1이 struct나 unsigned가 아니면 false
				if (strcmp(tokens[row - 1], "struct") != 0 && strcmp(tokens[row - 1], "unsigned") != 0)
					return false;
			}
			//row == 1이고 현재토큰의 맨마지막이 문자일때
			else if (row == 1 && is_character(tokens[row][strlen(tokens[row]) - 1])) {
				//extern,unsigned나 자료형이 없으면 false
				if (strcmp(tokens[0], "extern") != 0 && strcmp(tokens[0], "unsigned") != 0 && is_typeStatement(tokens[0]) != 2)
					return false;
			}
			//앞에 토큰이 2개있고 row-1토큰이 타입명일때
			else if (row > 1 && is_typeStatement(tokens[row - 1]) == 2) {
				//row-2에 unsigned, extern이 없으면 false
				if (strcmp(tokens[row - 2], "unsigned") != 0 && strcmp(tokens[row - 2], "extern") != 0)
					return false;
			}

		}

		//첫번째 토큰이고 그 토큰이 gcc인 경우 문자열 전체를 토큰에 넣어버림
		if ((row == 0 && !strcmp(tokens[row], "gcc"))) {
			//토큰을 담았던 tokens 리셋시키고
			clear_tokens(tokens);
			//문자열 토큰의 맨 처음에 넣고
			strcpy(tokens[0], str);
			//return 1
			return 1;
		}
		//한줄 증가
		row++;
	}

	//맨 마지막에 남은 토큰 처리

	//앞 토큰이 다 '*'이 포인터를 나타내는 것이면 row 증가x
	if (all_star(tokens[row - 1]) && row > 1 && !is_character(tokens[row - 2][strlen(tokens[row - 2]) - 1]))
		row--;
	//row ==1 이고 앞이 '*'를 나타내면 row 증가 x
	if (all_star(tokens[row - 1]) && row == 1)
		row--;
	//남은 문자 토큰처리
	for (i = 0; i < strlen(start); i++)
	{
		//공백은 무시하고 start증가
		if (start[i] == ' ')
		{
			while (start[i] == ' ')
				i++;
			if (start[0] == ' ') {
				start += i;
				i = 0;
			}
			else
				row++;

			i--;
		}
		//공백이 아니라면
		else
		{
			//문자 하나를 토큰에 집어넣고
			strncat(tokens[row], start + i, 1);
			//만약 '.'이고 아직 다 넣지 않았다면
			if (start[i] == '.' && i < strlen(start)) {
				//'.'다음 공백들을 무시함
				while (start[i + 1] == ' ' && i < strlen(start))
					i++;

			}
		}
		//토큰 앞뒤 공백을 제거시킴
		strcpy(tokens[row], ltrim(rtrim(tokens[row])));

		if (!strcmp(tokens[row], "lpthread") && row > 0 && !strcmp(tokens[row - 1], "-")) {
			strcat(tokens[row - 1], tokens[row]);
			memset(tokens[row], 0, sizeof(tokens[row]));
			row--;
		}
		//(앞에 1개토큰이 존재하고 현재토큰의 끝이 문자이고 (자료형이거나 row-1의끝이 문자거나 '.'))일때
		else if (row > 0 && is_character(tokens[row][strlen(tokens[row]) - 1])
			&& (is_typeStatement(tokens[row - 1]) == 2
				|| is_character(tokens[row - 1][strlen(tokens[row - 1]) - 1])
				|| tokens[row - 1][strlen(tokens[row - 1]) - 1] == '.')) {
			//앞에 토큰이 2개있고 row-2 가 여는 괄호인데
			if (row > 1 && strcmp(tokens[row - 2], "(") == 0)
			{
				//row-1이 struct나 unsigned가 아니면 false
				if (strcmp(tokens[row - 1], "struct") != 0 && strcmp(tokens[row - 1], "unsigned") != 0)
					return false;
			}
			//row == 1이고 현재토큰의 맨마지막이 문자일때
			else if (row == 1 && is_character(tokens[row][strlen(tokens[row]) - 1])) {
				//extern,unsigned나 자료형이 없으면 false
				if (strcmp(tokens[0], "extern") != 0 && strcmp(tokens[0], "unsigned") != 0 && is_typeStatement(tokens[0]) != 2)
					return false;
			}
			//앞에 토큰이 2개있고 row-1토큰이 타입명일때
			else if (row > 1 && is_typeStatement(tokens[row - 1]) == 2) {
				//row-2에 unsigned, extern이 없으면 false
				if (strcmp(tokens[row - 2], "unsigned") != 0 && strcmp(tokens[row - 2], "extern") != 0)
					return false;
			}
		}
	}

	//앞에 토큰이 하나 있는데
	if (row > 0)
	{
		//#include계열이나 strcut계열이면
		if (strcmp(tokens[0], "#include") == 0 || strcmp(tokens[0], "include") == 0 || strcmp(tokens[0], "struct") == 0) {
			//토큰들 다 초기화시키고
			clear_tokens(tokens);
			//첫번째 토큰에다가 공백을 지운 문자열을 넣어줌
			strcpy(tokens[0], remove_extraspace(str));
		}
	}


	//첫번째 토큰이 자료형이거나 extern일 경우
	if (is_typeStatement(tokens[0]) == 2 || strstr(tokens[0], "extern") != NULL) {
		for (i = 1; i < TOKEN_CNT; i++) {
			//읽을 토큰이 없으면 break;
			if (strcmp(tokens[i], "") == 0)
				break;
			//i가 토큰배열의 가장 마지막이 아니라면
			if (i != TOKEN_CNT - 1)
				//extern 다음에 공백 하나 추가
				strcat(tokens[0], " ");
			//첫번째 토큰에다가 i번째 토큰을 연결 시켜줌 ( extern 변수명 )
			strcat(tokens[0], tokens[i]);
			//i번째 토큰을 0으로 초기화 시켜줌
			memset(tokens[i], 0, sizeof(tokens[i]));
		}
	}

	//자료형과 특정조건문을 만족하는 토큰을 찾으면 인덱스를 리턴받고 토큰리셋 진행
	while ((p_str = find_typeSpecifier(tokens)) != -1) {
		if (!reset_tokens(p_str, tokens))
			return false;
	}

	//struct 관련 토큰이 있는 index를 찾아서 p_str에 넣고 토큰리셋 진행
	while ((p_str = find_typeSpecifier2(tokens)) != -1) {
		if (!reset_tokens(p_str, tokens))
			return false;
	}

	return true;
}

//트리제작
node* make_tree(node* root, char(*tokens)[MINLEN], int* idx, int parentheses)
{
	node* cur = root;
	node* new;
	node* saved_operator;
	node* operator;
	int fstart;
	int i;

	while (1)
	{
		//토큰이 비어있으면 끝
		if (strcmp(tokens[*idx], "") == 0)
			break;

		//닫는 괄호를 만나면 루트를 리턴
		if (!strcmp(tokens[*idx], ")"))
			return get_root(cur);
		//','를 만나도 루트를 리턴
		else if (!strcmp(tokens[*idx], ","))
			return get_root(cur);
		//여는 괄호일 경우
		else if (!strcmp(tokens[*idx], "("))
		{
			//이전에 토큰이 1개 존재하고 이전 토큰이 연산자가 아니고 ','가 아니면
			if (*idx > 0 && !is_operator(tokens[*idx - 1]) && strcmp(tokens[*idx - 1], ",") != 0) {
				//괄호 시작을 알리는 flag
				fstart = true;

				while (1)
				{
					//괄호는 트리에 안넣고 인덱스 하나 증가
					*idx += 1;

					//현재 토큰이 닫는 괄호라면 스탑
					if (!strcmp(tokens[*idx], ")"))
						break;

					//닫는괄호가 아니라면 새로운 트리를 생성하게 하고 괄호갯수를 하나 증겨서 만듬
					new = make_tree(NULL, tokens, idx, parentheses + 1);

					//트리를 생성했다면
					if (new != NULL) {
						//여는괄호 flag가 세워져있다면 cur이 가리키는 노드의 자식으로 연결
						if (fstart == true) {
							cur->child_head = new;
							new->parent = cur;

							fstart = false;
						}
						//그렇지 않으면 next로 연결시켜줌
						else {
							cur->next = new;
							new->prev = cur;
						}

						cur = new;
					}
					//닫는 괄호라면 break 다시한번 체크
					if (!strcmp(tokens[*idx], ")"))
						break;
				}
			}
			else {
				*idx += 1;
				//괄호갯수인자를 하나 추가해서 추가적인 트리를 제작
				new = make_tree(NULL, tokens, idx, parentheses + 1);

				//트리가 없는 상태이면 cur = new
				if (cur == NULL)
					cur = new;

				//cur의 이름과 new의 이름이 같으면
				else if (!strcmp(new->name, cur->name)) {
					//순서를 바꿔도 상관이 없는 연산자들의 경우
					//조건문의 연산자의 경우
					if (!strcmp(new->name, "|") || !strcmp(new->name, "||")
						|| !strcmp(new->name, "&") || !strcmp(new->name, "&&"))
					{
						//현재노드의 마지막 자식을 가져온다
						cur = get_last_child(cur);

						//새로 만든 트리의 자식이 존재하면 
						//연산자 말고 변수들만 cur 다음에 연결시켜준다
						if (new->child_head != NULL) {
							new = new->child_head;

							new->parent->child_head = NULL;
							new->parent = NULL;
							new->prev = cur;
							cur->next = new;
						}
					}
					//+나 *도 순서가 바뀌어도 상관없음
					else if (!strcmp(new->name, "+") || !strcmp(new->name, "*"))
					{
						i = 0;

						while (1)
						{
							//공백이면 break;
							if (!strcmp(tokens[*idx + i], ""))
								break;
							//토큰이 연산자고 닫는괄호면 braek;
							if (is_operator(tokens[*idx + i]) && strcmp(tokens[*idx + i], ")") != 0)
								break;

							i++;
						}
						//토큰의 우선순위 새로만든트리의 우선순위를 비교해서 토큰이 더 위면
						if (get_precedence(tokens[*idx + i]) < get_precedence(new->name))
						{
							//마지막 자식 다음에 연결시켜준다
							cur = get_last_child(cur);
							cur->next = new;
							new->prev = cur;
							cur = new;
						}
						//새로만든 트리가 더 높거나 같으면
						else
						{
							//마지막 자식을 가리키고
							cur = get_last_child(cur);
							//새로만든 트리의 자식이 존재하면
							//cur next에다가 new를 연결시켜쥼
							if (new->child_head != NULL) {
								new = new->child_head;

								new->parent->child_head = NULL;
								new->parent = NULL;
								new->prev = cur;
								cur->next = new;
							}
						}
					}
					//순서가 중요한 연산자의 경우
					else {
						//cur노드에서 가장마지막자식다음에 연결시켜줌
						cur = get_last_child(cur);
						cur->next = new;
						new->prev = cur;
						cur = new;
					}
				}
				//이름이 다를 경우
				else
				{
					//가장 마지막 자식 다음에 연결시켜준다
					cur = get_last_child(cur);

					cur->next = new;
					new->prev = cur;

					cur = new;
				}
			}
		}
		//연산자일 경우
		else if (is_operator(tokens[*idx]))
		{
			//순서가 바뀌어도 상관없는 연산자의 경우
			if (!strcmp(tokens[*idx], "||") || !strcmp(tokens[*idx], "&&")
				|| !strcmp(tokens[*idx], "|") || !strcmp(tokens[*idx], "&")
				|| !strcmp(tokens[*idx], "+") || !strcmp(tokens[*idx], "*"))
			{
				//cur이 가리키고 있는게 위에 조건식에 나온 연산자와 똑같다면
				if (is_operator(cur->name) == true && !strcmp(cur->name, tokens[*idx]))
					operator = cur;
				//다를 경우
				else
				{
					//새로운 노드를 만들고
					new = create_node(tokens[*idx], parentheses);
					//새로운 노드를 기준으로 현재부터시작해서 최상위 우선순위 노드를 찾아서 operator에 저장
					operator = get_most_high_precedence_node(cur, new);

					//만약 operator가 트리중 루트 노드일경우
					if (operator->parent == NULL && operator->prev == NULL) {

						//우선순위를 비교해서 operator가 더 높으면
						if (get_precedence(operator->name) < get_precedence(new->name)) {
							//operator를 밑으로 new노드를 추가함
							cur = insert_node(operator, new);
						}

						//new노드가 더 높을경우
						else if (get_precedence(operator->name) > get_precedence(new->name))
						{
							//operator의 자식이 존재하면 가장 마지막 자식을 밑으로하여 new를 추가시킴
							if (operator->child_head != NULL) {
								operator = get_last_child(operator);
								cur = insert_node(operator, new);
							}
						}
						//같을 경우
						else
						{
							operator = cur;

							while (1)
							{
								//operator랑 토큰이 같은 연산자일 경우 break
								if (is_operator(operator->name) == true && !strcmp(operator->name, tokens[*idx]))
									break;
								//operator가 가리키는 것중 이전이 존재하면 이전으로 계속 이동시킴
								if (operator->prev != NULL)
									operator = operator->prev;
								else
									break;
							}
							//이전으로 계속 이동시킨 다음 토큰과 비교해서 다르면 operator의 부모를 operator에 저장
							if (strcmp(operator->name, tokens[*idx]) != 0)
								operator = operator->parent;
							//operator가 뭔가를 가리키고 있는 상태라면 토큰과 다시비교해줌
							if (operator != NULL) {
								if (!strcmp(operator->name, tokens[*idx]))
									cur = operator;
							}
						}
					}
					//그 외의 노드일 경우
					else
						//operator를 밑으로 new노드를 추가함
						cur = insert_node(operator, new);
				}

			}
			//그 외의 연산자의 경우
			else
			{
				//새로운 노드를 생성하고
				new = create_node(tokens[*idx], parentheses);

				//cur이 null이면 cur = new
				if (cur == NULL)
					cur = new;

				else
				{
					//new노드와 비교하여 가장높은 우선순위를 가지는 노드를 리턴받음
					operator = get_most_high_precedence_node(cur, new);

					//operator 노드가 괄호 갯수가 더 많으면 new밑에 operator노드를 연결시켜줌
					if (operator->parentheses > new->parentheses)
						cur = insert_node(operator, new);
					//operator가 최상단 노드일경우
					else if (operator->parent == NULL && operator->prev == NULL) {

						//operator와 new노드의 우선순위를 비교해서 새로운 노드가 우선순위가 더 높은경우
						if (get_precedence(operator->name) > get_precedence(new->name))
						{
							//operator의 자식노드가 존재하면 가장 마지막 자식에 new를 연결
							if (operator->child_head != NULL) {

								operator = get_last_child(operator);
								cur = insert_node(operator, new);
							}
						}
						//new가 더 낮거나 같을경우 new밑에 operator를 연결시켜줌 
						else
							cur = insert_node(operator, new);
					}
					//나머지의 경우 그냥 new밑에 operator를 연결
					else
						cur = insert_node(operator, new);
				}
			}
		}
		//변수명 같은 문자열의 경우
		else
		{
			//새로운 노드를 생성하고
			new = create_node(tokens[*idx], parentheses);

			if (cur == NULL)
				cur = new;
			//자식이 존재하지않으면 자식으로서 연결
			else if (cur->child_head == NULL) {
				cur->child_head = new;
				new->parent = cur;

				cur = new;
			}
			//자식 노드가 존재하면
			else {
				//가장 마지막 자식을 가져와서
				cur = get_last_child(cur);
				//마지막자식에 next로 연결
				cur->next = new;
				new->prev = cur;

				cur = new;
			}
		}
		//인덱스 증가
		*idx += 1;
	}
	//그 다음 루트를 리턴시켜줌
	return get_root(cur);
}

//자식을 교환해주는 함수
node* change_sibling(node* parent)
{
	node* tmp;

	tmp = parent->child_head;

	//단순히 부모노드를 인자로 건네받으면
	//자식의 순서를 변경해주는 코드
	parent->child_head = parent->child_head->next;
	parent->child_head->parent = parent;
	parent->child_head->prev = NULL;

	parent->child_head->next = tmp;
	parent->child_head->next->prev = parent->child_head;
	parent->child_head->next->next = NULL;
	parent->child_head->next->parent = NULL;

	return parent;
}

//노드를 생성하는 함수(이름과 괄호의 갯수)
node* create_node(char* name, int parentheses)
{
	node* new;

	//노드를 새로이 동적할당하고
	new = (node*)malloc(sizeof(node));
	//이름을 저장할 포인터에도 동적할당 시켜줌
	new->name = (char*)malloc(sizeof(char) * (strlen(name) + 1));
	//새로 할당한 배열에 이름을 복사해서 넣어줌
	strcpy(new->name, name);
	//괄호갯수 넣어주고 나머지는 null로 초기화
	new->parentheses = parentheses;
	new->parent = NULL;
	new->child_head = NULL;
	new->prev = NULL;
	new->next = NULL;

	return new;
}

//operators배열에서 우선순위를 가져옴
int get_precedence(char* op)
{
	int i;

	//같은 연산자가 operators의 존재하면 우선순위를 리턴
	for (i = 2; i < OPERATOR_CNT; i++) {
		if (!strcmp(operators[i].operator, op))
			return operators[i].precedence;
	}
	//없으면 0
	return false;
}

//연산자인지 확인
int is_operator(char* op)
{
	int i;

	//operators 배열에서 연산자인지 확인함
	for (i = 0; i < OPERATOR_CNT; i++)
	{
		//연산자배열에서 null이면 break;
		if (operators[i].operator == NULL)
			break;
		//연산자라면 return true;
		if (!strcmp(operators[i].operator, op)) {
			return true;
		}
	}
	//없다면 return false
	return false;
}

//끝에서부터 역으로 문자 출력
void print(node* cur)
{
	//cur노드의 자식이 있으면 자식 노드로 내려감
	if (cur->child_head != NULL) {
		print(cur->child_head);
		printf("\n");
	}
	//cur 다음에 next가 있으면 next로 이동
	if (cur->next != NULL) {
		print(cur->next);
		printf("\t");
	}
	//그 노드의 저장된 문자 출력
	printf("%s", cur->name);
}

//연산자를 가져온다
node* get_operator(node* cur)
{
	//현재노드가 null이면 그냥 cur을 리턴
	if (cur == NULL)
		return cur;

	//현재 노드에서 이전이 존재하면 이전이 없을때까지 이전으로 이동시킴
	if (cur->prev != NULL)
		while (cur->prev != NULL)
			cur = cur->prev;
	//이전으로 다 이동시킨 다음 현재노드의 부모를 리턴
	return cur->parent;
}

//현재노드에서 root를 리턴시켜주는 함수
node* get_root(node* cur)
{
	if (cur == NULL)
		return cur;

	//이전이 존재하면 이전으로 계속이동
	while (cur->prev != NULL)
		cur = cur->prev;

	//부모노드가 존재하면 부모노드로 이동후 다시 get_root실행
	if (cur->parent != NULL)
		cur = get_root(cur->parent);

	//그 다음 cur이 가리키는 노드 리턴
	return cur;
}

//우선순위가 높은 노드를 찾아서 리턴 받음
node* get_high_precedence_node(node* cur, node* new)
{
	//현재 노드가 연산자라면
	if (is_operator(cur->name))
		//연산자의 우선순위를 비교함 현재노드의 우선순위가 더 높으면 cur을 리턴
		if (get_precedence(cur->name) < get_precedence(new->name))
			return cur;

	//연산자가 아니라면
	//이전이 존재할경우 이전으로 이동시키고
	if (cur->prev != NULL) {
		while (cur->prev != NULL) {
			cur = cur->prev;
			//한 칸 이전으로 이동하고 거기서 다시 함수 호출
			return get_high_precedence_node(cur, new);
		}

		//부모가 존재하면 부모로 이동 후 다시 함수 호출
		if (cur->parent != NULL)
			return get_high_precedence_node(cur->parent, new);
	}
	//부모노드가 없을경우 cur return
	if (cur->parent == NULL)
		return cur;
}

//가장 높은 우선순위 노드를 찾음
node* get_most_high_precedence_node(node* cur, node* new)
{
	//현재 노드와 새로운 노드 중 우선순위가 높은 노드를 가져옴
	node* operator = get_high_precedence_node(cur, new);
	node* saved_operator = operator;

	while (1)
	{
		//부모 노드가 없으면 saved_operator가 최상단 노드라는 소리
		if (saved_operator->parent == NULL)
			break;

		//이전이 존재하면 이전으로 이동해서 우선순위 비교후 높은걸 리턴받음
		if (saved_operator->prev != NULL)
			operator = get_high_precedence_node(saved_operator->prev, new);

		//부모가 존해하면 부모로 이동해서 우선순위 비교후 높은걸 리턴받음
		else if (saved_operator->parent != NULL)
			operator = get_high_precedence_node(saved_operator->parent, new);

		saved_operator = operator;
	}
	//가장높은 우선순위의 노드를 리턴
	return saved_operator;
}

//new노드 밑에 old로 넣어주고 이전노드도 new를 가리키게 함
node* insert_node(node* old, node* new)
{
	//old노드의 이전이 존재하면
	//새로운노드의 이전이 old의 이전노드가되고
	//그 이전노드가 가리키는 다음은 new가 됨
	//그리고 old의 이전은 null
	if (old->prev != NULL) {
		new->prev = old->prev;
		old->prev->next = new;
		old->prev = NULL;
	}
	//새로운 노드의 자식은 기존 노드가 됨
	new->child_head = old;
	//기존노드의 부모는 새로운 노드
	old->parent = new;

	return new;
}

//현재노드에서 가장 마지막 자식을 리턴시켜줌
node* get_last_child(node* cur)
{
	//자식이 존재하면 일단 자식노드로 이동
	if (cur->child_head != NULL)
		cur = cur->child_head;
	//그 다음 next가 없을때까지 계속이동
	while (cur->next != NULL)
		cur = cur->next;
	//그럼 마지막 자식을 리턴
	return cur;
}

//형제들의 갯수를 셈
int get_sibling_cnt(node* cur)
{
	int i = 0;

	//일단 이전이 없을때까지 이전으로 계속 이동시킨뒤
	while (cur->prev != NULL)
		cur = cur->prev;

	//next로 이동하면서 갯수를 세준다
	while (cur->next != NULL) {
		cur = cur->next;
		i++;
	}
	//갯수를 리턴
	return i;
}

//노드들을 해방?해제?시켜준다
void free_node(node* cur)
{
	//자식이 존재하면 자식으로 내려가서 함수 다시 진행
	if (cur->child_head != NULL)
		free_node(cur->child_head);

	//다음 노드가 존재하면 다음 노드로 이동해서 다음 노드의 next나 자식들을 free시켜줌
	if (cur->next != NULL)
		free_node(cur->next);

	//현재 노드가 존재하면 값들을 다 null로 바꿔줌
	if (cur != NULL) {
		cur->prev = NULL;
		cur->next = NULL;
		cur->parent = NULL;
		cur->child_head = NULL;
		//메모리 해제
		free(cur);
	}
}

//문자인지 확인 0~9, a~z, A~Z
int is_character(char c)
{
	return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

//gcc인지 데이터타입인지 미리 확인
int is_typeStatement(char* str)
{
	char* start;
	char str2[BUFLEN] = { 0 };
	char tmp[BUFLEN] = { 0 };
	char tmp2[BUFLEN] = { 0 };
	int i;

	start = str;
	//str2에다가 인자로 들어온 문자열 복사
	strncpy(str2, str, strlen(str));
	//str2의 사이사이 공백들 삭제
	remove_space(str2);

	//앞에있는 공백들 무시
	while (start[0] == ' ')
		start += 1;


	//문자열에 gcc가 있을경우
	if (strstr(str2, "gcc") != NULL)
	{
		//tmp2에 복사후
		strncpy(tmp2, start, strlen("gcc"));
		//복사가 잘되었으면 return 2, 안되었으면 0
		if (strcmp(tmp2, "gcc") != 0)
			return 0;
		else
			return 2;
	}
	//데이터타입을 확인한다
	for (i = 0; i < DATATYPE_SIZE; i++)
	{
		//데이터타입이 걸리는 것이 있다면
		if (strstr(str2, datatype[i]) != NULL)
		{
			//tmp와 tmp2에 데이터타입의 길이 만큼 복사해주고
			strncpy(tmp, str2, strlen(datatype[i]));
			strncpy(tmp2, start, strlen(datatype[i]));

			//복사가 정상적으로되었으면 return2 안되었으면 return 0
			if (strcmp(tmp, datatype[i]) == 0)
				if (strcmp(tmp, tmp2) != 0)
					return 0;
				else
					return 2;
		}

	}
	//아무것도 안걸리면 1
	return 1;

}

//타입지정자 찾기
int find_typeSpecifier(char tokens[TOKEN_CNT][MINLEN])
{
	int i, j;

	//토큰 갯수 만큼
	for (i = 0; i < TOKEN_CNT; i++)
	{
		//각각 토큰마다 데이터타입갯수 만큼 확인
		for (j = 0; j < DATATYPE_SIZE; j++)
		{
			//i는0보다 크고 토큰이랑 데이터타입이 맞아떨어지는것이 있다면
			if (strstr(tokens[i], datatype[j]) != NULL && i > 0)
			{
				//밑에 조건일경우 토큰위치를 리턴
				if (!strcmp(tokens[i - 1], "(") && !strcmp(tokens[i + 1], ")")
					&& (tokens[i + 2][0] == '&' || tokens[i + 2][0] == '*'
						|| tokens[i + 2][0] == ')' || tokens[i + 2][0] == '('
						|| tokens[i + 2][0] == '-' || tokens[i + 2][0] == '+'
						|| is_character(tokens[i + 2][0])))
					return i;
			}
		}
	}
	//아무것도 맞는게 없으면 -1 리턴
	return -1;
}

//타입지정자 찾기2
int find_typeSpecifier2(char tokens[TOKEN_CNT][MINLEN])
{
	int i, j;

	//토큰 갯수만큼확인
	for (i = 0; i < TOKEN_CNT; i++)
	{
		//데이터타입크기만큼확인
		for (j = 0; j < DATATYPE_SIZE; j++)
		{
			//struct관련 지정자 찾기
			//현재 토큰이 struct이고 다음 토큰이 변수명이면 return i
			if (!strcmp(tokens[i], "struct") && (i + 1) <= TOKEN_CNT && is_character(tokens[i + 1][strlen(tokens[i + 1]) - 1]))
				return i;
		}
	}
	return -1;
}

//다 '*'문자인지 확인
int all_star(char* str)
{
	int i;
	//문자열의 길이 연산
	int length = strlen(str);

	//길이가 0이면 return 0
	if (length == 0)
		return 0;

	//문자열줄 '*'이 아닌것이 있다면 return 0;	
	for (i = 0; i < length; i++)
		if (str[i] != '*')
			return 0;
	//다 '*'이면 return 1
	return 1;

}

//다 문자인지 확인
int all_character(char* str)
{
	int i;

	//문자열 길이만큼 str[i]가 문자인지 확인
	for (i = 0; i < strlen(str); i++)
		if (is_character(str[i]))
			return 1;
	return 0;

}

//struct와 unsigned 등등 처리화 나머지 토큰들 초기화 시킴
int reset_tokens(int start, char tokens[TOKEN_CNT][MINLEN])
{
	int i;
	int j = start - 1;
	int lcount = 0, rcount = 0;
	int sub_lcount = 0, sub_rcount = 0;
	//시작위치가 -1이 아닐 경우
	if (start > -1) {
		//struct일 경우
		if (!strcmp(tokens[start], "struct")) {
			//struct 다음에 공백을 추가 시킨뒤
			strcat(tokens[start], " ");
			//다음 토큰은 변수명일테니 다음토큰을 현재 토큰에 연결 시켜줌
			strcat(tokens[start], tokens[start + 1]);

			//그 이후 모든 토큰들은 모두 초기화 시킴
			for (i = start + 1; i < TOKEN_CNT - 1; i++) {
				strcpy(tokens[i], tokens[i + 1]);
				memset(tokens[i + 1], 0, sizeof(tokens[0]));
			}
		}
		//unsigned일 경우 && 다음 토큰이 닫는 괄호가 아닐 경우
		else if (!strcmp(tokens[start], "unsigned") && strcmp(tokens[start + 1], ")") != 0) {
			//unsigned 다음에 공백 추가해주고
			strcat(tokens[start], " ");
			//자료형 뒤에 추가
			strcat(tokens[start], tokens[start + 1]);
			//변수명 뒤에 추가
			strcat(tokens[start], tokens[start + 2]);

			//나머지 토큰 모드 초기화 시킴
			for (i = start + 1; i < TOKEN_CNT - 1; i++) {
				strcpy(tokens[i], tokens[i + 1]);
				memset(tokens[i + 1], 0, sizeof(tokens[0]));
			}
		}
		//start를 1증가 시킨 상태에서  닫는괄호랑 토큰이 같은 동안 계속 증가시키면서 닫는 괄호 갯수를 세줌
		j = start + 1;
		while (!strcmp(tokens[j], ")")) {
			rcount++;
			if (j == TOKEN_CNT)
				break;
			j++;
		}
		//start를 하나 줄인 상태에서 뒤로 이동하면서 여는 괄호의 갯수를 세줌
		j = start - 1;
		while (!strcmp(tokens[j], "(")) {
			lcount++;
			if (j == 0)
				break;
			j--;
		}
		if ((j != 0 && is_character(tokens[j][strlen(tokens[j]) - 1])) || j == 0)
			lcount = rcount;
		//괄호 갯수가 다를 경우 false
		if (lcount != rcount)
			return false;

		//마지막 여는 괄호 다름 이전이 sizeof일 경우 true 리턴 시켜줌
		if ((start - lcount) > 0 && !strcmp(tokens[start - lcount - 1], "sizeof")) {
			return true;
		}
		//현재 토큰이 unsigned나 struct이고 다음 토큰이 닫는 괄호가 아니라면
		else if ((!strcmp(tokens[start], "unsigned") || !strcmp(tokens[start], "struct")) && strcmp(tokens[start + 1], ")")) {
			//마지막 여는 괄호가 존재하는 토큰에다가 현재 토큰, 다음 토큰 뒤에다가 추가시켜줌
			strcat(tokens[start - lcount], tokens[start]);
			strcat(tokens[start - lcount], tokens[start + 1]);
			//마지막 닫는 괄호도 추가시켜줌
			strcpy(tokens[start - lcount + 1], tokens[start + rcount]);

			//이후 나머지 토큰들 초기화 시켜줌
			for (int i = start - lcount + 1; i < TOKEN_CNT - lcount - rcount; i++) {
				strcpy(tokens[i], tokens[i + lcount + rcount]);
				memset(tokens[i + lcount + rcount], 0, sizeof(tokens[0]));
			}
		}
		else {
			//다다음 토큰의 시작문자가 (일 경우
			if (tokens[start + 2][0] == '(') {
				//start를 2칸이동한 지점 부터
				j = start + 2;
				//닫는 괄호와 여는 괄호를 각각 세준뒤
				while (!strcmp(tokens[j], "(")) {
					sub_lcount++;
					j++;
				}
				if (!strcmp(tokens[j + 1], ")")) {
					j = j + 1;
					while (!strcmp(tokens[j], ")")) {
						sub_rcount++;
						j++;
					}
				}
				else
					return false;
				//닫는 괄호과 여는괄호 갯수가 서로 다르면 return false
				if (sub_lcount != sub_rcount)
					return false;

				strcpy(tokens[start + 2], tokens[start + 2 + sub_lcount]);
				//나머지 토큰들 전부 초기화 시켜줌
				for (int i = start + 3; i < TOKEN_CNT; i++)
					memset(tokens[i], 0, sizeof(tokens[0]));

			}
			//마지막 여는 괄호가 존재하는 토큰에다가 현재 토큰, 다음 토큰 뒤에다가 추가시켜줌
			strcat(tokens[start - lcount], tokens[start]);
			strcat(tokens[start - lcount], tokens[start + 1]);
			//마지막 닫는 괄호도 추가시켜줌
			strcat(tokens[start - lcount], tokens[start + rcount + 1]);

			//이후 나머지 토큰들 초기화 시켜줌
			for (int i = start - lcount + 1; i < TOKEN_CNT - lcount - rcount - 1; i++) {
				strcpy(tokens[i], tokens[i + lcount + rcount + 1]);
				memset(tokens[i + lcount + rcount + 1], 0, sizeof(tokens[0]));

			}
		}
	}
	return true;
}

//토큰 배열 초기화
void clear_tokens(char tokens[TOKEN_CNT][MINLEN])
{
	int i;

	//토큰배열 각각 초기화
	for (i = 0; i < TOKEN_CNT; i++)
		memset(tokens[i], 0, sizeof(tokens[i]));
}

//오른쪽 공백을 없애줌
char* rtrim(char* _str)
{
	char tmp[BUFLEN];
	char* end;

	//tmp에다가 원래 문자열 복사
	strcpy(tmp, _str);
	//끝을 tmp시작에다가 길이만큼 더하고 - 1
	//맨끝을 가리킴
	end = tmp + strlen(tmp) - 1;
	//시작지점이 아니고 가리키는 문자가 공백인동안 end--
	while (end != _str && isspace(*end))
		--end;

	//맨끝에다가 널문자를 넣어줌
	*(end + 1) = '\0';
	//공백을 제거한 문자열을 저장
	_str = tmp;
	return _str;
}

//왼쪽 공백들을 없애줌
char* ltrim(char* _str)
{
	char* start = _str;
	//널 문자가 아니고 공백인 동안
	while (*start != '\0' && isspace(*start))
		//하나씩 증가
		++start;
	//처음으로 공백이 아닌게 나오면 그 문자를 가리키게 설정
	_str = start;
	return _str;
}
//추가적인 공간들 삭제 시키는 함수
char* remove_extraspace(char* str)
{
	int i;
	char* str2 = (char*)malloc(sizeof(char) * BUFLEN);
	char* start, * end;
	char temp[BUFLEN] = "";
	int position;
	//인자에서 #include<가 존재하면
	if (strstr(str, "include<") != NULL) {
		start = str;
		//end를 <를 가리키게 만듬
		end = strpbrk(str, "<");
		//position은 
		position = end - start;
		//tmp에다가 str에 position 수 만큼 뒤에 붙혀주고
		strncat(temp, str, position);
		//공백 추가
		strncat(temp, " ", 1);
		//그 다음 문자들 추가
		strncat(temp, str + position, strlen(str) - position + 1);

		str = temp;
	}
	//str의 길이만큼 진행한다
	for (i = 0; i < strlen(str); i++)
	{
		//문자가 공백일 경우
		if (str[i] == ' ')
		{
			//시작지점이 공백이라면
			if (i == 0 && str[0] == ' ')
				//공백이 아닐때까지 i증가
				while (str[i + 1] == ' ')
					i++;
			else {
				//이전 문자가 공백이 아니면 str2의 뒤에다가 문자 추가
				if (i > 0 && str[i - 1] != ' ')
					str2[strlen(str2)] = str[i];
				//공백은 무시하고 진행
				while (str[i + 1] == ' ')
					i++;
			}
		}
		//아니라면 str2의 맨뒤에다가 문자 추가
		else
			str2[strlen(str2)] = str[i];
	}

	return str2;
}


//입력받은 문자열에서 공백들을 없애줌
void remove_space(char* str)
{
	char* i = str;
	char* j = str;

	//j가 가리키는게 0이 아닐때까지
	//결론적으로 공백을 만나면 포인터를 하나씩 움직이면서 뒤에 문자들을 앞으로 땡겨줌
	while (*j != 0)
	{
		*i = *j++;
		//공백을 만나면 i를 하나 증가시킴
		if (*i != ' ')
			i++;
	}
	//맨 마지막에 널 문자
	*i = 0;
}

//괄호 갯수를 체크
int check_brackets(char* str)
{
	//입력받은 문자열을 가르키는 문자열 포인터
	char* start = str;
	int lcount = 0, rcount = 0;

	while (1) {
		//"()"중 하나라도 만날때까지 start를 이동하고 처음만나는 ()중 하나의 위치를 리턴
		if ((start = strpbrk(start, "()")) != NULL) {
			//지금 가리키는게 '('면 왼쪽 갯수 하나 증가
			if (*(start) == '(')
				lcount++;
			//')'라면 오른쪽 갯수 하나증가
			else
				rcount++;
			//포인터 하나 이동
			start += 1;
		}
		//없다면 break
		else
			break;
	}
	//왼쪽괄호 오른쪽괄호 갯수 가 다르면 return 0
	if (lcount != rcount)
		return 0;
	//같으면 return 1
	else
		return 1;
}

//토큰의 갯수를 리턴시켜줌
int get_token_cnt(char tokens[TOKEN_CNT][MINLEN])
{
	int i;

	//토큰이 빈거 일때 스탑함
	for (i = 0; i < TOKEN_CNT; i++)
		if (!strcmp(tokens[i], ""))
			break;
	//공백을 만났을때의 i값 리턴(토큰갯수)
	return i;
}

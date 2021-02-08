#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include "ssu_score.h"

#define SECOND_TO_MICRO 1000000

//���α׷� ���� �ð� ����
void ssu_runtime(struct timeval *begin_t, struct timeval *end_t);

int main(int argc, char *argv[])
{
	struct timeval begin_t, end_t;
	//���۽ð� üũ
	gettimeofday(&begin_t, NULL);

	ssu_score(argc, argv);
	//������ �ð� üũ
	gettimeofday(&end_t, NULL);
	//���� �ð� ��� �� ���
	ssu_runtime(&begin_t, &end_t);

	exit(0);
}

void ssu_runtime(struct timeval *begin_t, struct timeval *end_t)
{
	//end_t���ٰ� �����ð� - ������ �ð��� ����
	end_t->tv_sec -= begin_t->tv_sec;

	//���� �ð��� ����ũ���ʰ� �� ���� ��� �ʸ� �ϳ� ���̰� ����ũ���� ����Ͽ� �ݿ�
	if(end_t->tv_usec < begin_t->tv_usec){
		end_t->tv_sec--;
		end_t->tv_usec += SECOND_TO_MICRO;
	}
	//����ũ���ʵ� ���� �ð� - ���۽ð�
	end_t->tv_usec -= begin_t->tv_usec;
	//�ɸ��ð� �ʶ� ����ũ���ʱ��� ���
	printf("Runtime: %ld:%06ld(sec:usec)\n", end_t->tv_sec, end_t->tv_usec);
}

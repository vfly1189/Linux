#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include "ssu_score.h"

#define SECOND_TO_MICRO 1000000

//프로그램 수행 시간 측정
void ssu_runtime(struct timeval *begin_t, struct timeval *end_t);

int main(int argc, char *argv[])
{
	struct timeval begin_t, end_t;
	//시작시간 체크
	gettimeofday(&begin_t, NULL);

	ssu_score(argc, argv);
	//끝나는 시간 체크
	gettimeofday(&end_t, NULL);
	//실행 시간 계산 및 출력
	ssu_runtime(&begin_t, &end_t);

	exit(0);
}

void ssu_runtime(struct timeval *begin_t, struct timeval *end_t)
{
	//end_t에다가 끝난시각 - 시작한 시각을 저장
	end_t->tv_sec -= begin_t->tv_sec;

	//끝난 시각의 마이크로초가 더 작을 경우 초를 하나 줄이고 마이크로초 계산하여 반영
	if(end_t->tv_usec < begin_t->tv_usec){
		end_t->tv_sec--;
		end_t->tv_usec += SECOND_TO_MICRO;
	}
	//마이크로초도 끝난 시각 - 시작시각
	end_t->tv_usec -= begin_t->tv_usec;
	//걸리시간 초랑 마이크로초까지 출력
	printf("Runtime: %ld:%06ld(sec:usec)\n", end_t->tv_sec, end_t->tv_usec);
}

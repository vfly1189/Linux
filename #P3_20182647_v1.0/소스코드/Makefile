ssu_rsync : ssu_rsync.o ssu_crontab ssu_crond
	gcc ssu_rsync.o -o ssu_rsync
ssu_rsync.o : ssu_rsync.c ssu_rsync.h
	gcc -c ssu_rsync.c

ssu_crontab.o : ssu_crontab.c ssu_crontab.h
	gcc -c ssu_crontab.c
ssu_crontab : ssu_crontab.o
	gcc ssu_crontab.o -o ssu_crontab

ssu_crond.o : ssu_crond.c
	gcc -c ssu_crond.c
ssu_crond : ssu_crond.o
	gcc ssu_crond.o -o ssu_crond -lpthread

all : ssu_crond.o ssu_crontab.o ssu_rsync.o
	gcc ssu_crond.o -o ssu_crond -lpthread
	gcc ssu_crontab.o -o ssu_crontab
	gcc ssu_rsync.o -o ssu_rsync

clean :
	rm *.o


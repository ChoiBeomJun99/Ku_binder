#include <stdio.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdlib.h>

#include "ku_binder.h"

static int fd;

int kbinder_init(void){
	// success return 0 , fail return -1
	struct service user_service;
	fd = open("/dev/201811299_dev", O_RDWR);

	if (fd == -1) {
		// init failed
		return -1;
	}
	// init success
	return 0;
}

int kbinder_query(char *sname){
	// success return service number, fail return -1
	// If client, user not execution kbinder_init
	
	//printf("fd: %d\n ", fd);
	if(fd == 0) {
		printf("First, you must init kbinder please / use kbinder_init()\n");
		return -1;
	}

	int ret;
	struct serviceInfo user_service;
	strcpy(user_service.name, sname);
	
	//printf("Client want to find sname: %s service \n", user_service.name);
	ret = ioctl(fd, QUERY, &user_service);
	//printf("ret from ioctl (find service number) : %d \n", ret);

	//close(fd);
	//printf("fd after close : %d\n", fd);
	return ret;
}

int kbinder_rpc(int snum, int fcode, void *param){
	// succes return 0 , fail return -1
	int ret;
	struct param_buf param_buf = { fcode, param };
	struct rpc_buf buf = { snum, &param_buf };
	
	if(fd == 0){
		printf("First, you must init kbinder please / use kbinder_init()\n");
		return -1;
	}
	
	ret = ioctl(fd, REQ_SERVICE, &buf);
	//close(fd);
	return ret;
}

int kbinder_reg(char *sname){
	// success return service number , fail return -1
	int ret;
	struct serviceInfo user_service;

	//printf("fd : %d \n", fd);
	// fd == 0 is not init
	if(fd == 0) {
		printf("First, you must init kbinder please / use kbinder_init()\n");
		return -1;
	}

	// init user_service
	strcpy(user_service.name, sname);
	user_service.serviceNumber = 0;

	printf("send to ioctl arg info : %d: [%s]\n", user_service.serviceNumber, user_service.name);

	ret = ioctl(fd, REG, &user_service);
	//printf("ret from ioctl (new service number) : %d\n", ret);
	ioctl(fd, LOOKUP_SERVICE_LIST, &user_service);
	
	//close(fd);
	return ret;
}

int kbinder_read(int snum, void *buf){
	// success return 0 , fail return -1
	int ret;
	struct rpc_buf user_service = { snum, buf };

	if(fd == 0) {
	// fd == 0 is not init
		printf("First, you must init kbinder please / use kbinder_init() \n");
		return -1;
	}

	printf("send to ioctl arg info: snum=%d\n", user_service.snum);
	ret = ioctl(fd, READ_REQ, &user_service);

	//close(fd);
	return ret;
}


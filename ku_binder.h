#define IOCTL_START_NUM 0x80
#define IOCTL_NUM1 IOCTL_START_NUM+1
#define IOCTL_NUM2 IOCTL_START_NUM+2
#define IOCTL_NUM3 IOCTL_START_NUM+3
#define IOCTL_NUM4 IOCTL_START_NUM+4
#define IOCTL_NUM5 IOCTL_START_NUM+5

#define SIMPLE_IOCTL_NUM 'z'
#define QUERY _IOWR(SIMPLE_IOCTL_NUM, IOCTL_NUM1, unsigned long)
#define REQ_SERVICE _IOWR(SIMPLE_IOCTL_NUM, IOCTL_NUM2, unsigned long)
#define REG _IOWR(SIMPLE_IOCTL_NUM, IOCTL_NUM3, unsigned long)
#define READ_REQ _IOWR(SIMPLE_IOCTL_NUM, IOCTL_NUM4, unsigned long)
#define LOOKUP_SERVICE_LIST _IOWR(SIMPLE_IOCTL_NUM, IOCTL_NUM5, unsigned long)

#define KBINDER_NAME_MAX 11
#define KBINDER_SNUM_MAX 5
#define KBINDER_PARAM_MAX 20

struct param_buf {
        int fcode;
        char param[KBINDER_PARAM_MAX];
};

struct rpc_buf {
        int snum;
        struct param_buf *param;
};

// node of req_info
struct node {
        struct param_buf *data;
        struct node *next;
};

// queue of req_list 
typedef struct queue {
	struct node *front;
        struct node *rear;
        int count;
}Queue;

struct serviceInfo {
	int serviceNumber;
	char name[KBINDER_NAME_MAX];
};

struct service { 
	struct serviceInfo info;
	Queue req_list;
};

// lib function
int kbinder_init(void);
int kbinder_query(char *sname);
int kbinder_rpc(int snum, int fcode, void *param);
int kbinder_reg(char *sname);
int kbinder_read(int snum, void *buf);



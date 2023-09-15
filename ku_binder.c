#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/sched.h>

#include "ku_binder.h"
MODULE_LICENSE("GPL");
#define DEV_NAME "201811299_dev"

// init queue
void init_queue(Queue *queue) {
	queue->front = queue->rear = NULL;
	queue->count = 0;
}

// check empty of queue 
int empty_check(Queue *queue) {
	return queue->count == 0;
}

// enqueue 
void enqueue(Queue *queue, struct param_buf *data) {
	struct node *now = (struct node*)kmalloc(sizeof(struct node), GFP_KERNEL);
	now->data = data;
	now->next = NULL;

	if (empty_check(queue)) {
		queue->front = now;
	} else {
		queue->rear->next = now;
	}

	queue->rear = now;
	queue->count++;
}

// dequeue
struct param_buf* dequeue(Queue *queue) {
	struct param_buf* tmp;
	struct node *now;
	
	now = queue->front;
	tmp = now->data;
	queue->front = now->next;
	kfree(now);
	queue->count--;
	return tmp;
}


// service list
struct service_list {
	struct list_head list;
	struct service element;
};

struct service_list service_list_head;


spinlock_t my_lock;
static dev_t dev_num;
static struct cdev *cd_cdev;

//count of element in queue each service (-1: no register service /  -1 < n : n is count of element in queue)
int eachQueueCount[5] = {-1, -1, -1, -1, -1};

static int list_count = 0; // how many service in service_list

wait_queue_head_t my_wq;

static long binder_ioctl(struct file *file, unsigned int cmd, unsigned long arg){
	struct service_list* node = 0;
	struct serviceInfo* argService;

	struct rpc_buf* argRpc;

	int successFlag;
	int ret;
	
	switch(cmd){
		case QUERY:
			// first find sname service in service list
			argService = (struct serviceInfo*)arg;
			if (!list_empty(&service_list_head.list)) {
				int serviceNumber = -1; // return value to client
				struct serviceInfo* tmp = (struct serviceInfo*)kmalloc(sizeof(struct serviceInfo), GFP_KERNEL);

				spin_lock(&my_lock);
				ret = copy_from_user(tmp, argService, sizeof(struct serviceInfo));			
				list_for_each_entry(node, &service_list_head.list, list) {
					printk("match %s from node %s",tmp->name, node->element.info.name);
					if (strcmp(node->element.info.name, tmp->name) == 0){
						// If linked list have service that we find return service number
						//printk(" match %s to %s \n ", node->serviceInfo.name, tmp->name);
						//ret = copy_to_user(argService, &(node->serviceInfo), (KBINDER_PARAM_MAX));
						serviceNumber = node->element.info.serviceNumber;
					}
                        			
				}
				spin_unlock(&my_lock);
				// If service list don't have service reutrn -1
				return serviceNumber;
			} else { 
				printk("empty service list\n");
				// If service list is empty return -1
				return -1;
			}
			break;
		case REG:
			// register service of server
			// check count of service in service list
			// if service count more than 5 -> can't register
			argService = (struct serviceInfo*)arg;
			
			if (list_count < 5) {
				node = (struct service_list*)kmalloc(sizeof(struct service_list), GFP_KERNEL);
				
				spin_lock(&my_lock);
				ret = copy_from_user(&(node->element.info), argService, sizeof(struct serviceInfo));
				printk(" copy_from_user result in REG : %d\n", ret);
				node->element.info.serviceNumber = list_count;
				eachQueueCount[list_count] = 0;
				list_count ++;

				// queue init
				init_queue(&node->element.req_list);		
		
				// add to tail
				list_add_tail(&node->list, &service_list_head.list);
				printk("complete register service count: %d \n", list_count);
				spin_unlock(&my_lock);

				return list_count-1;
			} else {
				return -1;
			}
		case REQ_SERVICE:
			successFlag = -1;
			argRpc = (struct rpc_buf*)arg;
			struct rpc_buf* tmp = (struct rpc_buf*)kmalloc(sizeof(struct rpc_buf), GFP_KERNEL);

			spin_lock(&my_lock);
			ret = copy_from_user(tmp, argRpc, sizeof(struct rpc_buf));
			printk("copy from user result in req service : %d\n", ret);
			list_for_each_entry(node, &service_list_head.list, list) {
                                        if (node->element.info.serviceNumber == tmp->snum) {
						// enqueue service request in service what client find
						enqueue(&node->element.req_list, tmp->param);
						eachQueueCount[tmp->snum] ++;
						printk("now service Number : %d queue have %d req \n", node->element.info.serviceNumber, node->element.req_list.count);
						printk("new node value(address) %d\n", node->element.req_list.rear->data);
						successFlag = 0;
                                        }

                        }			
			spin_unlock(&my_lock);
			wake_up(&my_wq);
			return successFlag;
		case READ_REQ:
			// if have service that we find -> sleep in wq (process)
			successFlag = -1; // fail
			argRpc = (struct rpc_buf*)arg;
			struct rpc_buf* paramTmp = (struct rpc_buf*)kmalloc(sizeof(struct rpc_buf), GFP_KERNEL);

			spin_lock(&my_lock);
			ret = copy_from_user(paramTmp, argRpc, sizeof(struct rpc_buf));
			spin_unlock(&my_lock);

			printk("snum : %d \n", paramTmp->snum);
			if(eachQueueCount[paramTmp->snum] == 0) {
				// if queue is empty process go sleep
				wait_event(my_wq, eachQueueCount[paramTmp->snum] > 0);
			}

			spin_lock(&my_lock);
			list_for_each_entry(node, &service_list_head.list, list){
				if(node->element.info.serviceNumber == paramTmp->snum) {
					if(node->element.req_list.count > 0) {
						printk("count of queue : %d \n", node->element.req_list.count);
						// copy_to_user -> send data but this line is error,,,, 
						// ret = copy_to_user(paramTmp->param, &node->element.req_list.front->data, sizeof(struct param_buf));
						dequeue(&node->element.req_list);
						eachQueueCount[paramTmp->snum] --;
						//printk("dequeue param value(address): %d\n", paramTmp->param);
						successFlag = 0;
					}
				}
			}
			spin_unlock(&my_lock);
			return successFlag;
		case LOOKUP_SERVICE_LIST:
			// struct list_head *pos = 0;
			list_for_each_entry(node, &service_list_head.list, list) {
				printk("look up the service list= serviceNumber: %d, serviceName: %s\n", node->element.info.serviceNumber, node->element.info.name);
			}
			return 0;
		default:
			return -2;
	}
}

static int binder_open(struct inode *inode, struct file *file) {
	return 0;
}

static int binder_release(struct inode *inode, struct file *file) {
	return 0;
}

struct file_operations binder_fops = {
	.unlocked_ioctl = binder_ioctl,
	.open = binder_open,
	.release = binder_release
};

static int __init binder_init(void){
	printk("binder: Init Module\n");
	alloc_chrdev_region(&dev_num, 0, 1, DEV_NAME);
	cd_cdev = cdev_alloc();
	cdev_init(cd_cdev, &binder_fops);
	cdev_add(cd_cdev, dev_num, 1);

	// init list, wq, spin_lock
	INIT_LIST_HEAD(&service_list_head.list);
	spin_lock_init(&my_lock);
	init_waitqueue_head(&my_wq);

	return 0;
}

static void __exit binder_exit(void){
	printk("binder: Exit Module \n");
	struct service_list* node = 0;

	struct list_head* pos = 0;
	struct list_head* q = 0;

	// free list
	list_for_each_safe(pos, q, &service_list_head.list){
		node = list_entry(pos, struct service_list, list);
		list_del(pos);
		kfree(node);
	}
	
	cdev_del(cd_cdev);
	unregister_chrdev_region(dev_num, 1);
}

module_init(binder_init);
module_exit(binder_exit);

























#include <linux/init.h>
#include <linux/module.h>
#include <linux/param.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/fcntl.h>
#include <linux/cdev.h>
#include <linux/slab.h>
//#include <linux/8250_pci.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/kobject.h>
#include <linux/device.h>
#include <linux/wait.h>
//#include <linux/device.h>
//#include <linux/kdev_t.h>
#define MAX_SIZE 1024
#define NUM_DEVICES 3
#define BLOCK_COMPLICATED
MODULE_LICENSE("GPL");
struct file_operations scpd_ops;
MODULE_AUTHOR("Wu Kun");
dev_t scpd_dev_t;
int scpd_major=0;
int scpd_minor=0;
struct class* this_class;
struct scpd_dev{

    struct semaphore sem;
    struct cdev cdev;
    void* data;
    int begpos;//first index that stores current data(includes)
    int endpos;//first index to write data(includes)
    int curr_size;
    int size;
    wait_queue_head_t inq;
    wait_queue_head_t outq;

};
struct scpd_dev* scpd_devices;

ssize_t scpd_read(struct file *filp, char __user *buf, size_t count,
                   loff_t *f_pos){
    struct scpd_dev *dev = filp->private_data;
    if (down_interruptible(&dev->sem)){
        return -ERESTARTSYS;
    }
#ifdef BLOCK_COMPLICATED
    while(dev->curr_size==0){
        up(&dev->sem);
        if (filp->f_flags & O_NONBLOCK)
            return -EAGAIN;
        if (wait_event_interruptible(dev->inq,(dev->curr_size!=dev->size))){
            return -ERESTARTSYS;
        }
        if (down_interruptible(&dev->sem))
            return -ERESTARTSYS;
    }
#endif
    if (count>dev->curr_size)
        count=dev->curr_size;
    int err1=0;
    int already_finished=0;
    if (dev->curr_size+dev->begpos>dev->size){//data distributed on two ends of the array

        err1|=copy_to_user(buf,dev->data+dev->begpos,dev->size-dev->begpos);
        if (err1)
        {
             up(&dev->sem);
             return -EFAULT;}
        dev->curr_size-=(dev->size-dev->begpos);
        already_finished+=(dev->size-dev->begpos);
        dev->begpos=0;
    }
    err1|=copy_to_user(buf+already_finished,dev->data+dev->begpos,count-already_finished);
    if (err1)
     {
         up(&dev->sem);
#ifdef BLOCK_COMPLICATED
         wake_up_interruptible(&dev->outq);
#endif
         return -EFAULT;}
    dev->curr_size-=(count-already_finished);
    dev->begpos+=(count-already_finished);
    already_finished+=count-already_finished;

    up(&dev->sem);
#ifdef BLOCK_COMPLICATED
    wake_up_interruptible(&dev->outq);
#endif
    printk(KERN_INFO "SCPD: reading %d characters from SCPD",already_finished);
    return already_finished;

}

ssize_t scpd_write(struct file * filp, const char __user *buf, size_t count, loff_t *f_pos){
    struct scpd_dev *dev = filp->private_data;
    if (down_interruptible(&dev->sem)){
        return -ERESTARTSYS;
    }
    int err1=0;
    int already_finished=0;
#ifdef BLOCK_COMPLICATED
    while(dev->curr_size==dev->size){
        up(&dev->sem);
        if (filp->f_flags & O_NONBLOCK)
            return -EAGAIN;
        if (wait_event_interruptible(dev->outq,(dev->curr_size!=0))){
            return -ERESTARTSYS;
        }
        if (down_interruptible(&dev->sem))
            return -ERESTARTSYS;
    }
#endif
    if (count>dev->size-dev->curr_size)
        count=dev->size-dev->curr_size;

    if (count+dev->endpos>dev->size){//data distributed on two ends of the array

        err1|=copy_from_user(dev->data+dev->endpos,buf,dev->size-dev->endpos);
        if (err1)
        {
             up(&dev->sem);
             return -EFAULT;}
        dev->curr_size+=(dev->size-dev->endpos);

        already_finished+=(dev->size-dev->endpos);
        dev->endpos=0;
    }
    err1|=copy_from_user(dev->data+dev->endpos,buf+already_finished,count-already_finished);
    if (err1)
     {
         up(&dev->sem);

#ifdef BLOCK_COMPLICATED
         wake_up_interruptible(&dev->inq);
#endif
         return -EFAULT;}
    dev->curr_size+=(count-already_finished);

    dev->endpos+=(count-already_finished);
    already_finished+=count-already_finished;
    up(&dev->sem);
#ifdef BLOCK_COMPLICATED
    wake_up_interruptible(&dev->inq);
#endif
    printk(KERN_INFO "SCPD: writing %d characters to SCPD",already_finished);
    return already_finished;


}

int scpd_open(struct inode * inode, struct file * filp){
    //this module is read-only
    struct scpd_dev *dev;
    dev=container_of(inode->i_cdev, struct scpd_dev,cdev);

    filp->private_data=dev;


    return 0;
}
int scpd_release(struct inode *inode, struct file *filp){
    return 0;
}

struct file_operations scpd_ops = {
    .owner = THIS_MODULE,
    .open  = scpd_open,
    .read  = scpd_read,
    .write = scpd_write,
    .release = scpd_release
};
static void scpd_setup_cdev(struct scpd_dev *dev, int index){
    int err, devno=MKDEV(scpd_major,scpd_minor+index);
    cdev_init(&dev->cdev,&scpd_ops);
    dev->cdev.owner=THIS_MODULE;
    dev->cdev.ops=&scpd_ops;
    err = cdev_add(&dev->cdev,devno,1);
    device_create(this_class,NULL,scpd_dev_t+index,NULL,"scpd%d",index);
    //kobject_uevent(&dev->cdev.kobj,KOBJ_ADD);
    printk(KERN_INFO "SCPD: notifying udevd to add device file %d",index);
    if(err)
        printk(KERN_CRIT "SCPD: Error %d: adding cdev %d",err,index );
    else    
	printk(KERN_INFO "SCPD: scpd_setup_cdev successful %d",index);
}


void scpd_exit(void){
    int i=0;
    for(;i<NUM_DEVICES;i++){
	printk(KERN_INFO "SCPD:removing device file %d",i);
	
        device_destroy(this_class,scpd_dev_t+i);
	cdev_del(&scpd_devices[i].cdev);
  
	//kobject_uevent(&scpd_devices[i].cdev.kobj,KOBJ_REMOVE);
	kfree((void*)scpd_devices[i].data);
    }
    class_destroy(this_class);
    unregister_chrdev_region(scpd_dev_t,NUM_DEVICES);
    
    kfree((void*)scpd_devices);
}
static char *scpd_devnode(struct device *dev, umode_t *mode){
    *mode =0666;
    return NULL;
}
static int scpd_init(void){
	printk(KERN_INFO "SCPD:123");
    int result;

    this_class=class_create(THIS_MODULE,"scpd");
    this_class->devnode = scpd_devnode;
    if (IS_ERR(this_class))
        return PTR_ERR(this_class);
    result = alloc_chrdev_region(&scpd_dev_t, scpd_minor, NUM_DEVICES,
                    "/dev/scpd");
	printk(KERN_INFO "SCPD: alloc chrdev region: %d",result);
    scpd_major = MAJOR(scpd_dev_t);
    if (result<0){
	printk(KERN_CRIT "SCPD: cannot alloc chrdev major %d",result);
	return result;}
    printk(KERN_INFO "SCPD: SCPD project use only now loaded with major %d",scpd_major);
    scpd_devices = kmalloc(NUM_DEVICES * sizeof(struct scpd_dev), GFP_KERNEL);
    if (!scpd_devices) {
        result = -ENOMEM;
	printk(KERN_CRIT "SCPD: cannot malloc scpd_devices");
	scpd_exit();
        return -1;}
    memset(scpd_devices, 0, NUM_DEVICES * sizeof(struct scpd_dev));
    printk(KERN_INFO "SCPD: 123");
    int i=0;
    for(;i<NUM_DEVICES;i++){
        scpd_devices[i].data=kmalloc(MAX_SIZE* sizeof(char),GFP_KERNEL);
        scpd_devices[i].size=MAX_SIZE;
        scpd_devices[i].begpos=0;
        scpd_devices[i].endpos=0;
        scpd_devices[i].curr_size=0;
        sema_init(&scpd_devices[i].sem,1);
        init_waitqueue_head(&scpd_devices[i].inq);
        init_waitqueue_head(&scpd_devices[i].outq);
 	//static DECLARE_WAIT_QUEUE_HEAD(scpd_devices[i].inq);
        //static DECLARE_WAIT_QUEUE_HEAD(scpd_devices[i].outq);
        scpd_setup_cdev(&scpd_devices[i], i);

    }
    printk(KERN_INFO "SCPD: SCPD project use only now loaded with major %d",scpd_major);
    return 0;

}
static int hello_init(void)
{

    printk(KERN_ALERT "Hello, world\n");
    return 0;
}

static void hello_exit(void){
    printk(KERN_ALERT "Goodbye, cruel world \n");

}






module_init(scpd_init);
module_exit(scpd_exit);

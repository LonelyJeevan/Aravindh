#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/uaccess.h>  //copy_to/from_user()
#include <linux/gpio.h>     //GPIO
#include <linux/err.h>
#define GPIO_pin (23)
 
dev_t dev = 0;
static struct class *gpio_dev_class;
static struct cdev gpio_led_cdev;
 
static int __init gpio_led_driver_init(void);
static void __exit gpio_led_driver_exit(void);
 
 
static int gpio_led_open(struct inode *inode, struct file *file);
static int gpio_led_release(struct inode *inode, struct file *file);
static ssize_t gpio_led_read(struct file *filp, 
                char __user *buf, size_t len,loff_t * off);
static ssize_t gpio_led_write(struct file *filp, 
                const char *buf, size_t len, loff_t * off);
 
static struct file_operations fops =
{
  .owner          = THIS_MODULE,
  .read           = gpio_led_read,
  .write          = gpio_led_write,
  .open           = gpio_led_open,
  .release        = gpio_led_release,
};

static int gpio_led_open(struct inode *inode, struct file *file)
{
  if(gpio_is_valid(GPIO_pin) == false){
    pr_err("GPIO %d is not valid\n", GPIO_pin);
    device_destroy(gpio_dev_class,dev);
    return -1;
  }
  if(gpio_request(GPIO_pin,"GPIO_pin") < 0){
    pr_err("ERROR: GPIO %d request\n", GPIO_pin);
    gpio_free(GPIO_pin);
    return -1;
  }
  gpio_direction_output(GPIO_pin, 0);
  gpio_export(GPIO_pin, false);
  pr_info("Device File Opened...!!!\n");
  return 0;
}

static int gpio_led_release(struct inode *inode, struct file *file)
{
  gpio_unexport(GPIO_pin);
  gpio_free(GPIO_pin);
  pr_info("Device File Closed...!!!\n");
  return 0;
}

static ssize_t gpio_led_read(struct file *filp, 
                char __user *buf, size_t len, loff_t *off)
{
  uint8_t gpio_state = 0;
  
  gpio_state = gpio_get_value(GPIO_pin);
  
  len = 1;
  if( copy_to_user(buf, &gpio_state, len) > 0) {
    pr_err("ERROR: Not all the bytes have been copied to user\n");
  }
  
  pr_info("Read function : GPIO_pin = %d \n", gpio_state);
  
  return 0;
}

static ssize_t gpio_led_write(struct file *filp, 
                const char __user *buf, size_t len, loff_t *off)
{
  uint8_t rec_buf[10] = {0};
  
  if( copy_from_user( rec_buf, buf, len ) > 0) {
    pr_err("ERROR: Not all the bytes have been copied from user\n");
  }
  
  pr_info("Write Function : GPIO_pin Set = %c\n", rec_buf[0]);
  
  if (rec_buf[0]=='1') {
    gpio_set_value(GPIO_pin, 1);
  } else if (rec_buf[0]=='0') {
    gpio_set_value(GPIO_pin, 0);
  } else {
	  gpio_set_value(GPIO_pin, 0);
    pr_err("Unknown command : Please provide either 1 or 0 \n");
  }
  
  return len;
}

static int __init gpio_led_driver_init(void)
{
  if((alloc_chrdev_region(&dev, 0, 1, "gpio_led_Dev")) <0){
    pr_err("Cannot allocate major number\n");
    goto r_unreg;
  }
  pr_info("Major = %d Minor = %d \n",MAJOR(dev), MINOR(dev));
 
  cdev_init(&gpio_led_cdev,&fops);
 
  if((cdev_add(&gpio_led_cdev,dev,1)) < 0){
    pr_err("Cannot add the device to the system\n");
    goto r_del;
  }
 
  if(IS_ERR(gpio_dev_class = class_create(THIS_MODULE,"gpio_led_class"))){
    pr_err("Cannot create the struct class\n");
    goto r_class;
  }
 
  if(IS_ERR(device_create(gpio_dev_class,NULL,dev,NULL,"gpio_led_device"))){
    pr_err( "Cannot create the Device \n");
    goto r_device;
  }
  
  pr_info("Device Driver Insert...Done!!!\n");
  return 0;
 
r_device:
  device_destroy(gpio_dev_class,dev);
r_class:
  class_destroy(gpio_dev_class);
r_del:
  cdev_del(&gpio_led_cdev);
r_unreg:
  unregister_chrdev_region(dev,1);
  
  return -1;
}

static void __exit gpio_led_driver_exit(void)
{
  device_destroy(gpio_dev_class,dev);
  class_destroy(gpio_dev_class);
  cdev_del(&gpio_led_cdev);
  unregister_chrdev_region(dev,1);
  pr_info("Device Driver Remove...Done!!\n");
}
 
module_init(gpio_led_driver_init);
module_exit(gpio_led_driver_exit);
 
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Aravindh.S");
MODULE_DESCRIPTION("A simple device driver - GPIO Driver");
MODULE_VERSION("1.32");

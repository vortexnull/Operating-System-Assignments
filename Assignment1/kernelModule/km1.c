#include<linux/init.h>
#include<linux/kernel.h>
#include<linux/module.h>

int km1_init(void)
{
    printk(KERN_INFO "Kernel Module Loaded\n");
    return 0;
}

void km1_exit(void)
{
    printk(KERN_INFO "Kernel Module Removed\n");
}

module_init(km1_init);
module_exit(km1_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("HELLO WORLD");
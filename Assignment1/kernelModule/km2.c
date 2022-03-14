#include<linux/init.h>
#include<linux/kernel.h>
#include<linux/module.h>
#include<linux/sched/signal.h>

int km2_init(void)
{
    struct task_struct *task;

    pr_info("ps -el module created\n");

    for_each_process(task){
        if(task->state == TASK_RUNNING)
            pr_info("%6d %10s %40s", task->pid, "R", task->comm);
        else if(task->state == TASK_INTERRUPTIBLE)
            pr_info("%6d %10s %40s", task->pid, "S", task->comm);
        else if(task->state == TASK_UNINTERRUPTIBLE)
            pr_info("%6d %10s %40s", task->pid, "D", task->comm);
        else
            pr_info("%6d %10s %40s", task->pid, "I", task->comm);
    }

    return 0;
}

void km2_exit(void)
{
    pr_info("ps -el module removed\n");
}

module_init(km2_init);
module_exit(km2_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("ps -el module");
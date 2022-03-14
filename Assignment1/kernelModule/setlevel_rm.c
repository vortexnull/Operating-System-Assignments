#include<stddef.h>
#include<stdlib.h>
#include<sys/klog.h>

int main(int argc, char **argv){

    // restoring original console_loglevel
    int level = 4;  
    
    // klogctl is wrapper for syslog system call
    // syslog sets console_loglevel to level
    if(klogctl(8, NULL, level) == -1)
        exit(1);
        
    exit(0);
}
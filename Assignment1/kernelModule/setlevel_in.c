#include<stddef.h>
#include<stdlib.h>
#include<sys/klog.h>

int main(int argc, char **argv){

    // to print messages with log level less than 7 
    int level = 7;  
    
    // klogctl is wrapper for syslog system call
    // syslog sets console_loglevel to level
    if(klogctl(8, NULL, level) == -1)
        exit(1);
        
    exit(0);
}
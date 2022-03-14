#include "types.h"
#include "user.h"
#include "date.h"

int main(int argc, char *argv[]){
    
    if(argc != 2){
        printf(1, "wrong arguments provided\n");
        exit();
    }

    int n = atoi(argv[1]);
    int pid;
    double limit = 1e7;
    double a = 0;

    for(int i = 0; i < n; i++){
        pid = fork();

        if (pid > 0){
            if(pid % 3 == 0)
                chpr(pid, 1);
            else if(pid % 3 == 1)
                chpr(pid, 2);
            else
                chpr(pid, 3);            
        }

        if(pid == 0){
            for(double k = 0.0; k < limit; k+=1.0)
                a += 3.14 * limit * limit;

            exit();
        }
    }

    int priority, timerun, timeready, timeslept;
    int avgtimerun[3], avgtimeready[3], avgtimeslept[3], avgtrnarndtime[3];

    for(int i = 0; i < 3; i++){
        avgtimerun[i] = 0;
        avgtimeready[i] = 0;
        avgtimeslept[i] = 0;
        avgtrnarndtime[i] = 0;
    }

    for(int i = 0; i < n; i++){
        pid = waitnstats(&timerun, &timeready, &timeslept);
        priority = (pid % 3) + 1;

        printf(1, "\npid: %d\npriority: %d\n", pid, priority);
        printf(1, "run time: %d\nready time: %d\nsleep time: %d\n", timerun, timeready, timeslept);
        printf(1, "turnaround time: %d\n\n", timerun + timeready + timeslept);

        avgtimerun[priority - 1] += timerun;
        avgtimeready[priority - 1] += timeready;
        avgtimeslept[priority - 1] += timeslept;
    }

    for(int i = 0; i < 3; i++){
        avgtimerun[i] = avgtimerun[i] / n;
        avgtimeready[i] = avgtimeready[i] / n;
        avgtimeslept[i] = avgtimeslept[i] / n;
        avgtrnarndtime[i] = avgtimerun[i] + avgtimeready[i] + avgtimeslept[i];

        printf(1, "\nPriority: %d\n", i + 1);
        printf(1, "avg run time: %d\navg ready time: %d\navg sleep time: %d\n", avgtimerun[i], avgtimeready[i], avgtimeslept[i]);
        printf(1, "avg turnaround time: %d\n", avgtrnarndtime[i]);
    }

    exit();
}
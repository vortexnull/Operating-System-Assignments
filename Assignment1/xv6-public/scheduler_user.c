#include "types.h"
#include "user.h"
#include "date.h"

#define CPU     0
#define SCPU    1
#define IO      2

int main(int argc, char *argv[]){
    
    if(argc != 2){
        printf(1, "wrong arguments provided\n");
        exit();
    }

    int n = atoi(argv[1]);
    int pid, j, r = 5;
    int limit1 = 1e9, limit2 = 1e4, limit3 = 1e3;
    double a = 0;

    for(int i = 0; i < n; i++){
        pid = fork();

        if(pid == 0){
            j = getpid() % 3;

            if(j == CPU){
                for(int k = 0; k < limit1; k++)
                    a += 3.14 * r * r;
            }
            else if(j == SCPU){
                for(int k = 0; k < limit2; k++){
                    for(int m = 0; m < limit1; m++)
                        a += 3.14 * r * r;

                    yield();
                }
            }
            else{
                for(int k = 0; k < limit3; k++)
                    sleep(1);
            }

            exit();
        }
    }

    int timerun, timeready, timeslept;
    int avgtimerun = 0, avgtimeready = 0, avgtimeslept = 0, avgtrnarndtime = 0;

    for(int i = 0; i < n; i++){
        pid = waitnstats(&timerun, &timeready, &timeslept);

        printf(1, "\npid: %d\n", pid);

        if(pid % 3 == CPU)
            printf(1, "type: %s\n", "CPU");
        else if(pid % 3 == SCPU)
            printf(1, "type: %s\n", "S-CPU");
        else
            printf(1, "type: %s\n", "IO");
        
        printf(1, "run time: %d\nready time: %d\nsleep time: %d\n", timerun, timeready, timeslept);
        printf(1, "turnaround time: %d\n\n", timerun + timeready + timeslept);

        avgtimerun += timerun;
        avgtimeready += timeready;
        avgtimeslept += timeslept;
    }

    avgtimerun = avgtimerun / n;
    avgtimeready = avgtimeready / n;
    avgtimeslept = avgtimeslept / n;
    avgtrnarndtime = avgtimerun + avgtimeready + avgtimeslept;

    printf(1, "Average times:\n", "");
    printf(1, "avg run time: %d\navg ready time: %d\navg sleep time: %d\n", avgtimerun, avgtimeready, avgtimeslept);
    printf(1, "avg turnaround time: %d\n", avgtrnarndtime);

    exit();
}
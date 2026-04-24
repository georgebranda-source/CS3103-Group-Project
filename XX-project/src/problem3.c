#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_JOBS 100

typedef struct {
    char job_id[10];
    int arrival_time;
    int burst_time;
    int priority;
    int completion_time;
    int turnaround_time;
    int waiting_time;
} Job;

void fcfs(Job jobs[], int n) {
    printf("=== FCFS ===\nGantt: ");

    //FCFS Calculation loop & Gantt chart (iterates by job in list):
    int time = 0;
    for (int i = 0; i < n; i++) {
        if (time < jobs[i].arrival_time) {
            time = jobs[i].arrival_time;
        }
        time += jobs[i].burst_time;
        jobs[i].completion_time = time;
        jobs[i].turnaround_time = jobs[i].completion_time - jobs[i].arrival_time;
        jobs[i].waiting_time = jobs[i].turnaround_time - jobs[i].burst_time;
        printf("| %s ", jobs[i].job_id);
    }
    printf("|\n");

    //Prints the time that each job in the Gantt began, followed by the final time of completion
    printf("Time: ");
    for (int i = 0; i < n; i++) {
        printf("%d ", jobs[i].completion_time - jobs[i].burst_time);
    }
    printf("%d", jobs[n-1].completion_time);
    printf("\n\n");
/*
    printf("Job\tAT\tBT\tCT\tTAT\tWT\n");
    double total_tat = 0, total_wt = 0;
    for (int i = 0; i < n; i++) {
        printf("%s\t%d\t%d\t%d\t%d\t%d\n",
               jobs[i].job_id,
               jobs[i].arrival_time,
               jobs[i].burst_time,
               jobs[i].completion_time,
               jobs[i].turnaround_time,
               jobs[i].waiting_time);
        total_tat += jobs[i].turnaround_time;
        total_wt += jobs[i].waiting_time;
    }
    printf("\nAverage Turnaround Time : %.2f\n", total_tat / n);
    printf("Average Waiting Time : %.2f\n", total_wt / n);
    printf("CPU Utilisation : 100.0%%\n");
    */
}

int main() {
    FILE *fp = fopen("./test_cases/q3/case1.txt", "r");
    if (!fp) {
        perror("DISASTER: TEST CASES NOT FOUND");
        return 1;
    }

    Job jobs[MAX_JOBS];
    int n = 0;

    while (fscanf(fp, "%s %d %d %d",
              jobs[n].job_id,
              &jobs[n].arrival_time,
              &jobs[n].burst_time,
              &jobs[n].priority) == 4) {
	printf("Read: %s\n", jobs[n].job_id);
        n++;
    }

    fclose(fp);

    fcfs(jobs, n);

    // TODO: implement Preemptive SJF, RR (q=3,6), MLFQ
    return 0;
}

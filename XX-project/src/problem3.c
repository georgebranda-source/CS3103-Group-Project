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


// --- FCFS calculation ---
void fcfs(Job jobs[], int n, Job results[]) {
    int time = 0;
    for (int i = 0; i < n; i++) {
        if (time < jobs[i].arrival_time) {
            time = jobs[i].arrival_time;
        }
        time += jobs[i].burst_time;

        // copy original fields
        strcpy(results[i].job_id, jobs[i].job_id);
        results[i].arrival_time = jobs[i].arrival_time;
        results[i].burst_time   = jobs[i].burst_time;
        results[i].priority     = jobs[i].priority;

        // computed fields
        results[i].completion_time = time;
        results[i].turnaround_time = results[i].completion_time - results[i].arrival_time;
        results[i].waiting_time    = results[i].turnaround_time - results[i].burst_time;
    }
}

// --- Generic printer ---
void print_schedule(const char *label, Job jobs[], int n) {
    printf("\n=== %s ===\nGantt: ", label);

    // Gantt chart jobs
    for (int i = 0; i < n; i++) {
        printf("| %s ", jobs[i].job_id);
    }
    printf("|\n");

    // Timeline
    printf("Time: ");
    for (int i = 0; i < n; i++) {
        printf("%d ", jobs[i].completion_time - jobs[i].burst_time);
    }
    printf("%d\n\n", jobs[n-1].completion_time);

    // Table
    printf("Job\tArriv.\tBurst\tComp.\tTurn.\tWait.\n");
    double total_tat = 0, total_wt = 0, total_activet = 0;
    for (int i = 0; i < n; i++) {
        printf("%s\t%d\t%d\t%d\t%d\t%d\n",
               jobs[i].job_id,
               jobs[i].arrival_time,
               jobs[i].burst_time,
               jobs[i].completion_time,
               jobs[i].turnaround_time,
               jobs[i].waiting_time);
        total_tat += jobs[i].turnaround_time;
        total_wt  += jobs[i].waiting_time;
	total_activet += jobs[i].burst_time;
    }
    printf("\nAverage Turnaround Time : %.2f\n", total_tat / n);
    printf("Average Waiting Time : %.2f\n", total_wt / n);
    printf("CPU Utilisation : %.1f%%\n\n", (total_activet / jobs[n-1].completion_time) * 100);
}

int main() {
    FILE *fp = fopen("./test_cases/q3/case1.txt", "r");
    if (!fp) {
        perror("DISASTER: TEST CASES NOT FOUND");
        return 1;
    }

    Job jobs[MAX_JOBS];
    Job fcfs_results[MAX_JOBS];
    int n = 0;

    while (fscanf(fp, "%s %d %d %d",
              jobs[n].job_id,
              &jobs[n].arrival_time,
              &jobs[n].burst_time,
              &jobs[n].priority) == 4) {
	//printf("Read: %s\n", jobs[n].job_id);
        n++;
    }

    fclose(fp);

    fcfs(jobs, n, fcfs_results);
    print_schedule("FCFS", fcfs_results, n);

    // TODO: implement Preemptive SJF, RR (q=3,6), MLFQ
    return 0;
}

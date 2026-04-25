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

typedef struct {
    char job_id[10];  // which job ran
    int start_time;   // when it started
    int end_time;     // when it finished
} GanttEntry;


// --- FCFS calculation ---
int fcfs(Job jobs[], int n, Job results[], GanttEntry gantt[]) {
    int time = 0;
    int gcount = 0;  // number of Gantt entries

    for (int i = 0; i < n; i++) {
        if (time < jobs[i].arrival_time) {
            time = jobs[i].arrival_time;
        }

        // copy original fields
        strcpy(results[i].job_id, jobs[i].job_id);
        results[i].arrival_time = jobs[i].arrival_time;
        results[i].burst_time   = jobs[i].burst_time;
        results[i].priority     = jobs[i].priority;

        // record Gantt entry
        gantt[gcount].start_time = time;
        time += jobs[i].burst_time;
        gantt[gcount].end_time = time;
        strcpy(gantt[gcount].job_id, jobs[i].job_id);
        gcount++;

        // computed fields
        results[i].completion_time = time;
        results[i].turnaround_time = results[i].completion_time - results[i].arrival_time;
        results[i].waiting_time    = results[i].turnaround_time - results[i].burst_time;
    }

    return gcount; // return how many entries were filled
}

void print_schedule(const char *label, Job jobs[], int n, GanttEntry gantt[], int gcount) {
    printf("\n=== %s ===\n", label);

    // Gantt chart
    printf("Gantt: ");
    for (int i = 0; i < gcount; i++) {
        printf("| %s ", gantt[i].job_id);
    }
    printf("|\n");

    // Timeline
    printf("Time: ");
    for (int i = 0; i < gcount; i++) {
        printf("%d ", gantt[i].start_time);
    }
    printf("%d\n\n", gantt[gcount-1].end_time);

    // Table of metrics
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

    Job jobs[MAX_JOBS], fcfs_results[MAX_JOBS];
    GanttEntry gantt[MAX_JOBS * 10]; // allow extra for RR/MLFQ
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

    int gcount = fcfs(jobs, n, fcfs_results, gantt);
    print_schedule("FCFS", fcfs_results, n, gantt, gcount);

    // TODO: implement Preemptive SJF, RR (q=3,6), MLFQ
    return 0;
}

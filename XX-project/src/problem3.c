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

int sjf_preemptive(Job jobs[], int n, Job results[], GanttEntry gantt[]) {
    int completed = 0, time = 0, gcount = 0;
    int remaining[MAX_JOBS];
    int is_completed[MAX_JOBS] = {0};

    for (int i = 0; i < n; i++) {
        remaining[i] = jobs[i].burst_time;
    }

    while (completed < n) {
        // find job with shortest remaining time among arrived jobs
        int idx = -1;
        int min_remaining = 1e9;
        for (int i = 0; i < n; i++) {
            if (jobs[i].arrival_time <= time && !is_completed[i] && remaining[i] < min_remaining && remaining[i] > 0) {
                min_remaining = remaining[i];
                idx = i;
            }
        }

        if (idx == -1) {
            // no job available, CPU idle
            time++;
            continue;
        }

        // record Gantt entry if new job or continuation of different job
        if (gcount == 0 || strcmp(gantt[gcount-1].job_id, jobs[idx].job_id) != 0) {
            gantt[gcount].start_time = time;
            strcpy(gantt[gcount].job_id, jobs[idx].job_id);
            gcount++;
        }

        // run job for 1 unit
        time++;
        remaining[idx]--;

        // update end_time of current Gantt slice
        gantt[gcount-1].end_time = time;

        if (remaining[idx] == 0) {
            is_completed[idx] = 1;
            completed++;

            results[idx] = jobs[idx]; // copy original fields
            results[idx].completion_time = time;
            results[idx].turnaround_time = time - jobs[idx].arrival_time;
            results[idx].waiting_time = results[idx].turnaround_time - jobs[idx].burst_time;
        }
    }

    return gcount;
}

int round_robin(Job jobs[], int n, Job results[], GanttEntry gantt[], int quantum) {
    int remaining[MAX_JOBS];
    int time = 0, completed = 0, gcount = 0;
    int is_completed[MAX_JOBS] = {0};

    for (int i = 0; i < n; i++) {
        remaining[i] = jobs[i].burst_time;
    }

    // ready queue indices
    int queue[MAX_JOBS];
    int front = 0, rear = 0;

    // initially enqueue jobs that arrive at time 0
    for (int i = 0; i < n; i++) {
        if (jobs[i].arrival_time <= time) {
            queue[rear++] = i;
        }
    }

    while (completed < n) {
        if (front == rear) {
            // no jobs ready, advance time
            time++;
            for (int i = 0; i < n; i++) {
                if (jobs[i].arrival_time <= time && !is_completed[i] && remaining[i] > 0) {
                    queue[rear++] = i;
                }
            }
            continue;
        }

        int idx = queue[front++];
        if (front == MAX_JOBS) front = 0; // circular queue

        if (remaining[idx] <= 0 || is_completed[idx]) continue;

        // record Gantt entry
        gantt[gcount].start_time = time;
        strcpy(gantt[gcount].job_id, jobs[idx]);

        int run_time = (remaining[idx] < quantum) ? remaining[idx] : quantum;
        time += run_time;
        remaining[idx] -= run_time;

        gantt[gcount].end_time = time;
        gcount++;

        // enqueue newly arrived jobs during this quantum
        for (int i = 0; i < n; i++) {
            if (jobs[i].arrival_time > gantt[gcount-1].start_time &&
                jobs[i].arrival_time <= time &&
                !is_completed[i] && remaining[i] > 0) {
                queue[rear++] = i;
                if (rear == MAX_JOBS) rear = 0;
            }
        }

        // re-enqueue current job if not finished
        if (remaining[idx] > 0) {
            queue[rear++] = idx;
            if (rear == MAX_JOBS) rear = 0;
        } else {
            is_completed[idx] = 1;
            completed++;

            results[idx] = jobs[idx];
            results[idx].completion_time = time;
            results[idx].turnaround_time = time - jobs[idx].arrival_time;
            results[idx].waiting_time = results[idx].turnaround_time - jobs[idx].burst_time;
        }
    }

    return gcount;
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
    int earliest_arrival = jobs[0].arrival_time;
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
        if (jobs[i].arrival_time < earliest_arrival) {
            earliest_arrival = jobs[i].arrival_time;
        }
    }

    double schedule_length = gantt[gcount-1].end_time - earliest_arrival;
    printf("\nAverage Turnaround Time : %.2f\n", total_tat / n);
    printf("Average Waiting Time : %.2f\n", total_wt / n);
    printf("CPU Utilisation : %.1f%%\n\n", (total_activet / schedule_length) * 100.0);
}

int mlfq(Job jobs[], int n, Job results[], GanttEntry gantt[]) {
    int remaining[MAX_JOBS];
    for (int i = 0; i < n; i++) {
        remaining[i] = jobs[i].burst_time;
    }

    int gcount = 0;
    int time = 0;
    int completed = 0;
    int is_completed[MAX_JOBS] = {0};

    // --- Level 1: RR q=4 ---
    while (completed < n) {
        int progress = 0;
        for (int i = 0; i < n; i++) {
            if (!is_completed[i] && jobs[i].arrival_time <= time && remaining[i] > 0) {
                progress = 1;
                gantt[gcount].start_time = time;
                strcpy(gantt[gcount].job_id, jobs[i].job_id);

                int run_time = (remaining[i] < 4) ? remaining[i] : 4;
                time += run_time;
                remaining[i] -= run_time;

                gantt[gcount].end_time = time;
                gcount++;

                if (remaining[i] == 0) {
                    is_completed[i] = 1;
                    completed++;
                    results[i] = jobs[i];
                    results[i].completion_time = time;
                    results[i].turnaround_time = time - jobs[i].arrival_time;
                    results[i].waiting_time = results[i].turnaround_time - jobs[i].burst_time;
                }
            }
        }
        if (!progress) break; // move to next level
    }

    // --- Level 2: RR q=8 ---
    while (completed < n) {
        int progress = 0;
        for (int i = 0; i < n; i++) {
            if (!is_completed[i] && remaining[i] > 0) {
                progress = 1;
                gantt[gcount].start_time = time;
                strcpy(gantt[gcount].job_id, jobs[i].job_id);

                int run_time = (remaining[i] < 8) ? remaining[i] : 8;
                time += run_time;
                remaining[i] -= run_time;

                gantt[gcount].end_time = time;
                gcount++;

                if (remaining[i] == 0) {
                    is_completed[i] = 1;
                    completed++;
                    results[i] = jobs[i];
                    results[i].completion_time = time;
                    results[i].turnaround_time = time - jobs[i].arrival_time;
                    results[i].waiting_time = results[i].turnaround_time - jobs[i].burst_time;
                }
            }
        }
        if (!progress) break; // move to next level
    }

    // --- Level 3: FCFS ---
    for (int i = 0; i < n; i++) {
        if (!is_completed[i] && remaining[i] > 0) {
            gantt[gcount].start_time = time;
            strcpy(gantt[gcount].job_id, jobs[i].job_id);

            time += remaining[i];
            remaining[i] = 0;

            gantt[gcount].end_time = time;
            gcount++;

            is_completed[i] = 1;
            completed++;
            results[i] = jobs[i];
            results[i].completion_time = time;
            results[i].turnaround_time = time - jobs[i].arrival_time;
            results[i].waiting_time = results[i].turnaround_time - jobs[i].burst_time;
        }
    }

    return gcount;
}


int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input_file>\n", argv[0]);
        return 1;
    }

    const char *filepath = argv[1];
    FILE *fp = fopen(filepath, "r");
    if (!fp) {
        perror("FILEPATH TO .txt NOT FOUND. SELF DESTRUCTING...");
        return 1;
    }

    Job jobs[MAX_JOBS], fcfs_results[MAX_JOBS], sjf_results[MAX_JOBS], mlfq_results[MAX_JOBS];
    Job rr3_results[MAX_JOBS], rr6_results[MAX_JOBS];
    GanttEntry gantt_fcfs[MAX_JOBS * 10], gantt_sjf[MAX_JOBS * 50];
    GanttEntry gantt_rr3[MAX_JOBS * 50], gantt_rr6[MAX_JOBS * 50];
    GanttEntry gantt_mlfq[MAX_JOBS * 50];
    int n = 0;

    while (fscanf(fp, "%s %d %d %d",
                  jobs[n].job_id,
                  &jobs[n].arrival_time,
                  &jobs[n].burst_time,
                  &jobs[n].priority) == 4) {
        n++;
    }
    fclose(fp);

    // FCFS
    int gcount_fcfs = fcfs(jobs, n, fcfs_results, gantt_fcfs);
    print_schedule("FCFS", fcfs_results, n, gantt_fcfs, gcount_fcfs);

    // Preemptive SJF
    int gcount_sjf = sjf_preemptive(jobs, n, sjf_results, gantt_sjf);
    print_schedule("Preemptive SJF", sjf_results, n, gantt_sjf, gcount_sjf);

    // Round Robin q=3
    int gcount_rr3 = round_robin(jobs, n, rr3_results, gantt_rr3, 3);
    print_schedule("Round Robin (q=3)", rr3_results, n, gantt_rr3, gcount_rr3);

    // Round Robin q=6
    int gcount_rr6 = round_robin(jobs, n, rr6_results, gantt_rr6, 6);
    print_schedule("Round Robin (q=6)", rr6_results, n, gantt_rr6, gcount_rr6);

    //MLFQ
    int gcount_mlfq = mlfq(jobs, n, mlfq_results, gantt_mlfq);
    print_schedule("MLFQ (RR q=4 → RR q=8 → FCFS)", mlfq_results, n, gantt_mlfq, gcount_mlfq);

    return 0;
}

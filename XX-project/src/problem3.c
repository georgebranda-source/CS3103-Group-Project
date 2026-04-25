/* Group: Group number unknown
   Members: George Branda (gfbranda2, ID: 40172013)
            Chu Wing Hang  (whcchu2, ID: 58668101)
            Jaehaeng HER (jaehaeher3, ID: 55772629)
*/
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
    char job_id[10];
    int start_time;
    int end_time;
} GanttEntry;

/**
 * fcfs: First Come First Serve scheduling algorithm
 * @jobs: input array of jobs
 * @n: number of jobs
 * @results: output array with computed metrics
 * @gantt: output array of Gantt chart entries
 * Returns the number of gantt entries, ie. the number of times the CPU was allocated to a job
 * 
 * Calculates job scheduling results using the FCFS algorithm.
 */
int fcfs(Job jobs[], int n, Job results[], GanttEntry gantt[]) {
    int time = 0, gcount = 0;
    for (int i = 0; i < n; i++) {
        if (time < jobs[i].arrival_time) time = jobs[i].arrival_time;

        // Add job to results and Gantt chart
        strcpy(results[i].job_id, jobs[i].job_id);
        results[i].arrival_time = jobs[i].arrival_time;
        results[i].burst_time = jobs[i].burst_time;
        results[i].priority = jobs[i].priority;

        gantt[gcount].start_time = time;
        time += jobs[i].burst_time;
        gantt[gcount].end_time = time;
        strcpy(gantt[gcount].job_id, jobs[i].job_id);
        gcount++;

        results[i].completion_time = time;
        results[i].turnaround_time = time - results[i].arrival_time;
        results[i].waiting_time = results[i].turnaround_time - results[i].burst_time;
    }
    return gcount;
}

/**
 * sjf_preemptive: Preemptive Shortest Job First scheduling algorithm
 * @jobs: input array of jobs
 * @n: number of jobs
 * @results: output array with computed metrics
 * @gantt: output array of Gantt chart entries
 * Returns the number of Gantt entries, ie. the number of times the CPU was allocated to a job
 * 
 * Calculates job scheduling results using the SJF preemptive algorithm.
 */
int sjf_preemptive(Job jobs[], int n, Job results[], GanttEntry gantt[]) {
    int completed = 0, time = 0, gcount = 0;
    int remaining[MAX_JOBS], is_completed[MAX_JOBS] = {0};
    for (int i = 0; i < n; i++) remaining[i] = jobs[i].burst_time;
    
    while (completed < n) {
        // First, find the job with the shortest remaining time that has arrived and is not completed
        int id = -1, min_remaining = 1000000;
        for (int i = 0; i < n; i++) {
            if (jobs[i].arrival_time <= time && !is_completed[i] &&
                remaining[i] < min_remaining && remaining[i] > 0) {
                min_remaining = remaining[i]; id = i;
            }
        }

        //If a job cannot be found, increment time & skip this iteration
        if (id == -1) { time++; continue; }

        //If it is found, note that it is being processed in the Gantt
        if (gcount == 0 || strcmp(gantt[gcount-1].job_id, jobs[id].job_id) != 0) {
            gantt[gcount].start_time = time;
            strcpy(gantt[gcount].job_id, jobs[id].job_id);
            gcount++;
        }
        time++; remaining[id]--;
        gantt[gcount-1].end_time = time;
        if (remaining[id] == 0) {
            is_completed[id] = 1; completed++;
            results[id] = jobs[id];
            results[id].completion_time = time;
            results[id].turnaround_time = time - jobs[id].arrival_time;
            results[id].waiting_time = results[id].turnaround_time - results[id].burst_time;
        }
    }

    return gcount;
}

/**
 * round_robin: Round Robin scheduling algorithm
 * @jobs: input array of jobs
 * @n: number of jobs
 * @results: output array with computed metrics
 * @gantt: output array of Gantt chart entries
 * @quantum: time quantum
 * Returns the number of Gantt entries, ie. the number of times the CPU was allocated to a job
 * 
 * Calculates job scheduling results using the RR algorithm. Takes as input the time quantum to use.
 */
int round_robin(Job jobs[], int n, Job results[], GanttEntry gantt[], int quantum) {
    int remaining[MAX_JOBS], time = 0, completed = 0, gcount = 0;
    int is_completed[MAX_JOBS] = {0}, queue[MAX_JOBS], front = 0, back = 0;
    for (int i = 0; i < n; i++) remaining[i] = jobs[i].burst_time;
    for (int i = 0; i < n; i++) if (jobs[i].arrival_time <= time) queue[back++] = i;

    //Go until completed jobs=number of jobs ie, all jobs are completed. Populate a queue
    while (completed < n) {
        //If its at the back of the queue, check for incomplete jobs and go again
        if (front == back) {
            time++;
            for (int i = 0; i < n; i++)
                if (jobs[i].arrival_time <= time && !is_completed[i] && remaining[i] > 0)
                    queue[back++] = i;
            continue;
        }

        int id = queue[front++]; if (front == MAX_JOBS) front = 0;
        if (remaining[id] <= 0 || is_completed[id]) continue;
        gantt[gcount].start_time = time;
        strcpy(gantt[gcount].job_id, jobs[id].job_id);

        int run_time = (remaining[id] < quantum) ? remaining[id] : quantum;
        time += run_time; remaining[id] -= run_time;
        gantt[gcount].end_time = time; gcount++;

        // Add all new arrivals
        for (int i = 0; i < n; i++) {
            if (jobs[i].arrival_time > gantt[gcount-1].start_time &&
                jobs[i].arrival_time <= time &&
                !is_completed[i] && remaining[i] > 0) {
                queue[back++] = i; if (back == MAX_JOBS) back = 0;
            }
        }

        // Add incomplete job back to the queue
        if (remaining[id] > 0) {
            queue[back++] = id; if (back == MAX_JOBS) back = 0;
        } else {
            is_completed[id] = 1; completed++;
            results[id] = jobs[id];
            results[id].completion_time = time;
            results[id].turnaround_time = time - jobs[id].arrival_time;
            results[id].waiting_time = results[id].turnaround_time - results[id].burst_time;
        }
    }
    return gcount;
}

/**
 * mlfq: Multi-Level Feedback Queue scheduling
 * @jobs: input array of jobs
 * @n: number of jobs
 * @results: output array with computed metrics
 * @gantt: output array of Gantt chart entries
 * Returns the number of Gantt entries, ie. the number of times the CPU was allocated to a job
 * 
 * Calculates job scheduling results using the MLFQ algorithm.
 */
int mlfq(Job jobs[], int n, Job results[], GanttEntry gantt[]) {
    int remaining[MAX_JOBS];
    for (int i = 0; i < n; i++) {
        remaining[i] = jobs[i].burst_time;
    }

    int gcount = 0;
    int time = 0;
    int completed = 0;
    int is_completed[MAX_JOBS] = {0};

    //1 round of RR with quantum 4
    for (int i = 0; i < n; i++) {
        if (!is_completed[i] && jobs[i].arrival_time <= time && remaining[i] > 0) {
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
                results[i].waiting_time = results[i].turnaround_time - results[i].burst_time;
            }
        }
    }

    //RR with quantum 8
    for (int i = 0; i < n; i++) {
        if (!is_completed[i] && remaining[i] > 0) {
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
                results[i].waiting_time = results[i].turnaround_time - results[i].burst_time;
            }
        }
    }

    //Finally FCFS
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
            results[i].waiting_time = results[i].turnaround_time - results[i].burst_time;
        }
    }

    return gcount;
}


/**
 * print_schedule: Formats and prints the Gantt chart, table, and performance metrics given scheduling results
 * @label: name of algorithm
 * @jobs: array of jobs with computed metrics
 * @n: number of jobs
 * @gantt: array of Gantt chart entries
 * @gcount: number of Gantt entries
 * Returns nothing, but prints the formatted data
 */
void print_schedule(const char *label, Job jobs[], int n, GanttEntry gantt[], int gcount) {
    printf("\n=== %s ===\n", label);
    printf("Gantt: "); for (int i = 0; i < gcount; i++) printf("| %s ", gantt[i].job_id); printf("|\n");
    printf("Time: "); for (int i = 0; i < gcount; i++) printf("%d ", gantt[i].start_time);
    printf("%d\n\n", gantt[gcount-1].end_time);
    printf("Job\tArriv.\tBurst\tComp.\tTurn.\tWait.\n");

    double total_tat = 0, total_wt = 0, total_activet = 0;
    int earliest_arrival = jobs[0].arrival_time;
    for (int i = 0; i < n; i++) {
        printf("%s\t%d\t%d\t%d\t%d\t%d\n", jobs[i].job_id, jobs[i].arrival_time,
               jobs[i].burst_time, jobs[i].completion_time,
               jobs[i].turnaround_time, jobs[i].waiting_time);
               
        total_tat += jobs[i].turnaround_time;
        total_wt  += jobs[i].waiting_time;
        total_activet += jobs[i].burst_time;
        if (jobs[i].arrival_time < earliest_arrival) earliest_arrival = jobs[i].arrival_time;
    }

    double schedule_length = gantt[gcount-1].end_time - earliest_arrival;
    printf("\nAverage Turnaround Time : %.2f\n", total_tat / n);
    printf("Average Waiting Time : %.2f\n", total_wt / n);
    printf("CPU Utilisation : %.1f%%\n\n", (total_activet / schedule_length) * 100.0);
}

/**
 * main
 * @argc: number of command-line arguments
 * @argv: array of command-line arguments (expects input file path at argv[1])
 * Return: 0 on success, non-zero on error
 *
 * Reads job data from a text file, executes multiple scheduling algorithms and prints their Gantt charts and performance data.
 */
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

    int gcount_fcfs = fcfs(jobs, n, fcfs_results, gantt_fcfs);
    print_schedule("FCFS", fcfs_results, n, gantt_fcfs, gcount_fcfs);

    int gcount_sjf = sjf_preemptive(jobs, n, sjf_results, gantt_sjf);
    print_schedule("Preemptive SJF", sjf_results, n, gantt_sjf, gcount_sjf);

    int gcount_rr3 = round_robin(jobs, n, rr3_results, gantt_rr3, 3);
    print_schedule("Round Robin (q=3)", rr3_results, n, gantt_rr3, gcount_rr3);

    int gcount_rr6 = round_robin(jobs, n, rr6_results, gantt_rr6, 6);
    print_schedule("Round Robin (q=6)", rr6_results, n, gantt_rr6, gcount_rr6);

    int gcount_mlfq = mlfq(jobs, n, mlfq_results, gantt_mlfq);
    print_schedule("MLFQ (RR q=4 → RR q=8 → FCFS)", mlfq_results, n, gantt_mlfq, gcount_mlfq);

    return 0;
}
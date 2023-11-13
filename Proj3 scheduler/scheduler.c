#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <limits.h>


// TODO: Add more fields to this struct
struct job {
    int id;
    int arrival;
    int length;
    int length2;
    int preempted;
    int first_run;
    struct job *next;
};

/*** Globals ***/
int seed = 100;
int time;
#define arrLen 20
int response[arrLen];
int wait[arrLen];
int turnaround[arrLen];

//This is the start of our linked list of jobs, i.e., the job list
struct job *head = NULL;

/*** Globals End ***/

void resetAnalysisArrays() {
    for (int i = 0; i < arrLen; i++) {
        response[i] = -1;
        wait[i] = -1;
        turnaround[i] = -1;
    }
}

/*Function to append a new job to the list*/
void append(int id, int arrival, int length) {
    // create a new struct and initialize it with the input data
    struct job *tmp = (struct job *) malloc(sizeof(struct job));

    //tmp->id = numofjobs++;
    tmp->id = id;
    tmp->length = length;
    tmp->arrival = arrival;

    // the new job is the last job
    tmp->next = NULL;

    // Case: job is first to be added, linked list is empty
    if (head == NULL) {
        head = tmp;
        return;
    }

    struct job *prev = head;

    //Find end of list
    while (prev->next != NULL) {
        prev = prev->next;
    }

    //Add job to end of list
    prev->next = tmp;
    return;
}


/*Function to read in the workload file and create job list*/
void read_workload_file(char *filename) {
    int id = 0;
    FILE *fp;
    size_t len = 0;
    ssize_t read;
    char *line = NULL,
            *arrival = NULL,
            *length = NULL;

    //struct job **head_ptr = malloc(sizeof(struct job*));

    if ((fp = fopen(filename, "r")) == NULL)
        exit(EXIT_FAILURE);

    while ((read = getline(&line, &len, fp)) > 1) {
        arrival = strtok(line, ",\n");
        length = strtok(NULL, ",\n");

        // Make sure neither arrival nor length are null.
        assert(arrival != NULL && length != NULL);

        append(id++, atoi(arrival), atoi(length));
    }

    fclose(fp);

    // Make sure we read in at least one job
    assert(id > 0);

    return;
}

//void printJobInfo(int num; int arrive; int time)
void printJobInfo(struct job *job) {
    printf("t=%i: [Job %i] arrived at [%i], ran for: [%i]\n", time, job->id, job->arrival, job->length);
}

void policy_FIFO(struct job *head) {

    printf("Execution trace with FIFO:\n");

    struct job *prev = head;

    while (1) {

        printJobInfo(prev);
        time += prev->length;

        if (prev->next == NULL) break;
        prev = prev->next;

    }

    printf("End of execution with FIFO.\n");

    return;
}

void policy_FIFO_copy(struct job *head){

    time = 0;
    struct job *prev = head;
    int response_time;
    int turnaround_time;
    int wait_time;

    int i = 0;
    while (1) {


        response_time = time - prev->arrival;
        wait_time = response_time;
        

        //printJobInfo(prev);
        time += prev->length;

        turnaround_time = time - prev->arrival;

        response[i] = response_time;
        wait[i] = wait_time;
        turnaround[i] = turnaround_time;

        if (prev->next == NULL) break;
        prev = prev->next;
        i++;

    }

    return;
}

void analyze_FIFO(struct job *head) {
    // TODO: Fill this in
    /*int response_time;
    int turnaround_time;
    int wait_time;*/
    resetAnalysisArrays();
    policy_FIFO_copy(head);


    float averageResponse = 0.0;
    float averageWait = 0.0;
    float averageTurnaround = 0.0;

      struct job *prev = head;
      
      int i;
      for (i = 0; i < arrLen; i++) {
          printf("Job %i -- Response time: %i  Turnaround: %i  Wait: %i\n", prev->id, response[i], turnaround[i], wait[i]);
          averageResponse += response[i];
          averageWait += wait[i];
          averageTurnaround += turnaround[i];
       
          if(prev->next == NULL) {
            break;
          }
          prev = prev->next;
      }
      averageResponse = averageResponse / (i + 1.0);
      averageTurnaround = averageTurnaround / (i + 1.0);
      averageWait = averageWait / (i + 1.0);
      printf("Average -- Response: %0.2f  Turnaround %0.2f  Wait %0.2f\n", averageResponse, averageTurnaround, averageWait);
      resetAnalysisArrays();

    return;
}


void policy_SJF(struct job *head) {
    printf("Execution trace with SJF:\n");
    int total = 0;
    int shortest;
    int idle;

    int response_time;
    int turnaround_time;
    int wait_time;

    struct job *prev = head;

    while (1) {
        total++;

        if (prev->next == NULL) break;
        prev = prev->next;

    }

    prev = head;

    struct job *jobs = malloc(total * sizeof(*jobs));

    int i = 0;
    while (1) {
        //jobs[i] = prev;
        jobs[i].id = prev->id;
        jobs[i].arrival = prev->arrival;
        jobs[i].length = prev->length;
        jobs[i].next = prev->next;

        if (prev->next == NULL) break;

        prev = prev->next;
        i++;

    }

    shortest = 0;
    int counter = 0;
    while (counter < total) {

        idle = 1;
        for (i = 0; i < total; i++) {

            if (i == 0 && counter == 0) {
                idle = 0;
            }

            if (jobs[i].arrival != -1) {
                if (jobs[i].arrival <= time) {
                    if (jobs[i].length < jobs[shortest].length) {
                        shortest = i;
                        idle = 0;
                    }

                }

            }

        }


        if (idle == 1) {
            time++;
            continue;
        }

        printJobInfo(&jobs[shortest]);

        response_time = time - jobs[shortest].arrival;
        wait_time = response_time;

        time += jobs[shortest].length;

        turnaround_time = time - jobs[shortest].arrival;

        response[shortest] = response_time;
        wait[shortest] = wait_time;
        turnaround[shortest] = turnaround_time;

        jobs[shortest].arrival = -1;
        jobs[shortest].length = INT_MAX;
        counter++;

    }
    printf("End of execution with SJF.\n");

}


void analyze_SJF(struct job *head) {
    float averageResponse = 0.0;
    float averageWait = 0.0;
    float averageTurnaround = 0.0;

    struct job *prev = head;

    
    int i;
    for (i = 0; i < arrLen; i++) {
        printf("Job %i -- Response time: %i  Turnaround: %i  Wait: %i\n", prev->id, response[i], turnaround[i], wait[i]);
        averageResponse += response[i];
        averageWait += wait[i];
        averageTurnaround += turnaround[i];
     
        if(prev->next == NULL) {
          break;
        }
        prev = prev->next;
    }
    averageResponse = averageResponse / (i + 1.0);
    averageTurnaround = averageTurnaround / (i + 1.0);
    averageWait = averageWait / (i + 1.0);
    printf("Average -- Response: %0.2f  Turnaround %0.2f  Wait %0.2f\n", averageResponse, averageTurnaround, averageWait);


}

void printJobInfoRR(struct job *job, int run_time) {
    printf("t=%i: [Job %i] arrived at [%i], ran for: [%i]\n", time, job->id, job->arrival, run_time);
}


void policy_RR(struct job *head, int slice_duration) {
  printf("Execution trace with RR:\n");
  int arrived;
  int run;
  int run_time;
  struct job *prev = head;
  int i;
  //int temp_length;
  //time -= 1;


  while(1) {

    prev->length2 = prev->length;

    if(prev->next == NULL) break;
    prev = prev->next;

  }


  while (1) {
    //time++;
    prev = head;
    arrived = 0;
    run = 0;
    i = -1;

    while (1) {
      i++;
      run_time = 0;

      if(prev->preempted != -1) {
        
        run = 1;
        if(prev->arrival <= time) {
          arrived = 1;
          do {

            prev->length--;

            if (prev->length == 0) {

              run_time++;
              prev->preempted = -1;
              //turnaround[i] = time - prev->arrival;

              break;

            }

            prev->preempted = 1;

            run_time++;

          } while(run_time%slice_duration != 0);


          if (!prev->first_run) {
            response[i] = time - prev->arrival;
            prev->first_run = 1;
          }


          printJobInfoRR(prev, run_time);
          time+=run_time;
          turnaround[i] = time - prev->arrival;
          //printf("%i, %i, %i\n",time, prev->arrival, prev->length);
          wait[i] = time - prev->arrival - prev->length2;

        } 

        if(prev->next == NULL) break;
        prev = prev->next;
        
      } else if (prev->next == NULL) {

        break;

      } else {

        prev = prev->next;

      }
      

    }

    if(!run) {
      break;
    } else {
      if(!arrived) time++;
    }

  }

printf("End of execution with RR.\n");
}


void analysis_RR(struct job *head, int slice_duration) {
  printf("Begin analyzing RR:\n");

    float averageResponse = 0.0;
    float averageWait = 0.0;
    float averageTurnaround = 0.0;

    struct job *prev = head;
    
    int i;
    for (i = 0; i < arrLen; i++) {
        printf("Job %i -- Response time: %i  Turnaround: %i  Wait: %i\n", prev->id, response[i], turnaround[i], wait[i]);
        averageResponse += response[i];
        averageWait += wait[i];
        averageTurnaround += turnaround[i];
     
        if(prev->next == NULL) {
          break;
        }
        prev = prev->next;
    }
    averageResponse = averageResponse / (i + 1.0);
    averageTurnaround = averageTurnaround / (i + 1.0);
    averageWait = averageWait / (i + 1.0);
    printf("Average -- Response: %0.2f  Turnaround %0.2f  Wait %0.2f\n", averageResponse, averageTurnaround, averageWait);

    printf("End analyzing RR.\n");

}


int main(int argc, char **argv) {
    time = 0;

    resetAnalysisArrays();

    if (argc < 4) {
        fprintf(stderr, "missing variables\n");
        fprintf(stderr, "usage: %s analysis-flag policy workload-file slice-duration\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int analysis = atoi(argv[1]);
    char *policy = argv[2],
            *workload = argv[3];

    int slice_duration = atoi(argv[4]);

    // Note: we use a global variable to point to
    // the start of a linked-list of jobs, i.e., the job list
    read_workload_file(workload);

    if (strcmp(policy, "FIFO") == 0) {
        policy_FIFO(head);
        if (analysis) {
            printf("Begin analyzing FIFO:\n");
            analyze_FIFO(head);
            printf("End analyzing FIFO.\n");
        }

        exit(EXIT_SUCCESS);
    }

    // TODO: Add other policies

    if (!strcmp(policy, "SJF")) {
        policy_SJF(head);
        if (analysis) {
          printf("Begin analyzing SJF:\n");
          analyze_SJF(head);
          printf("End analyzing SJF.\n");
        }
    }

    if (!strcmp(policy, "RR")) {
        policy_RR(head, slice_duration);
        if (analysis) {
          analysis_RR(head, slice_duration);
        }
    }

    exit(EXIT_SUCCESS);
}

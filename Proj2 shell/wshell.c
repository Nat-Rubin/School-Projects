#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#define MAX_HISTORY 10
#define MAX_JOBS 255



//////////////////////////////
////// GLOBAL VARIABLES //////
//////////////////////////////



static char *history[MAX_HISTORY];
static unsigned history_count = 0;
static char *jobs[MAX_JOBS];
static int job_ids[MAX_JOBS];
static int job_pids[MAX_JOBS];
static int current_job_id = 0;



/////////////////////
////// STRUCTS //////
/////////////////////



struct tokensAndArgs {
	int num_tokens;
	char** argv;

};



/////////////////////////////
////// OTHER FUNCTIONS //////
/////////////////////////////



void printArgv(int argc, char** argv) {
	int i;
	for(i = 0; i < argc; i++) {
		if(i != 0 && argv[i] != NULL) printf(" ");
		if(argv[i] != NULL) printf("%s", argv[i]);
	}
	printf("\n");
}


char* getLLD() {
	char cwd[PATH_MAX];
	if(getcwd(cwd, sizeof(cwd)) != NULL) {
		// trim cwd to last "/"
		int cwdlen = strlen(cwd);
		int lastfsindex;
		for(int i = 0; i < cwdlen; i++) {
			if(cwd[i] == 47) {
				lastfsindex = i;
			}
		}

		int lldlen = cwdlen - lastfsindex;
		char *lld = malloc(lldlen + 1);  // last level directory
		if(lldlen == 1) {
			lld[0] = '/';
			lld[1] = '\0';
			return lld;
		}
		
		for(int i = lastfsindex; i < cwdlen; i++) {
			lld[i - lastfsindex] = cwd[i+1];
		}

		return lld;

	} else {
		return "getcwd() error";
		exit(1);
	}

}


void addToHistory(int argc, char **argv) {
	// concactinate strings
	char input[100];

	strcpy(input, argv[0]);
	for(int i = 1; i < argc-1; i++) {
		strcat(input, " ");
		strcat(input, argv[i]);
	}

	if(history_count < MAX_HISTORY) {
		history[history_count++] = strdup(input);
	} else {
		free(history[0]);
		for(int i = 1; i < MAX_HISTORY; i++) {
			history[i-1] = history[i];
		}
		history[MAX_HISTORY -1] = strdup(input);
	}


}


void addJob(int argc, char **argv, pid_t pid) {
	char input[100];
	int current_job_id_copy;

	strcpy(input, argv[0]);
	for(int i = 1; i < argc-2; i++) {
		strcat(input, " ");
		strcat(input, argv[i]);
	}

	jobs[current_job_id] = strdup(input);
	job_pids[current_job_id] = pid;
	current_job_id_copy = current_job_id;

	current_job_id++;
	job_ids[current_job_id_copy] = current_job_id;
	printf("[%i]\n", current_job_id);
	fflush(stdout);

}



////////////////////////////////
////// BUILT IN FUNCTIONS //////
////////////////////////////////



/// exit ///
void exitFunc() {
	exit(0);
	return;

}


/// echo ///
void echo(char **argv) {
	int i = 1;
	while(argv[i] != NULL) {
		if(i != 1) printf(" ");
		printf("%s", argv[i++]);
	}

	printf("\n");
	return;

}

/// cd ///
void cd(char **argv) {
	char cwd[PATH_MAX];
	char *gdir;
	char *dir;
	char *to;

	// check for no args getenv("HOME")
	if(argv[1] == NULL) {
		chdir(getenv("HOME"));
		return;

	}


	if (argv[2] != NULL){
	// check for too many args
		printf("wshell: cd: too many arguments\n");
		return;

	}


	to = argv[1];


	if(!chdir(to)) {
		return;

	} else {

		gdir = getcwd(cwd, sizeof(cwd));
		dir = strcat(gdir, "/");
		to = strcat(dir, argv[1]);
		if(!chdir(to)) {
			printf("\n");

			return;

		} else {

			printf("wshell: no such directory: %s\n", argv[1]);
			return;

		}

	}

}


/// pwd ///
void pwd(char** argv) {
	char cwd[PATH_MAX];
	char *gdir = getcwd(cwd, sizeof(cwd));
	// Trim off last /
	int cwdlen = strlen(gdir);
	int lastfsindex = 0;
	if(getcwd(gdir, sizeof(gdir)) != NULL) {
		
		for(int i = 0; i < cwdlen; i++) {
			if(gdir[i] == 47) {
				lastfsindex = i;

			}
		}
	}

	char *lld = malloc(cwdlen + 1);  // last level directory
	if(cwdlen == 1) {
		lld[0] = '/';
		lld[1] = '\0';

	} else {

		for(int i = lastfsindex; i < cwdlen; i++) {
			lld[i - lastfsindex] = gdir[i+1];

		}
	}


	printf("%s\n", lld);
	return;
}


/// jobs ///
void displayJobs() {
	for(int i = 0; i < current_job_id; i++) {
		if(job_ids[i] != 0)
			printf("%i: %s\n", job_ids[i], jobs[i]);
	}
}


/// history ///
void displayHistory() {
	for(int i = 0; i < 10; i++) {
		if(history[i] == NULL) break;
		printf("%s\n", history[i]);
	}

	return;
}

/// killJobs ///

void deleteJob(int id) {
	kill(job_pids[id], SIGSEGV);
	job_ids[id] = 0;
	jobs[id] = NULL;
	job_pids[id] = 0;
}

void killJob(char **argv) {
	int found = 0;
	int id = atoi(argv[1]);

	for(int i = 0; i < current_job_id; i++){
		if(id == job_ids[i] && job_ids[i] != 0) {
			found = 1;
			break;
		}
	}
	if(!found) {
		printf("wshell: no such background job: %i\n", id);
		return;
	} else {
		deleteJob(id-1);
	}

}


/// getInput ///
struct tokensAndArgs getInput() {
	struct tokensAndArgs tAA;
	char **argv;
	ssize_t nchars;
	char *lp = NULL;  // line pointer
	char *lpc = NULL;
	char *delim = " \n";
	char *token;
	int num_tokens = 0;
	size_t n = 0;
	int i;

	nchars = getline(&lp, &n, stdin);

	lpc = malloc(sizeof(char) * nchars);
	strcpy(lpc, lp);

	token = strtok(lp, delim);

	while (token != NULL) {
		num_tokens++;
		token = strtok(NULL, delim);
	}
	num_tokens++;

	argv = malloc(sizeof(char *) * num_tokens);

	token = strtok(lpc, delim);

	for(i = 0; token != NULL; i++) {
		argv[i] = malloc(sizeof(char) * strlen(token));
		strcpy(argv[i], token);

		token = strtok(NULL, delim);
	}

	argv[i] = NULL;

	tAA.num_tokens = num_tokens;
	tAA.argv = argv;

	free(lp);
	free(lpc);

	return tAA;
	
}



/// execmd ///
int execmd(int num_tokens, char **argv) {
	char *command = NULL;

	if(argv) {
		command = argv[0];


		if(!strcmp(command, "exit")) {
			exitFunc();

		} else if (!strcmp(command, "echo")) {
			echo(argv);

		} else if (!strcmp(command, "cd")) {
			cd(argv);

		} else if (!strcmp(command, "pwd")) {
			pwd(argv);

		} else if (!strcmp(command, "history")) {
			displayHistory();

		} else if (!strcmp(command, "jobs")) {
			displayJobs();

		} else if (!strcmp(command, "kill")) {
			killJob(argv);

		} else {
			return -1;

		}

	}
	return 0;

}


void doNothing() {
	;
}


void updateJobs() {
	pid_t pid;
	int status;
	int waitpid_val;

	int i;
	for(i = 0; i < current_job_id; i++) {
		if(job_ids[i] != 0){
			pid = job_pids[i];
			waitpid_val = waitpid(pid, &status, WNOHANG);
			if(waitpid_val == pid) {
				printf("[%i] Done: %s\n", job_ids[i], jobs[i]);
				fflush(stdout);
				deleteJob(i);

			}
		}
	}
}



int main(int argc, char **argv) {
	char *command;
	//char *params[50];
	(void) argc;
	int isArgv = 1;
	struct tokensAndArgs tAA;
	char** new;
	pid_t pid;
	int status;

	while(1) {
		// check if user entered command line args
		if(argc > 1 && (isArgv)) {
			new = malloc(argc * sizeof(char*));
			for(int i = 1; i < argc; i++) {
				new[i-1] = argv[i];

			}

			new[argc-1] = NULL;

			tAA.argv = new;
			tAA.num_tokens = argc;

			isArgv = 0;
			goto here;
		}

			char* line = getLLD();
			printf("%s$ ", line);
			fflush(stdout);

			

			tAA = getInput();
			updateJobs();


			here:
			if(tAA.argv[0] == NULL) continue;

			addToHistory(tAA.num_tokens, tAA.argv);

			command = tAA.argv[0];

			char op[3];
			char *current_argv[100];
			char *next_argv[200];
			int no_op;

			int current_command;

			int current_tokens;
			int printed = 1;
			int ampersand;
			int remaining_tokens;

			for(int j = 0; j < tAA.num_tokens; j++) {
				next_argv[j] = tAA.argv[j];

			}
			remaining_tokens = tAA.num_tokens;

		do {
			no_op = 1;  // set to 1 if no op
			current_tokens = 0;
			ampersand = 0;  // no ampersand
			current_command = 0;

			while(1){
				current_tokens++;
				if(next_argv[current_command] == NULL) break;
				if(!strcmp(next_argv[current_command], "&&") || !strcmp(next_argv[current_command], "||")) {
					no_op = 0;
					break;

				}
				current_command++;
			}
			

			if(!no_op) strcpy(op, next_argv[current_command]);
			else *op = '\0';
			current_command++;

			if(!no_op) current_tokens = current_command;

			remaining_tokens = remaining_tokens - (current_tokens);

			int i;
			for(i = 0; i < current_command-1; i++) {
				//strcpy(current_argv[i], next_argv[i]);
				current_argv[i] = next_argv[i];
			}
			current_argv[i] = NULL;

			//Move everything down
			int ccc = current_command;  // current command copy
			int j;
			for(j = 0; j < current_tokens+1; j++) {
				next_argv[j] = next_argv[ccc];
				ccc++;

			}
			next_argv[j] = NULL;

			//
			if(printed){
				char *command2 = NULL;
				command2 = tAA.argv[0];
				if(!isatty(fileno(stdin))) {
					// file
					if (tAA.argv[1] == NULL) {
						printf("%s\n", command2);

					} else {
						printArgv(tAA.num_tokens, tAA.argv);
					}
				}
				
				fflush(stdout);
			}
			printed = 0;

			// background task
			if(!strcmp(current_argv[current_tokens-2], "&")) {
				ampersand = 1;  // yes ampersand

				current_argv[current_tokens-2] = '\0';
			}


			int success = execmd(current_tokens, current_argv);

			if(!success){
				continue;
			}


			//if(!strcmp("false", command)) break; //*op = '\0';
			// false: returns a non 0 return code, execvp succeeds
			// child executes
			// entire process finishes - denotes success (return code)
			// exec doesn't return but false will give it a return code
			// get value of child process return code : status by calling wait
			// code in lec 3 wait_pid

			// waitpid with nohang
			// store process ids in array with other info
			// iterate with waitpid and see processes


			pid = fork();
			int kill_status = -1;
			if(!pid) {

				if(execvp(command, current_argv) == -1) {
					printf("wshell: could not execute command: %s\n", command);
					if(!strcmp(op, "&&")) break;
					else if(!strcmp(op, "||")) {

						continue;

					} else {

						kill_status = 0;

					}

				}

				exit(1);

			} else if (pid > 0) {
				if(!ampersand) {
					do {
						//updateJobs();
						waitpid(pid, &status, WUNTRACED);
						if(WIFEXITED(status)) {
							if(status != 0) {
								if(!strcmp(op, "||")){
									;
								} else {
									*op = '\0';
									break;
								}
							} else if (!status) {
								if(!strcmp(op, "||")) {
									kill_status = 0;
								}
							}
						}
						updateJobs();

					} while (!WIFEXITED(status) && !WIFSIGNALED(status));

				} else {
					int added = 0;
					waitpid(pid, &status, WNOHANG);
					if(!added){
						addJob(current_tokens, current_argv, pid);
						added = 1;
					}
				}

			}

			if(!kill_status) {
				*op = '\0';
			}

		} while (op[0] != '\0');


		
	}
	return 0;

}

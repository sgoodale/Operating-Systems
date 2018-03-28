#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <errno.h>
#include <signal.h>

#define DIR_LENGTH 1000
#define COMM_LENGTH 1024
#define CHANGE_DIR "cd"
#define EXIT_FLAG "exit"
#define DELIMITER " "
#define BACKGROUND "bg"
#define BACKGROUND_LIST "bglist"


/*******************************************************
					LINKED LIST
*******************************************************/

//linked list for background process
typedef struct bg_proc {
    pid_t pid;
    char command[COMM_LENGTH];
    struct bg_proc* next;
} bg_proc;

//print linked list
void print_list(bg_proc** root) {
    bg_proc* curr = (*root);
    while (curr->next != NULL) {
        printf("%d: %s\n", curr->pid, curr->command);
        curr = curr->next;
    }
    free(curr);
    return;
}

//print linked list
char* get_command(bg_proc** root, int pid) {
    bg_proc* curr = (*root);
    while (curr != NULL) {
        if (curr->pid == pid) {
            return(curr->command);
        }
        curr = curr->next;
    }
    free(curr);
    return;
}

//add_list to front of linked list
void add_list(bg_proc** root, int pid, char* command) {
    bg_proc* new_node = malloc(sizeof(bg_proc));
    new_node->pid = pid;
    strcpy(new_node->command, command);
    new_node->next = (*root);
    (*root) = new_node;
    return;
}

//remove given pid from list
void remove_list(bg_proc** root, int pid) {
    //check root is not null
    if ((*root) == NULL) {
        return;
    } 
	//traverse list to find value to remove
    bg_proc* curr = (*root);    
    bg_proc* prev = NULL;
    while (curr->pid != pid && curr->next != NULL) {
        prev = curr;
        curr = curr->next;
    }
    //remove current value
    if (curr->pid == pid) {
        //if curr is in middle of list
        if (prev != NULL) {
            prev->next = curr->next;
        //if curr is root
        } else {
            (*root) = curr->next;
        }
        free(curr);   
    }
    return;
}

//kill all child processes
void kill_children(bg_proc** root) {
	//traverse list
    bg_proc* curr = (*root);
	bg_proc* prev;
    while (curr->next != NULL) {
        //kill process
        kill(curr->pid, SIGKILL);
		printf("Killed %d: %s\n", curr->pid, curr->command);
        //remove root
        prev = curr;
		curr = curr->next;
		free(prev);
    }
    return;
}


/*******************************************************
					SHELL FUNCTIONS
*******************************************************/

//get current directory
void get_curr_directory(char* curr_dir) {
    //if getcwd returns null, exit
	if (!getcwd(curr_dir, DIR_LENGTH)) {
		printf("Problem fetching directory\n");
		exit(0);
	}
    return;
}

//update prompt
void update_prompt(char* prompt) {
    //get current directory
	char* curr_dir = malloc(sizeof(char)*DIR_LENGTH);
	get_curr_directory(curr_dir);
    //update prompt string
	snprintf(prompt, DIR_LENGTH+10, "SSI: %s > ", curr_dir);
    free(curr_dir);
    return;	
}

//parse given string into array of words; return number of words in array
int parse(char* string, char** parsed) {
    //parse string into words
    parsed[0] = strtok(string, " \n");
    int i = 0;
    while (parsed[i] != NULL) {
        parsed[i+1] = strtok(NULL, " \n");
        i++;
    }
	return(i);
}

//change directories
void change_directory(char** parsed, int num_words, char* prompt) {
    //more than 2 parameters: invalid command
    if (num_words > 2) {
        printf("Invalid command\n");
        return;  
    //1 parameter or special char "~": change directory to home
    } else if (num_words == 1 || strcmp(parsed[1], "~") == 0) {
        if (chdir(getenv("HOME")) != 0) {
            printf("Directory change failed\n");
        }
    //2 parameters: move to given directory
    } else {
        if (chdir(parsed[1]) != 0) {
            printf("Directory change failed\n");
        }
    } 
    //update prompt
    update_prompt(prompt);
    return;
}

void fork_process(bg_proc** root, char** parsed, int bg_flag, char* user_input) {

    int pid = fork();
    //error forking
    if (pid < 0) {
        printf("Problem forking\n"); 
    //if child process
    } else if (pid == 0) {
        //execute command
        if (execvp(parsed[0], parsed) == -1) {
            printf("Problem executing command\n");
        }
    //if parent process
    } else {
        //if background process
        if (bg_flag == 1) {
            //add pid to list
            add_list(root, pid, user_input);
        } else {
            //wait for forefront process to finish
            pid_t completed = wait(NULL);
            if (completed < 0) {
                printf("Problem waiting...");
            }
        }
		//delay parent
        usleep(15000);
    }
    return;
}


/*******************************************************
						MAIN
*******************************************************/

//main function to run shell
int main() {
	
	//initialize linked list to store background processes
    bg_proc* root = malloc(sizeof(bg_proc));

    printf("\nWelcome to the Simple Shell Interpreter.\n"
    "Type 'exit' at any time to exit the program.\n\n");
	
	//update prompt
	char* prompt = malloc(sizeof(char)*(DIR_LENGTH+10));
	update_prompt(prompt);
    
	//exit flag
	int exit_ssi = 0;
	//while user doesn't exit program
	while (!exit_ssi) {
        
        //read user input
		char* user_input = malloc(sizeof(char)*COMM_LENGTH);
		user_input = readline(prompt);
		char* user_input_cpy = malloc(sizeof(char)*COMM_LENGTH);
        strcpy(user_input_cpy, user_input);
        
        //check if any child has terminated
        if (root != NULL) {
            pid_t completed = waitpid(0, NULL, WNOHANG);
			//while any processes have finished
            while (completed > 0) {
                printf("%d: '%s' has terminated\n", completed, get_command(&root, completed));
				//remove finished process from list
                remove_list(&root, completed);
                completed = waitpid(0, NULL, WNOHANG);
            }
        }
		
        if (strcmp(user_input, "") != 0) {
            //parse user input string
			char* parsed[strlen(user_input)];
            int num_words = parse(user_input, parsed);
            //if user input reads 'exit', exit program
            if (!strcmp(parsed[0], EXIT_FLAG)) {
                exit_ssi = 1;
            } else {
                //change directory
                if (strcmp(parsed[0], CHANGE_DIR) == 0) {
                    change_directory(parsed, num_words, prompt);
                //background process
                } else if (strcmp(parsed[0], BACKGROUND) == 0) {
                    fork_process(&root, &parsed[1], 1, user_input_cpy);
                //background process
                } else if (strcmp(parsed[0], BACKGROUND_LIST) == 0) {
                    print_list(&root);
                //all other commands
                } else {
                    fork_process(&root, parsed, 0, user_input_cpy);
                }
            }
        }
		free(user_input_cpy);   
		free(user_input); 
    }
	//kill running processes
    kill_children(&root);
	printf("\nGoodbye\n\n");
	free(prompt);
	return(0);
}
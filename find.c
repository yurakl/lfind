#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <limits.h>

#define FNLEN 512

// List of Directories
struct file_entry {
    char name[FNLEN]; 
    struct file_entry *prev; 
    struct file_entry *next;
};

//Multithreading variables
int n_threads = 0;
int num_threads = 0;
int search_files = 1;
int search_folders = 1;
unsigned int num_files = UINT_MAX;
unsigned int n_files = 0;
char root_folder[FNLEN] = "/";
char file[FNLEN];



struct thread_args {
    char *arg1;
    char *arg2;
};

pthread_mutex_t mutex;

struct file_entry * pop_first(struct file_entry * head)
{  
	if (head->next) { 
		head = head->next;  
		free(head->prev);
		head->prev = NULL;		
	} else {
		free(head);  
		head = NULL;
	}  
	return head;
}

void *list_folder_contents(void *arg)
{

	struct thread_args *args = (struct thread_args *) arg;
	char *path = args->arg1;
	char *filename = args->arg2;
 
	DIR *dir;
	struct dirent *entry;
	

	dir = opendir(path);
	if (!dir) {
		perror("opendir");
		return NULL;
	}
	
	struct file_entry *head = (struct file_entry *) malloc(sizeof(struct file_entry));
	struct file_entry *tail;  
	
	strcpy(head->name, path);
	if (head->name[strlen(head->name - 1)] != '/') {
		strcat(head->name, "/");		
	}
	head->prev = NULL;
	head->next = NULL;
	
	tail = head;  
	closedir(dir);
	char parent[FNLEN];
	
	while (head)
	{  
		dir = opendir(head->name);
		if (dir) {
			memset(parent, 0, FNLEN);
			strcpy(parent, head->name);  
			while ((entry = readdir(dir)) != NULL) {
				
				pthread_mutex_lock(&mutex);
				if (n_files >= num_files) {
						pthread_mutex_unlock(&mutex);
						return NULL;							
				}
				pthread_mutex_unlock(&mutex);

				if (entry->d_type == DT_REG && search_files) {
					
					if (strcmp(entry->d_name, filename) == 0) {
						printf("%s%s\n", parent, entry->d_name);
						pthread_mutex_lock(&mutex);
						n_files++;
						pthread_mutex_unlock(&mutex);
						
					}
					
				} else if (entry->d_type == DT_DIR && strcmp(entry->d_name, "..") && strcmp(entry->d_name, ".")) {

					if (search_folders) {
						if (strcmp(entry->d_name, filename) == 0) {
							printf("%s%s\n", parent, entry->d_name);
							pthread_mutex_lock(&mutex);
							n_files++;
							pthread_mutex_unlock(&mutex);
						}
					}

					tail->next = (struct file_entry *)malloc(sizeof(struct file_entry));
					if (!tail->next) {
						perror("malloc");
						break;
					}
					tail->next->prev = tail;
					tail = tail->next;
					tail->next = NULL;
					memset(tail->name, 0, FNLEN);
					strcpy(tail->name, parent); 
					strcat(tail->name, entry->d_name); 
					strcat(tail->name,"/");
				}
				
			} 
			closedir(dir); 
		}

		head = pop_first(head); 
		
		if (n_threads < num_threads && head) {
			struct thread_args *args = (struct thread_args *) malloc(sizeof(struct thread_args));
			args->arg1 = (char *) malloc(strlen(head->name) + 1);
			strcpy(args->arg1, head->name);
			args->arg2 = (char *) malloc(strlen(filename) + 1);
			strcpy(args->arg2, filename);
			pthread_t thread;
			pthread_create(&thread, NULL, list_folder_contents, args);
			head = pop_first(head);
			pthread_mutex_lock(&mutex);
			n_threads++;
			pthread_mutex_unlock(&mutex);			

		}
		
		
	} 
	pthread_mutex_lock(&mutex);
	n_threads--;
	pthread_mutex_unlock(&mutex);
	return NULL;
}

int main(int argc, char *argv[]) 
{
	int minima = 0;
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-i") == 0) {
			minima++;
			if (i + 1 < argc) {
				memset(file, 0, FNLEN);
				strcpy(file, argv[i + 1]);
				i++;
			} else {
				printf("Error: -i option requires a value.\n");
				return 1;
			} 
		} else if (strcmp(argv[i], "-j") == 0) {
			if (i + 1 < argc) {
				num_threads = atoi(argv[i + 1]);
				i++;
			} else {
				printf("Error: -j option requires a value.\n");
				return 1;
			}
		} else if (strcmp(argv[i], "-f") == 0) {
			search_files = 1;
			search_folders = 0;
		} else if (strcmp(argv[i], "-d") == 0) {
			search_files = 0;
			search_folders = 1;
		} else if (strcmp(argv[i], "-n") == 0) {
			if (i + 1 < argc) {
				num_files = atoi(argv[i + 1]);
				i++;
			} else {
				printf("Error: -n option requires a value.\n");
				return 1;
			}
		} else if (strcmp(argv[i], "-r") == 0) {
			if (i + 1 < argc) {
				memset(root_folder, 0, FNLEN);
				strcpy(root_folder, argv[i + 1]);
				i++;
			} else {
				printf("Error: -r option requires a value.\n");
				return 1;
			}
		} else if (strcmp(argv[i], "-help") == 0) {
			printf("Usage: %s [options] [file_name]\n", argv[0]);
			printf("Options:\n");
			printf("  -i name    Name of file or folder to find\n");
			printf("  -j num     Number of threads\n");
			printf("  -f         Search for files\n");
			printf("  -d         Search for folders\n");
			printf("  -n num     Number of files to find\n");
			printf("  -r path    Root folder\n");
		return 0;
		} else {
			printf("Error: Unknown option or argument '%s'\n", argv[i]);
			printf("Use -help option for usage information.\n");
			return 1;
		}
	}
	if (minima == 0) {
		printf("Error: no target specified\n");
		printf("Use -help option for usage information.\n");
		return 1;
	}

	/*
	printf("Number of threads: %d\n", num_threads);
	printf("Search files: %d\n", search_files);
	printf("Search folders: %d\n", search_folders);
	printf("Number of files to find: %d\n", num_files);
	printf("Root folder: %s\n", root_folder);
	printf("File: %s\n", file);
	*/
	//clock_t start_time = clock();

	struct thread_args arg = {root_folder, argv[2]};

	pthread_mutex_init(&mutex, NULL);

	list_folder_contents(&arg);
	//clock_t end_time = clock();

	//double execution_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;

    	//printf("Execution time: %f seconds\n", execution_time);

	return 0;
}

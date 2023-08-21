#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>

#define FNLEN 512

// Структура для представлення елемента списку
struct file_entry {
    char name[FNLEN]; 
    struct file_entry *prev; 
    struct file_entry *next;
};

//Multithreading variables
int max_threads = 0;
int n_threads = 0;

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

	pthread_t thread_id = pthread_self();
	printf("Enter Thread ID: %lu\t%s\n", thread_id, path);
	
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
		sleep(1);
		dir = opendir(head->name);
		if (dir) {
			memset(parent, 0, FNLEN);
			strcpy(parent, head->name);  
			while ((entry = readdir(dir)) != NULL) {
				
				if (entry->d_type == DT_REG) {
					
					if (strcmp(entry->d_name, filename) == 0) {
						printf("File: %s%s\n", parent, entry->d_name);
					}
					
				} else if (entry->d_type == DT_DIR && strcmp(entry->d_name, "..") && strcmp(entry->d_name, ".")) {
					
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
		
		if (n_threads < max_threads && head	) {
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
	clock_t start_time = clock();
	if (argc < 3) {
		printf("Usage: %s folder_path filename\n", argv[0]);
		return 1;
	}

	struct thread_args arg = {argv[1], argv[2]};

	pthread_mutex_init(&mutex, NULL);

	list_folder_contents(&arg);
	clock_t end_time = clock();

	double execution_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;

    	printf("Execution time: %f seconds\n", execution_time);

	return 0;
}

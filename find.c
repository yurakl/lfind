#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>

#define FNLEN 512
// Структура для представлення елемента списку
struct file_entry {
    char name[FNLEN]; 
    struct file_entry *prev; 
    struct file_entry *next;
};

// Функція для виводу вмісту папки за допомогою звичайного списку
void list_folder_contents(const char *path, const char *filename) 
{
	DIR *dir;
	struct dirent *entry;
	

	dir = opendir(path);
	if (!dir) {
		perror("opendir");
		return;
	}
	
	struct file_entry *head = (struct file_entry *) malloc(sizeof(struct file_entry));
	struct file_entry *tail;  
	
	strcpy(head->name, path);
	if (head->name[strlen(head->name - 1)] != '/') {
		strcat(head->name, "/");		
	}
	
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
			
			if (head->next) {
				head = head->next; 
				free(head->prev);
				
			}
			else {
				free(head); 
				head = NULL;
			} 
		}
	}
}

int main(int argc, char *argv[]) 
{
	if (argc < 3) {
		printf("Usage: %s folder_path filename\n", argv[0]);
		return 1;
	}

	const char *folder_path = argv[1];
	const char *filename = argv[2];
	list_folder_contents(folder_path, filename);

	return 0;
}

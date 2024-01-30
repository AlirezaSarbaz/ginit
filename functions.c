#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#define MAX_ADDRESS_LENGTH 1024
#define MAX_LINE_LENGTH 1024

int check_ginit_exist();
void run_init();
void copy_file_source_to_dest(FILE* dest , FILE* source);

int exist_file_or_dir(const char* filepath);
void list_files_recursively(const char* basePath , const char* filename , int depth);

int is_track(char* pathspec);

int check_ginit_exist() {
    char cwd[MAX_ADDRESS_LENGTH];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("getcwd\n");
        exit(EXIT_FAILURE);
    }
    char tmp_cwd[MAX_ADDRESS_LENGTH];
    bool exists = false;
    struct dirent *entry;
    do {
        DIR *dir = opendir(".");
        if (dir == NULL) {
            perror("Error opening current directory");
            exit(EXIT_FAILURE);
        }
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_type == DT_DIR && !strcmp(entry->d_name, ".ginit"))
                exists = true;
        }
        closedir(dir);
        if (getcwd(tmp_cwd, sizeof(tmp_cwd)) == NULL) exit(EXIT_FAILURE);
        if (strcmp(tmp_cwd, "/")) {
            if (chdir("..")) exit(EXIT_FAILURE);
        }
    } while (strcmp(tmp_cwd, "/"));
    if (chdir(cwd)) {
        perror("chdir\n");
        exit(EXIT_FAILURE);
    }
    if (exists) return 1;
    else return 0;
}
void run_init() {
    mkdir(".ginit" , 0755);
    char cwd[MAX_ADDRESS_LENGTH];
    getcwd(cwd , sizeof(cwd));
    FILE* local_config = fopen(".ginit/config" , "wb");
    const char* home = getenv("HOME"); chdir(home);
    FILE* global_config = fopen(".ginitconfig" , "rb");
    copy_file_source_to_dest(local_config , global_config);
    chdir(cwd);
    FILE* file = fopen(".ginit/staging" , "w"); fclose(file);
    file = fopen(".ginit/tracks" , "w"); fclose(file);
    //file = fopen(".ginit/tracks" , "w"); fclose(file);
    //file = fopen(".ginit/HEAD" , "w"); fclose(file);
}
void copy_file_source_to_dest(FILE* dest , FILE* source) {
    char buffer[MAX_LINE_LENGTH];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), source)) > 0) {
        fwrite(buffer, 1, bytes_read, dest);
    }
    fclose(dest); fclose(source);
}
void run_add(int argc, char *const argv[]) {
    // TODO: handle command in non-root directories 
    if (argc < 3) {
        perror("please specify a file or directory\n");
        exit(EXIT_FAILURE);
    }
    add_to_staging(argc , argv);
}
int add_to_staging(int argc , char* argv[]) {
    if (argc == 3 && !strcmp(argv[2] , "-redo")) {
        // دستور redo
    }
    else if (argc == 3) {
        if (exist_file_or_dir(argv[2]) == -1) {
            fprintf(stderr,"pathspec \"%s\" dosen't match any file\n" , argv[2]);
            exit(EXIT_FAILURE);
        }
        else if (exist_file_or_dir(argv[2]) == DT_REG) {
            if (!is_track(argv[2])) {
                FILE* file = fopen(".ginit/tracks" , "a");
                fprintf(file , "%s 1\n" ,argv[2]);
                fclose(file);
            }
        }
        else if (exist_file_or_dir(argv[2]) == DT_DIR) {
            if (!is_track(argv[2])) {
                FILE* file = fopen(".ginit/tracks" , "a");
                fprintf(file , "%s 1\n" ,argv[2]); fclose(file);
                char path[MAX_ADDRESS_LENGTH]; char cwd[MAX_ADDRESS_LENGTH];
                getcwd(cwd , sizeof(cwd));
                sprintf(path , "%s/%s" , cwd , argv[2]);
                list_files_recursively(path , ".ginit/tracks" , 2);
            }
        }
    }
    return 0;
}
int exist_file_or_dir(const char* pathspec) {
    struct dirent* entry;
    DIR* dir = opendir(".");
    if (dir == NULL) {
        perror("Error opening directory");
        exit(EXIT_FAILURE);
    }
    while ((entry = readdir(dir)) != NULL) {
        if (!strcmp(entry->d_name , pathspec)) {
            closedir(dir);
            return entry->d_type;
        }
    }
    closedir(dir);
    return -1;
}
void list_files_recursively(const char* basePath , const char* filename , int depth) {
    char path[MAX_ADDRESS_LENGTH];
    struct  dirent* entry;
    DIR* dir = opendir(basePath);
    if (dir == NULL) {
        perror("Error opening directory");
        exit(EXIT_FAILURE);
    }   
    while ((entry = readdir(dir)) != NULL) {
        FILE* file = fopen(filename , "a");
        if (entry->d_type == DT_DIR) {
            if (strcmp(entry->d_name , ".") && strcmp(entry->d_name , "..")) {
                sprintf(path , "%s/%s" , basePath , entry->d_name);
                fprintf(file , "%s %d\n" , entry->d_name , depth);
                list_files_recursively(path , filename , depth + 1);
            }
        }
        else {
            fprintf(file , "%s %d\n" , entry->d_name , depth);
        }
        fclose(file);
    }
    closedir(dir);
}
int is_track(char* pathspec) {
    FILE* file= fopen(".ginit/tracks" , "r+");
    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), file)) {
        char *found = strstr(line, pathspec);
        if (found != NULL) {
            return 1;
        }
    }
    fclose(file);
    return 0; 
}
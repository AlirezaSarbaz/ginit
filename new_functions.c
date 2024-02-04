#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <ctype.h>

int check_ginit_exist();
void copy_file_source_to_dest(FILE* dest , FILE* src);
void run_init();
void run_config(char* argv[]);
int is_in_a_ref_file(char* pathspec , char* ref_filename);
void list_files_recursively(char* basePath , char* filename , int depth , int l);
int is_exist_directory_or_file(char path[]);
void add_to_tracks_and_stages(int argc , char* argv[]);
void run_add(int argc, char* argv[]);
int check_files_modified(char* file_path);
void update_modified();
void update_added();
void update_deleted();
void update_tracks();
void update_stages();
void run_status();
int is_file_empty(char *filename);

int check_ginit_exist() {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("getcwd\n");
        exit(EXIT_FAILURE);
    }
    char tmp_cwd[1024];
    bool exists = false;
    struct dirent *entry;
    do {
        DIR *dir = opendir(".");
        if (dir == NULL) {
            perror("Error opening current directory");
            exit(EXIT_FAILURE);
        }
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_type == 4 && !strcmp(entry->d_name, ".ginit"))
                exists = true;
        }
        closedir(dir);
        if (getcwd(tmp_cwd, sizeof(tmp_cwd)) == NULL) exit(EXIT_FAILURE);
        if (strcmp(tmp_cwd, "/") && chdir("..")) {
            exit(EXIT_FAILURE);
        }
    } while (strcmp(tmp_cwd, "/"));
    if (chdir(cwd)) {
        perror("chdir\n");
        exit(EXIT_FAILURE);
    }
    if (exists) return 1;
    else return 0;
}
void copy_file_source_to_dest(FILE* dest , FILE* src) {
    char buffer[1024];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), src)) > 0) {
        fwrite(buffer, 1, bytes_read, dest);
    }
    fclose(dest); fclose(src);
}
void run_init() {
    mkdir(".ginit" , 0755);
    char cwd[1024];
    getcwd(cwd , sizeof(cwd));
    FILE* local_config = fopen(".ginit/config" , "wb");
    if (local_config == NULL) {
        perror("error opening local_config file\n");
        exit(EXIT_FAILURE);
    }
    char* home = getenv("HOME"); chdir(home);
    FILE* global_config = fopen(".ginitconfig" , "rb");
    if (global_config == NULL) {
        perror("error opening global_config file\n");
        exit(EXIT_FAILURE);
    }
    copy_file_source_to_dest(local_config , global_config);
    chdir(cwd);
    mkdir(".ginit/refs" , 0755); mkdir(".ginit/commits" , 0755); mkdir(".ginit/branches" , 0755);
    FILE* file = fopen(".ginit/refs/stages" , "w"); fclose(file); file = fopen(".ginit/refs/tracks" , "w"); fclose(file); file = fopen(".ginit/refs/modified" , "w"); fclose(file);
    file = fopen(".ginit/time" , "w"); fclose(file); file = fopen(".ginit/refs/deleted" , "w"); fclose(file); file = fopen(".ginit/refs/added" , "w"); fclose(file); 
    file = fopen(".ginit/commit_ids" , "w"); fprintf(file , "00000000\n"); fclose(file); file = fopen(".ginit/branches/master" , "w"); fprintf(file , "00000000");fclose(file); file = fopen(".ginit/HEAD" , "w"); fprintf(file , "00000000 master");fclose(file);
    file = fopen(".ginit/branch" , "w"); fprintf(file , "master\n");fclose(file); file = fopen(".ginit/logs" , "w"); fclose(file);
}
void run_config(char* argv[]) {
    if (!strcmp(argv[2] , "-global")) {
        char cwd[1024]; getcwd(cwd , sizeof(cwd));
        char* home = getenv("HOME"); chdir(home);
        FILE* global_config = fopen(".ginitconfig" , "wr");
        if (global_config == NULL) {
            perror("error opening global_config file\n");
            exit(EXIT_FAILURE);
        }
        char line[1024] , current_username[100] , current_email[100];
        fgets(line , sizeof(line) , global_config);
        sscanf(line , "username : %s email : %s" , current_username , current_email);
        if (!strcmp(argv[3] , "user.name")) {
            fprintf(global_config , "username : %s email : %s" , argv[4] , current_email);
        }
        else if (!strcmp(argv[3] , "user.email")) {
            fprintf(global_config , "username : %s email : %s" , current_username , argv[4]);
        }
        fclose(global_config);
        chdir(cwd);
    }
    else if (!strcmp(argv[2] , "user.name")) {
        FILE* local_config = fopen(".ginit/config" , "wr");
        if (local_config == NULL) {
            perror("error opening local_config file\n");
            exit(EXIT_FAILURE);
        }
        char line[1024] , current_username[100] , current_email[100];
        fgets(line , sizeof(line) , local_config);
        sscanf(line , "username : %s email : %s" , current_username , current_email);
        fprintf(local_config , "username : %s email : %s" , argv[3] , current_email);
        fclose(local_config);
    }
    else if (!strcmp(argv[2] , "user.email")) {
        FILE* local_config = fopen(".ginit/config" , "wr");
        if (local_config == NULL) {
            perror("error opening local_config file\n");
            exit(EXIT_FAILURE);
        }
        char line[1024] , current_username[100] , current_email[100];
        fgets(line , sizeof(line) , local_config);
        sscanf(line , "username : %s email : %s" , current_username , current_email);
        fprintf(local_config , "username : %s email : %s" , current_username , argv[3]);
        fclose(local_config);
    }
}
int is_in_a_ref_file(char* pathspec , char* ref_filename) {
    FILE* file = fopen(ref_filename , "r");
    if (file == NULL) {
        perror("error opening file in is_in_a_ref_file\n");
        fclose(file);
        exit(EXIT_FAILURE);
    }
    char line[1024] , tmp[1024];
    while (fgets(line, sizeof(line), file)) {
        int length = strlen(line);
        line[length - 1] = '\0';
        sscanf(line , "%s " , tmp);
        if (!strcmp(pathspec , tmp)) {
            fclose(file);
            return 1;
        }
    }
    fclose(file);
    return 0; 
}
void list_files_recursively(char* basePath , char* filename , int depth , int l) {
    char path[1024], name[1024];
    struct  dirent* entry;
    DIR* dir = opendir(basePath);
    if (dir == NULL) {
        perror("error opening directory in list_files_recursively func");
        exit(EXIT_FAILURE);
    }   
    while ((entry = readdir(dir)) != NULL) {
        FILE* file = fopen(filename , "a");
        if (file == NULL) {
            perror("error opening file in list_files_recursively func\n");
            fclose(file);
            exit(EXIT_FAILURE);
        }
        if (entry->d_type == 4) {
            if (strcmp(entry->d_name , ".") && strcmp(entry->d_name , "..")) {
                sprintf(path , "%s/%s" , basePath , entry->d_name);
                strcpy(name , path); memmove(name , name + l , strlen(name) - l + 1);
                if (!is_in_a_ref_file(name, filename)) {
                    fprintf(file , "%s %s %d\n" , name ,path , depth); fclose(file);
                }
                list_files_recursively(path , filename , depth + 1 , l);
            }
        }
        else {
            sprintf(path , "%s/%s" , basePath , entry->d_name);
            strcpy(name , path); memmove(name , name + l , strlen(name) - l + 1);
            if (!is_in_a_ref_file(name , filename)) {
                fprintf(file , "%s %s %d\n" , name ,path , depth); fclose(file);
            }
        }
    }
    closedir(dir);
}
int is_exist_directory_or_file(char path[]) {
    struct stat path_stat;
    if (!stat(path , &path_stat)) {
        return S_ISDIR(path_stat.st_mode);
    }
    else {
        return -1;
    }
}    
void add_to_tracks_and_stages(int argc , char* argv[]) {
    char cwd[512];
    getcwd(cwd , sizeof(cwd));
    if (argc == 3 && !strcmp(argv[2] , "-redo")) {
    // دستور redo
    //
    //
    //
    //
    //
    }
    else if (argc == 3) {
        if (!is_in_a_ref_file(argv[2] , ".ginit/refs/allfiles")) {
            fprintf(stderr,"pathspec \"%s\" dosen't match any file\n" , argv[2]);
            exit(EXIT_SUCCESS);
        }
        else {
            char path[1024];
            int l = strlen(cwd) + 1;
            sprintf(path , "%s/%s" , cwd , argv[2]);
            if (is_exist_directory_or_file(path) == 1) {
                if (!is_in_a_ref_file(argv[2] , ".ginit/refs/tracks")) {
                    FILE* file = fopen(".ginit/refs/tracks" , "a");
                    if (file == NULL) {
                        perror("error opening tracks in add to tracks for dir\n");
                        fclose(file);
                        exit(EXIT_FAILURE);
                    }
                    fprintf(file ,"%s %s \n" , argv[2] , path); fclose(file);
                }
                if (!is_in_a_ref_file(argv[2] , ".ginit/refs/stages")) {
                    FILE* file = fopen(".ginit/refs/stages" , "a");
                    if (file == NULL) {
                        perror("error opening stages in add to tracks for dir\n");
                        fclose(file);
                        exit(EXIT_FAILURE);
                    }
                    fprintf(file ,"%s %s \n" , argv[2] , path); fclose(file);
                }
                list_files_recursively(path , ".ginit/refs/stages" , 1 , l); list_files_recursively(path , ".ginit/refs/tracks" , 1 , l);
            }
            else {
                if (!is_in_a_ref_file(argv[2] , ".ginit/refs/tracks")) {
                    FILE* file = fopen(".ginit/refs/tracks" , "a");
                    if (file == NULL) {
                        perror("error opening tracks in add to tracks for file\n");
                        fclose(file);
                        exit(EXIT_FAILURE);
                    }
                    fprintf(file ,"%s %s \n" , argv[2] , path); fclose(file);
                }
                if (!is_in_a_ref_file(argv[2] , ".ginit/refs/stages")) {
                    FILE* file = fopen(".ginit/refs/stages" , "a");
                    if (file == NULL) {
                        perror("error opening tracks in add to tracks for file\n");
                        fclose(file);
                        exit(EXIT_FAILURE);
                    }
                    fprintf(file ,"%s %s \n" , argv[2] , path); fclose(file);
                }
            }
            chdir(cwd);
        }
    }
    else if (!strcmp(argv[2] , "-f")) {
        for (int i = 3; i < argc; i++){
            if (!is_in_a_ref_file(argv[i] , ".ginit/refs/allfiles")) {
            printf("pathspec \"%s\" dosen't match any file\n" , argv[i]);
            }
            else {
                char path[1024];
                int l = strlen(cwd) + 1;
                sprintf(path , "%s/%s" , cwd , argv[i]);
                if (is_exist_directory_or_file(path) == 1) {
                    if (!is_in_a_ref_file(argv[i] , ".ginit/refs/tracks")) {
                        FILE* file = fopen(".ginit/refs/tracks" , "a");
                        if (file == NULL) {
                            perror("error opening tracks in add to tracks for dir\n");
                            fclose(file);
                            exit(EXIT_FAILURE);
                        }
                        fprintf(file ,"%s %s \n" , argv[i] , path); fclose(file);
                    }
                    if (!is_in_a_ref_file(argv[i] , ".ginit/refs/stages")) {
                        FILE* file = fopen(".ginit/refs/stages" , "a");
                        if (file == NULL) {
                            perror("error opening stages in add to tracks for dir\n");
                            fclose(file);
                            exit(EXIT_FAILURE);
                        }
                        fprintf(file ,"%s %s \n" , argv[i] , path); fclose(file);
                    }
                    list_files_recursively(path , ".ginit/refs/stages" , 1 , l); list_files_recursively(path , ".ginit/refs/tracks" , 1 , l);
                }
                else {
                    if (!is_in_a_ref_file(argv[i] , ".ginit/refs/tracks")) {
                        FILE* file = fopen(".ginit/refs/tracks" , "a");
                        if (file == NULL) {
                            perror("error opening tracks in add to tracks for file\n");
                            fclose(file);
                            exit(EXIT_FAILURE);
                        }
                        fprintf(file ,"%s %s \n" , argv[i] , path); fclose(file);
                    }
                    if (!is_in_a_ref_file(argv[i] , ".ginit/refs/stages")) {
                        FILE* file = fopen(".ginit/refs/stages" , "a");
                        if (file == NULL) {
                            perror("error opening tracks in add to tracks for file\n");
                            fclose(file);
                            exit(EXIT_FAILURE);
                        }
                        fprintf(file ,"%s %s \n" , argv[i] , path); fclose(file);
                    }
                }
                chdir(cwd);
            }
        }
    }
    else if (!strcmp(argv[2] , "-n")) {
        FILE* file = fopen(".ginit/refs/allfiles" , "r");
        if (file == NULL) {
            perror("error opening file in add_to_tracks_and_stages in -n\n");
            exit(EXIT_FAILURE);
        }
        char line[1024]; int d = atoi(argv[3]);
        while (fgets(line , sizeof(line) , file)){
            char name[100] , address[1024]; int depth;
            sscanf(line , "%s %s %d" , name , address , &depth);
            if (depth <= d) {
                if (is_in_a_ref_file(name , ".ginit/refs/stages")) {
                    printf("\033[1;32m+\033[0m file name : %s ; depth : %d\n" , name , depth);
                }
                else {
                    printf("\033[1;31m-\033[0m file name : %s ; depth : %d\n" , name , depth);
                }
            }
        }
    }
}
void run_add(int argc, char* argv[]) {
    if (argc < 3) {
        perror("please specify a file or directory\n");
        exit(EXIT_SUCCESS);
    }
    add_to_tracks_and_stages(argc , argv);
}
int check_files_modified(char* file_path) {
    FILE* file = fopen(".ginit/time" , "r+");
    if (file == NULL) {
        perror("error opening time file\n");
        fclose(file);
        exit(EXIT_FAILURE);
    }
    unsigned long time_value;
    char line[30];
    fgets(line , sizeof(line) , file);
    line[10] = '\0';
    time_value = atoi(line);
    struct stat file_info;
    if (!stat(file_path, &file_info)) {
        time_t last_modified_time = file_info.st_mtime;
        if (time_value < (unsigned long)last_modified_time) {
            fclose(file);
            return 1;
        } else {
            fclose(file);
            return 0;
        }
    }
    fclose(file);
    return 0;
}
void update_modified() {
    FILE* file = fopen(".ginit/refs/allfiles" , "r");
    if (file == NULL) {
        perror("error opening allfiles in update_modified\n");
        exit(EXIT_FAILURE);
    }
    char line[1024];
    while (fgets(line, sizeof(line), file)) {
        char a[100] , b[512]; int depth;
        sscanf(line , "%s %s %d" , a , b , &depth);
        if ((is_exist_directory_or_file(b) != -1) && check_files_modified(b) && !is_in_a_ref_file(a , ".ginit/refs/modified")) {
            FILE* modified = fopen(".ginit/refs/modified" , "a");
            if (modified == NULL) {
                perror("error opening modified in update_modified\n");
                fclose(file);
                exit(EXIT_FAILURE);
            }
            fprintf(modified , "%s" , line);
            fclose(modified);
        }
    }
    fclose(file);
}
void update_added() {
    FILE* file = fopen(".ginit/refs/allfiles" , "r");
    if (file == NULL) {
        perror("error opening allfiles in update_added\n");
        fclose(file);
        exit(EXIT_FAILURE);
    }
    char line[1024];
    while (fgets(line, sizeof(line), file)) {
        char a[1024] , b[1024]; int depth;
        sscanf(line , "%s %s %d" , a , b , &depth);
        if ((is_in_a_ref_file(a , ".ginit/refs/allfiles_copy") == 0) && (is_in_a_ref_file(a , ".ginit/refs/allfiles") == 1) && (is_in_a_ref_file(a , ".ginit/refs/added") == 0)) {
            FILE* added = fopen(".ginit/refs/added" , "a");
            if (file == NULL) {
                perror("error opening added in update_added\n");
                fclose(file);
                exit(EXIT_FAILURE);
            }
            fprintf(added , "%s" , line); fclose(added);
        }
    }
    fclose(file);
}
void update_deleted() {
    FILE* file = fopen(".ginit/refs/allfiles_copy" , "r");
    if (file == NULL) {
        perror("error opening allfiles_copy in update_deleted\n");
        fclose(file);
        exit(EXIT_FAILURE);
    }
    char line[1024];
    while (fgets(line, sizeof(line), file)) {
        char a[100] , b[512]; int depth;
        sscanf(line , "%s %s %d" , a , b , &depth);
        if ((is_in_a_ref_file(a , ".ginit/refs/allfiles") == 0) && (is_in_a_ref_file(a , ".ginit/refs/deleted") == 0)) {
            FILE* deleted = fopen(".ginit/refs/deleted" , "a");
            if (file == NULL) {
                perror("error opening deleted in update_deleted\n");
                fclose(file);
                exit(EXIT_FAILURE);
            }
            fprintf(deleted , "%s" , line); fclose(deleted);
        }
    }
    fclose(file);
}
void update_tracks() {
    FILE* file= fopen(".ginit/refs/tracks" , "r+");
    if (file == NULL) {
        perror("error opening tracks in update_tracks\n");
        fclose(file);
        exit(EXIT_FAILURE);
    }
    char line[1024];
    while (fgets(line, sizeof(line), file) != NULL) {
        char a[100] , b[512]; int depth;
        sscanf(line , "%s %s %d" , a , b , &depth);
        if (is_in_a_ref_file(a , ".ginit/refs/deleted")) {
            long pos = ftell(file); 
            FILE *temp = fopen(".ginit/refs/temp", "w");
            fseek(file, 0, SEEK_SET);
            char buffer[1024];
            while (fgets(buffer, sizeof(buffer), file) != NULL) {
                if (ftell(file) != pos) {
                    fputs(buffer, temp);
                }
            }
            if (temp != NULL) {
                fclose(temp);
            }
            fclose(file);
            remove(".ginit/refs/tracks");
            rename(".ginit/refs/temp", ".ginit/refs/tracks");
        }
    }
    fclose(file);
}
void update_stages() {
    FILE* file = fopen(".ginit/refs/stages" , "r+");
    if (file == NULL) {
        perror("error opening stages in update_stages\n");
        fclose(file);
        exit(EXIT_FAILURE);
    }
    char line[1024];
    while (fgets(line, sizeof(line), file) != NULL) {
        char a[100] , b[512]; int depth;
        sscanf(line , "%s %s %d" , a , b , &depth);
        if (is_in_a_ref_file(a , ".ginit/refs/modified") || is_in_a_ref_file(a , ".ginit/refs/deleted")) {
            long pos = ftell(file); 
            FILE *temp = fopen(".ginit/refs/temp", "w");
            fseek(file, 0, SEEK_SET);
            char buffer[1024];
            while (fgets(buffer, sizeof(buffer), file) != NULL) {
                if (ftell(file) != pos) {
                    fputs(buffer, temp);
                }
            }
            if (temp != NULL) {
                fclose(temp);
            }
            fclose(file);
            remove(".ginit/refs/stages");
            rename(".ginit/refs/temp", ".ginit/refs/stages");
        }
    }
    fclose(file);
}
void run_status() {
    if (!is_file_empty(".ginit/refs/added")) {
        FILE* file = fopen(".ginit/refs/added" , "r");
        if (file == NULL) {
            perror("error opening added in status\n");
            fclose(file);
            exit(EXIT_FAILURE);
        }
        printf("Added files : \n");
        char line[1024];
        while (fgets(line , sizeof(line) , file) != NULL) {
            char name[100] , address[512]; int depth;
            sscanf(line , "%s %s %d " , name , address , &depth);
            if (is_in_a_ref_file(name , ".ginit/refs/stages")) {
                printf("\033[1;32m file name : %s \033[0m\n" , name);
            }
            else {
                printf("\033[1;31m file name : %s \033[0m\n" , name);
            }
        }
    }
    if (!is_file_empty(".ginit/refs/deleted")) {
        FILE* file = fopen(".ginit/refs/deleted" , "r");
        if (file == NULL) {
            perror("error opening deleted in status\n");
            fclose(file);
            exit(EXIT_FAILURE);
        }
        printf("Deleted files : \n");
        char line[1024];
        while (fgets(line , sizeof(line) , file) != NULL) {
            char name[100] , address[512]; int depth;
            sscanf(line , "%s %s %d " , name , address , &depth);
            printf("\033[1;31m file name : %s \033[0m\n" , name);
        }
    }
    if (!is_file_empty(".ginit/refs/modified")) {
        FILE* file = fopen(".ginit/refs/modified" , "r");
        if (file == NULL) {
            perror("error opening modified in status\n");
            fclose(file);
            exit(EXIT_FAILURE);
        }
        printf("Modified files : \n");
        char line[1024];
        while (fgets(line , sizeof(line) , file) != NULL) {
            char name[100] , address[512]; int depth;
            sscanf(line , "%s %s %d " , name , address , &depth);
            if (is_in_a_ref_file(name , ".ginit/refs/stages")) {
                printf("\033[1;32m file name : %s \033[0m\n" , name);
            }
            else {
                printf("\033[1;31m file name : %s \033[0m\n" , name);
            }
        }
    }
}
int is_file_empty(char *filename) {
    FILE *file = fopen(filename, "r");
    fseek(file, 0, SEEK_END);
    if (ftell(file) == 0) { 
        fclose(file);
        return 1; 
    }
    fclose(file);
    return 0; 
}
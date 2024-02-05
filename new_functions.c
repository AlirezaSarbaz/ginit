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

#define min(a , b)  ((a > b) ? b : a)
#define max(a , b)  ((a > b) ? a : b)

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
void run_commit(int argc , char* argv[]);
void add_to_logs(char* argv[] , char* commit_id , int number);
int copy_stagedfiles_to_commit_dir(char* commit_id , char* new_stage_path);
char* generate_commit_id();
void run_branch(int argc , char* argv[]);
void run_diff(int argc , char* argv[]);
void remove_nullspaces(char* filename1 , int begin , int end);
int is_blanck_line(char* line , int length);
void find_diffs (char* file1_name , char* file2_name);
int number_of_lines(FILE* file);
void run_diff_for_commit(char* address1 , char* address2 , char* name , char* commit_id1 , char* commit_id2);
int is_ok_for_checkout_or_merge();
void run_checkout(char* argv[]);
void remove_tracks();
void run_reset(int argc , char* argv[]);
void run_merge(char* argv[]);

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
    file = fopen(".ginit/branch" , "w"); fprintf(file , "master\n");fclose(file); file = fopen(".ginit/logs" , "w"); fclose(file); file = fopen(".ginit/reset_undo" , "w"); fclose(file);
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
            if (strcmp(entry->d_name , ".") && strcmp(entry->d_name , "..") && strcmp(entry->d_name , ".ginit")) {
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
    char cwd[1024];
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
            char path[1100];
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
                    list_files_recursively(path , ".ginit/refs/tracks" , 1 , l);
                }
                if (!is_in_a_ref_file(argv[2] , ".ginit/refs/stages")) {
                    FILE* file = fopen(".ginit/refs/stages" , "a");
                    if (file == NULL) {
                        perror("error opening stages in add to tracks for dir\n");
                        fclose(file);
                        exit(EXIT_FAILURE);
                    }
                    fprintf(file ,"%s %s \n" , argv[2] , path); fclose(file);
                    list_files_recursively(path , ".ginit/refs/stages" , 1 , l);
                }
                FILE* file = fopen(".ginit/reset_undo_copy" , "a");
                fprintf(file ,"%s %s \n" , argv[2] , path); fclose(file);
                list_files_recursively(path , ".ginit/reset_undo_copy" , 1 , l);
                file = fopen(".ginit/reset_undo_copy" , "a");
                fprintf(file ,"\n"); fclose(file);
                file = fopen(".ginit/reset_undo" , "rb+"); FILE* reset_undo_tmp = fopen(".ginit/reset_undo_tmp" , "wb+");
                copy_file_source_to_dest(reset_undo_tmp , file);
                file = fopen(".ginit/reset_undo" , "wb+"); FILE* reset_undo_copy = fopen(".ginit/reset_undo_copy" , "rb+");
                copy_file_source_to_dest(file , reset_undo_copy);
                reset_undo_tmp = fopen(".ginit/reset_undo_tmp" , "r"); file = fopen(".ginit/reset_undo" , "a");
                char line2[1024];
                while (fgets(line2 , sizeof(line2) , reset_undo_tmp) != NULL) {
                    fprintf(file , "%s" , line2);
                }
                fclose(file);fclose(reset_undo_tmp);
                remove(".ginit/reset_undo_tmp"); remove(".ginit/reset_undo_copy");
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
                FILE* file = fopen(".ginit/reset_undo_copy" , "a");
                fprintf(file ,"%s %s \n\n" , argv[2] , path); fclose(file);
                file = fopen(".ginit/reset_undo" , "rb+"); FILE* reset_undo_tmp = fopen(".ginit/reset_undo_tmp" , "wb+");
                copy_file_source_to_dest(reset_undo_tmp , file);
                file = fopen(".ginit/reset_undo" , "wb+"); FILE* reset_undo_copy = fopen(".ginit/reset_undo_copy" , "rb+");
                copy_file_source_to_dest(file , reset_undo_copy);
                reset_undo_tmp = fopen(".ginit/reset_undo_tmp" , "r"); file = fopen(".ginit/reset_undo" , "a");
                char line2[1024];
                while (fgets(line2 , sizeof(line2) , reset_undo_tmp) != NULL) {
                    fprintf(file , "%s" , line2);
                }
                fclose(file);fclose(reset_undo_tmp);
                remove(".ginit/reset_undo_tmp"); remove(".ginit/reset_undo_copy");
            }
            chdir(cwd);
        }
    }
    else if (!strcmp(argv[2] , "-f")) {
        for (int i = 3; i < argc; i++){
            if (!is_in_a_ref_file(argv[2] , ".ginit/refs/allfiles")) {
            fprintf(stderr,"pathspec \"%s\" dosen't match any file\n" , argv[2]);
            exit(EXIT_SUCCESS);
        }
        else {
            char path[1100];
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
                FILE* file = fopen(".ginit/reset_undo_copy" , "a");
                fprintf(file ,"%s %s \n" , argv[2] , path); fclose(file);
                list_files_recursively(path , ".ginit/refs/stages" , 1 , l); list_files_recursively(path , ".ginit/refs/tracks" , 1 , l); list_files_recursively(path , ".ginit/reset_undo_copy" , 1 , l);
                file = fopen(".ginit/reset_undo_copy" , "a");
                fprintf(file ,"\n"); fclose(file);
                file = fopen(".ginit/reset_undo" , "rb+"); FILE* reset_undo_tmp = fopen(".ginit/reset_undo_tmp" , "wb+");
                copy_file_source_to_dest(reset_undo_tmp , file);
                file = fopen(".ginit/reset_undo" , "wb+"); FILE* reset_undo_copy = fopen(".ginit/reset_undo_copy" , "rb+");
                copy_file_source_to_dest(file , reset_undo_copy);
                reset_undo_tmp = fopen(".ginit/reset_undo_tmp" , "r"); file = fopen(".ginit/reset_undo" , "a");
                char line2[1024];
                while (fgets(line2 , sizeof(line2) , reset_undo_tmp) != NULL) {
                    fprintf(file , "%s" , line2);
                }
                fclose(file);fclose(reset_undo_tmp);
                remove(".ginit/reset_undo_tmp"); remove(".ginit/reset_undo_copy");
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
                FILE* file = fopen(".ginit/reset_undo_copy" , "a");
                fprintf(file ,"%s %s \n\n" , argv[2] , path); fclose(file);
                file = fopen(".ginit/reset_undo" , "rb+"); FILE* reset_undo_tmp = fopen(".ginit/reset_undo_tmp" , "wb+");
                copy_file_source_to_dest(reset_undo_tmp , file);
                file = fopen(".ginit/reset_undo" , "wb+"); FILE* reset_undo_copy = fopen(".ginit/reset_undo_copy" , "rb+");
                copy_file_source_to_dest(file , reset_undo_copy);
                reset_undo_tmp = fopen(".ginit/reset_undo_tmp" , "r"); file = fopen(".ginit/reset_undo" , "a");
                char line2[1024];
                while (fgets(line2 , sizeof(line2) , reset_undo_tmp) != NULL) {
                    fprintf(file , "%s" , line2);
                }
                fclose(file);fclose(reset_undo_tmp);
                remove(".ginit/reset_undo_tmp"); remove(".ginit/reset_undo_copy");
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
        char a[100] , b[1024]; int depth;
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
        char a[100] , b[1024]; int depth;
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
        char a[100] , b[1024]; int depth;
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
        char a[100] , b[1024]; int depth;
        sscanf(line , "%s %s %d" , a , b , &depth);
        if (is_in_a_ref_file(a , ".ginit/refs/deleted") || is_in_a_ref_file(a , ".ginit/refs/modified")) {
            long pos = ftell(file); 
            FILE *temp = fopen(".ginit/refs/temp2", "w");
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
            rename(".ginit/refs/temp2", ".ginit/refs/stages");
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
            char name[100] , address[1024]; int depth;
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
            char name[100] , address[1024]; int depth;
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
void run_commit(int argc , char* argv[]) {
    argc ++;
    char path[1024] , commit_id[9] , line[1024] , line2[1024] ,current_commit_id[9] , current_branch[100] , stage_address[1100];
    strcpy(commit_id , generate_commit_id());
    sprintf(path , ".ginit/commits/%s" , commit_id);
    mkdir(path , 0755);
    sprintf(stage_address , "%s/stages" , path);
    char branch_path[1100]; sprintf(branch_path , ".ginit/branches/%s" , current_branch);
    FILE* file = fopen(".ginit/commit_ids" , "rb"); FILE* commit_ids_copy = fopen(".ginit/commit_ids_copy" , "wb+");
    copy_file_source_to_dest(commit_ids_copy , file);
    file = fopen(".ginit/commit_ids" , "w");
    fprintf(file , "%s \n" , commit_id); fclose(file);
    commit_ids_copy = fopen(".ginit/commit_ids_copy" , "r"); file = fopen(".ginit/commit_ids" , "a");
    while (fgets(line2 , sizeof(line2) , commit_ids_copy) != NULL) {
        fprintf(file , "%s" , line2);
    }
    fclose(file);fclose(commit_ids_copy);
    remove(".ginit/commit_ids_copy");
    int number = copy_stagedfiles_to_commit_dir(commit_id , stage_address);
    add_to_logs(argv , commit_id , number);
    file = fopen(".ginit/HEAD" , "r+");
    fgets(line , sizeof(line) , file);
    sscanf(line , "%s %s" , current_commit_id , current_branch);
    fseek(file , 0 , SEEK_SET); fprintf(file , "%s %s" , commit_id , current_branch);fclose(file);
    file = fopen(branch_path , "w"); fprintf(file ,"%s" , commit_id); fclose(file);
    file = fopen(".ginit/refs/stages" , "w"); fclose(file);
    file = fopen(".ginit/refs/deleted" , "w"); fclose(file);
    file = fopen(".ginit/refs/added" , "w"); fclose(file);
    file = fopen(".ginit/refs/modified" , "w"); fclose(file);
    file = fopen(".ginit/refs/allfiles_copy" , "w"); fclose(file);
    char cwd[1024]; getcwd(cwd , sizeof(cwd));
    int l = strlen(cwd) + 1;
    list_files_recursively(cwd , ".ginit/refs/allfiles_copy" , 1 , l);
}
char* generate_commit_id() {
    char* result = (char*)malloc(9);
    srand((unsigned int)time(NULL));
    char chars[] = {"abcdefghijklmnopqrstuvwxyz1234567890"};
    for (int i = 0; i < 8; i++) {
        int random_index = rand() % (sizeof(chars) - 1);
        result[i] = chars[random_index];
    }
    result[8] = '\0';
    return result;
}
void add_to_logs(char* argv[] , char* commit_id , int number) {
    FILE* head = fopen(".ginit/HEAD" , "r"); FILE* config = fopen(".ginit/config" , "r");
    char current_commit_id[9] , current_branch[100] , username[100] , email[100], line1[100] , line2[100];
    fgets(line1 , sizeof(line1) , config); fgets(line2 , sizeof(line2) , head);
    sscanf(line1 , "username : %s email : %s" , username , email); sscanf(line2 , "%s %s" , current_commit_id , current_branch);
    FILE* file = fopen(".ginit/logs" , "rb"); FILE* logs_copy = fopen(".ginit/logs_copy" , "wb+");
    copy_file_source_to_dest(logs_copy , file);
    time_t current_time;
    struct tm *local_time;
    char date_string[11] , time_string[9];
    current_time = time(NULL);
    local_time = localtime(&current_time);
    strftime(time_string, sizeof(time_string), "%H:%M", local_time);
    strftime(date_string, sizeof(date_string), "%Y:%m:%d", local_time);
    file = fopen(".ginit/logs" , "w");
    fprintf(file , "%s %s <%s> <%s> <%s> <%s> %s <%s> %d\n" ,current_commit_id , commit_id , username , email , time_string , date_string ,current_branch , argv[3] , number); fclose(file);
    logs_copy = fopen(".ginit/logs_copy" , "r"); file = fopen(".ginit/logs" , "a");
    char line[1024];
    while (fgets(line , sizeof(line) , logs_copy) != NULL) {
        fprintf(file , "%s" , line);
    }
    fclose(file); fclose(head); fclose(config);fclose(logs_copy);
    remove(".ginit/logs_copy");
}
int copy_stagedfiles_to_commit_dir(char* commit_id , char* new_stage_path) {
    FILE* tracks = fopen(".ginit/refs/stages" , "r");
    if (tracks == NULL) {
        perror("Error opening tracks in copy_stagedfiles_to_commit_dir");
        fclose(tracks);
        exit(EXIT_FAILURE);
    }
    char line[1024] , name[100] , address[1100] , cwd[1024] ,new_path[12000];
    getcwd(cwd , sizeof(cwd));
    FILE* new_stage = fopen(new_stage_path , "a");
    int counter = 0;
    while (fgets(line , sizeof(line) , tracks)) {
        sscanf(line , "%s %s " , name , address);
        sprintf(new_path , "%s/.ginit/commits/%s/%s" , cwd , commit_id , name);
        fprintf(new_stage ,"%s %s\n" , name , new_path);
        if (is_in_a_ref_file(name , ".ginit/refs/tracks") && (is_exist_directory_or_file(address) != -1)) {
            if (is_exist_directory_or_file(address) == 1) {
                mkdir(new_path , 0755);
            }
            else {
                FILE* src = fopen(address , "rb+"); 
                if (src == NULL) {
                    perror("Error opening file");
                    exit(EXIT_FAILURE);
                }
                FILE* dest = fopen(new_path , "wb+");
                if (dest == NULL) {
                    perror("Error opening file");
                    exit(EXIT_FAILURE);
                }
                copy_file_source_to_dest(dest , src);
            }
            counter ++;
        }
    }
    fclose(tracks); fclose(new_stage);
    return counter;
}
void run_branch(int argc , char* argv[]) {
    if (argc == 3) {
        FILE* file = fopen(".ginit/HEAD" , "r");
        char current_commit_id[9] , line[500];
        fgets(line , sizeof(line) , file); sscanf(line , "%s " , current_commit_id ); fclose(file);
        file = fopen(".ginit/branch" , "a"); fprintf(file , "%s\n" ,argv[2]); fclose(file);
        char path[1024]; sprintf(path , ".ginit/branches/%s" , argv[2]);
        file = fopen(path , "w"); fprintf(file , "%s" , current_commit_id); fclose(file);
    }
    if (argc == 2) {
        FILE* file = fopen(".ginit/HEAD" , "r");
        char current_commit_id[9] , current_branch[100] , line[500];
        fgets(line , sizeof(line) , file); sscanf(line , "%s %s " , current_commit_id , current_branch); fclose(file);
        file = fopen(".ginit/branch" , "r");
        while (fgets(line , sizeof(line) , file) != NULL) {
            char branch_name[100];
            sscanf(line , "%s " , branch_name);
            if (!strcmp(branch_name , current_branch)) {
                printf("\033[1;32m* %s\033[0m\n" , current_branch);
            }
            else {
                printf("  %s\n" , branch_name);
            }
        }
        fclose(file);
    }
}
void run_diff(int argc , char* argv[]) {
    if (!strcmp(argv[2] , "-f")) {
        int flag = 0;
        if (!is_in_a_ref_file(argv[3] , ".ginit/refs/allfiles")) {
            fprintf(stderr , "the file %s not exist.\n" , argv[3]);
            flag ++;
        }
        if (!is_in_a_ref_file(argv[4], ".ginit/refs/allfiles")) {
            fprintf(stderr , "the file %s not exist.\n" , argv[4]);
            flag ++;
        }
        if (flag) {
            exit(EXIT_SUCCESS);
        }
        if (argc == 5) {
            remove_nullspaces(argv[3] , 1 , 10000); remove_nullspaces(argv[4] , 1 , 10000);
            find_diffs(argv[3] , argv[4]);
        }
        else if (argc == 9) {
            int end1 , end2 , begin1 , begin2;
            sscanf(argv[6] , "%d-%d" , &begin1 , &end1); sscanf(argv[8] , "%d-%d" , &begin2 , &end2);
            remove_nullspaces(argv[3] , begin1 , end1); remove_nullspaces(argv[4] , begin2 , end2);
            find_diffs(argv[3] , argv[4]);
        }
    }
    else if (!strcmp(argv[2] , "-c")) {
        if (!is_in_a_ref_file(argv[3] , ".ginit/commit_ids") || !is_in_a_ref_file(argv[4] , ".ginit/commit_ids")) {
            perror("enter valid commit ids\n");
            exit(EXIT_FAILURE);
        }
        else {
            char cwd[1024] , stages1[1100] , stages2[1100] , line[1024] , line2[1024] , name[1024] , find[100] , address[1024] , address2[1024];
            getcwd(cwd , sizeof(cwd));
            sprintf(stages1 , "%s/.ginit/commits/%s/stages" , cwd , argv[3]); sprintf(stages2 , ".ginit/commits/%s/stages" , argv[4]);
            FILE* stages_1 = fopen(stages1, "r"); FILE* stages_2 = fopen(stages2 , "r");
            while (fgets(line , sizeof(line) , stages_1) != NULL) {
                sscanf(line ,"%s " , name);
                if (!is_in_a_ref_file(name , stages2)) {
                    printf("file %s exist in commit %s and not exist in commit %s\n" , name , argv[3] , argv[4]);
                }
            }
            while (fgets(line , sizeof(line) , stages_2)) {
                sscanf(line ,"%s " , name);
                if (!is_in_a_ref_file(name , stages1)) {
                    printf("file %s exist in commit %s and not exist in commit %s\n" , name , argv[4] , argv[3]);
                }
            }
            fclose(stages_2); 
            rewind(stages_1);
            while (fgets(line , sizeof(line) , stages_1)) {
                sscanf(line , "%s %s", find , address);
                if (is_exist_directory_or_file(address) == 0) {
                    FILE* file = fopen(stages2 , "r");
                    while (fgets(line2, sizeof(line2), file)) {
                        sscanf(line2, "%s %s", name, address2);
                        if (!strcmp(name, find)) {
                            run_diff_for_commit(address , address2 , name , argv[3] , argv[4]);
                        }
                    }  
                    fclose(file);
                }
            }
        }
    }
}
void remove_nullspaces(char* filename1 , int begin , int end) {
    char file_path[1100], file_copy_path[1100] , cwd[1024] , line[1024];
    int counter = 1;
    getcwd(cwd , sizeof(cwd));
    sprintf(file_path , "%s/%s" , cwd , filename1);
    sprintf(file_copy_path , "%s/.ginit/%s_copy" , cwd , filename1);
    FILE* input = fopen(file_path , "r"); FILE* output = fopen(file_copy_path , "w");
    while (fgets(line , sizeof(line) , input) != NULL) {
        int length = strlen(line);
        line[length - 1] = '\0';
        if (!is_blanck_line(line , length - 1)){
            if (counter >= begin && counter <= end) { 
                int i = 0;
                while (line[i] != '\0') {
                    if (isspace(line[i]) && isspace(line[i + 1])) {
                        memmove(line + i , line + i + 1 , strlen(line) - 1);
                    }
                    else {
                        i ++;
                    }
                }
                fprintf(output , "%s\n" , line);
            }  
            counter ++;
        }
    }
    fclose(input); fclose(output);
}
int is_blanck_line(char* line , int length) {
   for (int i = 0; i < length; i++) {
        if (!isspace(line[i])) {
            return 0;
        }
    }
    return 1;
}
void find_diffs (char* file1_name , char* file2_name) {
    char file1_path[1100], file2_path[1100] , cwd[1024] , line1[1024] , line2[1024];
    getcwd(cwd , sizeof(cwd));
    sprintf(file1_path , "%s/.ginit/%s_copy" , cwd , file1_name);
    sprintf(file2_path , "%s/.ginit/%s_copy" , cwd , file2_name);
    FILE* file1 = fopen(file1_path , "r"); FILE* file2 = fopen(file2_path , "r");
    int num1 = number_of_lines(file1) , num2 = number_of_lines(file2); fclose(file1); fclose(file2);
    file1 = fopen(file1_path , "r"); file2 = fopen(file2_path , "r");
    for (int i = 0; i < min(num1 , num2); i++) {
        fgets(line1 , sizeof(line1) , file1); fgets(line2 , sizeof(line2) , file2);
        if (strcmp(line1 , line2)) {
            printf("<<<<<\n<filename : %s> - <line nember : %d>\n", file1_name , i + 1);
            printf("\033[1;31m%s\033[0m" , line1);
            printf("<filename : %s> - <line nember : %d>\n", file2_name , i + 1);
            printf("\033[1;34m%s\033[0m" , line2);
            printf(">>>>>\n");
        }
    }
    fclose(file1); fclose(file2);
    remove(file1_path); remove(file2_path);
}
int number_of_lines(FILE* file) {
    int lineCount = 0;
    int currentChar;
    while ((currentChar = fgetc(file)) != EOF) {
        if (currentChar == '\n') {
            lineCount++;
        }
    }
    return lineCount;
}
void run_diff_for_commit(char* address1 , char* address2 , char* name , char* commit_id1 , char* commit_id2) {
    char copy1[1100] , copy2[1100] , line[1024] , line1[1024] , line2[1024];
    sprintf(copy1 ,"%s_copyyyyyyyyyyyyyyyy" , address1); sprintf(copy2 ,"%s_copyyyyyyyyyyyyyyyy" , address2);
    FILE* input1 = fopen(address1 , "r"); FILE* input2 = fopen(address2 , "r"); FILE* output1 = fopen(copy1 , "w+"); FILE* output2 = fopen(copy2 , "w+");
    while (fgets(line , sizeof(line) , input1) != NULL) {
        int length = strlen(line);
        line[length - 1] = '\0';
        if (!is_blanck_line(line , length - 1)){
            line[length - 1] = '\n';
            fprintf(output1 , "%s" , line);
        }
    }
    while (fgets(line , sizeof(line) , input2) != NULL) {
        int length = strlen(line);
        if (!is_blanck_line(line , length - 1)){
            int i = 0;
            while (line[i] != '\0') {
                if (isspace(line[i]) && isspace(line[i + 1])) {
                    memmove(line + i , line + i + 1 , strlen(line) - 1);
                }
                else {
                    i ++;
                }
            }
            fprintf(output2 , "%s\n" , line);
        }
    }
    rewind(output1); rewind(output2);
    int num1 = number_of_lines(output1) , num2 = number_of_lines(output2);
    rewind(output1); rewind(output2);
    for (int i = 0; i < min(num1 , num2); i++) {
        fgets(line1 , sizeof(line1) , output1); fgets(line2 , sizeof(line2) , output2);
        if (strcmp(line1 , line2)) {
            printf("<<<<<\n<filename : %s of commit %s> - <line nember : %d>\n", name , commit_id1 , i + 1);
            printf("\033[1;31m%s\033[0m" , line1);
            printf("<filename : %s of commit %s> - <line nember : %d>\n", name ,commit_id2 , i + 1);
            printf("\033[1;34m%s\033[0m" , line2);
            printf(">>>>>\n");
        }
    }
    fclose(input1); fclose(input2); fclose(output1); fclose(output2);
    remove(copy1); remove(copy2);
}


int is_ok_for_checkout_or_merge(){
    FILE* file = fopen(".ginit/refs/allfiles" , "r+");
    char line[1024] , name[100];
    while (fgets(line, sizeof(line), file)) {
        sscanf(line , "%s " , name);
        if (is_in_a_ref_file(name , ".ginit/refs/tracks") && is_in_a_ref_file(name , ".ginit/refs/modified")) {
            return 0;
        }
    }
    fclose(file);
    return 1;
}
void run_checkout(char* argv[]) {
    if (!strcmp(argv[2] , "HEAD")) {
        char HEAD_commit_id[9] , path[1100] , mvCommand[2200] , cwd[1024]; getcwd(cwd , sizeof(cwd));
        FILE* file = fopen(".ginit/HEAD" , "r"); fscanf(file , "%s " , HEAD_commit_id); fclose(file);
        remove_tracks();
        sprintf(path , "%s/.ginit/commits/%s/*" , cwd ,HEAD_commit_id);
        sprintf(mvCommand , "cp -r %s %s" , path , cwd);
        system(mvCommand); remove("stages");
    }
    else if (!is_in_a_ref_file(argv[2] , ".ginit/commit_ids") && !is_in_a_ref_file(argv[2] , ".ginit/branch")) {
        fprintf(stderr,"pathspec \"%s\" dosen't match any branch or commit\n" , argv[2]);
        exit(EXIT_FAILURE);
    }
    else if (is_in_a_ref_file(argv[2] , ".ginit/commit_ids")) {
        char path[1100] , mvCommand[2200] , cwd[1024]; getcwd(cwd , sizeof(cwd));
        remove_tracks();
        sprintf(path , "%s/.ginit/commits/%s/*" , cwd ,argv[2]);
        sprintf(mvCommand , "cp -r %s %s" , path , cwd);
        system(mvCommand); remove("stages");
    }
    else if (is_in_a_ref_file(argv[2] , ".ginit/branch")) {
        char path[1050] , mvCommand[2200] , cwd[1024] , commit_id[9] , branch_path[100]; getcwd(cwd , sizeof(cwd));
        sprintf(branch_path , ".ginit/branches/%s" , argv[2]);
        FILE* file = fopen(branch_path , "r"); fscanf(file , "%s " , commit_id); fclose(file);
        remove_tracks();
        sprintf(path , "%s/.ginit/commits/%s/*" , cwd ,commit_id);
        sprintf(mvCommand , "cp -r %s %s" , path , cwd);
        system(mvCommand); remove("stages");
        file = fopen(".ginit/HEAD" , "w");
        fprintf(file , "%s %s" , commit_id , argv[2]); fclose(file);
        FILE* stages = fopen(".ginit/refs/stages" , "w"); fclose(stages);
        FILE* deleted = fopen(".ginit/refs/deleted" , "w"); fclose(deleted);
        FILE* added = fopen(".ginit/refs/added" , "w"); fclose(added);
        FILE* modified = fopen(".ginit/refs/modified" , "w"); fclose(modified);
        FILE* allfiles_copy = fopen(".ginit/refs/allfiles_copy" , "w"); fclose(allfiles_copy);
        int l = strlen(cwd) + 1;
        list_files_recursively(cwd , ".ginit/refs/allfiles_copy" , 1 , l);
    }
    /*if () {
        //handle HEAD-n
    }*/
}
void remove_tracks() {
    FILE* file = fopen(".ginit/refs/tracks" , "r+");
    char line[1024] , name[100] , address[1024] , rmCommand[1100];
    while (fgets(line, sizeof(line), file)) {
        sscanf(line , "%s %s " , name , address);
        if (is_exist_directory_or_file(address) == 0) {
            sprintf(rmCommand , "rm %s" , address);
            system(rmCommand);
        }
        else if (is_exist_directory_or_file(address) == 1){
            sprintf(rmCommand , "rm -r %s" , address);
            system(rmCommand);
        }
    }
    fclose(file);
}
void run_merge(char* argv[]) {
    if (!is_in_a_ref_file(argv[4] , ".ginit/branch") || !is_in_a_ref_file(argv[3] , ".ginit/branch")) {
        perror("please enter valid branch names\n");
        exit(EXIT_SUCCESS);
    } 
    else {
        char commit_massage[1000];
        printf("enter new commit massage (up to 72 characters) : ");
        scanf("%s" , commit_massage);
        if (strlen(commit_massage) > 72) {
            printf("enter a massage up to 72 characters : ");
            scanf("%s" , commit_massage);
        }
        else {
            char branch1_path[1024] , branch2_path[1024] ,commit_id1[9] , commit_id2[9] , new_commit_id[9] , stages1[1024] , stages2[1024] , new_stage_address[1100] , path[1024] , username[100] , email[100];
            sprintf(branch1_path , ".ginit/branches/%s" , argv[3]); sprintf(branch2_path , ".ginit/branches/%s" , argv[4]);
            FILE* file = fopen(branch1_path , "r"); fscanf(file , "%s " , commit_id1); fclose(file);
            file = fopen(branch2_path , "r"); fscanf(file , "%s " , commit_id2); fclose(file); remove(branch2_path);
            sprintf(stages1 , ".ginit/commits/%s/stages" , commit_id1); sprintf(stages2 , ".ginit/commits/%s/stages" , commit_id2);
            strcpy(new_commit_id , generate_commit_id());
            sprintf(path , ".ginit/commits/%s" , new_commit_id);
            mkdir(path , 0755);
            sprintf(new_stage_address ,"%s/stages" , path);
            FILE* head = fopen(".ginit/HEAD" , "w");
            fprintf(head , "%s %s" , new_commit_id , argv[3]);fclose(head);
            FILE* deleted = fopen(".ginit/refs/deleted" , "w"); fclose(deleted);
            FILE* added = fopen(".ginit/refs/added" , "w"); fclose(added);
            FILE* modified = fopen(".ginit/refs/modified" , "w"); fclose(modified);
            file = fopen(".ginit/commit_ids" , "rb"); FILE* commit_ids_copy = fopen(".ginit/commit_ids_copy" , "wb+");
            copy_file_source_to_dest(commit_ids_copy , file);
            file = fopen(".ginit/commit_ids" , "w");
            fprintf(file , "%s \nmerge \n" , new_commit_id); fclose(file);
            commit_ids_copy = fopen(".ginit/commit_ids_copy" , "r"); file = fopen(".ginit/commit_ids" , "a");
            char line2[1024];
            while (fgets(line2 , sizeof(line2) , commit_ids_copy) != NULL) {
                fprintf(file , "%s" , line2);
            }
            fclose(file);fclose(commit_ids_copy);
            remove(".ginit/commit_ids_copy");
            FILE* stages_2 = fopen(stages2 , "r");
            char line[1100] , name[100] , address[1024] , cwd[1024];
            getcwd(cwd , sizeof(cwd));
            while (fgets(line , sizeof(line) , stages_2)) {
                sscanf(line , "%s %s " , name , address);
                if (!is_in_a_ref_file(name , ".ginit/refs/allfiles") || (is_exist_directory_or_file(address) == 1)) {
                    char new_address[1200];
                    sprintf(new_address , "%s/%s" , cwd , name);
                    if ((is_exist_directory_or_file(address) != -1)) {
                        if (is_exist_directory_or_file(address) == 1) {
                            mkdir(new_address , 0755);
                        }
                        else {
                            FILE* src = fopen(address , "rb+"); 
                            if (src == NULL) {
                                perror("Error opening file");
                                exit(EXIT_FAILURE);
                            }
                            FILE* dest = fopen(new_address , "wb+");
                            if (dest == NULL) {
                                perror("Error opening file");
                                exit(EXIT_FAILURE);
                            }
                            copy_file_source_to_dest(dest , src);
                        }
                        if (!is_in_a_ref_file(name , ".ginit/refs/tracks")) {
                            FILE* tracks = fopen(".ginit/refs/tracks" , "a");
                            fprintf(tracks , "%s %s \n" , name , new_address); fclose(tracks);
                        }
                    }
                }
                else {
                    if ((is_exist_directory_or_file(address) == 0)) {
                        char address_in_workdir[1200] , line1[1024] , line2[1024];
                        sprintf(address_in_workdir , "%s/%s" , cwd , name);
                        FILE* file_in_work_dir = fopen(address_in_workdir , "r");
                        FILE* file_in_other_branch = fopen(address , "r");
                        int num1 = number_of_lines(file_in_other_branch);
                        int num2 = number_of_lines(file_in_work_dir);
                        fclose(file_in_other_branch); fclose(file_in_work_dir);
                        file_in_work_dir = fopen(address_in_workdir , "r");
                        file_in_other_branch = fopen(address , "r");
                        FILE* tmp = fopen(".ginit/tmp" , "a");
                        for (int i = 0; i < max(num1 , num2); i++) {
                            if (fgets(line1 , sizeof(line1) , file_in_work_dir) == NULL){
                                strcpy(line1 , "\n");
                            }
                            if (fgets(line2 , sizeof(line2) , file_in_other_branch) == NULL) {
                                strcpy(line2 , "\n");
                            }
                            int l1 = strlen(line1) , l2 = strlen(line2);
                            line1[l1 - 1] = '\0'; line2[l2 - 1] = '\0';
                            if (!strcmp(line1 , line2)) {
                                fprintf(tmp , "%s\n",line1);
                            }
                            else {
                                int n;
                                printf("There are some conflict in file %s in line %d\n " , name , i + 1);
                                printf("This line in the current file is : \n   <\033[1;32m%s\033[0m>\n" , line1);
                                printf("This line in the second file is : \n   <\033[1;34m%s\033[0m>\n" , line2);
                                printf("Which line do you prefer to be in the last file? [1/2] : ");
                                scanf("%d" , &n);
                                if (n == 1) {
                                    fprintf(tmp , "%s\n",line1);
                                }
                                else {
                                    fprintf(tmp , "%s\n",line2);
                                }
                            }
                        }
                        fclose(tmp); fclose(file_in_other_branch); fclose(file_in_work_dir);
                        file_in_work_dir = fopen(address_in_workdir , "wb"); tmp = fopen(".ginit/tmp" , "rb");
                        copy_file_source_to_dest(file_in_work_dir , tmp);
                        remove(".ginit/tmp");
                    }
                }
            }
            fclose(stages_2);
            FILE* tracks = fopen(".ginit/refs/tracks" , "rb");
            FILE* stages = fopen(".ginit/refs/stages" , "wb");
            copy_file_source_to_dest(stages , tracks);
            int number = copy_stagedfiles_to_commit_dir(new_commit_id , new_stage_address);
        }
    }
}
void run_reset(int argc , char* argv[]) {
    if (!strcmp(argv[2] , "-f")) {
        
    }
    else if (!strcmp(argv[2] , "-undo")) {
        FILE* file = fopen(".ginit/reset_undo" , "r+"); FILE* tmp = fopen(".ginit/tmp" , "w");
        char line[1024]; int counter = 0;
        while (fgets(line , sizeof(line) , file) != NULL && strcmp(line , "\n")) {
            fprintf(tmp , "%s" , line);
            counter ++;
        }
        fclose(file); fclose(tmp);
        file = fopen(".ginit/refs/stages" , "r");
        char line2[1024];
        while (fgets(line2, sizeof(line2), file) != NULL) {
            char a[100] , b[1024];
            sscanf(line2 , "%s %s " , a , b);
            if (is_in_a_ref_file(a , ".ginit/tmp")) {
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
    else {
        FILE* file = fopen(".ginit/reset_tmp" , "w"); 
        char cwd[1024] , path[1100]; getcwd(cwd , sizeof(cwd));
        sprintf(path , "%s/%s" , cwd ,argv[2]);
        if (is_exist_directory_or_file(path) == 0) {
            fprintf(file ,"%s\n" , argv[2]);
            fclose(file);
        }
        else if (is_exist_directory_or_file(path) == 1){
            fprintf(file ,"%s\n" , argv[2]);
            fclose(file);
            int l = strlen(path) + 1;
            list_files_recursively(path , ".ginit/reset_tmp" , 1 , l);
        }
        file = fopen(".ginit/refs/stages" , "r+");
        if (file == NULL) {
            perror("error opening stages in reset\n");
            fclose(file);
            exit(EXIT_FAILURE);
        }
        char line[1024];
        while (fgets(line, sizeof(line), file) != NULL) {
            char a[100] , b[1024]; int depth;
            sscanf(line , "%s %s %d" , a , b , &depth);
            if (is_in_a_ref_file(a , ".ginit/reset_tmp")) {
                long pos = ftell(file); 
                FILE *temp = fopen(".ginit/temp", "w");
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
                rename(".ginit/temp", ".ginit/refs/stages");
            }
        }
        fclose(file); remove(".ginit/reset_tmp");
    }
}
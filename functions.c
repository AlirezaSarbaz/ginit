#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#define MAX_ADDRESS_LENGTH 1024
#define MAX_LINE_LENGTH 1024
#define MAX_BRANCH_NAME 100
#define MAX_EMAIL_USERNAME_LENGHT 100
#define MAX_FILENAME 100

int check_ginit_exist();
void run_init();
void copy_file_source_to_dest(FILE* dest , FILE* src);
char* generate_commit_id();

void run_add(int argc, char *const argv[]);
int exist_file_or_dir(const char* filepath);
void list_files_recursively(const char* basePath , const char* filename , int depth , int l);
int add_to_tracking(int argc , char* argv[]);
int check_files_modified(const char* file_path);
void update_stages();
void run_commit(int argc , char* argv[]);
int is_stage(char* pathspec);
void add_to_logs(char* argv[] , const char* commit_id);
int is_directory(const char* path);

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
    FILE* file = fopen(".ginit/stages" , "w"); fclose(file);
    file = fopen(".ginit/tracks" , "w"); fclose(file);
    //file = fopen(".ginit/filesdata" , "w"); fclose(file);
    file = fopen(".ginit/time" , "w"); fclose(file);
    file = fopen(".ginit/HEAD" , "w"); fprintf(file , "00000000 master");fclose(file);
    file = fopen(".ginit/branches" , "w"); fprintf(file , "00000000 master\n");fclose(file);
    file = fopen(".ginit/logs" , "w"); fclose(file);
    mkdir(".ginit/commits" , 0755);
}
void copy_file_source_to_dest(FILE* dest , FILE* src) {
    char buffer[MAX_LINE_LENGTH];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), src)) > 0) {
        fwrite(buffer, 1, bytes_read, dest);
    }
    fclose(dest); fclose(src);
}
void run_add(int argc, char *const argv[]) {
    // TODO: handle command in non-root directories 
    if (argc < 3) {
        perror("please specify a file or directory\n");
        exit(EXIT_FAILURE);
    }
    add_to_tracking(argc , argv);
}
int add_to_tracking(int argc , char* argv[]) {
    char cwd[MAX_ADDRESS_LENGTH];
    getcwd(cwd , sizeof(cwd));
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
                char path[MAX_ADDRESS_LENGTH]; char cwd[MAX_ADDRESS_LENGTH];
                getcwd(cwd , sizeof(cwd));
                sprintf(path , "%s/%s" , cwd , argv[2]);
                fprintf(file , "%s %s 1\n" ,argv[2] , path); fclose(file);
            }
        }
        else if (exist_file_or_dir(argv[2]) == DT_DIR) {
            if (!is_track(argv[2])) {
                FILE* file = fopen(".ginit/tracks" , "a");
                char path[MAX_ADDRESS_LENGTH]; char cwd[MAX_ADDRESS_LENGTH];
                getcwd(cwd , sizeof(cwd));
                sprintf(path , "%s/%s" , cwd , argv[2]);
                fprintf(file , "%s %s 1\n" ,argv[2] , path); fclose(file);
                int l = strlen(cwd) + 1;
                list_files_recursively(path , ".ginit/tracks" , 2 , l);
            }
        }
    }
    chdir(cwd);
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
void list_files_recursively(const char* basePath , const char* filename , int depth , int l) {
    char path[MAX_ADDRESS_LENGTH], path_copy[MAX_ADDRESS_LENGTH] , name[MAX_ADDRESS_LENGTH];
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
                strcpy(path_copy , path); memmove(path_copy , path_copy + l , strlen(path_copy) - l + 1);
                sprintf(name , "%s/%s" , path_copy ,entry->d_name);
                fprintf(file , "%s %s %d\n" , path_copy ,path , depth); fclose(file);
                list_files_recursively(path , filename , depth + 1 , l);
            }
        }
        else {
            sprintf(path , "%s/%s" , basePath , entry->d_name);
            strcpy(path_copy , path); memmove(path_copy , path_copy + l , strlen(path_copy) - l + 1);
            sprintf(name , "%s/%s" , path_copy ,entry->d_name);
            fprintf(file , "%s %s %d\n" , path_copy ,path , depth); fclose(file);
        }
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
int is_stage(char* pathspec) {
    FILE* file= fopen(".ginit/stages" , "r+");
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
int check_files_modified(const char* file_path) {
    FILE* file = fopen(".ginit/time" , "r+");
    time_t time;
    fscanf(file , "%ld" , &time); fclose(file);
    struct stat file_info;
    if (!stat(file_path, &file_info)) {
        time_t last_modified_time = file_info.st_mtime;
        if (time < last_modified_time) {
            return 1;
        } else {
            return 0;
        }
    } else {
        exit(EXIT_FAILURE);
    }
}
void update_stages() {
    FILE* file= fopen(".ginit/tracks" , "r+");
    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), file)) {
        char a[MAX_LINE_LENGTH] , b[MAX_LINE_LENGTH]; int depth;
        sscanf(line , "%s %s %d" , a , b , &depth);
        if (check_files_modified(b)) {
            long pos = ftell(file); 
            FILE *temp = fopen(".ginit/temp", "w");
            fseek(file, 0, SEEK_SET);
            char buffer[MAX_LINE_LENGTH];
            while (fgets(buffer, sizeof(buffer), file)) {
                if (ftell(file) != pos) {
                    fputs(buffer, temp);
                }
            }
            fclose(file);
            if (temp != NULL) {
                fclose(temp);
            }
            remove(".ginit/tracks");
            rename(".ginit/temp", ".ginit/tracks");
        }
    }
    fclose(file);
    return 0; 
}
void run_commit(int argc , char* argv[]) {
    char path[MAX_ADDRESS_LENGTH] , commit_id[9] , line[MAX_LINE_LENGTH] , current_commit_id[9] , current_branch[MAX_BRANCH_NAME];
    strcpy(commit_id , generate_commit_id());
    sprintf(path , ".ginit/commits/%s" , commit_id);
    mkdir(path , 0755);
    add_to_logs(argv , commit_id);
    FILE* head = fopen(".ginit/HEAD" , "r+");
    fgets(line , sizeof(line) , head);
    sscanf(line , "%s %s" , current_commit_id , current_branch);
    fseek(head , 0 , SEEK_SET); fprintf(head , "%s %s" , commit_id , current_branch);fclose(head);
    copy_stagedfiles_to_commit_dir(commit_id);
}
char* generate_commit_id() {
    char* result = malloc(9 * sizeof(char));
    srand((unsigned int)time(NULL));
    const char chars[] = {"abcdefghijklmnopqrstuvwxyz1234567890"};
    for (int i = 0; i < 8; i++) {
        int random_index = rand() % (sizeof(chars) - 1);
        result[i] = chars[random_index];
    }
    result[8] = '\0';
    return result;
}
void add_to_logs(char* argv[] , const char* commit_id) {
    FILE* file = fopen(".ginit/logs" , "a"); FILE* head = fopen(".ginit/HEAD" , "r"); FILE* config = fopen(".ginit/config" , "r");
    if (file == NULL) {
        perror("Error opening file");
        fclose(file); fclose(head); fclose(config);
        exit(EXIT_FAILURE);
    }
    char current_commit_id[9] , current_branch[MAX_BRANCH_NAME] , username[MAX_EMAIL_USERNAME_LENGHT] , email[MAX_EMAIL_USERNAME_LENGHT], line1[MAX_LINE_LENGTH] , line2[MAX_LINE_LENGTH];
    fgets(line1 , sizeof(line1) , config); fgets(line2 , sizeof(line2) , head);
    sscanf(line1 , "username : %s email : %s" , username , email); sscanf(line2 , "%s %s" , current_commit_id , current_branch);
    fprintf(file , "%s %s %s <%s> %ld %s \"%s\"\n" ,current_commit_id , commit_id , username , email , time(NULL) , current_branch , argv[3] );
    fclose(file); fclose(head); fclose(config);
}
void copy_stagedfiles_to_commit_dir(const char* commit_id) {
    FILE* tracks = fopen(".ginit/tracks" , "r");
    if (tracks == NULL) {
        perror("Error opening file");
        fclose(tracks);
        exit(EXIT_FAILURE);
    }
    char line[MAX_LINE_LENGTH] , name[MAX_FILENAME] , address[MAX_ADDRESS_LENGTH] , cwd[MAX_ADDRESS_LENGTH] ,new_path[MAX_ADDRESS_LENGTH] , new_address[MAX_ADDRESS_LENGTH];
    getcwd(cwd , sizeof(cwd));
    int l = strlen(cwd) + 1;
    while (fgets(line , sizeof(line) , tracks)) {
        sscanf(line , "%s %s " , name , address);
        strcpy(new_address , address);
        memmove(new_address , new_address + l , strlen(new_address) - l + 1);
        sprintf(new_path , "%s/.ginit/commits/%s/%s" , cwd , commit_id , new_address);
        if (is_directory(address)) {
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
    }
    fclose(tracks);
}
int is_directory(const char* path) {
    struct stat path_stat;
    if (stat(path , &path_stat)) {
        perror("Error getting file status");
        exit(EXIT_FAILURE);
    }
    return S_ISDIR(path_stat.st_mode);
}
void run_branch(int argc , char* argv[]) {
    if (argc == 3) {
        FILE* head = fopen(".ginit/HEAD" , "r");
        char current_commit_id[9] , line[MAX_LINE_LENGTH];
        fgets(line , sizeof(line) , head); sscanf(line , "%s " , current_commit_id ); fclose(head);
        FILE* branches = fopen(".ginit/branches" , "a"); fprintf(branches , "%s %s\n" , current_commit_id , argv[2]); fclose(branches);
    }
}
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

#define MAX_ADDRESS_LENGTH 1024
#define MAX_LINE_LENGTH 1024
#define MAX_BRANCH_NAME 100
#define MAX_EMAIL_USERNAME_LENGHT 100
#define MAX_FILENAME 100

int check_ginit_exist();
void run_init();
void copy_file_source_to_dest(FILE* dest , FILE* src);
char* generate_commit_id();

void run_add(int argc, char* argv[]);
void list_files_recursively(const char* basePath , const char* filename , int depth , int l);
int add_to_tracks_and_stages(int argc , char* argv[]);
int check_files_modified(const char* file_path);
void update_stages();
void run_commit(int argc , char* argv[]);
int is_in_a_ref_file(const char* pathspec , const char* ref_filename);
void add_to_logs(char* argv[] , const char* commit_id);
void copy_stagedfiles_to_commit_dir(const char* commit_id , const char* new_stage_path);
int is_directory_or_file(const char* path);
void update_modified();
void update_deleted();
void update_added();
int is_ok_for_checkout();
void run_checkout(char* argv[]);
void remove_tracks();
void remove_nullspaces(char* filename1 , int begin , int end);
void run_diff(int argc , char* argv[]);
int is_blanck_line(const char* line , int length);
void find_diffs (const char* file1_name , const char* file2_name);
int number_of_lines(FILE* file);
void run_config(int argc , char* argv[]);
void run_diff_for_commit(const char* address1 , const char* address2 , const char* name , const char* commit_id1 , const char* commit_id2);

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
void run_init() {
    mkdir(".ginit" , 0755);
    char cwd[MAX_ADDRESS_LENGTH];
    getcwd(cwd , sizeof(cwd));

    FILE* local_config = fopen(".ginit/config" , "wb");
    const char* home = getenv("HOME"); chdir(home);
    FILE* global_config = fopen(".ginitconfig" , "rb");
    copy_file_source_to_dest(local_config , global_config);
    chdir(cwd);
    mkdir(".ginit/refs" , 0755); mkdir(".ginit/commits" , 0755); mkdir(".ginit/branches" , 0755);
    FILE* file = fopen(".ginit/refs/stages" , "w"); fclose(file);
    file = fopen(".ginit/refs/tracks" , "w"); fclose(file);
    file = fopen(".ginit/refs/allfiles" , "w"); fclose(file);
    file = fopen(".ginit/refs/allfiles_copy" , "w"); fclose(file);
    file = fopen(".ginit/refs/deleted" , "w"); fclose(file);
    file = fopen(".ginit/refs/added" , "w"); fclose(file);
    file = fopen(".ginit/refs/modified" , "w"); fclose(file);
    file = fopen(".ginit/commit_ids" , "w"); fprintf(file , "00000000\n"); fclose(file);
    file = fopen(".ginit/time" , "w"); fclose(file);
    file = fopen(".ginit/branches/master" , "w"); fprintf(file , "00000000");fclose(file);
    file = fopen(".ginit/HEAD" , "w"); fprintf(file , "00000000 master");fclose(file);
    file = fopen(".ginit/branch" , "w"); fprintf(file , "master\n");fclose(file);
    file = fopen(".ginit/logs" , "w"); fclose(file);
}
void copy_file_source_to_dest(FILE* dest , FILE* src) {
    char buffer[MAX_LINE_LENGTH];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), src)) > 0) {
        fwrite(buffer, 1, bytes_read, dest);
    }
    fclose(dest); fclose(src);
}
void run_add(int argc, char* argv[]) {
    // TODO: handle command in non-root directories 
    if (argc < 3) {
        perror("please specify a file or directory\n");
        exit(EXIT_FAILURE);
    }
    add_to_tracks_and_stages(argc , argv);
}
int add_to_tracks_and_stages(int argc , char* argv[]) {
    char cwd[MAX_ADDRESS_LENGTH];
    getcwd(cwd , sizeof(cwd));
    if (argc == 3 && !strcmp(argv[2] , "-redo")) {
    // دستور redo
    }
    else if (argc == 3) {
        if (!is_in_a_ref_file(argv[2] , ".ginit/refs/allfiles")) {
            fprintf(stderr,"pathspec \"%s\" dosen't match any file\n" , argv[2]);
            exit(EXIT_FAILURE);
        }
        else {
            char path[MAX_ADDRESS_LENGTH]; char cwd[MAX_ADDRESS_LENGTH];
            getcwd(cwd , sizeof(cwd));
            int l = strlen(cwd) + 1;
            sprintf(path , "%s/%s" , cwd , argv[2]);
            if (is_directory_or_file(path)) {
                if (!is_in_a_ref_file(argv[2] , ".ginit/refs/tracks")) {
                    FILE* file = fopen(".ginit/refs/tracks" , "a");
                    fprintf(file ,"%s %s \n" , argv[2] , path); fclose(file);
                }
                if (!is_in_a_ref_file(argv[2] , ".ginit/refs/stages")) {
                    FILE* file = fopen(".ginit/refs/stages" , "a");
                    fprintf(file ,"%s %s \n" , argv[2] , path); fclose(file);
                }
                list_files_recursively(path , ".ginit/refs/stages" , 1 , l); list_files_recursively(path , ".ginit/refs/tracks" , 1 , l);
            }
            else {
                 if (!is_in_a_ref_file(argv[2] , ".ginit/refs/tracks")) {
                    FILE* file = fopen(".ginit/refs/tracks" , "a");
                    fprintf(file ,"%s %s \n" , argv[2] , path); fclose(file);
                }
                if (!is_in_a_ref_file(argv[2] , ".ginit/refs/stages")) {
                    FILE* file = fopen(".ginit/refs/stages" , "a");
                    fprintf(file ,"%s %s \n" , argv[2] , path); fclose(file);
                }
            }
        }
    }
    chdir(cwd);
    return 0;
}
void list_files_recursively(const char* basePath , const char* filename , int depth , int l) {
    char path[MAX_ADDRESS_LENGTH], name[MAX_ADDRESS_LENGTH];
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
int is_in_a_ref_file(const char* pathspec , const char* ref_filename) {
    FILE* file = fopen(ref_filename , "r+");
    char line[MAX_LINE_LENGTH] , tmp[MAX_ADDRESS_LENGTH];
    while (fgets(line, sizeof(line), file)) {
        sscanf(line , "%s " , tmp);
        if (!strcmp(pathspec , tmp)) {
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
    FILE* file= fopen(".ginit/refs/stages" , "r+");
    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), file)) {
        char a[MAX_LINE_LENGTH] , b[MAX_LINE_LENGTH]; int depth;
        sscanf(line , "%s %s %d" , a , b , &depth);
        if ((check_files_modified(b) && !is_directory_or_file(b)) || (is_in_a_ref_file(a , ".ginit/refs/deleted") && !is_directory_or_file(b))) {
            long pos = ftell(file); 
            FILE *temp = fopen(".ginit/refs/temp", "w");
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
            remove(".ginit/refs/stages");
            rename(".ginit/refs/temp", ".ginit/refs/stages");
            fseek(file , 0 , SEEK_SET);
        }
    }
    fclose(file);
}
void run_commit(int argc , char* argv[]) {
    char path[MAX_ADDRESS_LENGTH] , commit_id[9] , line[MAX_LINE_LENGTH] , current_commit_id[9] , current_branch[MAX_BRANCH_NAME] , stage_address[MAX_ADDRESS_LENGTH];
    strcpy(commit_id , generate_commit_id());
    sprintf(path , ".ginit/commits/%s" , commit_id);
    mkdir(path , 0755);
    sprintf(stage_address , "%s/stages" , path);
    add_to_logs(argv , commit_id);
    FILE* head = fopen(".ginit/HEAD" , "r+");
    fgets(line , sizeof(line) , head);
    sscanf(line , "%s %s" , current_commit_id , current_branch);
    fseek(head , 0 , SEEK_SET); fprintf(head , "%s %s" , commit_id , current_branch);fclose(head);
    char branch_path[MAX_ADDRESS_LENGTH]; sprintf(branch_path , ".ginit/branches/%s" , current_branch);
    FILE* branch = fopen(branch_path , "w"); fprintf(branch ,"%s" , commit_id); fclose(branch);
    FILE* commit_ids = fopen(".ginit/commit_ids" , "a"); fprintf(commit_ids , "%s\n" , commit_id); fclose(commit_ids);
    copy_stagedfiles_to_commit_dir(commit_id , stage_address);
    FILE* stages = fopen(".ginit/refs/stages" , "w"); fclose(stages);
    FILE* deleted = fopen(".ginit/refs/deleted" , "w"); fclose(deleted);
    FILE* added = fopen(".ginit/refs/added" , "w"); fclose(added);
    FILE* modified = fopen(".ginit/refs/modified" , "w"); fclose(modified);
    FILE* allfiles_copy = fopen(".ginit/refs/allfiles_copy" , "w"); fclose(allfiles_copy);
    char cwd[MAX_ADDRESS_LENGTH]; getcwd(cwd , sizeof(cwd));
    int l = strlen(cwd) + 1;
    list_files_recursively(cwd , ".ginit/refs/allfiles_copy" , 1 , l);
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
void copy_stagedfiles_to_commit_dir(const char* commit_id , const char* new_stage_path) {
    FILE* tracks = fopen(".ginit/refs/stages" , "r");
    if (tracks == NULL) {
        perror("Error opening file");
        fclose(tracks);
        exit(EXIT_FAILURE);
    }
    char line[MAX_LINE_LENGTH] , name[MAX_FILENAME] , address[MAX_ADDRESS_LENGTH] , cwd[MAX_ADDRESS_LENGTH] ,new_path[MAX_ADDRESS_LENGTH];
    getcwd(cwd , sizeof(cwd));
    int l = strlen(cwd) + 1;
    FILE* new_stage = fopen(new_stage_path , "a");
    while (fgets(line , sizeof(line) , tracks)) {
        sscanf(line , "%s %s " , name , address);
        sprintf(new_path , "%s/.ginit/commits/%s/%s" , cwd , commit_id , name);
        fprintf(new_stage ,"%s %s\n" , name , new_path);
        if (is_directory_or_file(address)) {
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
    fclose(tracks); fclose(new_stage);
}
int is_directory_or_file(const char* path) {
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
        FILE* branches = fopen(".ginit/branch" , "a"); fprintf(branches , "%s\n" ,argv[2]); fclose(branches);
        char path[MAX_ADDRESS_LENGTH]; sprintf(path , ".ginit/branches/%s" , argv[2]);
        FILE* branch = fopen(path , "w"); fprintf(branch , "%s" , current_commit_id);
    }
}
void update_modified() {
    FILE* file= fopen(".ginit/refs/allfiles" , "r+");
    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), file)) {
        char a[MAX_LINE_LENGTH] , b[MAX_LINE_LENGTH]; int depth;
        sscanf(line , "%s %s %d" , a , b , &depth);
        if (check_files_modified(b) && !is_directory_or_file(b) && !is_in_a_ref_file(a , ".ginit/refs/modified")) {
            FILE* modified = fopen(".ginit/refs/modified" , "a"); fprintf(modified , "%s" , line); fclose(modified);
        }
    }
    fclose(file);
}
void update_deleted() {
    FILE* file = fopen(".ginit/refs/allfiles_copy" , "r+");
    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), file)) {
        char a[MAX_LINE_LENGTH] , b[MAX_LINE_LENGTH]; int depth;
        sscanf(line , "%s %s %d" , a , b , &depth);
        if (!is_in_a_ref_file(a , ".ginit/refs/allfiles") && is_in_a_ref_file(a , ".ginit/refs/allfiles_copy") && !is_in_a_ref_file(a , ".ginit/refs/deleted")) {
            FILE* deleted = fopen(".ginit/refs/deleted" , "a"); fprintf(deleted , "%s" , line); fclose(deleted);
        }
    }
    fclose(file);
}
void update_added() {
    FILE* file = fopen(".ginit/refs/allfiles" , "r+");
    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), file)) {
        char a[MAX_LINE_LENGTH] , b[MAX_LINE_LENGTH]; int depth;
        sscanf(line , "%s %s %d" , a , b , &depth);
        if (!is_in_a_ref_file(a , ".ginit/refs/allfiles_copy") && is_in_a_ref_file(a , ".ginit/refs/allfiles") && !is_in_a_ref_file(a , ".ginit/refs/added")) {
            FILE* added = fopen(".ginit/refs/added" , "a"); fprintf(added , "%s" , line); fclose(added);
        }
    }
    fclose(file);

}
void run_config(int argc , char* argv[]) {
    if (!strcmp(argv[2] , "-global")) {
        FILE* local_config = fopen(".ginit/config" , "wb");
        const char* home = getenv("HOME"); chdir(home);
        FILE* global_config = fopen(".ginitconfig" , "r+");
        char line[MAX_LINE_LENGTH] , tmp1[20] , tmp2[20] , username[100] , email[100];
        fgets(line , sizeof(line) , global_config); fclose(global_config); fopen(".ginitconfig" , "w");
        sscanf(line , "%s : %s %s : %s" , tmp1 , username , tmp2 , email);
        if (!strcmp(argv[3] , "username")) {
            strcpy(username , argv[4]);
            fprintf(global_config , "%s : %s %s : %s" , tmp1 , username , tmp2 , email);
        }
        else if (!strcmp(argv[3] , "email")) {
            strcpy(email , argv[4]);
            fprintf(global_config , "%s : %s %s : %s" , tmp1 , username , tmp2 , email);
        }
        fclose(global_config); fopen(".ginitconfig" , "rb");
        copy_file_source_to_dest(local_config , global_config);
    }
    else {

    }
}
int is_ok_for_checkout(){
   FILE* file = fopen(".ginit/refs/allfiles" , "r+");
    char line[MAX_LINE_LENGTH] , tmp[MAX_ADDRESS_LENGTH];
    while (fgets(line, sizeof(line), file)) {
        sscanf(line , "%s " , tmp);
        if (is_in_a_ref_file(tmp , ".ginit/refs/tracks") && is_in_a_ref_file(tmp , ".ginit/refs/modified") && !is_in_a_ref_file(tmp , ".ginit/refs/stages")) {
            return 0;
        }
    }
    fclose(file);
    return 1;
}
void run_checkout(char* argv[]) {
    if (!strcmp(argv[2] , "HEAD")) {
        char HEAD_commit_id[9] , path[MAX_ADDRESS_LENGTH] , mvCommand[MAX_ADDRESS_LENGTH] , cwd[MAX_ADDRESS_LENGTH]; getcwd(cwd , sizeof(cwd));
        FILE* file = fopen(".ginit/HEAD" , "r"); fscanf(file , "%s " , HEAD_commit_id); fclose(file);
        remove_tracks();
        sprintf(path , "%s/.ginit/commits/%s/*" , cwd ,HEAD_commit_id);
        sprintf(mvCommand , "cp -r %s %s" , path , cwd);
        system(mvCommand);
    }
    else if (!is_in_a_ref_file(argv[2] , ".ginit/commit_ids") && !is_in_a_ref_file(argv[2] , ".ginit/branch")) {
        fprintf(stderr,"pathspec \"%s\" dosen't match any branch or commit\n" , argv[2]);
        exit(EXIT_FAILURE);
    }
    else if (is_in_a_ref_file(argv[2] , ".ginit/commit_ids")) {
        char path[MAX_ADDRESS_LENGTH] , mvCommand[MAX_ADDRESS_LENGTH] , cwd[MAX_ADDRESS_LENGTH]; getcwd(cwd , sizeof(cwd));
        remove_tracks();
        sprintf(path , "%s/.ginit/commits/%s/*" , cwd ,argv[2]);
        sprintf(mvCommand , "cp -r %s %s" , path , cwd);
        system(mvCommand);
    }
    else if (is_in_a_ref_file(argv[2] , ".ginit/branch")) {
        char path[MAX_ADDRESS_LENGTH] , mvCommand[MAX_ADDRESS_LENGTH] , cwd[MAX_ADDRESS_LENGTH] , commit_id[9] , branch_path[MAX_ADDRESS_LENGTH]; getcwd(cwd , sizeof(cwd));
        sprintf(branch_path , ".ginit/branches/%s" , argv[2]);
        FILE* file = fopen(branch_path , "r"); fscanf(file , "%s " , commit_id); fclose(file);
        remove_tracks();
        sprintf(path , "%s/.ginit/commits/%s/*" , cwd ,commit_id);
        sprintf(mvCommand , "cp -r %s %s" , path , cwd);
        system(mvCommand);
    }
    /*if () {
        //handle HEAD-n
    }*/
}
void remove_tracks() {
    FILE* file = fopen(".ginit/refs/tracks" , "r+");
    char line[MAX_LINE_LENGTH] , tmp[MAX_ADDRESS_LENGTH] , tmp2[MAX_ADDRESS_LENGTH] , rmCommand[MAX_ADDRESS_LENGTH];
    while (fgets(line, sizeof(line), file)) {
        sscanf(line , "%s %s " , tmp , tmp2);
        if (access(tmp2 , F_OK) != -1) {
            if (!is_directory_or_file(tmp2)) {
                sprintf(rmCommand , "rm %s" , tmp2);
                system(rmCommand);
            }
            else {
                sprintf(rmCommand , "rm -r %s" , tmp2);
                system(rmCommand);
            }
        }
    }
    fclose(file);
}
void remove_nullspaces(char* filename1 , int begin , int end) {
    char file_path[MAX_ADDRESS_LENGTH], file_copy_path[MAX_ADDRESS_LENGTH] , cwd[MAX_ADDRESS_LENGTH] , line[MAX_LINE_LENGTH];
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
                line[length - 1] = '\n';
                fprintf(output , "%s" , line);
            }  
            counter ++;
        }
    }
    fclose(input); fclose(output);
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
            exit(EXIT_FAILURE);
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
            char cwd[MAX_ADDRESS_LENGTH] , stages1[MAX_ADDRESS_LENGTH] , stages2[MAX_ADDRESS_LENGTH] , line[MAX_ADDRESS_LENGTH] , line2[MAX_ADDRESS_LENGTH] , name[MAX_ADDRESS_LENGTH] , find[MAX_ADDRESS_LENGTH] , address[MAX_ADDRESS_LENGTH] , address2[MAX_ADDRESS_LENGTH];
            getcwd(cwd , sizeof(cwd));
            sprintf(stages1 , "%s/.ginit/commits/%s/stages" , cwd , argv[3]); sprintf(stages2 , ".ginit/commits/%s/stages" , argv[4]);
            FILE* stages_1 = fopen(stages1, "r"); FILE* stages_2 = fopen(stages2 , "r");
            while (fgets(line , sizeof(line) , stages_1)) {
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
                if (!is_directory_or_file(address)) {
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
int is_blanck_line(const char* line , int length) {
   for (int i = 0; i < length; i++) {
        if (!isspace(line[i])) {
            return 0;
        }
    }
    return 1;
}
void find_diffs (const char* file1_name , const char* file2_name) {
    char file1_path[MAX_ADDRESS_LENGTH], file2_path[MAX_ADDRESS_LENGTH] , cwd[MAX_ADDRESS_LENGTH] , line1[MAX_LINE_LENGTH] , line2[MAX_LINE_LENGTH];
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
void run_diff_for_commit(const char* address1 , const char* address2 , const char* name , const char* commit_id1 , const char* commit_id2) {
    char copy1[MAX_ADDRESS_LENGTH] , copy2[MAX_ADDRESS_LENGTH] , line[MAX_LINE_LENGTH] , line1[MAX_LINE_LENGTH] , line2[MAX_LINE_LENGTH];
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
        line[length - 1] = '\0';
        if (!is_blanck_line(line , length - 1)){
            line[length - 1] = '\n';
            fprintf(output2 , "%s" , line);
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
    //remove(copy1); remove(copy2);
}
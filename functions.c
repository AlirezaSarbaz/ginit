#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#define MAX_ADDRESS_LENGHT 1024

int check_ginit_exist();
void run_init();


void run_config(int argc , char* argv[]);
void copy_file_source_to_dest(FILE* dest , FILE* source);

int check_ginit_exist() {
    char cwd[MAX_ADDRESS_LENGHT];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("getcwd\n");
        exit(EXIT_FAILURE);
    }
    char tmp_cwd[MAX_ADDRESS_LENGHT];
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

    FILE* local_config = fopen(".ginit/config" , "wb");
    const char* home = getenv("HOME"); chdir(home);
    FILE* global_config = fopen(".ginitconfig" , "rb");
    copy_file_source_to_dest(local_config , global_config);

}
void run_config(int argc , char* argv[]) {
    if (!strcmp(argv[2] , "-global")) {
        //FILE* local_config = fopen(".ginit/config" , "wb");
        const char* home = getenv("HOME"); chdir(home);
        FILE* global_config = fopen(".ginitconfig" , "r+");
        char line[1024];
        while (fgets(line, sizeof(line), global_config)) {
            char *found = strstr(line, argv[3]);
            if (found != NULL) {
                printf("%d\n" , ftell(global_config));
                fseek(global_config, -strlen(line), SEEK_CUR);
                printf("%d\n" , ftell(global_config));
                fprintf(global_config, "%s : %s", argv[3] ,argv[4]);
            }
        }
        fclose(global_config); //fopen(".ginitconfig" , "rb");
        //copy_file_source_to_dest(local_config , global_config);
    }
    else {

    }
}
void copy_file_source_to_dest(FILE* dest , FILE* source) {
    char buffer[1024];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), source)) > 0) {
        fwrite(buffer, 1, bytes_read, dest);
    }
    fclose(dest); fclose(source);
}
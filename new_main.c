#include "new_functions.c"

int main(int argc , char* argv[]) {
    if (argc < 2) {
        fprintf(stderr ,"please enter a valid command\n");
        return 1;
    }
    if (!strcmp(argv[1] , "init")) {
        if (check_ginit_exist() == 0) {
            run_init();
            FILE* file = fopen(".ginit/time" , "w");
            if (file == NULL) {
                perror("error opening time file");
                exit(EXIT_FAILURE);
            }
            fprintf(file , "%ld\n" , time(NULL));
            fclose(file);
            char cwd[1024]; getcwd(cwd , sizeof(cwd));
            int l = strlen(cwd) + 1;
            list_files_recursively(cwd , ".ginit/refs/allfiles" , 1 , l);
            list_files_recursively(cwd , ".ginit/refs/allfiles_copy" , 1 , l);
            return 0;
        }
        else {
            fprintf(stderr , "ginit repository has already initialized\n");
            return 1;
        }
    }
    else {
        if (check_ginit_exist() == 0) {
            fprintf(stderr , "not a ginit repository (or any parent up to mount point /)\n");
            return 1;
        }
        char cwd[1024]; getcwd(cwd , sizeof(cwd));
        FILE* local_config = fopen(".ginit/config" , "wb");
        if (local_config == NULL) {
            perror("error opening local_config file\n");
            return 1;
        }
        char* home = getenv("HOME"); chdir(home);
        FILE* global_config = fopen(".ginitconfig" , "rb");
        if (global_config == NULL) {
            perror("error opening global_config file\n");
            return 1;
        }
        copy_file_source_to_dest(local_config , global_config);
        chdir(cwd);
        int l = strlen(cwd) + 1;
        FILE* file = fopen(".ginit/refs/allfiles" , "w"); fclose(file);
        list_files_recursively(cwd , ".ginit/refs/allfiles" , 1 , l);
        update_modified();
        update_added();
        update_deleted();
        update_tracks();


        if (!strcmp(argv[1] , "config")) {
            run_config(argv);
        }
        else if (!strcmp(argv[1] , "add")) {
            run_add(argc, argv);
        }
        else if (!strcmp(argv[1] , "status")) {
            run_status();
        }


        file = fopen(".ginit/time" , "w+");
        if (file == NULL) {
            perror("error opening time file");
            exit(EXIT_FAILURE);
        }
        fprintf(file , "%ld\n" , time(NULL)); fclose(file);
    }
    return 0;
}
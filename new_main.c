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
        FILE* local_alis = fopen(".ginit/local_alias" , "wb+");
        if (local_config == NULL) {
            perror("error opening local_alias file\n");
            return 1;
        }
        char* home = getenv("HOME"); chdir(home);
        FILE* global_config = fopen(".ginitconfig" , "rb");
        if (global_config == NULL) {
            perror("error opening global_config file\n");
            return 1;
        }
        FILE* global_alias = fopen(".ginitalias" , "rb");
        if (global_config == NULL) {
            perror("error opening global_alias file\n");
            return 1;
        }
        copy_file_source_to_dest(local_config , global_config);
        copy_file_source_to_dest(local_alis , global_alias);
        chdir(cwd);
        int l = strlen(cwd) + 1;
        FILE* file = fopen(".ginit/refs/allfiles" , "w"); fclose(file);
        list_files_recursively(cwd , ".ginit/refs/allfiles" , 1 , l);
        update_added();
        update_deleted();
        update_modified();
        update_tracks();
        update_stages();

        if (!strcmp(argv[1] , "config")) {
            run_config(argv);
        }
        else if (!strcmp(argv[1] , "add")) {
            run_add(argc, argv);
        }
        else if (!strcmp(argv[1] , "status")) {
            run_status();
        }
        else if (!strcmp(argv[1] , "commit")) {
            run_commit(argc , argv);
        }
        else if (!strcmp(argv[1] , "set")) {
            run_set(argv);
        }
        else if (!strcmp(argv[1] , "replace")) {
            run_replace(argv);
        }
        else if (!strcmp(argv[1] , "remove")) {
            run_remove(argv);
        }
        else if (!strcmp(argv[1] , "branch")) {
            run_branch(argc , argv);
        }
        else if (!strcmp(argv[1] , "diff")) {
            run_diff(argc , argv);
        }
        else if (!strcmp(argv[1] , "reset")) {
            run_reset(argc , argv);
        }
        else if (!strcmp(argv[1] , "merge")) {
            run_merge(argv);
        }
        else if (!strcmp(argv[1] , "grep")) {
            run_grep(argc , argv);
        }
        else if (!strcmp(argv[1] , "log")) {
            run_log(argc , argv);
        }
        else if (!strcmp(argv[1] , "tag")) {
            run_tag(argc , argv);
        }
        else if (!strcmp(argv[1] , "checkout")) {
            if (!is_ok_for_checkout_or_merge()) {
                perror("please commit your changes or stash them before you checkout\n");
                exit(EXIT_FAILURE);
            }
            else {
                run_checkout(argv);
            }
        }
        else {
            if (is_in_a_ref_file(argv[1] , ".ginit/local_alias")) {
                file = fopen(".ginit/local_alias" , "r");
                char line[1024];
                while (fgets(line , sizeof(line) , file) != NULL) {
                    char name[100] , command[100];
                    sscanf(line , "%s %[^\n]s" , name , command);
                    if (!strcmp(name , argv[1])) {
                        char rmcommand[100];
                        sprintf(rmcommand , "ginit %s" , cut_end_and_first(command));
                        system(rmcommand);
                        break;
                    }
                }
                fclose(file);
            }
            else {
                perror("enter valid cammand\n");
                exit(EXIT_SUCCESS);
            }
        }
        file = fopen(".ginit/time" , "w+");
        if (file == NULL) {
            perror("error opening time file");
            exit(EXIT_FAILURE);
        }
        fprintf(file , "%ld\n" , time(NULL)); fclose(file);
    }
    exit(EXIT_SUCCESS);
}

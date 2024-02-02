#include "functions.c"

int main(int argc , char* argv[]) {
    if (argc < 2) {
        perror("please enter a valid command\n");
        return 1;
    }
    if (!strcmp(argv[1] , "init")) {
        if (!check_ginit_exist()) {
            run_init();
            FILE* file = fopen(".ginit/time" , "w+");
            fprintf(file , "%ld" , time(NULL)); fclose(file);
            char cwd[MAX_ADDRESS_LENGTH]; getcwd(cwd , sizeof(cwd));
            int l = strlen(cwd) + 1;
            list_files_recursively(cwd , ".ginit/refs/allfiles" , 1 , l);
            list_files_recursively(cwd , ".ginit/refs/allfiles_copy" , 1 , l);
            return 0;
        }
        else {
            perror("ginit repository has already initialized\n");
            return 1;
        }
    }
    else {
        if (!check_ginit_exist()) {
            perror("not a ginit repository (or any parent up to mount point /)\n");
            exit(EXIT_FAILURE);
        }
        char cwd[MAX_ADDRESS_LENGTH]; getcwd(cwd , sizeof(cwd));
        int l = strlen(cwd) + 1;
        FILE* file = fopen(".ginit/refs/allfiles" , "w");fclose(file);
        list_files_recursively(cwd , ".ginit/refs/allfiles" , 1 , l);
        update_deleted();
        update_stages();
        update_modified();
        update_added();
        if (!strcmp(argv[1] , "add")) {
            run_add(argc , argv);
        }
        else if (!strcmp(argv[1] , "commit")) {
            run_commit(argc , argv);
        }
        else if (!strcmp(argv[1] , "branch")) {
            run_branch(argc , argv);
        }
        else if (!strcmp(argv[1] , "config")) {
            run_config(argc , argv);
        }
        else if (!strcmp(argv[1] , "diff")) {
            run_diff(argc , argv);
        }
        else if (!strcmp(argv[1] , "checkout")) {
            if (!is_ok_for_checkout()) {
                perror("please commit your changes or stash them before you checkout\n");
                exit(EXIT_FAILURE);
            }
            else {
                run_checkout(argv);
            }
            
        }
    }
    FILE* file = fopen(".ginit/time" , "w+");
    fprintf(file , "%ld" , time(NULL)); fclose(file);

    return 0;
}

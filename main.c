#include "functions.c"

int main(int argc , char* argv[]) {
    if (argc < 2) {
        perror("please enter a valid command\n");
        return 1;
    }
    else {
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
        else if (!strcmp(argv[1] , "add")) {
            run_add(argc , argv);
        }
        else if (!strcmp(argv[1] , "commit")) {
            run_commit(argc , argv);
        }
        else if (!strcmp(argv[1] , "branch")) {
            run_branch(argc , argv);
        }
        char cwd[MAX_ADDRESS_LENGTH]; getcwd(cwd , sizeof(cwd));
        int l = strlen(cwd) + 1;
        FILE* file = fopen(".ginit/refs/allfiles" , "w");fclose(file);
        list_files_recursively(cwd , ".ginit/refs/allfiles" , 1 , l);
        update_stages();
        update_modified();
        update_deleted();
        update_added();
    }
    FILE* file = fopen(".ginit/time" , "w+");
    fprintf(file , "%ld" , time(NULL)); fclose(file);
    return 0;
}

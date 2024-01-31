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
        update_stages();
    }
    FILE* file = fopen(".ginit/time" , "w+");
    fprintf(file , "%ld" , time(NULL)); fclose(file);
    return 0;
}

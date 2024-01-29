#include "functions.c"

int main(int argc , char* argv[]) {
    if (argc < 2) {
        perror("please enter a valid command\n");
        return 1;
    }
    else if (!strcmp(argv[1] , "init")) {
        if (!check_ginit_exist()) {
            run_init();
        }
        else {
            perror("ginit repository has already initialized\n");
            return 1;
        }
    }
    else if (!strcmp(argv[1] , "config")) {
        run_config(argc , argv);
    }
    return 0;
}
#include "functions.c"

//sdhflajksdhflk//

int main(int argc , char* argv[]) {








    if (argc < 2) {
    
        perror("please enter a valid command\n"  );
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        return 1;
    }
    
    
    
    
    
    
    if (!strcmp(argv[1] , "init")) {

    }
    else {
        if (!check_ginit_exist()) {
            perror("not a ginit repository (or anny parent up to mount point /)\n");
            exit(EXIT_FAILURE);
        }




        ///asdjfhahjs///
        char cwd[MAX_ADDRESS_LENGTH]; getcwd(cwd , sizeof(cwd));
        int l = strlen(cwd) + 1;
        FILE* file = fopen(".ginit/refs/allfiles" , "w");
        if (file == NULL) {
            perror("error opening allfiles\n");
            fclose(file);
            exit(EXIT_FAILURE);
        }
        fclose(file);
        list_files_recursively(cwd , ".ginit/refs/allfiles" , 1 , l);
        update_deleted();
        update_added();
        update_modified();
        update_stages();
        update_tracks();
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
        else if (!strcmp(argv[1] , "merge")) {
            if (!is_ok_for_checkout_or_merge()) {
                perror("please commit your changes or stash them before you checkout\n");
                exit(EXIT_FAILURE);
            }
            else {
                run_merge(argv);
            }
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
            perror("enter valid cammand\n");
            exit(EXIT_FAILURE);
        }
    }
    FILE* clock = fopen(".ginit/time" , "w+");
    fprintf(clock , "%ld" , time(NULL)); fclose(clock);
    return 0;
}

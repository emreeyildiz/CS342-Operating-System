#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/time.h>
int main(int argc, char const *argv[]) {
    if(argc != 3){
      perror("Invalid number of arguments\n");
      exit(1);
    }

    else{
      int arg2 = atoi(argv[2]);
      int arg1 = atoi(argv[1]);

      if(arg2 > 2 || arg2 < 1){
        printf("Invalid mode is entered\n");
        exit(1);
      }

      while(1){
        char buffer[1024];
        char *b = buffer;
        size_t bufsize = 1024;
        printf("\033[1;34m");
        printf("./isp: ");
        printf("\033[0m");
        getline(&b, &bufsize, stdin);
        char* NewBuffer = strtok(buffer, "\n");
        int flag = -1;
        char bar[] = "|";
        char* token;
        char* rest = NewBuffer;
        char *strings[64];
        int counter = 0;
        if(!NewBuffer){
          continue;
        }
        while((token = strtok_r(rest, " ", &rest))){

          if(strcmp(bar, token) == 0){
            flag = counter;
          }
          else{
            strings[counter] = token;
            counter = counter + 1;
          }
        }
        struct timeval t1, t2;
        gettimeofday(&t1, NULL);
        if(arg2 == 1){
          int child1 = -1;
          int child2 = -1;
          int pip1[2]; //pip1
          if(pipe(pip1) < 0){
            perror("pip is failed");
            exit(1);
          }
          child1 = fork();
          if(child1 < 0){
            perror("Child1 is failed");
            exit(1);
          }

          if(flag == -1 && child1 == 0){
            char *cmd = strings[0];
            char *argv[counter + 1];
            for(int i = 0; i < counter; i++){
              argv[i] = strings[i];
            }
            argv[counter] = NULL;
            execvp(cmd, argv);
          }

          if(child1 > 0){
            child2 = fork();
          }
          if(child1 == 0){
            close(pip1[0]);
            dup2(pip1[1],1);
            close(pip1[1]);
            char *cmd = strings[0];
            char *argv[flag + 1];
            for(int i = 0; i < flag; i++){
              argv[i] = strings[i];
            }
            argv[flag] = NULL;
            execvp(cmd, argv);
            perror("Child 1 execv failed");
            exit(1);
          }

          if(child2 == 0){
            close(pip1[1]);
            dup2(pip1[0], 0);
            close(pip1[0]);

            char *cmd = strings[flag];
            char *argv[counter - flag + 1];
            for(int i = 0; i < counter - flag; i++){
              argv[i] = strings[counter-flag + i];
            }
            argv[counter-flag] = NULL;
            execvp(cmd, argv);
            perror("Child 2 execv failed");
            exit(1);
          }
          close(pip1[0]);
          close(pip1[1]);
          waitpid(child1, NULL,0);
          waitpid(child2, NULL,0);

        }

        //Mode 2 starts

        else if (arg2 == 2){
          int x;
          pid_t child1 = 0;
          pid_t child2 = 0;
          int pip1[2]; //pip1
          int pip2[2]; //pip2
          if(pipe(pip1) < 0){
            perror("pip is failed");
            exit(1);
          }
          if(pipe(pip2) < 0){
            perror("pip is failed");
            exit(1);
          }
          child1 = fork();
          if(child1 < 0){
            perror("Child1 is failed");
            exit(1);
          }


          if(flag == -1 && child1 == 0){
            char *cmd = strings[0];
            char *argv[counter + 1];
            for(int i = 0; i < counter; i++){
              argv[i] = strings[i];
            }
            argv[counter] = NULL;
            execvp(cmd, argv);
          }



          if(child1 > 0){
              child2 = fork();
          }
          if(child1 == 0){

            close(pip1[0]);
            close(pip2[0]);
            close(pip2[1]);

            x = dup2(pip1[1],1);

            if(x == -1){
              perror("dup2 failed");
              exit(1);
            }
            close(pip1[1]);

            char *cmd = strings[0];
            char *argv[flag + 1];
            for(int i = 0; i < flag; i++){
              argv[i] = strings[i];
            }
            argv[flag] = NULL;
            execvp(cmd, argv);
            perror("Child 1 execv failed");
            exit(1);
          }
          int writeCount = 0;
          int readCount = 0;
          int characterCount = 0;
          char* buffer1 = malloc(arg1*sizeof(char));
          if(child1 > 0 && child2 > 0){ //Parent
              close(pip1[1]);
              close(pip2[0]);
              int bytes;
              readCount = 1;
              while((bytes = read(pip1[0],buffer1,arg1))){
                write(pip2[1], buffer1, bytes);
                readCount = readCount + 1;
                characterCount = characterCount + bytes;
                writeCount = writeCount + 1;
              }

              close(pip1[0]);
              close(pip2[1]);

          }

            if(child2 == 0){
              printf("\033[0m");
              close(pip1[1]);
              close(pip1[0]);
              close(pip2[1]);
              dup2(pip2[0], 0);
              close(pip2[0]);
              char *cmd = strings[flag];
              char *argv[counter - flag + 1];
              for(int i = 0; i < counter - flag; i++){
                argv[i] = strings[counter-flag + i];
              }
              argv[counter-flag] = NULL;

              // char *cmd = "grep";
              // char *argv[3];
              // argv[0] = "grep";
              // argv[1] = "mode";
              // argv[2] = NULL;
              // execv("|", "|", "sort", NULL);
              execvp(cmd, argv);
              perror("Child 2 execv failed");
              exit(1);
            }

          waitpid(child1, NULL,0);
          waitpid(child2, NULL,0);
          printf("\033[1;32m");
          printf("character-count: %d\n", characterCount);

          printf("read-call-count: %d\n", readCount);

          printf("write-call-count: %d\n", writeCount);
          free(buffer1);

          printf("\033[0m");


          close(pip1[0]);
          close(pip1[1]);
          close(pip2[0]);
          close(pip2[1]);
        }
        printf("\e[0;33m");
        gettimeofday(&t2, NULL);
        double elapsedTime = (t2.tv_sec - t1.tv_sec)*1000;
        double elapsedTime2 = (t2.tv_usec - t1.tv_usec)/1000;
        printf("%f ms. \n ", elapsedTime + elapsedTime2);
      }
    }
}

#include <stdlib.h>
#include <stdio.h>
int main(int argc, char const *argv[]) {
  char* arr = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  for(int i = 0; i < atoi(argv[1]); i++){
    int rnd = rand()%62;
    printf("%c", arr[rnd]);
  }
  return 0;
}

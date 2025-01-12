#include <stdio.h>
#include <unistd.h>

int main(){
    for(int i=0; i<10; i++){
        printf("Hello World !\n");
    }
    sleep(2);
    return 0;
}
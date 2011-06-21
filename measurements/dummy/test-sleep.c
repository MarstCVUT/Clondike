#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/timeb.h>


int main(int argc, char *argv[])
{
    int sleepSeconds = 1;
  
    if ( argc > 1 ) {
      sleepSeconds = atoi(argv[1]);
    }
    
    printf("Going to sleep for %d seconds\n", sleepSeconds);
    sleep(sleepSeconds);
    printf("Waking up\n");
    
    return 0;
}

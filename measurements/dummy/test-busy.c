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
    int busySeconds = 1;
    int dummyNum = 2;
    int i = 0, duration;
    struct timeb beg, end;     
  
    if ( argc > 1 ) {
      busySeconds = atoi(argv[1]);
    }
    
    printf("Going to be busy for %d seconds\n", busySeconds);
    ftime(&beg);
    while (1) {
      for ( i = 0; i < 1000; i++ )
	dummyNum += i*i;
      
      ftime(&end);
      duration = ((end.millitm + end.time*1000) - (beg.millitm + beg.time*1000));
      if ( duration > busySeconds*1000 ) break;
    }
    printf("Waking up\n");
    
    return 0;
}

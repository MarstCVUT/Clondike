#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/timeb.h>

#define DEFAULT_HEIGHT 5000
#define DEFAULT_WIDTH 5000

int main(int argc, char *argv[])
{
    //each iteration, it calculates: newz = oldz*oldz + p, where p is the current pixel, and oldz stars at the origin
    double pr, pi;                   //real and imaginary part of the pixel p
    double newRe, newIm, oldRe, oldIm;   //real and imaginary parts of new and old z
    double zoom = 1, moveX = -0.5, moveY = 0; //you can change these to zoom and change position
  //  ColorRGB color; //the RGB color value for the pixel
    int maxIterations = 5000;//after how much iterations the function should stop
    int x, y, i;
    int width = DEFAULT_WIDTH;
    int height = DEFAULT_HEIGHT;
    int widthPerProc = width;
    int widthStart = 0;
    
    int procs = 1;
    pid_t* children = NULL;
    int isChild = 0;
    int* lineDataBuffer = NULL;
    
    struct timeb beg, end;     

    if ( argc > 3 ) {
      width = atoi(argv[2]);
      height = atoi(argv[3]);
    }

    if ( argc > 1 ) {
      procs = atoi(argv[1]);
      if ( procs < 1 )
	 procs = 1;
      
      widthPerProc = width/procs;
    }
    
    printf("Procs: %d, Width: %d, Height: %d\n", procs, width, height);
    
    if ( procs > 1 ) {
      children = malloc(sizeof(pid_t)*(procs-1));
      
      int procId = 0;
      for ( ; procId < (procs - 1); procId++ ) {
	pid_t pid = fork();

	if ( pid == 0 ) {
	  isChild = 1;
	  widthStart = procId * widthPerProc;
	  lineDataBuffer = malloc(2*sizeof(int)*height*widthPerProc);
	  break;	  
	} else {
	  children[procId] = pid;
	}	  	
      }
      
      if ( !isChild ) {
	widthStart = procId * widthPerProc;	
      }
    }

    sleep(3);

    printf("Starting calculation of pid %d\n", getpid());
    ftime(&beg);
    // Just taken from some public source, not sure about correctness, but it is not critical anyway..
    for(x = widthStart; x < (widthStart+widthPerProc) && x < width; x++) {
	for(y = 0; y < height; y++)
	{
	    pr = 1.5 * (x - width / 2) / (0.5 * zoom * width) + moveX;
	    pi = (y - height / 2) / (0.5 * zoom * height) + moveY;
	    newRe = newIm = oldRe = oldIm = 0; //these should start at 0,0
	    for(i = 0; i < maxIterations; i++)
	    {
		oldRe = newRe;
		oldIm = newIm;
		newRe = oldRe * oldRe - oldIm * oldIm + pr;
		newIm = 2 * oldRe * oldIm + pi;
		if((newRe * newRe + newIm * newIm) > 4) break;
	    }
	    if ( lineDataBuffer ) {
	      lineDataBuffer[2*((x-widthStart)*height + y)]  = newRe;
	      lineDataBuffer[2*((x-widthStart)*height + y)+1]  = newIm;
	    }
	}	
    }
    ftime(&end);        
    printf("Pid %d calculated columns %d to %d in %ld ms\n", getpid(), widthStart, widthStart + widthPerProc, (end.millitm + end.time*1000) - (beg.millitm + beg.time*1000));
    
    if ( !isChild ) {
      int procId = 0;
      int status;
      for ( ; procId < (procs - 1); procId++ ) {	
	
	char fname[100];
#define READ_BUFFER_SIZE 10000
	int readBuffer[10000];
	sprintf(fname, "/tmp/test-%d", children[procId]);	
	int singleRead = 0, totalRead = 0;
	
	waitpid(children[procId], &status, 0); // WUNTRACED | WCONTINUED);
	
	int fd = open(fname, O_RDONLY);
	
	while ( 1 ) {
	  singleRead = read(fd, readBuffer, READ_BUFFER_SIZE);
	  totalRead += singleRead;
	  if ( singleRead != READ_BUFFER_SIZE ) break;
	}
	close(fd);
		
	printf("Pid %d finished. Total read: %d Status: %d\n", children[procId], totalRead, status);
      }
    } else {
      char fname[100];
      sprintf(fname, "/tmp/test-%d", getpid());
      int fd = open(fname, O_CREAT | O_WRONLY, S_IRWXO | S_IRWXU | S_IRWXG);
      write(fd, lineDataBuffer, sizeof(int)*2*height*widthPerProc);      
      close(fd);
      printf("Pid %d written: %ld\n", getpid(), sizeof(int)*2*height*widthPerProc);
    }

    return 0;
}

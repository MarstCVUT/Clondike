#include <stdio.h>      /* for printf() and fprintf() */

#include "director-cli.h"

int main(int argc, char **argv) 
{
	initialize(4223);
	int result = check_permission("MIGJAoGBAPD+T3DFSp1VrcM4uvz8/WPbWtyKGFdH8kqkYohTXw4HDQZo7YHIeF9K9Xo2ZAEc5XcjqegzCl/jaaVLoybwRJ4/NKKdhVQW5/6ZOuSQDk2fVu3Waqi/PtOw/uav49a/A06h+7K1mJTc7gadCEfqpH+vc1cqPoogojZIkjeRQ6YNAgMBAAE=", "fs", "read", "/root");
	int result2 = check_permission("MIGJAoGBAPD+T3DFSp1VrcM4uvz8/WPbWtyKGFdH8kqkYohTXw4HDQZo7YHIeF9K9Xo2ZAEc5XcjqegzCl/jaaVLoybwRJ4/NKKdhVQW5/6ZOuSQDk2fVu3Waqi/PtOw/uav49a/A06h+7K1mJTc7gadCEfqpH+vc1cqPoogojZIkjeRQ6YNAgMBAAE=", "fs", "read", "/tmp");

	printf("Results: %d %d\n", result, result2);
	finalize();
}

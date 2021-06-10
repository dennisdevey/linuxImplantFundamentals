#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define WORKING_HOURS_START 8
#define WORKING_HOURS_STOP 16
#define START_DATE "10/06/2021"
#define END_DATE "10/06/2021"

// Print the current date and time in C
int main(void)
{
    // variables to store the date and time components
    int hours, minutes, seconds, day, month, year;
 
    // `time_t` is an arithmetic time type
    time_t now;
 
    // Obtain current time
    // `time()` returns the current time of the system as a `time_t` value
    time(&now);
 
    // Convert to local time format and print to stdout
//    printf("Today is %s", ctime(&now));
 
    // localtime converts a `time_t` value to calendar time and
    // returns a pointer to a `tm` structure with its members
    // filled with the corresponding values
    struct tm *local = localtime(&now);
 
    hours = local->tm_hour;         // get hours since midnight (0-23)
    minutes = local->tm_min;        // get minutes passed after the hour (0-59)
    seconds = local->tm_sec;        // get seconds passed after a minute (0-59)
 
    day = local->tm_mday;            // get day of month (1 to 31)
    month = local->tm_mon + 1;      // get month of year (0 to 11)
    year = local->tm_year + 1900;   // get year since 1900
 

#ifdef DEBUG 
    // print local time
     printf("Time is %02d:%02d:%02d\n", hours, minutes, seconds);
     printf("Date is: %02d/%02d/%d\n", day, month, year);
#endif
#ifdef WORKING_HOURS_START
    if ((hours > WORKING_HOURS_START ) & (hours < WORKING_HOURS_STOP)){

#ifdef DEBUG 
	    printf("Working Hours\n");
#endif
	    return -1;
    }
#endif
#ifdef START_DATE
     
    //printf("before start date\n");
    //return -1;
#endif

#ifdef END_DATE
    //printf("after end date\n");
    //return -1
#endif
    return 0;
    
}

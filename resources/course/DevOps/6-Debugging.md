# Debugging
Modify your program to use #ifdefs for printing error messages, if you haven't already. 

```
#ifdef DEBUG
    printf("An error occurred\n");
#endif 
```
Or even better, create a my_printf so there aren't as many #ifdefs throughout the program. The compiler will strip out your my_printfs if they don't do anything, which is neat! An example was included in the demo.c code. 


Once you've done that, compile two binaries, one with and one without debugging enabled.

Run strings, ldd, readelf, and strace on the two binaries. What are the differences you see?

Submit the commit.

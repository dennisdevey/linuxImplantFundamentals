# Debugging
Modify your program to use #ifdefs for printing error messages, if you haven't already. 

```
#ifdef DEBUG
    printf("An error occurred\n");
#endif 
```

Update the Makefile to include a debug build option.

Once you've done that, compile two binaries, one with and one without debugging enabled.

Run strings, ldd, readelf, and strace on the two binaries. What are the differences you see?

Submit the commit.

# Error Handling
In this organization, we handle our errors like adults, by printing them to screen. Unless of course, it is live on a network, in which case, we don't do that. We are going to be using #ifdef macros to add debugging functionality to our implants so that we can develop them in a sane manner. 

```
#ifdef DEBUG
    printf(We are debugging right now\n");
#endif 
```

Read this to learn about error handling in C. 

<https://www.tutorialspoint.com/cprogramming/c_error_handling.htm>

All errors will be handled, but all error print outs will be encased in DEBUG ifdefs... does that make sense? 

Add DEBUG into your code and submit the commit.
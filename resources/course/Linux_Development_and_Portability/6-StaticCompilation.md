# Static Compilation
So if we want to run with any libraries, static is a solid way to go... sort of. Due to size constraints, sometimes static binaries just won't cut it. There's more to discuss. Anyway, get sniffex.c to statically compile for 64 bit Linux and run locally.

1. What did you have to do to get that to work? Submit the compilation command.

Run strings, ldd, readelf, strace on the file

2. What do you find? What was the difference between static and dynamic? 

Run this version of sniffex on another Linux box ***with the same architecture*** if you have one lying around. If it didn't work, troubleshoot. A lot. Also, post something in Slack so we an troubleshoot together.

3. What happened? Why?

It should have worked. But, honestly, there are plenty of reasons that static compilation could have failed here. It's a ridiculously complicated problem that only gets more confusing as complexity increases.

4. How does static compilation effect the portability of code? How about the size/detectability?
5. What are the pros and cons of static, especially as an APT?

For this assignment submit the text for your responses.

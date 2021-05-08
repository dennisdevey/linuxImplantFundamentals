# Architecture Portability
Time for a bit of work, but it's important because in this APT we care about code portability, goddamnit.

1. What is code portability and why is it so important on Linux? Why is it even more important for us an APT?

**This section assumes your architecture is 64 bit Intel and you are running a modern Ubuntu. If you are not, you can still do everything, it just might be a bit more work.**

Get sniffex.c to compile for whatever your host is. Then compile for a 32 bit Intel (x86) architecture. Cross compilation is generally very complicated and terrible, but is fairly straightforward on Intel 64 bit hardware (x86_64) to Intel 32 bit (x86) because they are backwards-compatible... so you got lucky this time.

2. What is your host architecture? What did you have to do to get that to work? Submit the cross-compilation command.

3. You can run a binary built for a 32 bit Intel architecture on a 64 bit system, but can't run a binary built for a 64 bit Intel architecture on a 32 bit system.... so which should you default to building? (Of course it is more complicated than that)

4. Can you run a binary built for 64 bit Intel on 64 bit ARM architecture? Why or why not? <https://unix.stackexchange.com/questions/298281/are-binaries-portable-across-different-cpu-architectures>

Just to drive you insane, spend ~30 minutes trying to compile for ARM. If it successfully compiles in 30 minutes, you win, let us know in Slack. 

5. What would you have to do to get this C you have right now to compile for ARM, in general words. 

As an APT, we should be able to spring for an ARM box to compile on so we don't have to worry as much about cross compilation headaches. Now sure, there are different versions of ARM, and plenty of other architectures out there, so... yeah, it gets complicated fast. Someday we will have a full devops pipeline that tests changes on various architectures as we go... but it is not this day.

Going forward, compile all binaries for x86_64 unless told otherwise. 

For your assignment, write a few sentences on the complexity of compiling for different architectures. 
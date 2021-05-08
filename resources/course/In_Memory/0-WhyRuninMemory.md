# Why Run in Memory
Other than desperately wanting the approval of the other APTs for never touching disc, there are many other benefits to this. 

First read this to get an idea of what and why: <https://en.wikipedia.org/wiki/Fileless_malware>

Basically, if you don't hit disc, you make defender's lives more difficult, and forensics folks time very difficult. I should say more but for now:

* More time in memory is good
* If you want persistence, memory only is very difficult and requires reaccessing via other methods
* If you get swapped to disc, forensic material can exist

It is possible to minimize your chance of getting swapped by modifying "Swappiness". 

You can increase your chance of not getting swapped if you have the right privileges but it is fairly loud.

* calls in C to mlock(), mlockall(), mmap() 
* straight up modifying sysctl vm.swappiness or /proc/sys/vm/swappiness 

We're not going to worry about that stuff.

Submit one or two sentences on why in memory malware is great.
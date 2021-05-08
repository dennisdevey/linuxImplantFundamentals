# memfdcreate
The previous method was cool, but we can also get more modern. 

Enter memfd_create: 

First joining the kernel around 2014 in Linux 3.17, this allows us to build an anonymous file that lives purely in memory and does all the things we'd expect from a regular file descriptor. It's pretty great. 

This isn't a very technical explanation, but it works... so play around with it. <https://0x00sec.org/t/super-stealthy-droppers/3715> 

You might notice there is also a pretty decent shm_open implementation going on here if you want to review.

Build yourself a demo loader that reads into a memfd and execs an ELF. Submit the commit.
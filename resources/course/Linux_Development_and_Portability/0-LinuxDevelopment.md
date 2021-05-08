# Linux Development
So we've been developing on Linux for a bit now, you are likely decent at C or you wouldn't be taking this class, and you have an okay understanding of how Linux works. This is a good thing, but it is certainly not enough. Most of your time developing, certainly nearly all of mine, we are content to hang out at the top level of abstraction, in a lofty marble palace of C where things works and make sense, and sure, pointers suck sometimes, but you're not down in the weeds knocking your head on the decisions Linus Torvalds and his pals make 20 years ago. Don't worry though, you're about to start. 

## Linux 
While we are on Linus, let's talk about his creation. What is Linux? It's magical, it's beautiful, it's open source, whatever... what the hell is it? Linux is a Unix-like kernel. Everything on top of that is just abstraction. So when we say Linux, we mean the kernel. Just the kernel. Exclusively, the kernel. Plenty of things sit on the kernel, but they are not the kernel. 

What makes the Linux kernel special? Well, a lot of things, and we'll go into them over time, but to me, the thing that makes it special is that there are rules, and standards, and things do what they are supposed to do, and they stay that way, forever. It is a breath of fresh air from Windows and their foolishness. You will skim this with the intention of appreciating the Linux Kernel rather than understanding it, kind of like looking out at the ocean or the view from a nice mountaintop. Understanding is too much to ask for and would be a huge waste of time, appreciation is all we want. <https://en.wikipedia.org/wiki/Linux_kernel>

## GNU 
This is a falsely attributed quote, but it still is funny and people reference it all the time, and I'm going to include it anyway, because it does a good job of explaining things (which is how you can tell it isn't real)

> I'd just like to interject for a moment. What you're referring to as Linux, is in fact, GNU/Linux, or as I've recently taken to calling it, GNU plus Linux. Linux is not an operating system unto itself, but rather another free component of a fully functioning GNU system made useful by the GNU corelibs, shell utilities and vital system components comprising a full OS as defined by POSIX. Many computer users run a modified version of the GNU system every day, without realizing it. Through a peculiar turn of events, the version of GNU which is widely used today is often called ?Linux,? and many of its users are not aware that it is basically the GNU system, developed by the GNU Project. There really is a Linux, and these people are using it, but it is just a part of the system they use.

> Linux is the kernel: the program in the system that allocates the machine's resources to the other programs that you run. The kernel is an essential part of an operating system, but useless by itself; it can only function in the context of a complete operating system. Linux is normally used in combination with the GNU operating system: the whole system is basically GNU with Linux added, or GNU/Linux. All the so-called ?Linux? distributions are really distributions of GNU/Linux.

So yeah, it is important to understand, Linux is the Kernel, GNU is the base operating system. All the other stuff is built on that. 

## Linux Kernel Interfaces

Alright, this is where the magic happens and where we are about to inhabit. Read it for understanding. 
<https://en.wikipedia.org/wiki/Linux_kernel_interfaces>

 
Understand these specifically:

1. What is POSIX
2. What is a system call
3. What is glibc
4. How do these all work together
5. What is an ABI

This is the best book on the subject if you want to get way too in depth, or have a reference as you do things. I recommend buying it eventually. <https://en.wikipedia.org/wiki/The_Linux_Programming_Interface>

For this assignment, submit a sentence or two on what the Linux API and ABI are.
# tmpfs devshm and More
As we learned in the last section getting shellcode of any reasonable size and complexity working is pretty hellish, so let's learn a technique that will allow us to run an entire ELF from memory.  We are going back in history right now.

# tmpfs 

Everything in Linux is a file... but what about things that aren't actually files? tmpfs is a pretty neat part of the kernel which puts things into internal caches that only exist in memory.

<https://man7.org/linux/man-pages/man5/tmpfs.5.html>

# /dev/shm:

As we've moved forward in history, tmpfs has officially been moved to /dev/shm which we can access with shm_open. By default this area is executable, but it's pretty trivial to set NOEXEC and break this method. Still, good to know how to do it.

<https://gist.github.com/drmalex07/5b72ecb243ea1f5b4fec37a6073d9d23>

There's also some tomfoolery we can do with programs like gdb, python (or any other scripting language), or even dd to get them to run arbitrary shellcode for us, so keep those in mind, but we won't worry about them in this course.

Using the examples above, build a mini loader that can read an ELF from disc into a shm_opened tempfs file and then execute it.

Submit your commit.



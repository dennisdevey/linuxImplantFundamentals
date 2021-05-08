# Valgrind and Baby DevOps
Memory leaks are for chumps. Not to mention, they are super easy to avoid. 

Whenever you make major changes to your code you should  do a run of valgrind to ensure you aren't leaking memory. 

Modify your Makefile so that it runs valgrind with full checks when you call "make valgrind".

Look at you, basically a DevOps god these days. In a perfect world we'd compile the code for 5 different architectures, 4 different OS's, and 6 different versions of glibc, but hey, this is a start. 

Submit a link to your commit.
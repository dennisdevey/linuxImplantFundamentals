# Classic ELF Infector
Shoutout to Silvio Cesare and the old school crew.

<https://www.win.tue.nl/~aeb/linux/hh/virus/unix-viruses.txt>
```
THE NON ELF INFECTOR FILE VIRUS (FILE INFECTION)

An interesting, yet simple idea for a virus takes note, that when you append
one executable to another, the original executable executes, but the latter
executable is still intact and retrievable and even executable if copied to
a new file and executed.

# cat host >> parasite
# mv parasite host
# ./host
PARASITE Executed

Now.. if the parasite keeps track of its own length, it can copy the original
host to a new file, then execute it like normal, making a working parasite and
virus.  The algorithm is as follows:

	* execute parasite work code
	* lseek to the end of the parasite
	* read the remaining portion of the file
	* write to a new file
	* execute the new file

```

Implement this setup and comment the shit out of the C. If you need more help, check this out from Alexander Bartolich, another legend: <http://www.ouah.org/virus-writing-HOWTO/doing.it.in.c.html>

If you want more information, this is very well documented all over the internet, I'm just trying to stick with the classics. 

Submit your commit.
So we have a pretty solid implant written up, but how do we deal with the sticky situation of what to do with all the loot we are stealing? In a perfect world with no network monitoring we can siphon it out as we go, but most of the time we have to assume some level of monitoring so we don't want to run off with the goods yet. There are a variety of ways for us to handle this, namely zipping things and hiding them in random places throughout the filesystem, but what if we wanted a quiet and secure method of persisting data, modules, and basically anything else over time. 

A hallmark of a high quality operation is running a virtual file system on a hostile host. This allows us to do as many read/writes as we want, without going to disc every time or holding everything in memory. There are dozens of ways for us to build our own VFS, but our priorities are encryption and size in memory. 

If you want more background on what a VFS is, google it.  

Your first task is to modify one of these <https://github.com/search?l=C&q=linux+virtual+file+system&type=Repositories> so that your implant can use a VFS. I'll let you in on a dirty little secret here if you haven't realized it yet: You're not the first person to do any of these things, whether it's a VFS or kernel modules or whatever, and with a well tuned StackOverflow/sketchy forum search, you can find someone who has done 80% of what you need. Slap two or three projects together and you probably have 99% of the functionality you need once you get it to compile. Of course we can't do that in production, we're a serious organization and ... oh wait, Turla just ripped off the sniffex.c demo program from the TCPDump website. So yeah, everyone does it if they have the internet access to do so.

There are two approaches for encryption:

1. Encrypt the entire VFS and decrypt it into memory when you need to use it
2. Encrypt the Inode table which contains the indexes and encryption keys for the other files stored in the VFS

#2 is definitely smaller in memory but more complicated to implement. Do what feels right, I'd say start with #1 so you don't get too wrapped up in this and can move on to more interesting things. Ensure you can configure paths at compile time.

The following abilities should exist:
* Create a new VFS
* Load an existing VFS
* Save your VFS to Disc
	* Ensure to compress prior to encryption
	* Additionally, remember that compression functions can be used for fingerprinting your TTPs. Don't worry about adding extra compression functions in this course, but it's something to think about.
* Write a file to VFS 
* Open a file to VFS 
* Append a file in VFS
* Read a file from VFS


Submit your commit. 

# Host to VFS and Back

Now that you have a VFS, let's stress test it by making the jump from the Linux VFS to yours. Implement functions to send a file from the host FS to your VFS, and from your VFS to the host FS. Ensure you can specify where in the host FS you will be writing to. 

Submit your commit.


# Ring Buffer

Now that you have built up your VFS, we are going to implement a data structure in their. In addition to raw files like last section, we are going to add in the concept of ring buffers. 

Ring buffers are a concept in computing where you begin writing at the beginning of a buffer and once you get to the end of it, you overwrite the initial information. This is useful because we don't want our VFS to become too large, especially if we are doing something like capturing packet captures into it or recording keystrokes. 
For this section you will implement a ring buffer file structure that you can use for arbitrary functions. Add functions to read and write to the ring buffer.

Submit your commit. 


# Using Your VFS

Add a function(s) that your implant can use to store the output of various commands (STDOUT and STDERR) in a ring buffer. Make sure this is generalized so that the next time you have to use ring buffer functionality you'reable to easily adapt the code. Additionally, add to your implant's exit handling functionality to try and write the VFS back to disc so you don't lose work. This won't always work, but it should make a best effort.
 

Submit your commit.


# Hiding 

For bonus points, we could hide our VFS in unused space on disc so it is not visible on the file system. We can do that in a variety of ways, historically by marking parts of the disc bad, or we can partition the hard drive, though that is pretty loud. Read this for more info: <http://www.berghel.net/publications/data_hiding/data_hiding.php>. Additionally, we could hide in plain site and just have our VFS hidden inside of a common file format that could be overlooked by an investigator. For now, modify your existing VFS functions to hide inside of a .jpg file format. It doesn't need to open or work properly, just throw a jpg header on it and ensure that the '''file''' command states that it is a jpg. Should be good enough for now.

Submit your commit.


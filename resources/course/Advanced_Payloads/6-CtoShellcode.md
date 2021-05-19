# C to Shellcode
So, as I said earlier, converting large amounts of C code to shellcode is pretty damn hard, mostly because of linking. It's basically always linking. 

On the plus side, this is a problem enough people have dealt with that there are many writeups and tools for the subject. On the downside, most of them are for Windows, because, well,  malicious market share.

To get a good grasp of what goes on, read this: <https://nickharbour.wordpress.com/2010/07/01/writing-shellcode-with-a-c-compiler/>

I strongly don't recommend trying to figure this out yourself, use a tool someone has written for you. 
The tool I recommend is <http://www.secdev.org/projects/shellforge/>. It is old and dependencies are a pain, but this gets the job done. 

For your assignment, write a few sentences on the complexity of writing shellcode with a C compiler.

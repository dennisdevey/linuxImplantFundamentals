# ASM to Shellcode
Enough dummy functions, let's get some shells using shellcode! 

Using these resources, implement bind shells and reverse shells using shellcode that works for 32 bit and 64 bit implants. 

* <https://rastating.github.io/creating-a-bind-shell-tcp-shellcode/>
* <https://rastating.github.io/creating-a-reverse-tcp-shellcode/>

It is fairly complicated to convert C code to shellcode, but it is possible. For now, just do ASM to shellcode. 

Submit your code and comments on what you need to do to get this to work.


## Adding MSFvenom

Alright, that was a bit of work, but hopefully you learned something. Now let's use a pre-built shellcode generator to do this for us, like MSFvenom. 

Modify your Python generation script to use MSFvenom to generate shellcode, save it to the header files, and then compile the C with the shellcode. 

Test this using 64-bit reverse shell shellcode and your previously created activate/catch script.

With MSFvenom and shellcode execution working, we are a pretty fully-featured shellcode runner right now. Unfortunately, shellcode execution still looks pretty odd when random processes that have been dormant forever try to call out, and if application whitelisting is implemented, you're gonna get stopped. For that, you're going to need to figure out shellcode injection. That is really hard though, so we are going to save it for the end of this course.

Submit your commit.
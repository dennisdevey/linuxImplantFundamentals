# Compilers
## Compilers
You probably know what a compiler is/does, but skim this, you'll learn something: <https://en.wikipedia.org/wiki/Compiler>

## GCC

Skim this <https://en.wikipedia.org/wiki/GNU_Compiler_Collection> We will use GCC for most of our compilation, because that's what normal humans on Linux do. There are [plenty of others out there though...](https://www.ubuntupit.com/best-linux-compilers-for-modern-developers/)

## Cross Compilers

Skim this: <https://en.wikipedia.org/wiki/Cross_compiler>
We won't be cross compiling for a while, but it's good to know it's something we will have to do at some point. 

## The Actual C Compilation Process

I know you know this, but read this again, for understanding. Don't go ahead unless this makes sense. <https://www.geeksforgeeks.org/compiling-a-c-program-behind-the-scenes/>.

Then do this to make sure you actually get it. Work through it at your own pace, using Google, textbooks, and the #course_hardstuff channel in Slack. Don't get discouraged!

<http://cs-fundamentals.com/c-programming/how-to-compile-c-program-using-gcc.php>

1. Use `cpp` to convert your source code .c to a .i file.

2. Then use `gcc` to convert your .i file into a  .s  file. 

3. Then use `as` to convert your .s file into a .o binary.

What still needs to be done to get a .o binary to execute? 


Task: 

* Submit text discussing anything that you struggled with and didn't understand, and what you did to overcome that problem. Include all the commands you ran. If you still don't understand, tell us what specifically you did not understand.


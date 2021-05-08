# Linking and Loading
This is... complicated. 

First, read this:  <https://www.geeksforgeeks.org/static-vs-dynamic-libraries/>.

Then, you are going to read this. All of it. I promise it is well-written.

<https://tldp.org/HOWTO/Program-Library-HOWTO/introduction.html>

As you go through, keep asking yourself:


1. How does your compiler know which libraries to link to the binary? 
2. How do I tell what is linked? 
3. How does the correct library get loaded?
4. If you transplant your binary to another location, what happens if the linked library is not available?
5. What about if the linked library is a different version number or name?

Once you can answer those questions, move on to the assignment: 

## Technical Assignment 

This stuff is somehow even more confusing. Still using: <http://cs-fundamentals.com/c-programming/how-to-compile-c-program-using-gcc.php>

In a perfect world we would then use `ld` to build an executable file with our .o file, but unfortunately, the example in the link above will not work. 

Why will it not work? Likely because the names of the required libraries will be different or in different places, depending on your OS. So how would we figure this out? 

Work through this compilation tutorial and dynamically link your helloworld ELF using ld. 

Task: 

1. What is static compilation? 
2. What is dynamic compilation? 
3. Submit text discussing anything that you struggled with on the technical assignment  and what you did to overcome that problem. Include all the commands you ran. If you still don't understand, tell us what specifically you did not understand.



# Shellcode Execution
Those reverse shells are cool. But our functions that call back are very obviously bind/reverse shells to any anti-virus scanner.  

We are going to learn a little about shellcode and how we can evade using shellcode. It's all over the internet, and you can find basically anything you want on Google, <https://www.exploit-db.com/shellcodes>, or <http://shell-storm.org/shellcode/>

First, here is a resource on shellcode to read over:  <https://tuttlem.github.io/2017/10/28/executing-shellcode-in-c.html>. Work through it and fill in your knowledge with Google. 

Your first assignment is to write shellcode that executes in the context of our implant. (Executing and injecting shellcode are different, we'll do more on that later. ) For now, create the shellcode that prints the text "BANG!" to the terminal when activated, and build it into your implant. 

What does -fno-stack-protector  and -z execstack do?

Document what you did to get the shellcode working and submit your commented code. 
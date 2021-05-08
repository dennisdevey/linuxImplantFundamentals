# Userland Exec
So all of those cool loads are fun, but at the end of the day, we are still execing the normal way, and it's very loud and very obvious. The right security rules or SELINUX profiles can break them completely... so now we need to get into a very complicated field of research known as Userland Exec. 

The canonical start of this was written by the Grugq (again). Read it: <https://grugq.github.io/docs/ul_exec.txt>

This is one of the best modern writeups on the subject: <https://www.rapid7.com/blog/post/2019/01/03/santas-elfs-running-linux-executables-without-execve/>

So, Metasploit project fixed this problem by builiding a modified version of Meterpreter specifically so this works. We need a better solution so we will work off a more generalized solution from Bruce Ediger. 

Read this and inhale it: [http://www.stratigery.com/userlandexec.html](https://web.archive.org/web/20190812160432/http://www.stratigery.com/userlandexec.html)

Like all good things, you're going to be dependent on compiler versions, but it is the best we got right now. Ediger has done a ton of work on this and has the most recently working public version as far as I know.

Now that you understand it generally, do everything you can to get this project working in a test file. <https://github.com/bediger4000/userlandexec> Even if it doesn't work, you'll learn a shit ton.

 If that works and you feel confident, move on forward and add it to your project, or... just move on with your life. This is hella complicated. 
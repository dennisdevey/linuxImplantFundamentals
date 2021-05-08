# LDPreload Shared Objects
LD_PRELOAD is nifty, and while it isn't exactly injection, it's cool to know. Basically what happens is you load a shared library, and anything labeled as a constructor gets run in the context of the calling process. This means that your libraries functions will have the ability to read in /proc/self, as well as look like the parent process to outside entities, plus, the parent process will have no idea this is going on.

Yeah, LD_PRELOAD doesn't always work, but when it does it is very powerful. Along the same lines, most Linux userland rootkits rely on LD_PRELOAD to hide, but more on that later.

Do these tutorials for now.

1. <https://catonmat.net/simple-ld-preload-tutorial>
2. <https://catonmat.net/simple-ld-preload-tutorial-part-two>

Then write code that will use LD_PRELOAD to run the functions labeled __attribute__((constructor)); in a shared object when you load it. This is a good resource: <https://secureideas.com/blog/2021/02/ld_preload-how-to-run-code-at-load-time.html>

How you implement the source file for the .so is up to you, but I recommend something in the build process that allows you to specify, but otherwise default to a reverse shell.
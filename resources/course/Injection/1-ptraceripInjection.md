# ptrace rip Injection
With MSFvenom and shellcode execution working, we are a pretty fully featured shellcode runner right now. Unfortunately, shellcode execution still looks pretty odd when random processes that have been dormant forever try to call out, and if application whitelisting is implemented, you're gonna get stopped. Luckily, shellcode injection exists! 

***Author's Note: One year for the CDX cyber competition, I enabled application whitelisting so that beacons couldn't call home and was feeling quite proud of myself. I was sitting next to Raphael Mudge (aka the Cobalt Strike developer) on the Red Team that year and watched as he couldn't get callbacks for the first day. Eventually, he turned to me and said something along the lines of "you have whitelisting this year, nice". I then watched in horror as he rewrote Cobalt Strike on the fly to have the ability to do a blind process inject, featuring handwritten shellcode, prior to calling out. To date, it was the coolest thing I've ever seen.*** 

Read this: <https://www.fireeye.com/blog/threat-research/2019/10/staying-hidden-on-the-endpoint-evading-detection-with-shellcode.html> 

Long story short, there are processes to inject into that will make your implant's behavior look fairly normal.

We want to be like Raffi, so now we have to figure out shellcode injection. There are a ton of resources of varying quality, but most of them use ptrace. You should do that instead of dllinject unless you really feel like it.

First, follow this tutorial and get it working: <https://0x00sec.org/t/linux-infecting-running-processes/1097>. If you want another decent resource, check out <https://jm33.me/process-injection-on-linux.html>. I really need to write my own for this. 

Comment one of those examples and save it into your commentedCode directory.

Once you understand what is going on in that demo, add the ability to do a ptrace rip inject to a specified PID to your implant. Once you have that, figure out how to do it blind on an unseen system by running a process by name, getting the PID, and injecting into it. This is a ton of work, you will need to spend some quality time on this and should ask for help troubleshooting.

Whenever you are done, submit your commit that can do an inject into a specified PID. 
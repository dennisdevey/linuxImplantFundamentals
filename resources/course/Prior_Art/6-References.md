# References
Nothing against people running botnets out of Eastern Bloc apartment buildings, but there is no real need for operational excellence when you are going after gen pop. 

This isn't a hard rule, but generally, you only use the minimum tool required for the job. Don't use a string of 0 days when phishing works, don't use a custom implant when Cobalt Strike will do the job.  If Cobalt Strike does the job for everything you do, just use that. Goddamn is it good. 

But we are in a situation where we want that long term persistence with low observability. There are plenty of ways to do that, so we should copy the experts. No need to come up with things yourself. Thanks to the previously mentioned threat intel weenies, as well as the talented, hard-working and good-looking reverse engineers that work at the same companies as them, there is a ton of documentation on all the best implant families and the APTs that write and use them. 

You can spend a career reading all of this, and plenty of people do. I recommend reading a bit to get the creative juices flowing and then spend your time writing code instead of reading about Lithuanian teenagers but hey, you do you. 

Selected Links: 

*  https://github.com/CyberMonitor/APT_CyberCriminal_Campagin_Collections
* https://vx-underground.org/apts.html

These are some other writeups of varying quality on implant dev. Please don't leave my course to go and do these, I promise mine is significantly more comprehensive, if less polished. These are mostly here so I don't lose track of them.

* Implants
  * https://shogunlab.gitbook.io/building-c2-implants-in-cpp-a-primer/chapter-2-establishing-a-listening-post
  * https://github.com/p3nt4/Nuages/wiki/Tutorial:-Creating-a-custom-full-featured-implant
  * https://www.youtube.com/watch?v=2Ra1CCG8Guo&feature=youtu.be
  * https://www.scip.ch/en/?labs.20171005
  * https://0xpat.github.io/Malware_development_part_1/
  * https://www.varonis.com/blog/malware-coding-lessons-people-part-learning-write-custom-fud-fully-undetected-malware/
  * https://github.com/paranoidninja/Botnet-blogpost
* Rootkits
  * https://web.archive.org/web/20170712112638/https://d0hnuts.com/2016/12/21/basics-of-making-a-rootkit-from-syscall-to-hook/
  * https://h0mbre.github.io/Learn-C-By-Creating-A-Rootkit/#
  * https://ketansingh.net/overview-on-linux-userland-rootkits/
  * https://blog.netspi.com/function-hooking-part-i-hooking-shared-library-function-calls-in-linux/
  * http://www.infosecisland.com/blogview/22440-Analyzing-Jynx-and-LDPRELOAD-Based-Rootkits.html
  * https://volatility-labs.blogspot.com/2012/09/movp-24-analyzing-jynx-rootkit-and.html
  * https://theswissbay.ch/pdf/Whitepaper/Writing%20a%20simple%20rootkit%20for%20Linux%20-%20Ormi.pdf
  * https://infocondb.org/con/thotcon/thotcon-0xa/writing-your-own-linux-rootkit-for-fun-and-profit
  * Demystifying Modern Windows Rootkits- Bill Demapi
  * Developing a Linux Rootkit: Kernel Internals & Subversive Techniques
  * https://turbochaos.blogspot.com/2013/09/linux-rootkits-101-1-of-3.html
  * https://jm33.me/write-better-linux-rootkits.html
  * https://0x00sec.org/t/kernel-rootkits-getting-your-hands-dirty/1485
  * https://xcellerator.github.io/posts/linux_rootkits_01/
  * https://beneathclevel.blogspot.com/2013/06/a-linux-rootkit-tutorial-makefile-and.html


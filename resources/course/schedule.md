## Week 1:

* Day 1: Finish 'Prior Art', Understand cd00r and get it to compile. Continue to DevOps. 
* Day 2: Work as far through 'DevOps' as possible. 
* Day 3: Finish DevOps and do "Linux Development" and "Compilers" from the 'Development and Portability' section. 
   * I don't want you to do 'Development and Portability' all in a row, it is a bit brutal and technically dense. 
   * Once you have finished "Compilers", move on to the 'Backdoor Basics' section
 * Day 4: Complete as much of 'Backdoor Basics' as you can
 

## Week 2:

* Day 1: More 'Backdoor Basics'
* Day 2: REFACTOR! <https://cwe-pte.slack.com/archives/C0248P2N5EW/p1623089821003400>
   * We are going to build our implant in a modular and sustainable way. Going forward, instead of building from sniffex, we are going to use sniffex as a library to call functions from. This means we are going to start using multiple .h and .c files for each compilation. While this will complicate our build scripts, it will make future development a lot easier. I will bring a library of sniffex calls for you to class tomorrow, for now, refactor your validation code to be a function called from a new main.
   * By the end of the refactor you will have: 
      * main.c, functionality.h, sniffex.c, helper.h and anything else you want
      * Don't forget the goto's! 
 * Day 3: Finish 'Backdoor Basics'
 * Day 4: Finish 'Download and Execute'. Get a chain working where a first stage implant validates with the profiler, then downloads and execs a second stage.
  * Also get the binary to compile and run for 32bit Linux
 * Day 5: First RE Day & Antivirus Theory
  * We will spend the day reverse engineering and doing forensics on your implant so far. 

## Week 3: 

* Day 1: 'Fork and Exec', 'Daemonize', 'Reverse Shell', and 'Shellcode Execution'
* Day 2: Finish Day 1 Material and 'Command and Control' Up to 'Command Line Interface'. 
* Day 3: Automate the following:
  * First stage executable profiles box and runs validator
  * If validator succeeds, pass back profile via Get request
  * Compile a second stage with implant and host it on server
  * First stage downloads and executes second stage implant which begins checking in via C2
* Day 4: Privesc and Persistence Day
* Day 5: Second RE Day & Anti-Forensics / Competition Day
  * [Make a copy of the scorecard](https://docs.google.com/spreadsheets/d/1397x1XDjxkmSxBJaJ--S01PcngUwWwze_1ipeCBU3wY/edit#gid=0)

## Week 4: 

* Day 1: 'Anti-RE' Theory:
  * Anti-Virtualization
  * Anti-Debugging
  * Packers
* Day 2: LD-Preload Rootkit Part 1.
* Day 3: LD-Preload Rootkit Part 2.
* Day 4: Competition Day 1 
  * [Make a copy of the scorecard](https://docs.google.com/spreadsheets/d/1397x1XDjxkmSxBJaJ--S01PcngUwWwze_1ipeCBU3wY/edit#gid=0)
* Day 5: Competition Day 2

Things I'd Really Like to Get Done:

* Task Chaining
*  Fileless Dropper
*  RIP Injection
*  VFS

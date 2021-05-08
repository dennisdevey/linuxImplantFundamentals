# Intro to sniffex
In the previous section we stole someone else's code, got it working, and then learned what stripped and statically linked meant.

We are not 1337 yet, but at least we have an idea of what we are trying to do. unfortunately, we haven't written really any code yet, and our backdoor is easily detectable. 

So now let's go look for some basic code to reinplement the entire cd00r framework from scratch. Our basic functionality we are looking for is a packet sniffer, written in C. Googling for C packet sniffer will allow us to pretty quickly come across this sample code.

<https://www.tcpdump.org/sniffex.c>

Wow this is convenient... also if you look at the code side by side, clearly cd00r author's just totally jacked this guy's demo code and then added on the bare minimum of functionality..... so that is what we are going to do. 

First, lets get sniffex.c to compile for 64 bit Linux. 

1. What if anything did you have to do to get that to work?

Run the sniffer and throw some packets at it using netcat. 

2. What do you see? Experiment to see how it works. 

Copy  the code that you have written into your cloned repo from the last assignment. Your assignment is to submit a link to your commit. If you have the repo as private I won't be able to view it remember, so make sure to add 'deveynull' to the repo so I can check it.
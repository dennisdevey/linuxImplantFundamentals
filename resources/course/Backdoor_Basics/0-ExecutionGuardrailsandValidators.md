# Execution Guardrails and Validators
Much like firearm safety rules, this is non-negotiable. Everytime you do anything, you have a responsibility to minimize collateral damage, and not to mention, it makes the implant harder to detect in sandboxes. Know your target and what lies beyond, and all that. Even the non-shitty Russians do this. (On the other hand, if your tradecraft requirement for this implant is "look like an amateur who doesn't know what they're doing", forget about validators. Caring about collateral damage is an obvious sign of nation states.)

Back to being the good guys, this standard goes for testing internally or deployment, you have a moral and technical obligation to never allow an implant to run on a computer that it is not supposed to. (Similarly, in the event it is supposed to run on any computer outside of a well defined list of IPs/subnets, you better have that shit in writing from your boss' s boss). 

Read this from Mitre to learn about "environmental keying": <https://attack.mitre.org/techniques/T1480/001/>

As your first piece of customization of this backdoor, add code that will take in a list of IP addresses or subnets from a configuration file and only allow the implant to run on the machines specified. This is the most basic form of keying, so make sure that when you write the function you make it easy enough to modify to add alternative checks for the future.

## Validators

Another technique is to use something named a 'validator" to check that you are on the right host before dropping your actual implant. That validator can check what network it is on like the execution guardrails we just talked about, but can also do things like check for anti-virus, if you are on a host of interest, or things of that nature. If the validator passes, you can feel comfortable that you won't burn your implant if you drop it on the same host. 

Another way to do validators is to install it and wait for a few days. If it gets detected and removed, no harm done, don't target that host with your fancy implant. If it doesn't get detected, install your advanced implant and remove the validator. This way you don't burn your fancy implant with zero days and instead a DFIR team has to deal with their 10th Cobalt Strike response in a week. 

There is a lot of meta surrounding this. 

For this assignment, commit your code and submit the link.

For a hint: there is a ton of demo code in the src directory for this.

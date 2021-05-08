# Complexity under the RE Microscope
From a theory perspective, you want to not be worth looking at, but if they still do, you want to tell a story that the RE person understands and believes, and if not that, you want to waste the hell out of their time. 

What this looks like is not doing anything obvious during automated runs or sandbox executions, perhaps conducting some benign activity. 

If you wind up getting RE'd breaking dynamic execution is a good way to get looked at hard. What's better is to just do something different when being observed. This means you have to detect observation, which is hard, but doable. Of course, if your implant is determined to be detecting RE, then someone can basically immediately make the call that it is malicious and a gig is up.

Still though, someone will try to RE your implant so you might as well give them  a ride. First, make sure you've stripped your binary so it isn't too easy for them. Don't use modern glibc functions for everything, or use a different compiler, maybe even custom functions and custom compilers. Make sure that you have extra functions all over the place that get called and seem important. Try to disrupt dynamic execution as much as possible, and overall, try to waste their time. It's a silly thing to think about, but there are honestly only so many malware reverse engineers out there, and it is theoretically possible to tie them all up if you give them enough work. 

You've already lost the battle, but you can still help them lose the war. 

Open up your implant in Ghidra and spend some time playing around with what it looks like on the inside when compiled different ways. For your assignment, submit what you can do to make it less fun for a reverse engineer and what that looks like in your binary. 

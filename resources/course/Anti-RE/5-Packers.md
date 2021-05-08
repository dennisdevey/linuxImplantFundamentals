# Packers
This is in-process, looking for good resources and activities.

Packers are short for run-time packers, aka self-extracting archives. Basically, most of the code is encoded and packed, then when the file is executed, a small stub does the decryption and execution process. We've already sort of done this before, but now we'll use UPX, the most commonly used packer and slightly modify it.

Read this: <https://bsodtutorials.wordpress.com/2014/11/14/upx-packing-and-anti-packing-techniques/>

First, pack your implant with standard UPX and try to RE it... should be miserable.

Then, unpack it and attempt to RE again.

Then modify the standard UPX packer so that UPX -d doesn't work, using the post above. 

Once you have that done, manually unpack it using this resource as if you were a reverse engineer who didn't know how you had modified it: 
[RPISEC Malware Packing and Unpacking](https://github.com/RPISEC/Malware/blob/master/Lectures/09_Packers_and_Unpacking/09_Packers_and_Unpacking.pdf)


Submit a brief writeup on how to manually unpack and put it in your resources/misc section.
# Running cd00r
- Download the cd00r.c source code and get it to compile for 64 bit linux with debug disabled.
1. What are the steps that were required to make that happen?

- Execute the binary 

2. Why did you have to run as root? This is very important.


3. What ethernet interface are you listening on? Why do you have to specify the interface you are listening on as "-lo"? What happens if you specify the interface to be a non-loopback address?

- Run "sudo lsof | grep raw"

4. What does that show you? Why?

- Set up a nc listener to catch the reverse shell
- Activate the backdoor using nc -z 
- Start listening on the right interface in Wireshark and watch what happens. 


5. What do you see?

- Run strings, ldd, readelf, strace on the file

6. What do you find?


Submit the responses to these questions as a text document. Again, don't write too much or worry about making it perfect, I don't care.

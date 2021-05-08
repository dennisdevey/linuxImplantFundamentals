# EncodingEncryption
For the environmental keying and the "BANG!" you just added, we don't want those to pop up in cleartext when someone runs strings on the binary. And we are better than [stackstrings](https://www.fireeye.com/blog/threat-research/2014/08/flare-ida-pro-script-series-automatic-recovery-of-constructed-strings-in-malware.html), though not by much.

This means you will have to add a decode/decrypt function to the implant itself...  Encryption is a bit much, so let's just encode things for now. While we are at it, you should probably go back through the code you've written so far and see what cleartext strings are in there. 

Do a little XOR bit, it gets the job done and there's no need to get too crazy. Use this as a base: <https://rioasmara.com/2020/10/24/hide-strings-with-one-key-xor/>

This might take a bit, but take your time and lower the number of signatures that can be written for your implant. Remember, whatever strings you leave in your binary will be used to name the implant, as well as possibly name your APT, and be used to make wild guesses about who you are, so if you're going to leave any in, make sure they are cool.

As you go forward, ensure that you "encrypt" all strings and config information during the compilation process.

Submit your commit!
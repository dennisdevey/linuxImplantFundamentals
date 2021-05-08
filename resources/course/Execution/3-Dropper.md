# Dropper
Create a dropper that when run, decodes a binary blob into memory, writes it to disc, and then executes it.

This is slightly complicated and will require a rethinking of your compilation process. First you will need to build your backdoor, then encode it, then during compilation of your binary, pass that file in. Additionally, you will need to add in a decode and execute function to your first stage dropper. 

This is a very simple dropper, but they can be very complex in order to hide from anti-virus scans. We will spend more time on unpackers and other obfuscations later in the course. 

Submit your commit.
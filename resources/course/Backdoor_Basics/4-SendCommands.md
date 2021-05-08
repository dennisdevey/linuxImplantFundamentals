# Send Commands
If you are able to catch bytes via portknock and verify that it is a valid request, then you can add a encoded/encrypted string of bytes behind the magic bytes and do a follow on action, which could be calling back via C2 to an IP specified in those bytes (like Penguin does) or running a command that does actions on objective like lateral movement or self-removal, without having to callback via C2.

For now, let's just write a function that prints the contents of the encoded string of bytes behind the magic bytes. This will force you to define something somewhat like a protocol, but don't get too wrapped up in it now. Do something simple and just set a defined length for the number of payload bytes that get sent and then print those. 

1. What did you do to make that work? 
2. The link to your commit.



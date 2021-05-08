# Command Line Interface
There are 1000s of dev hours of work you can do to copy the features of Metasploit, but let's focus on the most important parts. Using the output of the logs from the Listening Post implement the following commands:

1. "list sessions": Print info about all available sessions, numbered
2. "set session _n_": Set context to a specific session _n_. If _n_ is "all", send to all sessions
3. "command _x_": Send  a shell command _x_ with arbitrary number of flags to be set at the listening post
4. "task _name_" Send a predefined task to be set at the listening post  
     * Writing all the different commands you find you might want will take forever. Try to stick with figuring out aliases for built in Linux commands that can get you the information you want in the format you want

Always:

* Display output of a session's response to a task when they check back in
* Display when a new session registers

Submit your commit.
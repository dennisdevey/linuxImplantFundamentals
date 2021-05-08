# Position Independent Code
Position independent code is important for us to understand in a variety of CNO contexts. Let's work through them. From Wikipedia: "In computing, position-independent code(PIC) or position-independent executable (PIE) is a body of machine code that, being placed somewhere in the primary memory, executes properly regardless of its absolute address."

***I need a good explanation of why this is needed and different from regular code:***

PIC is important in the context of:

1. Shared Objects
    * Shared objects don't know when and where they'll be loaded, but it has to work.
2. Position-independent executables (PIE) to enable ASLR
    * ASLR randomizes the base address so we have to figure out the GOT after it loads
3. Shellcode
    * Shellcode is the general term for raw bytes being executed. We don't know where our shellcode will run in memory, so it has to be independent

First, how does linking work in position independent code. Remember the GOT? From Wikipedia: 
"Procedure calls inside a shared library are typically made through small procedure linkage table stubs, which then call the definitive function. This notably allows a shared library to inherit certain function calls from previously loaded libraries rather than using its own versions.

Data references from position-independent code are usually made indirectly, through Global Offset Tables (GOTs), which store the addresses of all accessed global variables. There is one GOT per compilation unit or object module, and it is located at a fixed offset from the code (although this offset is not known until the library is linked). When a linker links modules to create a shared library, it merges the GOTs and sets the final offsets in code. It is not necessary to adjust the offsets when loading the shared library later." 

Now read about the -fpic option in 'man gcc'. 

Does this make sense? Don't move forward until it does.

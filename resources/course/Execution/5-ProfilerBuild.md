# Profile and Build

Using the profiler functionality previously built, we are going to send the profiler struct back to our Python shell handler. 

The handler will use ctypes to break out all the individual information from the struct, then compile a new listener using that information. Ensure that you have the ability to verify the glibc version prior to compilation so that you can use dynamic linking on the host. If you don't have that in your struct it is worth adding.

After compiling the second stage implant, host it and reach back down to the client with the URL so the first stagee can download and run it. 

This is a pretty advanced series of features but it is very very cool.

========================================================================
    CONSOLE APPLICATION : wsserver Project Overview
========================================================================


The task description can be found in Task.h file

Tech note:
Server is IP-agnostic (ipv4 and ipv6 both are used) and based on an asynchronous event-driven scheme (like nginx uses).
Dedicated instanses work with UDP and TCP protocols.
Each supported service saves its state into binary format file every N seconds.
There is a heartbeat server that monitors worker server activity, terminates suspended process,
and respawn another one. Saved state is loaded by respawned process.
The whole archicture based on classes specialization (high cohesion) and low coupling principles.
Though, this is just a working prototype solving the only task (see above).

Tools used:
Visual Studio 2015 + GitHub extension (installed from Gallery)
The only target platform is Microsoft Windows (x86-64).

Note on coding style:
Actually there is no any defined coding style. 
The code looks a bit eclectic because of C++11/14 syntax and library classes are mixed with Win API.
C++11 nullptr is not used because of NULL-based Win API dominance.
The code is Winsock 2.2 only oriented.
This example demonstrates the various techniques including meta-programming + interfaces + ATL.
Preprocessor and conditional compilation are used as well.

rightmark.development@gmail

/////////////////////////////////////////////////////////////////////////////

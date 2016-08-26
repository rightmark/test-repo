========================================================================
    CONSOLE APPLICATION : wsserver Project Overview
========================================================================

Purpose:
Create a client-server application for Windows.
Algorithm:
•	Clients send to the server random integer numbers from 0 to 1023. 
•	The server receives and stores these numbers in a binary tree using them as unique keys. 
•	Upon a client's request, the average of squares of the numbers including the newly received number is calculated and sent back to the client as a response.
•	A client receives this value and generates a new random number and then again sends it to the server and so on.
•	Every N seconds store all the items from the binary tree into a file in the binary format
Tests:
•	Output all the processes into console and the application's log file.
•	Run about 20 – 30 clients and keep the program working about 10-20 minutes.


/////////////////////////////////////////////////////////////////////////////

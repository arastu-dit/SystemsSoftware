# SystemsSoftware
Assignment

This is a multithreaded client-server socket program

The client creates a socket connection and sets it to the local address 127.0.0.1
It is configured to use port 8888 and it calls the connect function to establish a connection with the server
The server similar to the client, creates a socket to listen through the port 8888
Each time a connection is received, the server creates a threat to handle it.
Each thread receives the logging data and verifies it in the file "accounts". Before getting access to this file, it gets a lock due to which we avoid simultaneous access. The result of the verification is sent to the client. If it was sucessful the server receives the path and checks its validity. If it is valid, it proceeds to receive the file and sends the result of the transaction to the client. The lock is then acquried and the transaction is recorded.


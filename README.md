# Network-Calculator
Entry assignment from Distributed systems classes

# Assignment description:

Write a pair of programs, a server and a client, for KI's Group – in Java, for MB's Group – in C++11.

The client reads from the standard input an arithmetic expression. Such an expression consists of nonnegative numbers and plus and minus signs, potentially separated with spaces or tabulators. It is delimeted by an end-of-line sign or the end of input (whichever appears first). The expression may be arbitrarily long (it may not entirely fit in memory) and – in KI's Group only – a single number can exceed the range of the long type. The client then sends the expression to the server and awaits a reply. If the reply does not come within a configurable timeout, the client writes TIMEOUT to the standard output and exits with value 1; otherwise, it prints the received output and exists with value 0.

The server opens a TCP port (specified as a command line parameter) and spawns k+1 threads (where k is another command line parameter): 1 listener thread and k worker threads. The listener thread repeatedly listens on the specified port for incoming TCP connections. When a connection is established, the thread passes the connection to one of the workers. A worker thread repeatedly waits for established connections from the listener. When such a connection is passed, it receives the expression sent by the corresponding client, computes it, and replies with the result. If the expression is invalid, the result is ERROR. If there are no free workers to which an incoming connection can be passed, the listener waits until such a worker appears.

Running a client:

./client.sh -a <server_ipv4_address> -p <server_port_number>
Running a server:

./server.sh -p <server_port_number> -t <k>
Sample valid input:

1- 3    +10-111111+13
Sample invalid inputs:

1+-3
+1
-1
1+3-

# What is this project
This project is a demostration of a concurrent hashtable, and IPC through shared memory.

# Components
## Hashtable
The hashtable contains a number of ordered linked lists. Through hashing, a key determines which linked list it will join. 

Each block of the linked lists is protected by a read-write lock. This allows searching (reading) operations to avoid blocking each others. Inserting and deleting however are blocking operations due to the nature of read-write locks.

## Server
The server manages a number of workers (threads). Each worker can serve a client request at a time (read, insert, delete). 

The communication of the server and the clients is through shared memory. Initially, the server opens up a shared memory file, and initiates a producer-consumer pool. When there are available workers, the corresponding number of clients enters the pool, where they iterates through all workers and competes taking them. This can result in starvation, where a client is allowed to enter the pool but always fails to get a worker. But the possibility of this situation is extremely low. 

The communication between a worker and a client is also through shared memory. The client prepares a shared chunk containing the request information, and send the access id to a worker. The worker reads and performs the requested operation, prepares a response in another shared chunk if needed, and writes the access id back to the client's request chunk. The worker waits for the client to finish reading data from response chunk before becoming ready for the next request.

## Client
The client send a series of hashtable operation requests to the server, and may perform checking on the results.

By default, a client enters the "interactive mode", where it reads command from user input (stdin). The commands are in a "[Operation] [Key] [Value]" format.

Supported operations include:
- INSERT
- READ
- DELETE

When the operation is READ or DELETE, the Value will function as an expected result being used to compare with the return value from the server.

# How to compile
This project requires a **Linux** environment and **GCC** compiler. 

To compile, use the following command:

```make all```

The compiled binary are in `bin` directory, run with `--help` for more information.

# How to test
This project comes with a simple test script written in **Python**.

To run test, use the following command:

```make test```

The included test cases are:
- smoke: basic functionality
- big-smoke: longer version of smoke
- collaborate: 2 workers serve 1 client
- client-compete: 5 clients compete for 1 worker
- linear: multiple workers and clients working on 1 linked list
- mess: multiple worker, multiple clients, multiple linked lists
- big-mess: a heavier version of mess
- race: more like a test for the file system

You can also add your custom test cases, edit `tests` in `test.py` or feed client with a custom command list.

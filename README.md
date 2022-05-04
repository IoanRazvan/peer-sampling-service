# peer-sampling-service

This repository contains an implementation of a gossip based peer selection protocol as described [here](https://www.distributed-systems.net/my-data/papers/2007.tocs.pdf).

In short the goal of the peer sampling service is to provide a node in a distributed system with neighbors to comunicate with. Because of churn [^1] keeping a list of all nodes in the system in every node would require a large number of updates as participants join and leave. The peer sampling service avoids the penalty incurred by the naive approach while providing each node with a continuously changing distribution of peers using a gossip-based protocol.

[^1]: change in the set of participating nodes due to joins, graceful leaves, and failures

The program was developed using C++ and MPI. Each process spawned by MPI models a node in a distributed system. Each process has 3 threads: the main thread which actively selects peers to exchange information with, and two additional threads used to handle PULL and PUSH messages.

## Build and Run

To build the program you'll need the following:
- g++ compiler
- Makefile
- openmpi library.

### Installing the dependencies

C++

```bash
sudo apt install g++
```

Makefile

```bash
sudo apt install make
```

OpenMPI

```bash
sudo apt install libopenmpi-dev
```

### Compiling and running the program

To run the program with 50 processes use:

```bash
make run
```

To control the number of spawned processes use:

```bash
make run NPROC=number_of_processes
```

While running the proceess with rank 0 continuously prints it's partial view of the system, i.e. the neighbors he can exchange information with at a particular moment.

## Bibliography

- [Gossip-based Peer Sampling](https://www.distributed-systems.net/my-data/papers/2007.tocs.pdf)
- [Minimizing Churn in Distributed Systems](https://nymity.ch/sybilhunting/pdf/Godfrey2006a.pdf)
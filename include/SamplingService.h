#ifndef SAMPLING_SERVICE
#define SAMPLING_SERVICE

#include <vector>
#include <functional>
#include <mpi.h>
#include <chrono>
#include <mutex>
#include <random>
#include "Structures.h"
#include "View.h"


class SamplingService
{
public:
    SamplingService(int c, int H, int S, std::chrono::milliseconds delta);
    void init();

private:
    friend class View;

    // message tag constants
    const int PULL_MESSAGE_TAG = 0;
    const int PUSH_MESSAGE_TAG = 1;

    // peer sampling service parameters
    int c;
    int H;
    int S;

    // id of current process
    int ownRank;

    // gossip algorithm parameters
    std::chrono::milliseconds delta;
    bool pull = true;
    bool push = true;

    View view;
    
    // used to atomize multiple operations on a view
    // e.g. we wouldn't want to print the view while
    // the update function is running
    std::mutex viewAccessMutex; 

    // MPI datatype used for passing descriptors
    MPI::Datatype descriptorDatatype;

    // MPI communicator with virtual cartesian topology associated
    MPI::Cartcomm cartComm;
    

    void activeThread();
    void onPush();
    void onPull();
    void applyMessage(Message &m);
    void sendPush(int peer, std::vector<Descriptor> buffer);
    void sendPull(int peer, std::vector<Descriptor> buffer);

    std::vector<Descriptor> toSend();
    void update(std::vector<Descriptor> &buffer);
    int selectGPSPeer();
    void printView();
    std::vector<char> packMessage(Message & m);
    Message unpackMessage(std::vector<char> & v);
    std::vector<Descriptor> padBuffer(std::vector<Descriptor> & v);
    std::vector<Descriptor> removePadding(std::vector<Descriptor> & v);
    int getMessageSize() const;
    static int randomIndex(int range_from, int range_to);
    static bool isPrime(int n);
    static std::vector<int> getCloseFactors(int n);
};

#endif
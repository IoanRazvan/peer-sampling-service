#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <vector>

struct Descriptor
{
    int rank;
    int age;

    Descriptor(int rank, int age) : rank(rank), age(age) {}
    Descriptor() {}
    void increaseAge() { age++; }
};

// in the first message we are not guaranteed to have c / 2 - 1 entries in the view
// because our initial views will contain only 4 neighbors
// in that case we need to signal where the valid data ends
const Descriptor NULL_DESCRIPTOR = {-1, -1};

struct Message
{
    std::vector<Descriptor> buffer;
    int sender;
    Message(const std::vector<Descriptor> &buffer, int sender) : buffer(buffer), sender(sender) {}
    Message(int size) : buffer(size) {}
    Message() {}
};

#endif
#ifndef VIEW_H
#define VIEW_H

#include <vector>
#include <mutex>
#include <mpi.h>
#include "Structures.h"

class View
{
    using DescriptorsItType = std::vector<Descriptor>::iterator;
    std::vector<Descriptor> descriptors;
    // used to make the view datastructure thread safe
    mutable std::mutex m;
    std::vector<DescriptorsItType> findOldest(int H);
private:
    static int randomIndex(int from, int to);
public:
    void init(MPI::Cartcomm comm, int ownRank);
    void shuffle();
    void moveOldestToBack(int H);
    void removeOldItems(int size);
    void removeHead(int size);
    void removeAtRandom(int size);
    std::vector<Descriptor> head(int size) const;
    void increaseAge();
    void append(std::vector<Descriptor> &buffer);
    void removeDuplicates();
    int randomDescriptor() const;
    int size() const;
    void print() const;
};

#endif
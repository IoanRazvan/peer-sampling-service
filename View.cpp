#include "View.h"
#include <algorithm>
#include <set>
#include <random>

void View::init(MPI::Cartcomm comm, int ownRank)
{
    int top, bottom, right, left;
    comm.Shift(0, 1, top, bottom);
    comm.Shift(1, 1, left, right);
    std::set<int> neighbors = {top, bottom, right, left};
    for (int neighbor : neighbors)
        descriptors.push_back({neighbor, 0});
}

void View::shuffle()
{
    std::lock_guard<std::mutex> lock(m);
    std::random_shuffle(descriptors.begin(), descriptors.end());
}

void View::moveOldestToBack(int H)
{
    std::lock_guard<std::mutex> lock(m);
    if (H <= 0 || H >= descriptors.size())
        return;
    std::vector<std::vector<Descriptor>::iterator> oldestH = findOldest(H);
    int viewSize = descriptors.size();

    for (auto it = oldestH.begin(); it != oldestH.end(); it++)
        std::swap(descriptors[viewSize - 1 - (it - oldestH.begin())], **it);
}

std::vector<View::DescriptorsItType> View::findOldest(int H)
{
    if (H <= 0 || H >= descriptors.size())
        return std::vector<DescriptorsItType>();
    std::vector<DescriptorsItType> oldestH;
    int viewSize = descriptors.size();
    for (auto it = descriptors.begin(); it < descriptors.begin() + H; it++)
        oldestH.push_back(it);
    for (auto it = descriptors.begin() + H; it < descriptors.end(); it++)
    {
        auto youngerElement = std::find_if(oldestH.begin(), oldestH.end(), [it](auto d1)
                                           { return it->age > d1->age; });

        if (youngerElement != oldestH.end())
            *youngerElement = it;
    }
    return oldestH;
}

void View::removeOldItems(int H)
{
    std::lock_guard<std::mutex> lock(m);
    if (H <= 0 || H >= descriptors.size())
        return;
    auto oldestHIt = findOldest(H);
    int viewSize = descriptors.size();
    std::vector<Descriptor> oldestH;
    std::transform(oldestHIt.begin(), oldestHIt.end(), std::insert_iterator<std::vector<Descriptor>>(oldestH, oldestH.begin()), [](auto it)
                   { return Descriptor(it->rank, it->age); });

    for (auto it = oldestH.begin(); it != oldestH.end(); it++)
        std::remove_if(descriptors.begin(), descriptors.end(), [it](auto el)
                       { return el.rank == it->rank; });
    descriptors.resize(viewSize - H);
}

void View::removeHead(int size)
{
    std::lock_guard<std::mutex> lock(m);
    if (size <= 0 || size > descriptors.size())
        return;
    descriptors.erase(descriptors.begin(), descriptors.begin() + size);
}

void View::removeAtRandom(int size)
{
    std::lock_guard<std::mutex> lock(m);
    if (size <= 0 || size >= descriptors.size())
        return;
    for (int i = 0; i < size; i++)
    {
        int randIndex = randomIndex(0, (int)descriptors.size() - 1);
        descriptors.erase(descriptors.begin() + randIndex);
    }
}

int View::randomIndex(int range_from, int range_to)
{
    std::random_device rand_dev;
    std::mt19937 generator(rand_dev());
    std::uniform_int_distribution<int> distr(range_from, range_to);
    return distr(generator);
}

std::vector<Descriptor> View::head(int size) const
{
    std::lock_guard<std::mutex> lock(m);
    if (size <= 0 || size > descriptors.size())
        return descriptors;
    return std::vector<Descriptor>(descriptors.begin(), descriptors.begin() + size);
}

void View::increaseAge()
{
    std::lock_guard<std::mutex> lock(m);
    for (auto &descriptor : descriptors)
        descriptor.increaseAge();
}

void View::append(std::vector<Descriptor> &buffer)
{
    std::lock_guard<std::mutex> lock(m);
    descriptors.insert(descriptors.end(), buffer.begin(), buffer.end());
}

void View::removeDuplicates()
{
    std::map<int, int> m;
    std::map<int, bool> seen;
    std::lock_guard<std::mutex> lock(this->m);
    for (const auto &d : descriptors)
    {
        auto it = m.find(d.rank);
        if (it != m.end() && it->second > d.age)
            it->second = d.age;
        else
            m.insert({d.rank, d.age});
    }

    auto new_end = std::remove_if(descriptors.begin(), descriptors.end(), [&m, &seen](auto d){
        auto p = m.find(d.rank);
        bool outcome = d.age != p->second || seen[d.rank];
        if (outcome == false)
            seen[d.rank] = true;
        return outcome;
    });

    descriptors.erase(new_end, descriptors.end());
}

int View::randomDescriptor() const
{
    std::lock_guard<std::mutex> lock(m);
    return descriptors[randomIndex(0, (int)descriptors.size() - 1)].rank;
}

int View::size() const
{
    std::lock_guard<std::mutex> lock(m);
    return descriptors.size();
}

void View::print() const
{
    std::lock_guard<std::mutex> lock(m);
    for (Descriptor d : descriptors)
        std::cout << "rank: " << d.rank << ", age: " << d.age << std::endl;
    std::cout << "--------------" << std::endl;
}
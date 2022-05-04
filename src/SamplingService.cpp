#include "SamplingService.h"
#include <thread>
#include <algorithm>
#include <iostream>
#include <set>
#include <cmath>

SamplingService::SamplingService(int c, int H, int S, std::chrono::milliseconds delta)
{
    this->c = c;
    this->H = H;
    this->S = S;
    this->delta = delta;
}

void SamplingService::init()
{
    int providedThreadModel = MPI::Init_thread(MPI::THREAD_MULTIPLE);
    if (providedThreadModel != MPI::THREAD_MULTIPLE)
    {
        std::cerr << "Environment does not support thread model MPI_THREAD_MULTIPLE!" << std::endl;
        std::cerr << "Aborting..." << std::endl;
        MPI::COMM_WORLD.Abort(-1);
    }
    int nproc = MPI::COMM_WORLD.Get_size();
    if (nproc <= 3 || isPrime(nproc))
    {
        std::cerr << "The specified number of processes does not allow the creation of a 2D cartesian topology!" << std::endl;
        std::cerr << "Aborting..." << std::endl;
        MPI::COMM_WORLD.Abort(-1);
    }

    std::vector<int> dims = getCloseFactors(nproc);
    bool periods[] = {true, true};
    cartComm = MPI::COMM_WORLD.Create_cart(2, dims.data(), periods, true);
    ownRank = cartComm.Get_rank();
    view.init(cartComm, ownRank);

    int descriptorDatatypeBlockLenghts[] = {2};
    MPI::Aint descriptorDatatypeDispl[] = {0};
    MPI::Datatype descriptorDatatypeTypes[] = {MPI::INT};
    descriptorDatatype = MPI::Datatype::Create_struct(1, descriptorDatatypeBlockLenghts, descriptorDatatypeDispl, descriptorDatatypeTypes);
    descriptorDatatype.Commit();

    activeThread();
}

bool SamplingService::isPrime(int n)
{
    if (n <= 1)
        return false;
    for (int i = 2; i <= n - 1; i++)
        if (n % i == 0)
            return false;
    return true;
}

std::vector<int> SamplingService::getCloseFactors(int n)
{
    int i = (int)(std::sqrt(n) + 0.5);
    while (n % i != 0)
        i -= 1;
    return std::vector<int>({i, n / i});
}

void SamplingService::activeThread()
{
    std::thread t1(&SamplingService::onPush, this), t2(&SamplingService::onPull, this);
    int peer;
    while (true)
    {
        std::this_thread::sleep_for(delta);
        if (ownRank == 0)
            printView();

        peer = selectGPSPeer();
        sendPush(peer, toSend());
        view.increaseAge();
    }
}

void SamplingService::printView()
{
    std::lock_guard<std::mutex> lock(viewAccessMutex);
    view.print();
}

int SamplingService::selectGPSPeer()
{
    std::lock_guard<std::mutex> lock(viewAccessMutex);
    return view.randomDescriptor();
}

std::vector<Descriptor> SamplingService::toSend()
{
    std::vector<Descriptor> buffer{{ownRank, 0}};
    std::lock_guard<std::mutex> lock(viewAccessMutex);
    view.shuffle();
    view.moveOldestToBack(H);
    std::vector<Descriptor> viewHead = view.head(c / 2 - 1);
    buffer.insert(buffer.end(), viewHead.begin(), viewHead.end());
    return buffer;
}

void SamplingService::sendPush(int peer, std::vector<Descriptor> buffer)
{
    Message m = Message(buffer, ownRank);
    auto messageData = packMessage(m);
    cartComm.Isend(messageData.data(), messageData.size(), MPI::PACKED, peer, PUSH_MESSAGE_TAG);
}

std::vector<char> SamplingService::packMessage(Message &m)
{
    std::vector<char> messageData(getMessageSize());
    auto paddedBuffer = padBuffer(m.buffer);
    int pos = 0;

    MPI::INT.Pack(&m.sender, 1, messageData.data(), messageData.size(), pos, cartComm);
    descriptorDatatype.Pack(paddedBuffer.data(), paddedBuffer.size(), messageData.data(), messageData.size(), pos, cartComm);
    return messageData;
}

std::vector<Descriptor> SamplingService::padBuffer(std::vector<Descriptor> &v)
{
    std::vector<Descriptor> padded(v.begin(), v.end());
    if (padded.size() < (c / 2 + 1))
        padded.insert(padded.end(), (c / 2 + 1) - padded.size(), NULL_DESCRIPTOR);
    return padded;
}

Message SamplingService::unpackMessage(std::vector<char> &messageData)
{
    Message m(c / 2 + 1);
    int pos = 0;

    MPI::INT.Unpack(messageData.data(), messageData.size(), &m.sender, 1, pos, cartComm);
    descriptorDatatype.Unpack(messageData.data(), messageData.size(), m.buffer.data(), c / 2 + 1, pos, cartComm);
    m.buffer = removePadding(m.buffer);
    return m;
}

std::vector<Descriptor> SamplingService::removePadding(std::vector<Descriptor> &v)
{
    std::vector<Descriptor> unpadded(v.begin(), v.end());
    auto newEnd = std::remove_if(unpadded.begin(), unpadded.end(), [this](Descriptor d)
                                 { return d.rank = NULL_DESCRIPTOR.rank && d.age == NULL_DESCRIPTOR.age; });
    unpadded.erase(newEnd, unpadded.end());
    return unpadded;
}

void SamplingService::onPush()
{
    Message pushMessage;
    std::vector<char> messageData(getMessageSize());
    while (true)
    {
        cartComm.Recv(messageData.data(), messageData.size(), MPI::PACKED, MPI::ANY_SOURCE, PUSH_MESSAGE_TAG);
        pushMessage = unpackMessage(messageData);
        if (pull)
            sendPull(pushMessage.sender, toSend());
        applyMessage(pushMessage);
    }
}

void SamplingService::sendPull(int peer, std::vector<Descriptor> buffer)
{
    Message m = Message(buffer, ownRank);
    auto messageData = packMessage(m);
    cartComm.Isend(messageData.data(), messageData.size(), MPI::PACKED, peer, PULL_MESSAGE_TAG);
}

void SamplingService::onPull()
{
    Message pullMessage;
    std::vector<char> messageData(getMessageSize());
    while (true)
    {
        cartComm.Recv(messageData.data(), messageData.size(), MPI::PACKED, MPI::ANY_SOURCE, PULL_MESSAGE_TAG);
        pullMessage = unpackMessage(messageData);
        applyMessage(pullMessage);
    }
}

void SamplingService::applyMessage(Message &m)
{
    std::lock_guard<std::mutex> lock(viewAccessMutex);
    update(m.buffer);
    view.increaseAge();
}

void SamplingService::update(std::vector<Descriptor> &buffer)
{
    view.append(buffer);
    view.removeDuplicates();
    view.removeOldItems(std::min(H, view.size() - c));
    view.removeHead(std::min(S, view.size() - c));
    view.removeAtRandom(view.size() - c);
}

int SamplingService::getMessageSize() const
{
    return sizeof(int) + (c / 2 + 1) * sizeof(Descriptor);
}
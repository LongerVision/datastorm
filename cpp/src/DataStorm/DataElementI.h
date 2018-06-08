// **********************************************************************
//
// Copyright (c) 2003-2017 ZeroC, Inc. All rights reserved.
//
// This copy of Ice is licensed to you under the terms described in the
// ICE_LICENSE file included in this distribution.
//
// **********************************************************************

#pragma once

#include <DataStorm/InternalI.h>
#include <DataStorm/ForwarderManager.h>
#include <DataStorm/Contract.h>

#include <deque>

namespace DataStormInternal
{

class SessionI;
class TopicI;
class TopicReaderI;
class TopicWriterI;
class CallbackExecutor;

class TraceLevels;

class DataElementI : virtual public DataElement, public Forwarder, public std::enable_shared_from_this<DataElementI>
{
    struct ListenerKey
    {
        SessionI* session;
        std::string facet;

        bool operator<(const ListenerKey& other) const
        {
            if(session < other.session)
            {
                return true;
            }
            else if(other.session < session)
            {
                return false;
            }
            return facet < other.facet;
        }
    };

    struct Subscriber
    {
        long long int id;
        std::set<std::shared_ptr<Key>> keys;
        std::shared_ptr<Filter> filter;
        std::shared_ptr<Filter> sampleFilter;
        int priority;
    };

    struct Listener
    {
        Listener(const std::shared_ptr<DataStormContract::SessionPrx>& proxy, const std::string& facet) :
            proxy(facet.empty() ? proxy : Ice::uncheckedCast<DataStormContract::SessionPrx>(proxy->ice_facet(facet)))
        {
        }

        bool matchOne(const std::shared_ptr<Sample>& sample) const
        {
            for(const auto& s : subscribers)
            {
                if(!s.second.sampleFilter || s.second.sampleFilter->match(sample))
                {
                    return true;
                }
            }
            return false;
        }

        bool empty() const
        {
            return subscribers.empty();
        }

        bool add(long long int topicId, long long int elementId, long long int id, const std::shared_ptr<Key>& key,
                 const std::shared_ptr<Filter>& filter, const std::shared_ptr<Filter>& sampleFilter)
        {
            auto k = std::make_pair(topicId, elementId);
            auto p = subscribers.find(k);
            if(p == subscribers.end())
            {
                p = subscribers.emplace(k, Subscriber { id, {}, filter, sampleFilter }).first;
            }
            return p->second.keys.insert(key).second;
        }

        bool remove(long long int topicId, long long int elementId, const std::shared_ptr<Key>& key, long long int& id)
        {
            std::shared_ptr<Filter> ignore;
            return remove(topicId, elementId, key, id, ignore);
        }

        bool remove(long long int topicId, long long int elementId, const std::shared_ptr<Key>& key,
                    long long int& id, std::shared_ptr<Filter>& filter)
        {
            auto k = std::make_pair(topicId, elementId);
            auto p = subscribers.find(k);
            if(p != subscribers.end())
            {
                if(p->second.keys.erase(key))
                {
                    if(p->second.keys.empty())
                    {
                        id = p->second.id;
                        filter = p->second.filter;
                        subscribers.erase(p);
                    }
                    return true;
                }
            }
            return false;
        }

        std::shared_ptr<DataStormContract::SessionPrx> proxy;
        std::map<std::pair<long long int, long long int>, Subscriber> subscribers;
    };

public:

    DataElementI(TopicI*, long long int, const DataStorm::Config&);
    virtual ~DataElementI();

    void init();

    virtual void destroy() override;

    void attach(long long int,
                long long int,
                const std::shared_ptr<Key>&,
                const std::shared_ptr<Filter>&,
                SessionI*,
                const std::shared_ptr<DataStormContract::SessionPrx>&,
                const DataStormContract::ElementData&,
                const std::chrono::time_point<std::chrono::system_clock>&,
                DataStormContract::ElementDataAckSeq&);

    void attach(long long int,
                long long int,
                const std::shared_ptr<Key>&,
                const std::shared_ptr<Filter>&,
                SessionI*,
                const std::shared_ptr<DataStormContract::SessionPrx>&,
                const DataStormContract::ElementDataAck&,
                const std::chrono::time_point<std::chrono::system_clock>&,
                DataStormContract::DataSamplesSeq&);

    bool attachKey(long long int,
                   long long int,
                   const std::shared_ptr<Key>&,
                   const std::shared_ptr<Filter>&,
                   SessionI*,
                   const std::shared_ptr<DataStormContract::SessionPrx>&,
                   const std::string&,
                   long long int);

    void detachKey(long long int,
                   long long int,
                   const std::shared_ptr<Key>&,
                   SessionI*,
                   const std::string&,
                   bool);

    bool attachFilter(long long int,
                      long long int,
                      const std::shared_ptr<Key>&,
                      const std::shared_ptr<Filter>&,
                      SessionI*,
                      const std::shared_ptr<DataStormContract::SessionPrx>&,
                      const std::string&,
                      long long int,
                      const std::shared_ptr<Filter>&);

    void detachFilter(long long int,
                      long long int,
                      const std::shared_ptr<Key>&,
                      SessionI*,
                      const std::string&,
                      bool);

    virtual void onKeyConnect(std::function<void(Id, std::shared_ptr<Key>)>) override;
    virtual void onKeyDisconnect(std::function<void(Id, std::shared_ptr<Key>)>) override;
    virtual std::vector<std::shared_ptr<Key>> getConnectedKeys() const override;

    virtual void onFilterConnect(std::function<void(Id, std::shared_ptr<Filter>)>) override;
    virtual void onFilterDisconnect(std::function<void(Id, std::shared_ptr<Filter>)>) override;

    virtual void initSamples(const std::vector<std::shared_ptr<Sample>>&, long long int, long long int,
                             const std::chrono::time_point<std::chrono::system_clock>&, bool);
    virtual DataStormContract::DataSamples getSamples(const std::shared_ptr<Key>&,
                                                      const std::shared_ptr<Filter>&,
                                                      const std::shared_ptr<DataStormContract::ElementConfig>&,
                                                      long long int,
                                                      const std::chrono::time_point<std::chrono::system_clock>&);

    virtual void queue(const std::shared_ptr<Sample>&, SessionI*, const std::string&,
                       const std::chrono::time_point<std::chrono::system_clock>&, bool);

    virtual std::string toString() const = 0;
    virtual std::shared_ptr<Ice::Communicator> getCommunicator() const override;

    long long int getId() const
    {
        return _id;
    }

    std::shared_ptr<DataStormContract::ElementConfig> getConfig() const;

    void waitForListeners(int count) const;
    bool hasListeners() const;

    TopicI* getTopic() const
    {
        return _parent.get();
    }

protected:

    virtual void checkPriorityOnAttach(SessionI*, long long int, long long int, int);
    virtual void checkPriorityOnDetach(SessionI*, long long int, long long int);

    void notifyListenerWaiters(std::unique_lock<std::mutex>&) const;
    void disconnect();
    virtual void destroyImpl() = 0;

    std::shared_ptr<TraceLevels> _traceLevels;
    std::shared_ptr<DataStormContract::SessionPrx> _forwarder;
    mutable std::shared_ptr<Sample> _sample;
    long long int _id;
    size_t _listenerCount;
    std::shared_ptr<DataStormContract::ElementConfig> _config;
    std::shared_ptr<CallbackExecutor> _executor;

private:

    virtual void forward(const Ice::ByteSeq&, const Ice::Current&) const override;

    std::shared_ptr<TopicI> _parent;
    std::map<ListenerKey, Listener> _listeners;
    mutable size_t _waiters;
    mutable size_t _notified;

    std::function<void(Id, std::shared_ptr<Key>)> _onKeyConnect;
    std::function<void(Id, std::shared_ptr<Key>)> _onKeyDisconnect;
    std::function<void(Id, std::shared_ptr<Filter>)> _onFilterConnect;
    std::function<void(Id, std::shared_ptr<Filter>)> _onFilterDisconnect;
};

class DataReaderI : public DataElementI, public DataReader
{
public:

    DataReaderI(TopicReaderI*, long long int, const std::string&, std::vector<unsigned char>,
                const DataStorm::ReaderConfig&);

    virtual int getInstanceCount() const override;

    virtual std::vector<std::shared_ptr<Sample>> getAllUnread() override;
    virtual void waitForUnread(unsigned int) const override;
    virtual bool hasUnread() const override;
    virtual std::shared_ptr<Sample> getNextUnread() override;

    virtual void initSamples(const std::vector<std::shared_ptr<Sample>>&, long long int, long long int,
                             const std::chrono::time_point<std::chrono::system_clock>&, bool) override;
    virtual void queue(const std::shared_ptr<Sample>&, SessionI*, const std::string&,
                       const std::chrono::time_point<std::chrono::system_clock>&, bool) override;

    virtual void onInit(std::function<void(const std::vector<std::shared_ptr<Sample>>&)>) override;
    virtual void onSample(std::function<void(const std::shared_ptr<Sample>&)>) override;

protected:

    virtual void checkPriorityOnAttach(SessionI*, long long int, long long int, int) override;
    virtual void checkPriorityOnDetach(SessionI*, long long int, long long int) override;

    virtual bool matchKey(const std::shared_ptr<Key>&) const = 0;

    TopicReaderI* _parent;

    std::deque<std::shared_ptr<Sample>> _samples;
    int _instanceCount;
    DataStorm::DiscardPolicy _discardPolicy;
    std::chrono::time_point<std::chrono::system_clock> _lastSendTime;
    std::function<void(const std::vector<std::shared_ptr<Sample>>&)> _onInit;
    std::function<void(const std::shared_ptr<Sample>&)> _onSample;
};

class DataWriterI : public DataElementI, public DataWriter
{
public:

    DataWriterI(TopicWriterI*, long long int, const DataStorm::WriterConfig&);
    void init();

    virtual void publish(const std::shared_ptr<Key>&, const std::shared_ptr<Sample>&) override;

protected:

    virtual void send(const std::shared_ptr<Key>&, const std::shared_ptr<Sample>&) const = 0;

    TopicWriterI* _parent;
    std::shared_ptr<DataStormContract::SubscriberSessionPrx> _subscribers;
    std::deque<std::shared_ptr<Sample>> _samples;
};

class KeyDataReaderI : public DataReaderI
{
public:

    KeyDataReaderI(TopicReaderI*, long long int, const std::vector<std::shared_ptr<Key>>&, const std::string&,
                   std::vector<unsigned char>, const DataStorm::ReaderConfig&);

    virtual void destroyImpl() override;

    virtual void waitForWriters(int) override;
    virtual bool hasWriters() override;

    virtual std::string toString() const override;

private:

    virtual bool matchKey(const std::shared_ptr<Key>&) const override;

    const std::vector<std::shared_ptr<Key>> _keys;
};

class KeyDataWriterI : public DataWriterI
{
public:

    KeyDataWriterI(TopicWriterI*, long long int, const std::vector<std::shared_ptr<Key>>&,
                   const DataStorm::WriterConfig&);

    virtual void destroyImpl() override;

    virtual void waitForReaders(int) const override;
    virtual bool hasReaders() const override;

    virtual std::shared_ptr<Sample> getLast() const override;
    virtual std::vector<std::shared_ptr<Sample>> getAll() const override;

    virtual std::string toString() const override;
    virtual DataStormContract::DataSamples getSamples(const std::shared_ptr<Key>&,
                                                      const std::shared_ptr<Filter>&,
                                                      const std::shared_ptr<DataStormContract::ElementConfig>&,
                                                      long long int,
                                                      const std::chrono::time_point<std::chrono::system_clock>&) override;

private:

    virtual void send(const std::shared_ptr<Key>&, const std::shared_ptr<Sample>&) const override;

    const std::vector<std::shared_ptr<Key>> _keys;
};

class FilteredDataReaderI : public DataReaderI
{
public:

    FilteredDataReaderI(TopicReaderI*, long long int, const std::shared_ptr<Filter>&, const std::string&,
                        std::vector<unsigned char>, const DataStorm::ReaderConfig&);

    virtual void destroyImpl() override;

    virtual void waitForWriters(int) override;
    virtual bool hasWriters() override;

    virtual std::string toString() const override;

private:

    virtual bool matchKey(const std::shared_ptr<Key>&) const override;

    const std::shared_ptr<Filter> _filter;
};

}

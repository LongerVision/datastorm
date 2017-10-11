// **********************************************************************
//
// Copyright (c) 2003-2017 ZeroC, Inc. All rights reserved.
//
// This copy of Ice is licensed to you under the terms described in the
// ICE_LICENSE file included in this distribution.
//
// **********************************************************************

#pragma once

#include <DataStorm/Config.h>
#include <DataStorm/InternalI.h>

#ifndef DATASTORM_API
#   ifdef DATASTORM_API_EXPORTS
#       define DATASTORM_API DATASTORM_DECLSPEC_EXPORT
#   elif defined(DATASTORM_STATIC_LIBS)
#       define DATASTORM_API /**/
#   else
#       define DATASTORM_API DATASTORM_DECLSPEC_IMPORT
#   endif
#endif

namespace DataStorm
{

template<typename K, typename V, typename F> class TopicReader;
template<typename K, typename V, typename F> class TopicWriter;

/**
 * The Node class allows creating topic readers and writers.
 *
 * A Node is the main DataStorm object which allows creating topic readers or writers.
 */
class DATASTORM_API Node
{
public:

    /**
     * Construct a DataStorm node.
     *
     * A node is the main DataStorm object. It is required to construct topic readers or writers.
     * The node uses the given Ice communicator if provided.
     *
     * @param communicator The Ice communicator used by the topic factory for its configuration
     *                     and communications.
     */
    Node(const std::shared_ptr<Ice::Communicator>& communicator = nullptr);

    /**
     * Construct a DataStorm node.
     *
     * This constructor parses the command line arguments into Ice properties and
     * initialize a new Node.
     *
     * @param argc The number of command line arguments in the argv array.
     * @param argv The command line arguments
     */
    Node(int& argc, char* argv[]);

    /**
     * Destruct the node. The node destruction releases associated resources.
     */
    ~Node();

    /**
     * Returns the Ice communicator associated with the node.
     */
    std::shared_ptr<Ice::Communicator> getCommunicator() const;

private:

    std::shared_ptr<DataStormInternal::Instance> _instance;
    std::shared_ptr<DataStormInternal::TopicFactory> _factory;

    template<typename, typename, typename> friend class TopicReader;
    template<typename, typename, typename> friend class TopicWriter;

    const bool _ownsCommunicator;
};

}
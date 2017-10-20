// **********************************************************************
//
// Copyright (c) 2003-2017 ZeroC, Inc. All rights reserved.
//
// **********************************************************************

#include <DataStorm/DataStorm.h>

using namespace std;

int
main(int argc, char* argv[])
{
    //
    // Instantiates DataStorm node.
    //
    DataStorm::Node node(argc, argv);

    //
    // Instantiates the "hello" topic. The topic uses strings for keys and values
    // and also supports key filtering with the DataStorm::RegexFilter regular
    // expression filter.
    //
    DataStorm::Topic<string, string, DataStorm::RegexFilter<string>, string> topic(node, "hello");

    //
    // Instantiate writers
    //
    auto writera = DataStorm::makeKeyWriter(topic, "fooa");
    auto writerb = DataStorm::makeKeyWriter(topic, "foob");
    auto writerc = DataStorm::makeKeyWriter(topic, "fooc");
    auto writerd = DataStorm::makeKeyWriter(topic, "food");
    auto writere = DataStorm::makeKeyWriter(topic, "fooe");

    //
    // Publish a sample on each writer.
    //
    writera.update("hello");
    writerb.update("hello");
    writerc.update("hello");
    writerd.update("hello");
    writere.update("hello");

    //
    // Wait for a reader to connect and then disconnect.
    //
    topic.waitForReaders();
    topic.waitForNoReaders();

    return 0;
}
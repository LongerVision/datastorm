// **********************************************************************
//
// Copyright (c) 2003-2017 ZeroC, Inc. All rights reserved.
//
// This copy of Ice is licensed to you under the terms described in the
// ICE_LICENSE file included in this distribution.
//
// **********************************************************************

#include <DataStorm/DataStorm.h>

#include <TestCommon.h>

using namespace DataStorm;
using namespace std;

int
main(int argc, char* argv[])
{
    Node node(argc, argv);

    Topic<string, string> topic(node, "topic");
    Topic<string, bool> controller(node, "controller");

    auto writers = makeSingleKeyWriter(controller, "writers");
    auto readers = makeSingleKeyReader(controller, "readers");

    {
        WriterConfig config;
        config.clearHistory = ClearHistoryPolicy::Never;
        topic.setWriterDefaultConfig(config);
    }

    cout << "testing writer sampleCount... " << flush;
    {
        auto write = [&topic, &writers, &readers](WriterConfig config)
        {
            writers.update(false); // Not ready
            auto writer = makeSingleKeyWriter(topic, "elem1", config);
            writer.add("value1");
            writer.update("value2");
            writer.remove();
            writer.add("value3");
            writer.update("value4");
            writer.remove();
            writers.update(true); // Ready

            while(!readers.getNextUnread().getValue()); // Wait for reader to be done
        };

        // Keep all the samples in the history.
        {
            WriterConfig config;
            config.clearHistory = ClearHistoryPolicy::Never;
            write(config);
        }

        // Keep 4 samples in the history
        {
            WriterConfig config;
            config.sampleCount = 4;
            write(config);
        }

        // Keep last instance samples in the history
        {
            WriterConfig config;
            config.clearHistory = ClearHistoryPolicy::OnAdd;
            write(config);
        }
    }
    cout << "ok" << endl;

    cout << "testing reader sampleCount... " << flush;
    {
        auto write = [&topic, &writers, &readers]()
        {
            writers.update(false); // Not ready
            auto writer = makeSingleKeyWriter(topic, "elem1");
            writer.add("value1");
            writer.update("value2");
            writer.remove();
            writer.add("value3");
            writer.update("value4");
            writer.remove();
            writers.update(true); // Ready

            while(!readers.getNextUnread().getValue()); // Wait for reader to be done
        };

        write(); // Reader keeps all the samples in the history.
        write(); // Reader keeps 4 samples in the history.
        write(); // Reader keeps last instance samples in the history.
    }
    cout << "ok" << endl;

    cout << "testing writer sampleLifetime... " << flush;
    {
        writers.update(false); // Not ready

        // Keep 3ms worth of samples in the history
        WriterConfig config;
        config.sampleLifetime = 20;
        auto writer = makeSingleKeyWriter(topic, "elem1", config);
        writer.add("value1");
        writer.update("value2");
        writer.remove();
        this_thread::sleep_for(chrono::milliseconds(20));
        writers.update(true); // Ready
        writer.waitForReaders();
        writer.add("value3");
        writer.update("value4");
        writer.remove();

        while(!readers.getNextUnread().getValue()); // Wait for reader to be done
    }
    cout << "ok" << endl;

    cout << "testing reader sampleLifetime... " << flush;
    {
        writers.update(false); // Not ready
        auto writer = makeSingleKeyWriter(topic, "elem1");
        writer.add("value1");
        writer.update("value2");
        writer.remove();
        this_thread::sleep_for(chrono::milliseconds(200));
        writer.add("value3");
        writer.update("value4");
        writer.remove();
        writers.update(true); // Ready

        while(!readers.getNextUnread().getValue()); // Wait for reader to be done
    }
    cout << "ok" << endl;

    cout << "testing writer clearHistory... " << flush;
    {
        topic.setUpdater<string>("concat", [](string& value, string update) { value += update; });

        {
            writers.update(false); // Not ready
            WriterConfig config;
            config.clearHistory = ClearHistoryPolicy::Never;
            auto writer = makeSingleKeyWriter(topic, "elem1", config);
            writer.add("value1");
            for(int i = 0; i < 20; ++i)
            {
                writer.update("value2");
            }
            writer.remove();
            test(writer.getAll().size() == 22);
            writers.update(true); // Ready
            while(!readers.getNextUnread().getValue()); // Wait for reader to be done
        }

        {
            writers.update(false); // Not ready
            WriterConfig config;
            config.clearHistory = ClearHistoryPolicy::OnAdd;
            auto writer = makeSingleKeyWriter(topic, "elem1", config);
            writer.add("value1");
            writer.update("value2");
            writer.remove();
            writer.add("value3");
            writer.update("value4");
            test(writer.getAll().size() == 2);
            writers.update(true); // Ready
            while(!readers.getNextUnread().getValue()); // Wait for reader to be done
        }

        {
            writers.update(false); // Not ready
            WriterConfig config;
            config.clearHistory = ClearHistoryPolicy::OnRemove;
            auto writer = makeSingleKeyWriter(topic, "elem1", config);
            writer.add("value1");
            writer.update("value2");
            writer.remove();
            writer.add("value3");
            writer.update("value4");
            test(writer.getAll().size() == 3);
            writers.update(true); // Ready
            while(!readers.getNextUnread().getValue()); // Wait for reader to be done
        }

        {
            writers.update(false); // Not ready
            WriterConfig config;
            config.clearHistory = ClearHistoryPolicy::OnAll;
            auto writer = makeSingleKeyWriter(topic, "elem1", config);
            writer.add("value1");
            writer.update("value2");
            writer.remove();
            writer.add("value3");
            writer.update("value4");
            test(writer.getAll().size() == 1);
            writers.update(true); // Ready
            while(!readers.getNextUnread().getValue()); // Wait for reader to be done
        }

        {
            writers.update(false); // Not ready
            WriterConfig config;
            config.clearHistory = ClearHistoryPolicy::OnAllExceptPartialUpdate;
            auto writer = makeSingleKeyWriter(topic, "elem1", config);
            writer.add("value1");
            writer.update("value2");
            writer.update<string>("concat", "1");
            writer.remove();
            writer.add("value3");
            writer.update("value");
            writer.update<string>("concat", "1");
            writer.update<string>("concat", "2");
            writer.update<string>("concat", "3");
            test(writer.getAll().size() == 4);
            test(writer.getAll()[1].getValue() == "value1");
            writers.update(true); // Ready
            while(!readers.getNextUnread().getValue()); // Wait for reader to be done
        }
    }
    cout << "ok" << endl;

    cout << "testing reader clearHistory... " << flush;
    {
        writers.update(false); // Not ready
        WriterConfig config;
        config.clearHistory = ClearHistoryPolicy::Never;
        auto writer = makeSingleKeyWriter(topic, "elem1", config);
        writer.add("value1");
        writer.update("value2");
        writer.update<string>("concat", "1");
        writer.remove();
        writer.add("value3");
        writer.update("value");
        writer.update<string>("concat", "1");
        writer.update<string>("concat", "2");
        writer.update<string>("concat", "3");
        writers.update(true); // Ready

        while(!readers.getNextUnread().getValue()); // Wait for reader to be done
    }
    cout << "ok" << endl;

    return 0;
}
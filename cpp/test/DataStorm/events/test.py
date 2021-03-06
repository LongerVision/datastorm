# **********************************************************************
#
# Copyright (c) ZeroC, Inc. All rights reserved.
#
# **********************************************************************

traceProps = {
    "DataStorm.Trace.Topic" : 1,
    "DataStorm.Trace.Session" : 3,
    "DataStorm.Trace.Data" : 2,
    "Ice.Trace.Protocol" : 2,
    "Ice.Trace.Network" : 2
}

TestSuite(__file__, [ ClientServerTestCase(traceProps=traceProps) ])

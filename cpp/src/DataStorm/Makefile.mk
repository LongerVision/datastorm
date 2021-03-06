# **********************************************************************
#
# Copyright (c) ZeroC, Inc. All rights reserved.
#
# **********************************************************************

$(project)_libraries    = DataStorm

$(project)_generated_includedir := $(project)/generated/DataStorm

DataStorm_sliceflags    := -I$(ice_slicedir) --include-dir DataStorm -I$(slicedir)
DataStorm_targetdir     := $(libdir)
DataStorm_cppflags      := -DDATASTORM_API_EXPORTS -DICE_CPP11_MAPPING
DataStorm_dependencies  := Ice++11

projects += $(project)

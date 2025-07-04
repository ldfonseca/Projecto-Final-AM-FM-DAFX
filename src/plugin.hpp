#pragma once
#include <rack.hpp>


using namespace rack;

// Declare the Plugin, defined in plugin.cpp
extern Plugin* pluginInstance;

// Declare each Model, defined in each module source file
// extern Model* modelMyModule;
extern Model* modelAMFMAnalyzer;
extern Model* modelHybridIfer;
extern Model* modelWahIfer;
extern Model* modelOctIfer;
extern Model* modelChorIfer;


///////////////////////////
// This is a helper library for Plug-n-Script
// for more convenient way of adding
// input/output params and input/output strings
// for Native (C++) version
// see examples of usage in AngelScript version (add_io_params.as)
///////////////////////////
// written by Ilya Orlov (LetiMix) | ilya2k@gmail.com
// 2020 January-April | St. Petersburg, Russia
///////////////////////////

#include <string>
#include <vector>
#include <cstring>
#include "math.h"

DSP_EXPORT uint    audioInputsCount=0;
DSP_EXPORT uint    audioOutputsCount=0;
DSP_EXPORT uint    auxAudioInputsCount=0;
DSP_EXPORT uint    auxAudioOutputsCount=0;
DSP_EXPORT int     maxBlockSize=0;
DSP_EXPORT double  sampleRate=0;
DSP_EXPORT string  userDocumentsPath=null;
DSP_EXPORT string  scriptFilePath=null;
DSP_EXPORT void*   host=null;
DSP_EXPORT HostPrintFunc* hostPrint=null;

DSP_EXPORT array<double> inputParameters(0);
DSP_EXPORT array<string> inputParametersNames(0);
DSP_EXPORT array<string> inputParametersUnits(0);
DSP_EXPORT array<double> inputParametersMin(0);
DSP_EXPORT array<double> inputParametersMax(0);
DSP_EXPORT array<double> inputParametersDefault(0);
DSP_EXPORT array<string> inputParametersFormats(0);
DSP_EXPORT array<string> inputParametersEnums(0);
DSP_EXPORT array<int>    inputParametersSteps(0);

DSP_EXPORT array<string> inputStrings(0);
DSP_EXPORT array<string> inputStringsNames(0);

DSP_EXPORT array<double> outputParameters(0);
DSP_EXPORT array<string> outputParametersNames(0);
DSP_EXPORT array<string> outputParametersUnits(0);
DSP_EXPORT array<double> outputParametersMin(0);
DSP_EXPORT array<double> outputParametersMax(0);
DSP_EXPORT array<double> outputParametersDefault(0);
DSP_EXPORT array<string> outputParametersFormats(0);
DSP_EXPORT array<string> outputParametersEnums(0);

DSP_EXPORT array<string> outputStrings(0);
DSP_EXPORT array<string> outputStringsNames(0);
DSP_EXPORT array<int>    outputStringsMaxLengths(0);

// shortcuts for input/output parameters and strings
array<double>& IP = inputParameters;
array<double>& OP = outputParameters;
array<string>& IPS = inputStrings;
array<string>& OPS = outputStrings;

// split std::string by delimiter
std::vector<std::string> split(std::string stringToBeSplitted, std::string delimeter) {
    std::vector<std::string> splittedString;
    unsigned int startIndex = 0;
    unsigned int endIndex = 0;
    while( (endIndex = stringToBeSplitted.find(delimeter, startIndex)) < stringToBeSplitted.size() )
    {
        std::string val = stringToBeSplitted.substr(startIndex, endIndex - startIndex);
        splittedString.push_back(val);
        startIndex = endIndex + delimeter.size();
    }
    if(startIndex < stringToBeSplitted.size()) {
        std::string val = stringToBeSplitted.substr(startIndex);
        splittedString.push_back(val);
    }
    return splittedString;
}

// add (or update) input parameter
void ip(int index, string name, string units, double min, double max = 0, double def_no = 0,  string vf = ".1", int steps = 0, string enums = "") {
    if (index < 0) index = int(inputParameters.length); // auto-increase index
    uint nn = index+1;
    // verify arrays size
    if (inputParameters.length < nn) {
        inputParameters.resize(nn);
        inputParametersNames.resize(nn);
        inputParametersUnits.resize(nn);
        inputParametersMin.resize(nn);
        inputParametersMax.resize(nn);
        inputParametersDefault.resize(nn);
        inputParametersFormats.resize(nn);
        inputParametersSteps.resize(nn);
        inputParametersEnums.resize(nn);
    }
    // add param name (name)
    inputParametersNames[index] = name;
    inputParametersUnits[index] = units;
    inputParametersMin[index] = min;
    inputParametersMax[index] = max;
    inputParametersDefault[index] = def_no;
    inputParametersFormats[index] = vf;
    // auto calculation of steps if value_format = .0
    if ((steps < 1) and ((std::strtof(vf, NULL) - floor(std::strtof(vf, NULL))) == 0)) { 
        steps = int(max - min) + 1;
    }
    inputParametersSteps[index] = steps;
    inputParametersEnums[index] = enums;
}

// add or update enumerated input parameter
void ip(int index, string name, string enums = "", string def_value = ""){

    // check if we're adding enums
    if(strstr(enums, ";") != NULL) {
        std::string str_enums = enums;
        std::string str_def_value = def_value;
        std::vector<std::string> vals = split(str_enums, ";");
        size_t max = vals.size() - 1;
        double def_no = 0;
        for(uint i=0;i<=max;i++) { if (vals[i] == def_value) { def_no = (double)i; } }
        ip(index, name, "", 0, (double)max, def_no, "", (double)max+1, enums);
    } else {
        // add regular param
        string units = enums;
        ip(index, name, units, 0.0);
    }
}

// the same but adding item to the end of list and returning index
int ip(string name, string units, double min, double max = 0, double def_no = 0,  string vf = ".1", int steps = 0, string enums = ""){
    int index = (int)inputParameters.length;
    ip(index, name, units, min, max, def_no, vf, steps, enums);
    return index;
}
// adding enum param to end of list and returning index
int ip(string name, string enums = "%", string def_value = ""){
    int index = (int)inputParameters.length;
    ip(index, name, enums, def_value);
    return index;
}

// OUTPUT PARAMETERS

// add (or update) output parameter
void op(int index, string name, string units, double min, double max = 0, double def_no = 0,  string vf = ".1", string enums = "") {
    uint nn = index+1;
    // verify arrays size
    if (outputParameters.length < nn) {
        outputParameters.resize(nn);
        outputParametersNames.resize(nn);
        outputParametersUnits.resize(nn);
        outputParametersMin.resize(nn);
        outputParametersMax.resize(nn);
        outputParametersDefault.resize(nn);
        outputParametersFormats.resize(nn);
        outputParametersEnums.resize(nn);
    }
    // add param name (name)
    outputParametersNames[index] = name;
    outputParametersUnits[index] = units;
    outputParametersMin[index] = min;
    outputParametersMax[index] = max;
    outputParametersDefault[index] = def_no;
    outputParametersFormats[index] = vf;
    outputParametersEnums[index] = enums;
}

// add or update enumerated output parameter
void op(int index, string name, string enums = "%", string def_value = ""){
    // check if we're adding enums
    if(strstr(enums, ";") != NULL) {
        std::string str_enums = enums;
        std::string str_def_value = def_value;
        std::vector<std::string> vals = split(str_enums, ";");
        size_t max = vals.size() - 1;
        double def_no = 0;
        for(uint i=0;i<=max;i++) { if (vals[i] == def_value) { def_no = (double)i; } }
        // add enum
        op(index, name, "", 0, max, def_no, "", enums);
    } else {
        // add regular param
        string units = enums;
        op(index, name, units, 0.0);
    }
}

// the same but adding item to the end of list and returning index
int op(string name, string units, double min, double max = 0, double def_no = 0,  string vf = ".1", string enums = ""){
    int index = int(outputParameters.length);
    op(index, name, units, min, max, def_no, vf, enums);
    return index;
}
// adding enum param to end of list and returning index
int op(string name, string enums = "%", string def_value = ""){
    int index = int(outputParameters.length);
    op(index, name, enums, def_value);
    return index;
}

// INPUT STRINGS

// add/update input string via index
void ips(int index, string name) {
    uint nn = index+1;
    // verify arrays size
    if (inputStrings.length < nn) {
        inputStrings.resize(nn);
        inputStringsNames.resize(nn);
    }
    inputStringsNames[index] = name;
}

// adding input string to the end of list and returning index
int ips(string name){
    int index = int(inputStrings.length);
    ips(index, name);
    return index;
}

// OUTPUT STRINGS

// add/update output string via index
void ops(int index, string name, int maxlen = 1024) {
    uint nn = index+1;
    // verify arrays size
    if (outputStrings.length < nn) {
        outputStrings.resize(nn);
        outputStringsNames.resize(nn);
        outputStringsMaxLengths.resize(nn);
    }
    outputStringsNames[index] = name;
    outputStringsMaxLengths[index] = maxlen;
}

// adding output string to the end of list and returning index
int ops(string name, int len = 1024){
    int index = int(outputStrings.length);
    ops(index, name);
    return index;
}
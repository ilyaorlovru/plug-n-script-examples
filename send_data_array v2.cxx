/*
This is a demo script for Blue Cat Audio Plug-N-Script.
It shows how to send a lot of data from .cxx to .kuiml file.

P-n-S v3.1 gives us only 32 output parameters, so if we want to send a lot of
data to user interface (KUIML), for example for showing complex curves or bargraphs,
we have to cut big arrays of data into smaller pieces.

In this script data is sent in chunks of 16 values in each.
First 17 output params is used for that purpose (first param is for data control).
KUIML received this chunk of data and saves it in it's own array and replies using
inputStrings, so we know that chunk of data was received. Then we send next chunk.

This is version 2 of the original script that uses "data packing", 
meaning in every sent value we pack 3 values (losing some resolution).
We use 10 bits for every packed value, so the resolution depends on 
range of MIN-MAX values you plan to use. I recommend range of 0-102.3 
(because it converts good to values 0-1023 of 10bit resolution).

This data sending is a bit more complex to implement, but it helps us increase speed x3.

For faster updates you can edit Plug-n-Script skin file.
Find "../BC Plug'n Script ... data/Skins/default.xml"
Make a copy of it (or edit directly) and edit second line:
<SKIN ... refresh_priority="normal" refresh_time_ms="10">

After selecting this skin in plugin you'll have much faster user interface updates.

------
Written by Ilya Orlov
ilya2k@gmail.com
January 2019, St. Petersburg, Russia
*/

string name="Send Data Array To KUIML v2";

// size of output data array (sent to KUIML)
const int OUT_DATA_SIZE = 96; // set multiple of 3 if ENABLE_DATA_PACKING = true

// if we want to pack data (3 values into one)
const bool ENABLE_DATA_PACKING = true; // true or false, set same in KUIML

// range between MAX and MIN value for output data
// if data packing is used, better set range values according to bitdepth 
// (default 10 bits per value, so real values will be converted to values 0-1023)
const double MAX_VALUE = 0; // for output data (set the same in KUIML)
const double MIN_VALUE = -102.3; // for output data (set the same in KUIML)


// INPUT PARAMETERS
/*
const int IP_SOME_PARAM = 0;
array<string> inputParametersNames={"Some parameter"};
array<string> inputParametersUnits={""};
//array<string> inputParametersEnums={"Off;On"};
array<string> inputParametersFormats={".0"};
array<double> inputParametersMin={0};
array<double> inputParametersMax={1}; 
// array<int> inputParametersSteps={0};
array<double> inputParametersDefault={0}; 
array<double> inputParameters(inputParametersNames.length); 
*/

// INPUT STRINGS used to receive data from KUIML
array<string> inputStrings(1);
array<string> inputStringsNames={"IN Data Control"};

// OUTPUT PARAMETERS
// THERE ARE 32 OUTPUT PARAMS IN PLUG-N-SCRIPT
// WE USE first 17 for sending array data:
// first out param is DATA CONTROL
// then 16 params for data values 
// the other 15 output parameters are free to use

// values used as max and min for data sending
const double P_MIN = (ENABLE_DATA_PACKING) ? 0 : MIN_VALUE;
const double P_MAX = (ENABLE_DATA_PACKING) ? pow(2,26) : MAX_VALUE;


array<string> outputParametersNames={
	"DControl","D1","D2","D3","D4","D5","D6","D7","D8","D9","D10","D11","D12","D13","D14","D15","D16", 
	"Transfer speed",};
array<string> outputParametersFormats={".5",".2",".2",".2",".2",".2",".2",".2",".2",".2",".2",".2",".2",".2",".2",".2",".2",
	".0"};
array<double> outputParametersMin={0,P_MIN,P_MIN,P_MIN,P_MIN,P_MIN,P_MIN,P_MIN,P_MIN,P_MIN,P_MIN,P_MIN,P_MIN,P_MIN,P_MIN,P_MIN,P_MIN,
    -1};
array<double> outputParametersMax={OUT_DATA_SIZE,P_MAX,P_MAX,P_MAX,P_MAX,P_MAX,P_MAX,P_MAX,P_MAX,P_MAX,P_MAX,P_MAX,P_MAX,P_MAX,P_MAX,P_MAX,P_MAX,
    100000};
array<string> outputParametersUnits={"","","","","","","","","","","","","","","","","",
	"values/s"};
array<double> outputParameters(outputParametersNames.length);

// names of output parameters (for easy access)
const int OP_DATACONTROL = 0;
const int OP_DATA_START = 1;
const int OP_DATA_END = 16;
const int OP_DATA_LEN = OP_DATA_END-OP_DATA_START+1;
const int OP_TRANSFER_SPEED = 17;

// output data (array of values to send to KUIML)
array<double> outData(OUT_DATA_SIZE);
int outdata_cur_start_index = 0; // current start index of data to send

// other script vars
bool mute_output = false;
int values_sent = 0, samples_counter = 0;


void processSample(array<double>& ioSample) {
    
    // counting samples
    samples_counter++;

    // if one second has passed
    if (samples_counter >= sampleRate) {
        // showing how many data was sent
        print("TRANSFER SPEED: " + values_sent + " value/s, " + round((values_sent+0.0)/OUT_DATA_SIZE,2) + " arrays/s, " + round((values_sent/(ENABLE_DATA_PACKING ? 3 : 1))/OP_DATA_LEN,2) + " transaction/s");
        outputParameters[OP_TRANSFER_SPEED] = values_sent;
        values_sent = 0;
        samples_counter = 0;
    }

	// mute output if needed
	if (mute_output) {
	    for(uint channel=0;channel<audioInputsCount;channel++) { 
	        ioSample[channel]=0; 
	    }
	}
}

int data_control_from_kuiml = 0;
int old_data_control_from_kuiml = -1;
int no_data_control_changes = 0;

void updateInputParameters() {
    data_control_from_kuiml = parseInt(inputStrings[0]);
    // print("kuiml in: " + data_control_from_kuiml);
    // sending output data
    // sendOutDataScheduler();
}


void computeOutputData() {
    // sending output data
    sendOutDataScheduler();
}

// function is a scheduler
// sending 'outData' array in pieces via sendDataChunk function
void sendOutDataScheduler(){
    // flag - should we send next piece of data
    bool go_to_next_chunk = false;
    // print("received "+ data_control_from_kuiml + " cur:" + outdata_cur_start_index);

    // check if last piece of data was received by KUIML
    if (data_control_from_kuiml == outdata_cur_start_index) {
        // print("received "+ data_control_from_kuiml);
        go_to_next_chunk = true;
    }

    // increase start_index and send next chunk
    if (go_to_next_chunk) {
        // print("goto next chunk");
        if (ENABLE_DATA_PACKING) {
            outdata_cur_start_index += OP_DATA_LEN*3;
            // if we reached end of data array
            if (outdata_cur_start_index >= (OUT_DATA_SIZE-2)) {
                outdata_cur_start_index = 0;
                fillOutDataWithNewData();
            }
        } else {
            outdata_cur_start_index += OP_DATA_LEN;
            // if we reached end of data array
            if (outdata_cur_start_index >= OUT_DATA_SIZE) {
                outdata_cur_start_index = 0;
                fillOutDataWithNewData();
            }
        }
        
        // send current piece of data
        sendDataChunk(outData, outdata_cur_start_index);
    }
    
}

// packing data parameters
const int pack_bitsperval = 10;
const int pack_bitmask = int(pow(2,pack_bitsperval))-1;
const double pack_range = (MAX_VALUE-MIN_VALUE);
const double pack_coeff = pack_bitmask/pack_range;

// pack three values into one double
double pack3(double a, double b, double c){
    int64 packed = 0;
    int inta = int( (a+pack_range)*pack_coeff +.5);
    int intb = int( (b+pack_range)*pack_coeff +.5);
    int intc = int( (c+pack_range)*pack_coeff +.5);
    // print("a: " + inta + " b " + intb + " c: " + intc + " packed: " + packed);
    if (inta > pack_bitmask) inta = pack_bitmask; // limit max_value
    if (intb > pack_bitmask) intb = pack_bitmask;
    if (intc > pack_bitmask) intc = pack_bitmask;
    packed = ((inta & pack_bitmask) << pack_bitsperval*2) + ((intb & pack_bitmask) << pack_bitsperval) + (intc & pack_bitmask);
    // print("a: " + inta + " b " + intb + " c: " + intc + " packed: " + packed);
    return packed/100.0;
}

// function sends a fragment of array data
double data_control_tiny_offset = 0;
void sendDataChunk(array<double>& data, int start_index){
    int chunk_len = OP_DATA_LEN;
    int i_inc = 1;
    if (ENABLE_DATA_PACKING) {
        chunk_len = OP_DATA_LEN*3; // if packing x3
        i_inc = 3;
    }
    int out_index = OP_DATA_START;
    int data_max_index = int(data.length)-1;

    
    for (int i=start_index; i<start_index+chunk_len; i+=i_inc){
        if (i>data_max_index) break; // keep within data array range
        double val = data[i];
        // print("i: "+ i + " out_index:" + out_index + " val: " +val);
        if (ENABLE_DATA_PACKING) {
            if ((i+2)>data_max_index) break;
            val = pack3(data[i], data[i+1], data[i+2]);
            // print("i: "+ i + " out_index:" + out_index + " vals: " +data[i] + ", " +data[i+1]+","+ +data[i+2]);
        } 
        outputParameters[out_index]=val;
        
        out_index++;
        values_sent+=i_inc;
    }
    outputParameters[OP_DATACONTROL] = start_index+data_control_tiny_offset;
    
     // data offset is needed for KUIMN
    data_control_tiny_offset += 0.0001;
    if (data_control_tiny_offset > 0.01) data_control_tiny_offset = 0;
}

// demo function to fill array with random data
double demo_offset = 0;
void fillOutDataWithNewData(){
    double val;
    // val = rand(MIN_VALUE, MAX_VALUE); // one value for whole array
    // fill array with random data
    double half_range = (MAX_VALUE-MIN_VALUE)/2;
    for(int i=0; i<OUT_DATA_SIZE; i++){
        // val = rand(MIN_VALUE, MAX_VALUE); // or rand for each element
        val =  sin(demo_offset/3.14)*cos((i+demo_offset)/3.14)*half_range-half_range;
        outData[i] = val;
    }
    demo_offset += 3.14/2;
}

///////////////////////////////////
// HELPER FUNCTIONS
///////////////////////////////////

int roundDoubleToInt(double d)
{
 if(d<0)
     return int(d-.5);
 else
     return int(d+.5);
}

double round(double d, double p = 2) {
    double x = pow(10, p);
    double r = floor(d*x+0.5)/x ;
    //print("x: " + x + ", r: " + r);
    return r;
}

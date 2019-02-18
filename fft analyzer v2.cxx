string name="Simple Linear FFT Analyzer v2";

// size of output data array (sent to KUIML)
const int OUT_DATA_SIZE = 96; // set multiple of 3 if ENABLE_DATA_PACKING = true

// if we want to pack data (3 values into one)
const bool ENABLE_DATA_PACKING = true; // true or false, set same in KUIML

// range between MAX and MIN value for output data
// if data packing is used, better set range values according to bitdepth 
// (default 10 bits per value, so real values will be converted to values 0-1023)
const double MAX_VALUE = 0; // for output data (set the same in KUIML)
const double MIN_VALUE = -102.3; // for output data (set the same in KUIML)

// constants
const double PI = 3.141592653589793238462;

// FFT settings
const int N_TAPS = 96*2; // number of samples in FFT window
const int N_TAPS_HALF = N_TAPS/2; // this is how many freq bands we'll get

// INPUT PARAMETERS
const int IP_USE_WINDOW = 0;
array<string> inputParametersNames={"Use window"};
array<string> inputParametersUnits={""};
//array<string> inputParametersEnums={"Off;On"};
array<string> inputParametersFormats={".0"};
array<double> inputParametersMin={0};
array<double> inputParametersMax={1}; 
// array<int> inputParametersSteps={0};
array<double> inputParametersDefault={0}; 
array<double> inputParameters(inputParametersNames.length); 

// INPUT STRINGS used to transfer data from KUIML
array<string> inputStrings(1);
array<string> inputStringsNames={"Data Control"};

// OUTPUT PARAMETERS
// THERE ARE 32 OUTPUT PARAMS IN PLUG-N-SCRIPT
// WE USE first 17 for sending array data:
// 16 params for data values, and one more for data flow control
// the other 15 output parameters are free to use

// values used as max and min for data sending
const double P_MIN = (ENABLE_DATA_PACKING) ? 0 : MIN_VALUE;
const double P_MAX = (ENABLE_DATA_PACKING) ? pow(2,26) : MAX_VALUE;


array<string> outputParametersNames={
	"D1","D2","D3","D4","D5","D6","D7","D8","D9","D10","D11","D12","D13","D14","D15","D16","DControl",
	"SampleRate", "Domi Freq", "Domi Freq Level"};
array<string> outputParametersFormats={".2",".2",".2",".2",".2",".2",".2",".2",".2",".2",".2",".2",".2",".2",".2",".2",".5",
	".0", ".0", ".2"};
array<double> outputParametersMin={P_MIN,P_MIN,P_MIN,P_MIN,P_MIN,P_MIN,P_MIN,P_MIN,P_MIN,P_MIN,P_MIN,P_MIN,P_MIN,P_MIN,P_MIN,P_MIN,0,
    -1, 0, -300};
array<double> outputParametersMax={P_MAX,P_MAX,P_MAX,P_MAX,P_MAX,P_MAX,P_MAX,P_MAX,P_MAX,P_MAX,P_MAX,P_MAX,P_MAX,P_MAX,P_MAX,P_MAX,pow(2,26),
    100000, 96000, 24};
array<string> outputParametersUnits={"","","","","","","","","","","","","","","","","",
	"", "Hz", "dB"};
array<double> outputParameters(outputParametersNames.length);

// names of output parameters (for easy access)
const int OP_DATA_START = 0;
const int OP_DATA_END = 15;
const int OP_DATA_LEN = OP_DATA_END-OP_DATA_START+1;
const int OP_DATACONTROL = 16;
const int OP_SAMPLERATE = 17;
const int OP_DOMI_FREQ = 18;
const int OP_DOMI_FREQ_LEVEL = 19;

// misc script vars
bool mute_output = true;
int values_sent = 0, samples_counter = 0, one_s_samples_counter = 0;
int cnt_full_ffts_done, cnt_updateInputParameters; // debug counters

bool initialize() {

    // adding elements to the output params arrays 
    // to send frequency level values to KUIML
    outputParameters[OP_SAMPLERATE] = sampleRate;

    // get window coefficients
    window = getWindowCoeffs();
    return true;
}

void processSample(array<double>& ioSample) {

	// counting samples
	samples_counter++;
    one_s_samples_counter++;

    // if one second has passed
    if (one_s_samples_counter >= sampleRate) {
        
        // print some debugging (speed) info
        // print("arraySent: " + cnt_arraysSent + 
        //   ", dataScheduler called: " + cnt_sendOutDataScheduler +
        //    ", updateInParams called: " + cnt_updateInputParameters + 
        //    ", FFTs done: " + cnt_full_ffts_done);
        
        cnt_sendOutDataScheduler = 0;
        cnt_updateInputParameters = 0;
        cnt_arraysSent = 0;
        cnt_full_ffts_done = 0;
        one_s_samples_counter = 0;
    }

	// take sample
	double sample = ioSample[0];

	// add sample to array (buffer)
	inputSamples.removeAt(0); // remove first value
    inputSamples.insertLast(sample); // insert as last value

    // if we've got enough samples
    if (samples_counter == N_TAPS) {
        if (inputParameters[IP_USE_WINDOW] > 0.5) {
            apply_window(); // apply window function
        }
    	do_fast_fft(); // calculate FFT
        cnt_full_ffts_done++; // calculate for debug

    	// let's set samples_counter to negative value
    	// to calculate FFT not so often (say ~every 1/2 of a second)
    	samples_counter = -int(sampleRate/15);
    }

	// mute output if needed
	if (mute_output) {
	    for(uint channel=0;channel<audioInputsCount;channel++) { 
	        ioSample[channel]=0; 
	    }
	}
}


// receiving input parameter changes
void updateInputParameters() {
    cnt_updateInputParameters++; // counter for debug
}


// sending output data (on each buffer)
void computeOutputData() {
    // sending output data
    sendOutDataScheduler();
}

//////////////////////////////
// SENDING DATA TO KUIML FUNCTIONS
//////////////////////////////

//////////////////////////////
// SENDING DATA TO KUIML
//////////////////////////////

// output data (array of values to send to KUIML)
array<double> outData(OUT_DATA_SIZE);
int outdata_cur_start_index = 0; // current start index of data to send
bool update_data_control = false; // after which number of samples to update (to let other data be received)
double data_control = 0;

// counters for debugging
int cnt_sendOutDataScheduler = 0;
int cnt_arraysSent = 0;

// function is a scheduler
// sending 'outData' array in pieces via sendDataChunk function
void sendOutDataScheduler(){
    cnt_sendOutDataScheduler++; // count how many times was called

    // receive data from KUIML
    int data_control_from_kuiml = parseInt(inputStrings[0]);

    // print("from KUIML "+ data_control_from_kuiml + " cur:" + outdata_cur_start_index);   

    // if we need to update data_control value (used for sending outData)
    if (update_data_control) {
        outputParameters[OP_DATACONTROL] = data_control;
        update_data_control = false;
        return;
    }

    bool go_to_next_chunk = false; // should we send next chunk of data
    // check if previous sent chunk was received by KUIML
    if (data_control_from_kuiml == outdata_cur_start_index) {
        go_to_next_chunk = true; // ok we can send next one
    }

    // increase start_index and send next chunk
    if (go_to_next_chunk) {

        // increase start_index and check if we're in array boundaries
        bool go_to_start_of_array = false;
        if (ENABLE_DATA_PACKING) {
            outdata_cur_start_index += OP_DATA_LEN*3;
            if (outdata_cur_start_index >= (OUT_DATA_SIZE-2)) go_to_start_of_array = true;
        } else {
            outdata_cur_start_index += OP_DATA_LEN;
            if (outdata_cur_start_index >= OUT_DATA_SIZE) go_to_start_of_array = true;
        }

        // if full array was sent, go back to start of array
        if (go_to_start_of_array) {
            outdata_cur_start_index = 0;
            // get new data to send from the buffer
            parseFFTResults();
            cnt_arraysSent++; // count how many arrays sent
        }
    }

    // send current piece of data
    sendDataChunk(outData, outdata_cur_start_index);
}

// function sends a fragment of array data
double data_control_tiny_offset = 0; // used in sendDataChunk
void sendDataChunk(array<double> &in data, int start_index){

    // print("send chunk from: " +start_index);

    int chunk_len = OP_DATA_LEN;
    int i_inc = 1;
    if (ENABLE_DATA_PACKING) {
        chunk_len = OP_DATA_LEN*3; // if packing x3
        i_inc = 3;
    }
    int out_index = OP_DATA_START;
    int data_max_index = int(data.length)-1;
    int data_checksum = 0;

    for (int i=start_index; i<start_index+chunk_len; i+=i_inc){
        if (i>data_max_index) break; // keep within data array range
        double val;
        if (ENABLE_DATA_PACKING) {
            // with data packing
            if ((i+2)>data_max_index) break;
            val = pack3(data[i], data[i+1], data[i+2]);
        } else {
            // no data packing
            val = data[i];
        }
        data_checksum = data_checksum ^ int(val);
        outputParameters[out_index]=val;
        
        out_index++;
    }

    // get data control value with checksum
    data_checksum = data_checksum & 255;
    data_control = int((data_checksum << 16) + (int(start_index) & 65535));
    
    //print("data_control:" + data_control);
    //print("data_checksum:" + data_checksum);

    // add tiny offset for datacontrol to trigger KUIML value_changed event always
    data_control += data_control_tiny_offset;
    data_control_tiny_offset += 0.0001; if (data_control_tiny_offset > 0.001) data_control_tiny_offset = 0;

    // we can send data control value with little delay,
    // so that data values are surely already received by KUIML
    // update_data_control = true;
    
    // or we can instead send data_control immediately, 
    // it works faster, but with possible glitches
    // uncomment this line and comment "update_data_control = true;"
    outputParameters[OP_DATACONTROL] = data_control;
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
    if (inta < 0) inta = 0; // limit min_value
    if (intb > pack_bitmask) intb = pack_bitmask;
    if (intb < 0) intb = 0; // limit min_value
    if (intc > pack_bitmask) intc = pack_bitmask;
    if (intc < 0) intc = 0; // limit min_value
    packed = ((inta & pack_bitmask) << pack_bitsperval*2) + ((intb & pack_bitmask) << pack_bitsperval) + (intc & pack_bitmask);
    // print("a: " + inta + " b " + intb + " c: " + intc + " packed: " + packed);
    return packed/100.0;
}

///////////////////////////////////
// FFT FUNCTIONS
///////////////////////////////////

// arrays for FFT calculation
array<double> inputSamples(N_TAPS);
array<double> FFT_OUT_REAL(N_TAPS_HALF); // real part of FFT output
array<double> FFT_OUT_IMAG(N_TAPS_HALF); // imaginary part of FFT output
array<double> window(N_TAPS);

// forward FFT
// not optimized
// though we calculate only half of frequency domain array (because it's mirrored)
void do_fast_fft(){
	double curSample; // to keep current sample

    // Zero FFT_OUT_REAL[] and FFT_OUT_IMAG[] so they can be used as accumulators
	// (this is AngelScript way of doing that)
    FFT_OUT_REAL.resize(0); FFT_OUT_REAL.resize(N_TAPS_HALF);
    FFT_OUT_IMAG.resize(0); FFT_OUT_IMAG.resize(N_TAPS_HALF);

    // this is more traditional way for zeroing array (but it's slow in Angelscript)
    // for (int k = 0; k < N_TAPS_HALF; k++) { FFT_OUT_REAL[k] = 0; FFT_OUT_IMAG[k] = 0; }

    // Loop through each sample in the frequency domain 
    // we ignore second half of array, cause it's mirroring the first part
    for (int k = 0; k < N_TAPS_HALF; k++) {
        // Loop through each sample in the time domain
        for (int i = 0; i < N_TAPS; i++) {
        	curSample = inputSamples[i];
            FFT_OUT_REAL[k] += curSample * cos(2*PI*k*i/N_TAPS);
            FFT_OUT_IMAG[k] += -curSample * sin(2*PI*k*i/N_TAPS);
        }
    }
}

// output calculated data
double dominant_freq = 0, dominant_freq_level = -500; // keep dominant frequency info
void parseFFTResults(){
	double magnitude, rek, imk, k_freq, k_level_db = 0;
	double this_domi_freq_level = -500, this_domi_freq = 0;

	for (int k = 0; k < N_TAPS_HALF; k++) {	
    	rek = FFT_OUT_REAL[k];
    	imk = FFT_OUT_IMAG[k];
    	magnitude = sqrt((rek*rek) + (imk*imk)); // magnitude (level) of this freq bin
    	
    	k_level_db = round(20.0*log10(magnitude/N_TAPS_HALF), 3); // convert to level in db
    	outData[k] = k_level_db;

    	k_freq = k*((sampleRate/2)/N_TAPS_HALF); // what frequency it is
    	if (k_level_db > this_domi_freq_level) {
    		this_domi_freq_level = k_level_db;
    		this_domi_freq = k_freq;
    	}
    }

    // show dominant
    //if (dominant_freq != this_domi_freq) {
    //	print("freq "+dominant_freq + " level: " + dominant_freq_level);
    //}

    dominant_freq = this_domi_freq;
    dominant_freq_level = this_domi_freq_level;

    // output dominant frequency info
    outputParameters[OP_DOMI_FREQ] = dominant_freq;
    outputParameters[OP_DOMI_FREQ_LEVEL] = dominant_freq_level;
}

// function to apply window
void apply_window(){
    int N = N_TAPS;
    // normalize gain
    for (int x = 0; x < N; x++) {
        inputSamples[x] = inputSamples[x] * window[x];
    }
}


array<double> getWindowCoeffs(){
    int N = N_TAPS;
    array<double> win(N);

    // apply window
    for(int x = 0; x < N; x++) {
        double f = 0.42659-0.49656*cos(2*PI*x/(N-1))+0.076849*cos(4*PI*x/(N-1)); // Blackmann window
        // double f = 0.35875-0.48829*cos(2*PI*x/(N-1))+0.14128*cos(4*PI*x/(N-1))-0.01168*cos(6*PI*x/(N-1)); // Blackmann-Harris window
        win[x] = f;
    }
    return win;
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

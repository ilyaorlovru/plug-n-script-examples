string name="Simple FFT Analyzer";

// predefined constatns
const int NUM_OF_BARS = 31; // 31 bars max (set the same value in KUIML)

const double MAX_LEVEL = 0;
const double MIN_LEVEL = -90; // in db
const double PI = 3.141592653589793238462;

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

// OUTPUT PARAMETERS
// array for output parameters (we send levels of frequencies this way)
array<string> outputParametersNames={
    "L1","L2","L3","L4","L5","L6","L7","L8","L9","L10",
    "L11","L12","L13","L14","L15","L16","L17","L18","L19","L20",
    "L21","L22","L23","L24","L25","L26","L27","L28","L29","L30","L31"
    "SampleRate",};
array<string> outputParametersFormats={
    ".1",".1",".1",".1",".1",".1",".1",".1",".1",".1",
    ".1",".1",".1",".1",".1",".1",".1",".1",".1",".1",
    ".1",".1",".1",".1",".1",".1",".1",".1",".1",".1",".1",
    ,".0"};
array<double> outputParametersMin={
    MIN_LEVEL,MIN_LEVEL,MIN_LEVEL,MIN_LEVEL,MIN_LEVEL,MIN_LEVEL,MIN_LEVEL,MIN_LEVEL,MIN_LEVEL,MIN_LEVEL,
    MIN_LEVEL,MIN_LEVEL,MIN_LEVEL,MIN_LEVEL,MIN_LEVEL,MIN_LEVEL,MIN_LEVEL,MIN_LEVEL,MIN_LEVEL,MIN_LEVEL,
    MIN_LEVEL,MIN_LEVEL,MIN_LEVEL,MIN_LEVEL,MIN_LEVEL,MIN_LEVEL,MIN_LEVEL,MIN_LEVEL,MIN_LEVEL,MIN_LEVEL,MIN_LEVEL,
    0
};
array<double> outputParametersMax={
    MAX_LEVEL,MAX_LEVEL,MAX_LEVEL,MAX_LEVEL,MAX_LEVEL,MAX_LEVEL,MAX_LEVEL,MAX_LEVEL,MAX_LEVEL,MAX_LEVEL,
    MAX_LEVEL,MAX_LEVEL,MAX_LEVEL,MAX_LEVEL,MAX_LEVEL,MAX_LEVEL,MAX_LEVEL,MAX_LEVEL,MAX_LEVEL,MAX_LEVEL,
    MAX_LEVEL,MAX_LEVEL,MAX_LEVEL,MAX_LEVEL,MAX_LEVEL,MAX_LEVEL,MAX_LEVEL,MAX_LEVEL,MAX_LEVEL,MAX_LEVEL,MAX_LEVEL,
    400000
};
array<string> outputParametersUnits={
    "dB","dB","dB","dB","dB","dB","dB","dB","dB","dB",
    "dB","dB","dB","dB","dB","dB","dB","dB","dB","dB",
    "dB","dB","dB","dB","dB","dB","dB","dB","dB","dB","dB",
    ""};
array<double> outputParameters(outputParametersNames.length);

// names of output parameters (for easy access)
const int OP_SAMPLERATE = 31; 

// FFT settings
const int N_TAPS = NUM_OF_BARS*2; // number of samples in FFT window
const int N_TAPS_HALF = N_TAPS/2; // because we'll use this often

// arrays for FFT calculation
array<double> inputSamples(N_TAPS);
array<double> FFT_OUT_REAL(N_TAPS_HALF); // real part of FFT out
array<double> FFT_OUT_IMAG(N_TAPS_HALF); // imaginary part of FFT out
array<double> window(N_TAPS);

// other script vars
bool mute_output = true;
bool output_freqs = false; // print list of frequencies

int samples_counter;
int initialOutParamsSize, outParamsSize; // for auto-changing output parameters

void initialize() {

    outputParameters[OP_SAMPLERATE] = sampleRate;
    // get window coefficients
    window = getWindowCoeffs();
}

void processSample(array<double>& ioSample) {

	samples_counter++;

	// take sample
	double sample = ioSample[0];

	// add to array
	inputSamples.removeAt(0); // remove first value
    inputSamples.insertLast(sample); // insert as last value

    // if we've got enough samples
    if (samples_counter == N_TAPS) {
        if (inputParameters[IP_USE_WINDOW] > 0.5) apply_window(); // apply window function
    	do_fast_fft(); // calculate FFT
    	output_fft(); // output FFT results

    	// let's set samples_counter to negative value
    	// to calculate FFT not so often (say ~every 1/4 of a second)
    	samples_counter = -int(sampleRate/4);
    }

	// mute output if needed
	if (mute_output) {
	    for(uint channel=0;channel<audioInputsCount;channel++) { 
	        ioSample[channel]=0; 
	    }
	}

    // output frequencies list once
    if (output_freqs) {
        double k_freq;
        string f_out = "";
        for (int k = 0; k < N_TAPS_HALF; k++) { 
            k_freq = k*((sampleRate/2)/N_TAPS_HALF);
            f_out += formatFloat(k_freq) + " )";
        }
        output_freqs = false;
        print(f_out);
    }
}


void updateInputParameters() {

}

void computeOutputData() {
	
}

/*
// this function is just for testing KUIML output
void setRandomValues(){
	double freq_level = 0;
	for(int n=0;n<NUM_OF_BARS;n++) { 
	    freq_level = rand(MIN_LEVEL, MAX_LEVEL);
	    outputParameters[n] = freq_level;
	}
}
*/

///////////////////////////////////
// FFT FUNCTIONS
///////////////////////////////////

// forward FFT
// not optimized
// though we calculate only half of frequency domain array (because it's mirrored)
void do_fast_fft()
{
	double curSample; // to keep current sample

    // Zero FFT_OUT_REAL[] and FFT_OUT_IMAG[] so they can be used as accumulators
	// (this is AngelScript way of doing that)
    FFT_OUT_REAL.resize(0); FFT_OUT_REAL.resize(N_TAPS_HALF);
    FFT_OUT_IMAG.resize(0); FFT_OUT_IMAG.resize(N_TAPS_HALF);

    // this is more traditional way for zeroing array (but it's slow in Angelscript)
    // for (int k = 0; k < N_TAPS_HALF; k++) { FFT_OUT_REAL[k] = 0; FFT_OUT_IMAG[k] = 0; }

    // Loop through each sample in the frequency domain 
    // we ignore second half of array, cause it's mirroring the first part
    for (int k = 0; k < N_TAPS_HALF; k++)
    {
        // Loop through each sample in the time domain
        for (int i = 0; i < N_TAPS; i++)
        {
        	curSample = inputSamples[i];
            FFT_OUT_REAL[k] += curSample * cos(2*PI*k*i/N_TAPS);
            FFT_OUT_IMAG[k] += -curSample * sin(2*PI*k*i/N_TAPS);
        }
    }
}

// output calculated data
void output_fft(){
	double magnitude, rek, imk, k_freq, k_level_db;

	for (int k = 0; k < N_TAPS_HALF; k++) {	
    	rek = FFT_OUT_REAL[k];
    	imk = FFT_OUT_IMAG[k];
    	magnitude = sqrt((rek*rek) + (imk*imk)); // magnitude (level) of this freq bin
    	// k_freq = k*((sampleRate/2)/N_TAPS_HALF); // what frequency it is
    	k_level_db = round(20.0*log10(magnitude/N_TAPS_HALF), 3); // convert to level in db
    	outputParameters[k] = k_level_db;
    }

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

//////////////////////////////////////
// Simple FIR filter/convolution implementation
// written by Ilya Orlov (LetiMix) 
// https://www.youtube.com/channel/UCbS0EpAZytfizVvysUluEHQ
// St. Petersburg, Russia, 2019
//////////////////////////////////////


////////////////////////////
// script name

string name="Simple FIR Filter";
string description="";

////////////////////////////
// output parameters

array<string> outputParametersNames={"Filter Length"};
array<string> outputParametersFormats={".0"};
array<double> outputParametersMin={0};
array<double> outputParametersMax={100000};
array<string> outputParametersUnits={"taps"};
array<double> outputParameters(outputParametersNames.length);

////////////////////////////
// script vars

// filter coefficents (convolution kernel)
// you can calculate this coefficients for example on http://fiiir.com
// or take a look at FIR Filter designer: http://www.iowahills.com/8DownloadPage.html

array<double> h = {
    0.000975174370376644,
    0.001173339216292626,
    -0.001050721658046899,
    -0.002433527427258345,
    0.001158484039366984,
    0.005092600348788601,
    -0.000442576987326160,
    -0.009386323514638258,
    -0.002311407428124617,
    0.015169063775292986,
    0.008687539136637796,
    -0.021893762569846247,
    -0.020968799003849899,
    0.028694113886943335,
    0.043775851733155541,
    -0.034553502648665933,
    -0.093008567369917278,
    0.038521538201391828,
    0.313231679997613033,
    0.459139607803628891,
    0.313231679997613033,
    0.038521538201391828,
    -0.093008567369917305,
    -0.034553502648665940,
    0.043775851733155541,
    0.028694113886943335,
    -0.020968799003849896,
    -0.021893762569846251,
    0.008687539136637798,
    0.015169063775292986,
    -0.002311407428124618,
    -0.009386323514638265,
    -0.000442576987326160,
    0.005092600348788601,
    0.001158484039366983,
    -0.002433527427258348,
    -0.001050721658046899,
    0.001173339216292625,
    0.000975174370376644
};


int N = h.length; // calculate size of filter
array<double> inputSamples(N); // create input buffer (delay line)

// convolution function
double convolve(array<double> &input_samples, array<double> &fir_coeffs){
    double outsample = 0;
    for(int i=0;i<N;i++) {
        outsample += input_samples[i]*fir_coeffs[i]; 
    }
    return outsample;
}

////////////////////////////
// script functions

// initialize
void initialize(){
    // show filter length in GUI
    outputParameters[0] = N;
}


// per-sample processing function
void processSample(array<double>& ioSample)
{
	// take sample
	double sample = ioSample[0]; // mono or left channel

	// add to buffer (delay line)
	inputSamples.removeLast();
    inputSamples.insertAt(0, sample);

    // convolve buffer
    double outsample = convolve(inputSamples, h);

    // output sample
    for(uint channel=0;channel<audioInputsCount;channel++) { 
        ioSample[channel]=outsample; 
    }
}


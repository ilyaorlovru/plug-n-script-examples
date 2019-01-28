//////////////////////////////////////
// KUIML Plot Graph by points
// written by Ilya Orlov (LetiMix) ilya2k@gmail.com
// https://www.youtube.com/channel/UCbS0EpAZytfizVvysUluEHQ
// St. Petersburg, Russia, 2019
// 
// The idea is to use FORMULA_CURVE element
// and create an appropriate formula using given points
// to draw the plot. Three versions are available:
// stepped graph, linear graph or smooth graph (with variable smoothness).
//
// Feel free to use in your projects.
//////////////////////////////////////

string name="Plot graph examples";

// array for output parameters (we can send plot points this way)
// TO MAKE THESE WORK, uncomment PARAM_LINK in kuiml

array<string> outputParametersNames={"y1", "y2"};
array<double> outputParametersMin={-10, -10};
array<double> outputParametersMax={10, 10};
array<double> outputParameters(outputParametersNames.length);

////////////////////////////
// script functions

// per-sample processing function
void processSample(array<double>& ioSample){
    for(uint channel=0;channel<audioInputsCount;channel++) { 
        ioSample[channel]=0; 
    }
}

void computeOutputData() {
	// send random values (for demo purposes)
	outputParameters[0] = rand(-1,0);
	outputParameters[1] = rand(2,3);
	// to view changes uncomment PARAM_LINK in kuiml
}
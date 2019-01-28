//////////////////////////////////////
// KUIML Formulas viewer v2
// written by Ilya Orlov (LetiMix) ilya2k@gmail.com
// https://www.youtube.com/channel/UCbS0EpAZytfizVvysUluEHQ
// St. Petersburg, Russia, 2019
//////////////////////////////////////

// THE MAIN SCRIPT IS IN KUIML FILE

////////////////////////////
// script name

string name="Formula viewer v2";

////////////////////////////
// script functions

// per-sample processing function
void processSample(array<double>& ioSample){
    // mute output
    for(uint channel=0;channel<audioInputsCount;channel++) { 
        ioSample[channel]=0; 
    }
}

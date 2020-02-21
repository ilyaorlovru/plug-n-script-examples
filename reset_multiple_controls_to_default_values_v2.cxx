string name="Reset controls v2";

array<string> inputParametersNames={"Gain In", "Mix", "Gain Out"};
array<double> inputParameters(inputParametersNames.length); 
array<double> inputParametersMin={-20, 0, -20};
array<double> inputParametersMax={20, 100, 20};
array<double> inputParametersDefault={4, 50, -6};

// UNITS

void initialize(){

}
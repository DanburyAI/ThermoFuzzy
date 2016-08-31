/*
	ThermoFuzzy - 	a simple fuzzy rule algorithm
		     	for managing a simple smart thermostat
		     	based on climactic differences between an indoor
		     	location and outdoor conditions.
			
			The thermostat controls an A/C unit	

	Platform   - 	TinyDuino or Arduino Uno

	==========================================================

	10 Jul 2014	Initial implementation
*/


const int status_pin = 13;	// status indicator LED

// ranges for outdoor conditions (temperature, humdity)
const int OutdoorTemp_rng[] = {68, 90};
const int OutdoorHum_rng[] = {20, 75};

// ranges for indoor conditions (temperature, humdity)
const int IndoorTemp_rng[] = {68, 90};
const int IndoorHum_rng[] = {20, 75};

// indoor temperature value range
const int IT_val[] 	= {30,  40,  50, 60,  70,  80,  90, 100};
// indoor temperature membership functions
const float IT_cold[] 	= { 1, 0.5,0.25,  0,   0,   0,   0,   0};
const float IT_mild[] 	= { 0,   0, 0.5,  1, 0.5,   0,   0,   0};
const float IT_hot[] 	= { 0,   0,   0,  0,0.25, 0.5,   1,   1};

// indoor humidity value range
const int IH_val[] 	= {10,  20,  30, 40,  50,  60,  70,  80};
// indoor temperature membership functions
const float IH_dry[] 	= { 1, 0.5,0.25,   0,   0,   0,   0,   0};
const float IH_mild[] 	= { 0,   0, 0.5,   1, 0.5,   0,   0,   0};
const float IH_muggy[] 	= { 0,   0,0.25, 0.5,   1,   1,   1,   1};

// outdoor temperature value range
const int OT_val[] 	= {30,  40,  50, 60,  70,  80,  90, 100};
// outdoor temperature membership functions
const float OT_cold[] 	= { 1, 0.5,0.25,  0,   0,   0,   0,   0};
const float OT_mild[] 	= { 0,   0, 0.5,  1, 0.5,   0,   0,   0};
const float OT_hot[] 	= { 0,   0,   0,  0,0.25, 0.5,   1,   1};

// indoor humidity value range
const int OH_val[] 	= {10,  20,  30, 40,  50,  60,  70,  80};
// indoor temperature membership functions
const float OH_dry[] 	= { 1, 0.5,0.25,   0,   0,   0,   0,   0};
const float OH_mild[] 	= { 0,   0, 0.5,   1, 0.5,   0,   0,   0};
const float OH_muggy[] 	= { 0,   0,0.25, 0.5,   1,   1,   1,   1};

// AC value range (cfm)
const int AC_val[] 	= { 0,   5,  10,  15,  20,  25,  30};
// indoor temperature membership functions
const float AC_idle[] 	= { 1, 0.5,0.25,   0,   0,   0,   0};
const float AC_low[] 	= { 0,0.25, 0.5,   1, 0.5,0.25,   0};
const float AC_medium[] = { 0,   0,0.25, 0.5,   1, 0.5,0.25};
const float AC_high[] 	= { 0,   0,   0,0.25, 0.5,   1,   1};

// rule set

/*
	NOTE

	For all temperature parameters,

		(abbreviation)		(description)

		     "C"		cold
		     "M"		mild
		     "H"		hot

	For all humidity parameters,

		(abbreviation)		(description)

		     "D"		dry
		     "M"		mild
		     "U"		muggy

	For the A/C setting,

		(abbreviation)		(description)

		     "I"		idle
		     "L"		low
		     "M"		medium
		     "H"		high

	In all cases, "X" means that the parameter is not used.

	Structure of rules:

	1. Each column is a rule. First 4 values across the column are the LHS.
	   Last (5th) value is the RHS.

	2. Each 'row' (parameter array) represents target fuzzy values, 
	   with each value representing the target comparison fuzzy value for that rule.
		      		
*/

const char IT[]	= {'M','H','H','M'};
const char IH[]	= {'M','U','M','U'};
const char OT[]	= {'X','X','X','C'};
const char OH[]	= {'X','X','X','M'};
const char AC[]	= {'I','H','M','I'};

// cummulative LHS membership value for each rule

float muLHS[] = {1,1,1,1};

// RHS centroid value for each rule

float vRHS[] = {-1,-1,-1,-1};

// current indoor and outdoor condition settings
int IT_curr;
int IH_curr;
int OT_curr;
int OH_curr;

// current AC setting
float AC_set;

int iter=0;	// iteration counter for the loop


void setup()
{
	int i,j;
	float mn;

	// initially host messaging
	Serial.begin(9600);
	while (!Serial);

	pinMode(status_pin, OUTPUT);	// set the status LED as output only

	randomSeed(analogRead(0));	// initialize the random number generator for condition readings

	// calculate the centroids for each rule
	for(i=0;i<sizeof(IT);i++)
	{
		vRHS[i] = -1; mn = 0;
		switch(AC[i])
		{
			case 'I':
				for(j=0;j<int(sizeof(AC_idle)/4);j++)
				{
					if(AC_idle[j] > mn) 
					{
						vRHS[i] = AC_val[j]; mn = AC_idle[j];
					}
				}
				break;
			case 'L':
				for(j=0;j<int(sizeof(AC_low)/4);j++)
				{
					if(AC_low[j] > mn) 
					{
						vRHS[i] = AC_val[j]; mn = AC_low[j];
					}
				}
				break;
			case 'M':
				for(j=0;j<int(sizeof(AC_medium)/4);j++)
				{
					if(AC_medium[j] > mn) 
					{
						vRHS[i] = AC_val[j]; mn = AC_medium[j];
					}
				}
				break;
			case 'H':
				for(j=0;j<int(sizeof(AC_high)/4);j++)
				{
					if(AC_high[j] > mn) 
					{
						vRHS[i] = AC_val[j]; mn = AC_high[j];
					}
				}
				break;
		}
	}

}

void loop()
{
	int i;
	
	digitalWrite(status_pin, HIGH);	 // turn status LED on

	Serial.println("_____________________________________________"); Serial.flush();

	// obtain input parameter readings
	IT_curr = int(random(IndoorTemp_rng[0],IndoorTemp_rng[1]));  	// indoor temperature
	IH_curr = int(random(IndoorHum_rng[0],IndoorHum_rng[1]));	// indoor humidity
	OT_curr = int(random(OutdoorTemp_rng[0],OutdoorTemp_rng[1]));  	// outdoor temperature
	OH_curr = int(random(OutdoorHum_rng[0],OutdoorHum_rng[1]));	// outdoor humidity
	
	Serial.print("Readings: ");
	Serial.print(IT_curr,DEC); Serial.print(", ");
	Serial.print(IH_curr,DEC); Serial.print(", ");
	Serial.print(OT_curr,DEC); Serial.print(", ");
	Serial.print(OH_curr,DEC); Serial.println("."); Serial.flush(); 

	// perform scan for each rule per step 1 of fuzzy inference

	rule_scan();

	Serial.println("LHS: ");
	for(i=0;i<int(sizeof(muLHS)/4);i++)
	{
		Serial.println(muLHS[i],DEC);
	}
	Serial.println(); Serial.flush(); 
	Serial.println("RHS: ");
	for(i=0;i<int(sizeof(vRHS)/4);i++)
	{
		Serial.println(vRHS[i],DEC);
	}
	Serial.println(); Serial.flush(); 

	// perform centroid calculation per step 2 of fuzzy inference

	calc_centroid();

	Serial.print("A/C setting: "); Serial.print(AC_set,DEC); 
	Serial.println("."); Serial.flush(); 

	delay(250);
	digitalWrite(status_pin, LOW);	 // turn status LED off

	// 5 second delay (mimic 1 min. wait)
	delay(5000);

}
		
void rule_scan()
{
	int i, j;
	float x_val;
	
	for(i=0;i<sizeof(IT);i++)
	{
		// calculate the minimum mu value for the LHS

		muLHS[i] = 1;
		if(IT[i] != 'X') 
		{
			x_val = calc_mu(IT_curr,'T',IT[i]); muLHS[i] = min(muLHS[i],x_val);
		}
		if(IH[i] != 'X')
		{
			x_val = calc_mu(IH_curr,'H',IH[i]); muLHS[i] = min(muLHS[i],x_val);
		}
		if(OT[i] != 'X')
		{
			x_val = calc_mu(OT_curr,'T',OT[i]); muLHS[i] = min(muLHS[i],x_val);
		}
		if(OH[i] != 'X')
		{
			x_val = calc_mu(OH_curr,'H',OH[i]); muLHS[i] = min(muLHS[i],x_val);
		}
	}

	return;
}

void calc_centroid()
{
	int i;
	float ttl1, ttl2;

	ttl1 = 0; ttl2 = 0; AC_set = 0;

	for(i=0;i<int(sizeof(muLHS)/4);i++) 
	{
		ttl1 = ttl1 + muLHS[i] * vRHS[i];
		ttl2 = ttl2 + muLHS[i];
	}

	if(ttl2 != 0) AC_set = float(ttl1 / ttl2);

	return;
}

float calc_mu(int val_in, char parm, char mu)
{
	int i, idx, min_val, max_val;
	float min_mu, max_mu, x_val;

	// select the appropriate mu range based on parameter type (temperature, humidity)
	// and the mu function selected (cold, mild, dry, etc.)
	switch(parm)
	{
		case 'T':
			for(i=0;i<=int(sizeof(IT_val)/2)-2 && val_in > IT_val[i]; i++);
			idx = i-1; min_val = IT_val[idx]; max_val = IT_val[idx+1]; 
			switch(mu)
			{
				case 'C':
					min_mu = IT_cold[idx]; max_mu = IT_cold[idx+1]; 
					break;
				case 'M':
					min_mu = IT_mild[idx]; max_mu = IT_mild[idx+1]; 
					break;
				case 'H':
					min_mu = IT_hot[idx]; max_mu = IT_hot[idx+1]; 
					break;
				
			}
			break;
		case 'H':
			for(i=0;i<=int(sizeof(IH_val)/2)-2 && val_in > IH_val[i]; i++);
			idx = i-1; min_val = IH_val[idx]; max_val = IH_val[idx+1]; 
			switch(mu)
			{
				case 'D':
					min_mu = IH_dry[idx]; max_mu = IH_dry[idx+1]; 
					break;
				case 'M':
					min_mu = IH_mild[idx]; max_mu = IH_mild[idx+1]; 
					break;
				case 'U':
					min_mu = IH_muggy[idx]; max_mu = IH_muggy[idx+1]; 
					break;
				
			}
			break;
	}
	// flip the end points of the selected mu range, if they are not monitonically increasing
	if(min_mu > max_mu)
	{
		x_val = min_mu; min_mu = max_mu; max_mu = x_val;
	}

	// calculate the interpolated mu value for this parameter and mu selection
	x_val = val_in - min_val; x_val /= max_val - min_val;
	x_val *= max_mu - min_mu; x_val += min_mu;	

	return x_val;
}


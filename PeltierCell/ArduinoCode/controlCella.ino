float Vref = 1.2; //voltage reference to follow (input to the whole controlled system)

int NTC1a_Pin = 0; //input pins
int NTC1b_Pin = 1;

float Ki = 0.22; //PID constants
float Kp = 5;

int PeltierVoltage_Pin = 4;
int load = 5;

int NTC2a_Pin = 2;
int NTC2b_Pin = 3;

float Vcc = 5; //wheatstone bridge voltage supply
float K = 1; //amplification gain
float RTmax = 321000; //maximum NTC resistance
float RL = 12400; //W Bridge upper resistors

//S-H models costants
float c1 = 0.7629865204e-03;
float c2 = 2.087671760e-04;
float c3 = 1.230852049e-07;

int V1a, V1b, V2a, V2b, buttonState = 0; //op_amp output voltages
float NTC_R1a, NTC_R2a; //NTC resistance values
float logNTC_R1a, logNTC_R2a; //logarithm values of NTC resistance values
float NTC_T1K, NTC_T2K, NTC_T1C, NTC_T2C; //temperatures from NTC
double PeltierVoltage;
float A = RTmax / (RL + RTmax); //constants
int inputState = 0;
int Ts = 250;
float duty_cycle;
float duty_cycle_prec = 0;

int temperature_threshold = 100;
int temperature_recovery_threshold = 80;
int cell_threshold = 0.01;


int recovery = 0;  //Automatic recovery state when we cross the max temperature threshold, so the system won't destroy itself


float error = 0;  //Usefull for the finite difference equation
float error_prec = 0;

unsigned long current_millis = 0; //time istant necessary to respect the sampling frequency of 4Hz
unsigned long previous_sample_millis = 0;

void setup() {
  Serial.begin(9600);
}

void loop() { //the effective main of the program
  current_millis = millis();
  controller();
}


#include <math.h>



float readTemperature(int analogPin)
{
  int V1 = analogRead(analogPin);
  int V2 = analogRead(analogPin + 1); // Assuming the adjacent pin is the second pin in the bridge

  float NTC_R = (RL * (K * Vcc * A - ((float)(V1 - V2)) * 0.004887585)) /
                (((float)(V1 - V2) * 0.004887585 + K * Vcc * (1 - A))); //thermocouple

  float logNTC_R = log(NTC_R);

  // Steno-Hart's law
  float NTC_TK = (1.0 / (c1 + c2 * logNTC_R + c3 * logNTC_R * logNTC_R * logNTC_R));

  float NTC_TC = NTC_TK - 273.15;

  return NTC_TC;
}

void controller() { //we used a PID to make the state depending on the error (the bigger the error -> the more the state must grow so I should heat more)
  if (current_millis - previous_sample_millis >= Ts) { //respecting the sampling time of 250ms
    int sensorValue = analogRead(A5);
    Vref = mapFloat(sensorValue, 0, 1023, 0, 1.8); // Mapping the voltage from [0,1023] to [0,1.8] (its taken through a potentiometer that can be rotated from 0 to 1023)
	//note that the function 'map' only works with integers so mapFloat has been implemented by us.
    previous_sample_millis = millis();
    NTC_T1C = readTemperature(NTC1a_Pin);
    NTC_T2C = readTemperature(NTC2a_Pin);
    PeltierVoltage = analogRead(PeltierVoltage_Pin) * 0.004887585;
    //Serial.print(NTC_T1C);
    //Serial.print(",");
    //Serial.print(NTC_T2C);
    //Serial.print(",");
    Serial.print(duty_cycle);
    Serial.print(",");
    Serial.print(PeltierVoltage, 4);
    Serial.print(",");
    Serial.println(Vref);

    if (NTC_T1C > temperature_threshold or NTC_T2C > temperature_threshold) {     //Entering recovery state when one of the 2 temperatures (normally the hot one) crosses the critical threshold value
      recovery = 1;
      duty_cycle = 0; //shutting down the termoresistor

    } else {
      if (NTC_T1C < temperature_recovery_threshold and NTC_T2C < temperature_recovery_threshold and recovery == 1) {  //Exiting recovery state when BOTH the temperatures are below the recovery threshold value (it's another threshold, far below the critical one, to avoid a continous switching between working state and recovery state) and if recovery state was active
        recovery = 0;
      }

      if (recovery == 0) { //if outside the recovery state, so with a temperature control, the feedback control can start
        error = Vref - PeltierVoltage;
        
		//in continous time: x'= x(t+1) - x(t) / deltaT             --->     x(t+1) = x(t) + deltaT* K*e   (x'=K*e where K is the PID parameter (just I, since x= K* integ(e))  and e is the error (vref-vPelt) ) 
		//we are supposing to use only integrator effect since x is the output of the PID and e is the input of the PID. So the output is the integral of the input
		//so the error enters the PID (just I without P and D) controller and Y is the output, then it enters a saturation function and it becomes the PWM value for duty cycle
		
		//from simulink the implemented PI (digital version) equation corresponds to d(z) = [Kp + Ki * Ts * 1/z-1 ] * E(z)   -->
		// --> d(z)= ( (z-1) * Kp + Ki * Ts ) * E(z) / (z-1)    --> d(z) * (z-1) = ( (z-1) * Kp + Ki * Ts ) * E(z)   -->   antitrasforming
		// --> d(t+1) - d(t) = ( e(t+1) - e(t) )* Kp + Ki *Ts *e(t) --> 
		// finally <<traslating in time>>  --> d(t) = d(t-1) + ( e(t+1) - e(t) )* Kp + Ki *Ts *e(t)
		
        duty_cycle = duty_cycle_prec + (error - error_prec) * Kp + Ki * (Ts / 1000.0) * error_prec;       //IMPORTANTISSIMO METTERE 1000.0 SENNO' NON FUNZIONA NULLA
		//prendiamo errore (ingresso del PID, che qui è solo un PI), lo moltiplichiamo per una costante e per un'altra costante moltiplichiamo il suo integrale (lui all'istante precedente, difatti una memoria di ciò che è accaduto finora). Questo sarà l'ingresso al PWM.
        error_prec = error;

        if (duty_cycle > 1)     //Saturation function (applied on the state, that is the output of the PID)
          duty_cycle = 1;
        else if (duty_cycle < 0)
          duty_cycle = 0;
      }
    }
    duty_cycle_prec = duty_cycle;

    analogWrite(load, mapFloat(duty_cycle, 0, 1, 0, 255)); //piloting the termoresistor (extruder) with the calculated duty cycle
  }
}


float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

//NOTA SUL RAGIONAMENTO DEL PROF
//Considerando la variabile di stato saturata tra 0 e 1, essa non può andare oltre 1 (duty cycle non più grande di 100%). Quindi quando duty_cycle si trova ad 1 e vorrà salire non andrà oltre 1 e potrà subito scendere in qualsiasi momento a 0.99
//Al contrario se non saturiamo lo stato ma solo l'uscita del PID, significa che se lo stato si trova a 100 l'uscita del PID sta ad 1 ma quando poi lo stato scenderà a 99 l'uscita resterà ad 1 e così sarà finché lo stato non andrà sotto 1. 
//Causando un grande ritardo nell'abbassamento dello stato (e quindi poi dell'uscita)
//Noi saturiamo lo stato cioè duty_cycle che è anche l'uscita del nostro PID, avremmo potuto implementare un'altra variabile che sarebbe stata l'uscita del PID e saturare quella ma avrebbe creato il suddetto problema
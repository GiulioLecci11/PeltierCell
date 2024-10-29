//LEGGERE INSIEME A MAIN.INO
// Misura della Temperatura con NTC
#include <math.h>

int NTC1a_Pin = 0; //input pins
int NTC1b_Pin = 1;

int PeltierVoltage_Pin = 4;
int button_Pin = 2;
int load = 5;
int cycle = 0;
int N_acquisition = 10;// Numero di cicli per cui eseguire la lettura dei dati

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

int V1a, V1b, V2a, V2b, buttonState = 0; //op_amp output voltages + buttonState flag
float NTC_R1a, NTC_R2a; //NTC resistance values
float logNTC_R1a, logNTC_R2a; //logarithm values of NTC resistance values
float NTC_T1K, NTC_T2K, NTC_T1C, NTC_T2C; //temperatures from NTC
double PeltierVoltage;
float A = RTmax / (RL + RTmax); //constants
int inputState = 0;
int Ts = 250;
int duty_cycle;

int temperature_threshold = 100;
int cell_threshold = 0.01;

//unsigned long Ncycle_heating = (1000 / Ts) * 35;       NON SI USA PERCHE' DA PROBLEMI DI OVERFLOW A CASO NON ESSENDO NEL SETUP

int acquisition_finished = 0;

unsigned long millis_to_heat = 65000;  //durata fase di riscaldamento
unsigned long current_millis = 0; //current timestamp with millis
unsigned long start_heating_millis = 0;
unsigned long previous_sample_millis = 0;



float readTemperature(int analogPin)
{
  int V1 = analogRead(analogPin);
  int V2 = analogRead(analogPin + 1); // Assuming the adjacent pin is the second pin in the bridge for the voltage

  float NTC_R = (RL * (K * Vcc * A - ((float)(V1 - V2)) * 0.004887585)) /
                (((float)(V1 - V2) * 0.004887585 + K * Vcc * (1 - A)));

  float logNTC_R = log(NTC_R);

  // Steno-Hart's law
  float NTC_TK = (1.0 / (c1 + c2 * logNTC_R + c3 * logNTC_R * logNTC_R * logNTC_R));

  float NTC_TC = NTC_TK - 273.15;

  return NTC_TC;
}

void readData() { //MILLIS ULTRA NECESSARY FOR SYNCHRONIZATION MANAGING
  if (current_millis - previous_sample_millis >= Ts) { //controlla se tra adesso e il campione preso prima c'è più di un tempo di campionamento e se è così devo prendere un altro campione 
    previous_sample_millis = millis();//millis ritorna quanto tempo passato da avvio programma quindi in questa riga ri-setto il tempo dell'ultima acquisizione
    NTC_T1C = readTemperature(NTC1a_Pin);
    NTC_T2C = readTemperature(NTC2a_Pin);
    PeltierVoltage = analogRead(PeltierVoltage_Pin) * 0.004887585;  //this constant transforms an analog read between 0 and 1023 in a voltage between 0 and 5V (each step between 0 and 1023 adds 0.004.. volts)
	                                                                //It's identical to use the map command
    Serial.print(NTC_T1C);
    Serial.print(",");
    Serial.print(NTC_T2C);
    Serial.print(",");
    Serial.print(PeltierVoltage, 4);
    Serial.print(",");
    Serial.print(inputState);
    //Serial.print(" || cycle is: ");
    //Serial.print(cycle);
    //Serial.print(" || Acquisition number: ");
    //Serial.print(i);
    Serial.println();
  }
}

void buttonInterrupt() {
  buttonState = 1;
}

//LEGGERE INSIEME A AUTOMATIC_ACQUISITION_MILLIS.INO
void setup()
{
  Serial.begin(9600);
  attachInterrupt(digitalPinToInterrupt(button_Pin), buttonInterrupt, RISING); //interrupt che parte ad ogni pressione del bottone e fa partire ButtonInterrupt
  pinMode(LED_BUILTIN, OUTPUT); //segna un pin come output
  digitalWrite(LED_BUILTIN, LOW); //spegne led perché iniziamo ad acquisire
  acquisition_finished = 0;
}

void loop()
{
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN)); //il pin ledbuiltin fa accendere il led nella scheda (questo blinka quando leggo dati fuori da un'acquisizione)
  current_millis = millis(); //inizia a contare il tempo
  readData();//funzione nell'altro file, la prima volta viene chiamata al primo ciclo
//praticamente facciamo 250 giri (250 ms) a vuoto prima di richiamre per la seconda volta readData 
  if (buttonState == 1 and inputState == 0 and PeltierVoltage <= cell_threshold and acquisition_finished == 0) { //se bottone premuto e riscaldamento spento e cella a tensione nulla e numero di acquisizioni non finite (se finite mettiamo flag acquisition_finished a 1) allora entriamo qua
    digitalWrite(LED_BUILTIN, LOW); //spengo led perché sto acquisendo
    //Serial.println("Button Pressed");
    buttonState = 0; //probabilmente inutile perché tanto il button non lo ripremiamo più, è tutto automatico
    duty_cycle = 100;
    //analogWrite(load, map(duty_cycle, 0, 100, 0, 255));            versione con duty cycle
    digitalWrite(load, HIGH); //attivo estrusore
    start_heating_millis = millis(); //segno l'istante in cui inizio il riscaldamentoo
    inputState = 1; //segno che riscaldamento attivo
    int i = 0; //siamo all'acqusizione numero 0
    while (i < N_acquisition) {
      current_millis = millis();
      readData();
      if (current_millis - start_heating_millis >= millis_to_heat or NTC_T1C > temperature_threshold or NTC_T2C > temperature_threshold) { //controllo che sto riscaldando da più tempo di quello per cui dovrei riscaldare (65k millisecondi NOTA A FINE FILE) E ANCHE CHE NON STO SQUAGLIANDO TUTTE COSE
        digitalWrite(load, LOW);
        inputState = 0;
      }
      if (PeltierVoltage <= cell_threshold and inputState == 0) { //se cella si è raffreddata e ho anche spento il riscaldamento (perché cella potrebbe essere sottosoglia visto che ho appena iniziato a riscaldare)
        i = i + 1; //passo all'acquisizione successiva
        if (i < N_acquisition) { //se questa NON era l'ultima acquisizione
          //analogWrite(load,map(duty_cycle,0,100,0,255));
          digitalWrite(load, HIGH); //riaccendo riscaldamento
          start_heating_millis=millis(); //riconsidero il tempo da cui riscaldo
          inputState = 1;
          //Serial.println("ORA ACQUISIZIONE"+String(i+1));
        }
        else { //SE INVECE QUESTA ERA L'ULTIMA ACQUISIZIONE
          digitalWrite(LED_BUILTIN, LOW);
          acquisition_finished = 1;//segno che ho finito le acquisizioni e quindi ritornando all'inizio del loop NON entrerò mai nel primo if perché acquisition_finished=1 e perciò "leggo a vuoto"
        }
      }
    }
  }
}
//NOTA 65 secondi erano buoni quando NON C'ERA il BJT che causa una caduta di tensione di 3V quando lavora in saturazione e quindi l'estrusore col BJT lavora a 9V quindi a parità di tempo riscalda di meno e non si arriva a 70 gradi in 65 secondi ma circa a 60 gradi centigradi.

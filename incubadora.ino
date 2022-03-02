#include <TinyDHT.h>
#include <Servo.h>
#include <Wire.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h>


//LCD
LiquidCrystal_I2C lcd (0x27, 2, 1, 0, 4, 5, 6, 7);


//salidas rele
int ventilador1 = 5;
int ventilador2 = 6;
int luz = 8;


//Entradas digitales
int pulsador = 3;


//Sensor DHT
int pin_dht = 2;
int dht_humedad;
int dht_temperatura;
DHT dht(pin_dht,DHT11);


//Seteo de humedad y temperatura
int pin_humedad = A0;
int pin_temperatura = A1;
int set_humedad;
int set_temperatura;
int valor_humedad;
int valor_temperatura;


//control base
Servo servo1;
int PINSERVO = 11;
int PULSOMIN = 600;
int PULSOMAX = 2550;


//control con tiempo
unsigned long time_work = (millis()/3600000); //pasado a hora tiempo para trabajar oscilacion
int sumador_horas = 0;


void setup() {
  servo1.write(44);
  
  lcd.setBacklightPin(3,POSITIVE);
  lcd.setBacklight(HIGH);
  lcd.begin(16, 2);
  lcd.clear();
  
  servo1.attach(PINSERVO, PULSOMIN, PULSOMAX);

  dht.begin();
  
  pinMode(ventilador1, OUTPUT);
  pinMode(ventilador2, OUTPUT);
  pinMode(luz, OUTPUT);
  pinMode(pulsador, INPUT);

  // seteo de salidas a LOW
  digitalWrite(ventilador1, 1);
  digitalWrite(ventilador2, 1);
  digitalWrite(luz, 1);
  
  Serial.begin(9600);
}

void loop() {

  unsigned long time_run = (millis()/3600000) + sumador_horas; //pasado a hora, tiempo de funcionamiento
  int aux = 0;
  
  


  // control temperatura y humedad
  
  if(dht_humedad <= set_humedad){
    digitalWrite(ventilador1, 1);//apagado
    digitalWrite(ventilador2, 1);//apagado
    }  

  if(dht_temperatura == set_temperatura){
    digitalWrite(luz, 1);//apagada
    digitalWrite(ventilador1, 1);//apagado
  }

  if(dht_temperatura < set_temperatura / 1.05){
    digitalWrite(luz, 0);//prendida
    //digitalWrite(ventilador1, 0);//prendido
  }

  if(dht_temperatura > set_temperatura * 1.05){
    digitalWrite(ventilador1, 0);//prendido
  }

  if(dht_humedad > set_humedad * 1.05){
    digitalWrite(ventilador2, 0);//prendido
  }


  
  //seteo de humedad y temperatura
  valor_humedad = analogRead(pin_humedad);
  valor_temperatura = analogRead(pin_temperatura);
  set_humedad = map(valor_humedad, 0, 1023, 20, 80);
  set_temperatura = map(valor_temperatura, 0, 1023, 0, 50);


  //medicion de humedad y temperatura
  dht_humedad = dht.readHumidity();
  dht_temperatura = dht.readTemperature();


  // Imprimir estado de humedad y temperatura en LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(dht_temperatura);
  lcd.print("C > ");
  lcd.print(set_temperatura);
  lcd.print("C | TIME");
  lcd.setCursor(0, 1);
  lcd.print(dht_humedad);
  lcd.print("% > ");
  lcd.print(set_humedad);
  lcd.print("% | ");
  lcd.print(time_run);
  lcd.print("h");
  
  test_humedad();

  
  // control movimiento de base
  if ((time_work <= time_run) && (time_work < 337)){ // condicion para oscilar la base

    //imprimiendo estado
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("  Oscilando  ");

    //moviendo base
    mover_base(); 
    digitalWrite(ventilador1, 0);//prendido
    mover_base(); 
    digitalWrite(ventilador1, 1);//apagado
    time_work = time_run + 4; // cada cuatro horas activa la oscilacion de la base 
    imprimir_en_serial();
    }

  // sumador de horas 
  while(digitalRead(pulsador)==1){
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Sumar Horas");
    aux += 1;
    lcd.setCursor(0, 1);
    lcd.print(" ");
    lcd.print(  time_run);    
    lcd.print("h  >  ");
    lcd.print(  time_run + aux);    
    lcd.print("h  ");
    delay(500);
  }sumador_horas += aux;

}


void mover_base (){

  for(int i = 44; i<68 ; i++){
    servo1.write(i);
    delay(400);
  }
    for(int i = 68; i>44 ; i--){
    servo1.write(i);
    delay(400);
  }
  for(int i = 44; i>22 ; i--){
    servo1.write(i);
    delay(400);
  }
  for(int i = 22; i<44 ; i++){
    servo1.write(i);
    delay(400);
  }
  servo1.write(44);
}

  
  void imprimir_en_serial(){
    //time_work = (millis()/1000); //pasado a segundo
    Serial.print("Tiempo trabajando: ");
    Serial.print((millis()/3600000));
    Serial.print("\n");  
    Serial.print("Hora del proximo ciclo: ");
    Serial.print(time_work);
    Serial.print("\n");
}

void test_humedad(){
  if(((millis()/1000)%20) == 0){ // solo imprime cada 20 segundos, para testear preset en serial
    Serial.print("set humedad: ");
    Serial.print(set_humedad);
    Serial.print("\n");
    Serial.print("set temperatura: ");
    Serial.print(set_temperatura);
    Serial.print("\n");
    Serial.print("temepratura medicion: ");
    Serial.print(dht_temperatura);
    Serial.print(" Humedad medicion:");
    Serial.print(dht_humedad);
    }
}

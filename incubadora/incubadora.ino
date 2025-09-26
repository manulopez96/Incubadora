#include <TinyDHT.h>
#include <Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ================= LCD =================
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7);

// ================= Pines =================
const int PIN_VENT1 = 5;
const int PIN_VENT2 = 6;
const int PIN_LUZ   = 8;
const int PIN_PULS  = 3;
const int PIN_DHT   = 2;
const int PIN_HUMEDAD = A0;
const int PIN_TEMPSET = A1;
const int PIN_SERVO = 11;

// ================= Objetos =================
DHT dht(PIN_DHT, DHT11);
Servo servo1;

// ================= Variables =================
int set_humedad, set_temperatura;
int dht_humedad, dht_temperatura;

unsigned long ahora = millis();
unsigned long ultimaOscilacion = 0;
const unsigned long intervaloOscilacion = 4UL * 3600000; // 4 horas en ms
int sumador_horas = 0;

// ================= Servo =================

const int NIVEL_SERVO = 25;
const int MIN_SERVO = 0;
const int MAX_SERVO = 50;
const int PULSO_MIN = 900;
const int PULSO_MAX = 2100;
const int TIEMPO_PASO = 100; // menor = más rápido
int pos_servo = NIVEL_SERVO;
int direccion = 5;
int ciclos_osc = 0;
bool oscilando = false;
unsigned long ultimoMovimiento = 0;

// ================= Setup =================
void setup() {
  Serial.begin(9600);

  // Inicializar LCD
  lcd.begin(16, 2);
  lcd.setBacklightPin(3, POSITIVE);
  lcd.setBacklight(HIGH);
  lcd.clear();

  // Inicializar servo
  servo1.attach(PIN_SERVO, PULSO_MIN, PULSO_MAX);
  servo1.write(pos_servo);

  // Inicializar DHT
  dht.begin();

  // Configurar pines
  pinMode(PIN_VENT1, OUTPUT);
  pinMode(PIN_VENT2, OUTPUT);
  pinMode(PIN_LUZ, OUTPUT);
  pinMode(PIN_PULS, INPUT);

  // Apagar salidas
  digitalWrite(PIN_VENT1, HIGH);
  digitalWrite(PIN_VENT2, HIGH);
  digitalWrite(PIN_LUZ, HIGH);

  // Oscilación inicial al arrancar
  oscilando = true;
  ciclos_osc = 0;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Oscilando inicio...");
}

// ================= Loop =================
void loop() {
  
  unsigned long horasFuncionando = (ahora / 3600000);

  leerSensores();
  controlAmbiente();
  mostrarLCD(horasFuncionando);
  controlOscilacion();
  leerPulsador();
}

// ================= Funciones =================
void leerSensores() {
  int valor_humedad = analogRead(PIN_HUMEDAD);
  int valor_temperatura = analogRead(PIN_TEMPSET);

  set_humedad = map(valor_humedad, 0, 1023, 20, 80);
  set_temperatura = map(valor_temperatura, 0, 1023, 0, 50);

  dht_humedad = dht.readHumidity();
  dht_temperatura = dht.readTemperature();
}

void controlAmbiente() {
  if(dht_humedad <= set_humedad){
    digitalWrite(PIN_VENT1, HIGH);
    digitalWrite(PIN_VENT2, HIGH);
  } else {
    digitalWrite(PIN_VENT2, LOW);
  }

  if(dht_temperatura < set_temperatura){
    digitalWrite(PIN_LUZ, LOW);
  } else if(dht_temperatura > set_temperatura * 1.1){
    digitalWrite(PIN_VENT1, LOW);
  } else if(dht_temperatura > set_temperatura * 1.05){
    digitalWrite(PIN_LUZ, HIGH);
  }
}

void mostrarLCD(unsigned long horas) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(dht_temperatura);
  lcd.print("C > ");
  lcd.print(set_temperatura);
  lcd.print("C | ");
  lcd.print(horas);
  lcd.print("h");

  lcd.setCursor(0, 1);
  lcd.print(dht_humedad);
  lcd.print("% > ");
  lcd.print(set_humedad);
  lcd.print("%");
}

unsigned long tomarAhora(){
  return (millis() + sumador_horas * 3600000);
}

void controlOscilacion() {
  ahora = tomarAhora();

  // Iniciar oscilación si corresponde
  if (!oscilando && (ahora - ultimaOscilacion >= intervaloOscilacion)) {
    oscilando = true;
    ciclos_osc = 0;
    digitalWrite(PIN_LUZ, LOW); // prender luz mientras oscila
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Oscilando...");
  }

  if (oscilando) {
    if (ahora - ultimoMovimiento >= TIEMPO_PASO) {
      pos_servo += direccion;

      // Cambio de dirección al llegar a los límites
      if (pos_servo >= MAX_SERVO || pos_servo <= MIN_SERVO) {
        direccion *= -1;
        ciclos_osc++;
      }

      servo1.write(pos_servo);
      ultimoMovimiento = tomarAhora();

      // Si completó los ciclos de oscilación
      if (ciclos_osc >= 4) {
        oscilando = false;
        ultimaOscilacion = tomarAhora();
        // dejamos que después se mueva suavemente al centro
        direccion = (pos_servo > NIVEL_SERVO) ? -1 : 1;  // ajustar dirección hacia el centro
      }
    }
  } 
  else {
    // Cuando terminó la oscilación, llevar al centro suavemente
    if (abs(pos_servo - NIVEL_SERVO) > 0) {
      if (ahora - ultimoMovimiento >= TIEMPO_PASO) {
        pos_servo += (pos_servo > NIVEL_SERVO) ? -1 : 1;
        servo1.write(pos_servo);
        ultimoMovimiento = tomarAhora();
      }
    }
  }
}


void leerPulsador() {
  int aux = 0;
  if(digitalRead(PIN_PULS) == HIGH){
    while(digitalRead(PIN_PULS) == HIGH){
      aux++;
      lcd.setCursor(0,1);
      lcd.print("Horas +");
      lcd.print(aux);
      delay(500);
    }
    sumador_horas += aux;
    // Inicializar LCD
    lcd.begin(16, 2);
    lcd.setBacklightPin(3, POSITIVE);
    lcd.setBacklight(HIGH);
    lcd.clear();
  }
}

void imprimir_en_serial() {
  Serial.print("Tiempo trabajando: ");
  Serial.println((millis()/3600000));
  Serial.print("Hora del proximo ciclo: ");
  Serial.println(ultimaOscilacion/3600000 + 4);
}

#include <TinyDHT.h>
#include <Servo.h>
#include <Wire.h>
#include <LCD.h>
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
const int PULSOMIN  = 900;
const int PULSOMAX  = 2100;

// ================= Objetos =================
DHT dht(PIN_DHT, DHT11);
Servo servo1;

// ================= Variables =================
int set_humedad, set_temperatura;
int dht_humedad, dht_temperatura;

unsigned long ultimaOscilacion = 0;
unsigned long intervaloOscilacion = 4UL * 3600000; // 4 horas en ms
int sumador_horas = 0;

// ================= Setup =================
void setup() {
  Serial.begin(9600);

  // Servo
  servo1.attach(PIN_SERVO, PULSOMIN, PULSOMAX);
  servo1.write(25);

  // LCD
  lcd.setBacklightPin(3, POSITIVE);
  lcd.setBacklight(HIGH);
  lcd.begin(16, 2);
  lcd.clear();

  // DHT
  dht.begin();

  // Salidas
  pinMode(PIN_VENT1, OUTPUT);
  pinMode(PIN_VENT2, OUTPUT);
  pinMode(PIN_LUZ, OUTPUT);

  // Entradas
  pinMode(PIN_PULS, INPUT);

  // Apagar salidas
  digitalWrite(PIN_VENT1, HIGH);
  digitalWrite(PIN_VENT2, HIGH);
  digitalWrite(PIN_LUZ, HIGH);
}

// ================= Loop =================
void loop() {
  unsigned long ahora = millis();
  unsigned long horasFuncionando = (ahora / 3600000) + sumador_horas;

  // Seteo de consigna desde potenciómetros
  set_humedad = map(analogRead(PIN_HUMEDAD), 0, 1023, 20, 80);
  set_temperatura = map(analogRead(PIN_TEMPSET), 0, 1023, 0, 50);

  // Lectura DHT
  dht_humedad = dht.readHumidity();
  dht_temperatura = dht.readTemperature();

  if (!isnan(dht_humedad) && !isnan(dht_temperatura)) {
    controlHumedad();
    controlTemperatura();
    mostrarLCD(horasFuncionando);
  }

  controlOscilacion(ahora, horasFuncionando);
  leerPulsador(horasFuncionando);
  test_humedad();
}

// ================= Funciones =================
void controlHumedad() {
  if (dht_humedad <= set_humedad) {
    digitalWrite(PIN_VENT1, HIGH); // apagado
    digitalWrite(PIN_VENT2, HIGH);
  } else {
    digitalWrite(PIN_VENT2, LOW);  // prendido
  }
}

void controlTemperatura() {
  if (dht_temperatura < set_temperatura) {
    digitalWrite(PIN_LUZ, LOW);  // prendida
  } else if (dht_temperatura > set_temperatura * 1.1) {
    digitalWrite(PIN_VENT1, LOW); // ventilador prendido
  } else if (dht_temperatura > set_temperatura * 1.05) {
    digitalWrite(PIN_LUZ, HIGH);  // apagada
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

void controlOscilacion(unsigned long ahora, unsigned long horas) {
  if (ahora - ultimaOscilacion >= intervaloOscilacion && horas < 337) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Oscilando...");

    digitalWrite(PIN_LUZ, LOW); // prende luz
    mover_base();

    ultimaOscilacion = ahora;
    imprimir_en_serial(horas);
  }
}

void leerPulsador(unsigned long horas) {
  int aux = 0;
  if (digitalRead(PIN_PULS) == HIGH) {
    while (digitalRead(PIN_PULS) == HIGH) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Sumar Horas");
      aux += 1;
      lcd.setCursor(0, 1);
      lcd.print(horas);
      lcd.print("h > ");
      lcd.print(horas + aux);
      lcd.print("h");
      delay(500);
    }
    sumador_horas += aux;
  }
}

// Movimiento no bloqueante
void mover_base() {
  static int pos = 25;
  static int direccion = 1;
  static unsigned long ultimoMovimiento = 0;

  if (millis() - ultimoMovimiento >= 100) {
    pos += direccion;
    if (pos >= 50 || pos <= 0) direccion *= -1;
    servo1.write(pos);
    ultimoMovimiento = millis();
  }
}

void imprimir_en_serial(unsigned long horas) {
  Serial.print("Tiempo trabajando: ");
  Serial.println(horas);
  Serial.print("Hora del proximo ciclo: ");
  Serial.println((ultimaOscilacion + intervaloOscilacion) / 3600000);
}

void test_humedad() {
  if (((millis() / 1000) % 20) == 0) {
    Serial.print("Set humedad: ");
    Serial.println(set_humedad);
    Serial.print("Set temperatura: ");
    Serial.println(set_temperatura);
    Serial.print("Medicion T: ");
    Serial.print(dht_temperatura);
    Serial.print("C | H: ");
    Serial.println(dht_humedad);
  }
}

// Sensor
#define TRIG_PIN 10
#define ECHO_PIN 11
#define LDR_PIN A1

// Aktuator
const int ENADC = 4;
const int IN1DC = 3;
const int IN2DC = 2;
const int motorServo = 9;
const int potPin = A0;
const int buzzer = 6;
const int tombol = 7;

const int servoLock = 0;
const int servoOpen = 90;

const float R_FIXED = 10000.0; // Resistor tetap 10k ohm

enum State {
  STATE_DEFAULT,
  STATE_IDLE,
  STATE_MOVING,
  STATE_INTRUDER,
  STATE_RESET
};

State currentState = STATE_DEFAULT;

#include <Servo.h>
Servo servo;

bool intruderNotified = false; // dideklarasi di luar loop, global
bool isServoOpen = false;         // Menyimpan status servo (false = terkunci, true = terbuka)
bool tombolPressedLast = false;   // Debounce: melacak status tombol sebelumnya

// TIMING SENSOR 
unsigned long lastSensorSend = 0;
const unsigned long sensorInterval = 300;

void setup() {
  Serial.begin(9600);
  
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LDR_PIN, INPUT);
  
  pinMode(ENADC, OUTPUT);
  pinMode(IN1DC, OUTPUT);
  pinMode(IN2DC, OUTPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(tombol, INPUT_PULLUP);
  
  servo.attach(motorServo);
  servo.write(servoLock); // default terkunci

  // Set arah motor DC tetap (maju)
  digitalWrite(IN1DC, HIGH);
  digitalWrite(IN2DC, LOW);
}

void loop() {
  runFSM();            // Jalankan FSM utama
  sendSensorData();    // Kirim data sensor ke MATLAB GUI
}

void runFSM() {
  // Cek input dari MATLAB
  if (Serial.available()) {
    char command = Serial.read();
    switch (command) {
      case '1':
        currentState = STATE_IDLE;
        break;
      case '0':
        currentState = STATE_INTRUDER;
        break;
      case 'R':
        currentState = STATE_RESET;
        break;
    }
  }

  // FSM Logic
  switch (currentState) {
    case STATE_DEFAULT:
      // Matikan Motor DC total
      analogWrite(ENADC, LOW);         // PWM = 0
      digitalWrite(IN1DC, LOW);
      digitalWrite(IN2DC, LOW);
    
      // Matikan Buzzer
      noTone(buzzer);
    
      // Servo dalam posisi terkunci (default aman)
      servo.write(servoLock);
    
      // Tombol tidak aktif (tidak dicek dalam state ini)

      // Reset flag intruder jika digunakan
      intruderNotified = false;
      break;

    case STATE_IDLE: {
      int potValue = analogRead(potPin); // Baca nilai potensiometer (0–1023)
      int pwmSpeed = map(potValue, 10, 1023, 140, 225); // Kecepatan pelan – sedang
        
      // Jika diputar terlalu besar, pindah ke STATE_MOVING
      if (potValue >= 700) {
        currentState = STATE_MOVING;
      } else {
        // Nyalakan motor DC dengan kecepatan rendah – sedang
        analogWrite(ENADC, pwmSpeed); // Sesuai potensiometer
        digitalWrite(IN1DC, HIGH);
        digitalWrite(IN2DC, LOW);
            
        // Matikan buzzer
        noTone(buzzer);     // matikan suara
    
        // Logika toggle tombol servo (aktif hanya di IDLE)
        bool tombolState = digitalRead(tombol) == LOW;
        if (tombolState && !tombolPressedLast) {
          isServoOpen = !isServoOpen; // toggle
          servo.write(isServoOpen ? servoOpen : servoLock);
          delay(200); // debounce delay
        }
        tombolPressedLast = tombolState; // update status tombol
      }
      break;
    }

    case STATE_MOVING: {
      int potValue = analogRead(potPin);
      int pwmSpeed = map(potValue, 10, 1023, 140, 225); // Kecepatan menengah–maksimal
    
      // Jika potensiometer diputar lambat kembali, pindah ke IDLE
      if (potValue < 700) {
        currentState = STATE_IDLE;
      } else {
        // Motor DC tetap menyala dan semakin cepat
        analogWrite(ENADC, pwmSpeed);
        digitalWrite(IN1DC, HIGH);
        digitalWrite(IN2DC, LOW);
            
        // Kunci servo, tombol tidak aktif
        servo.write(servoLock);
      }
      break;
    }

    case STATE_INTRUDER:
      // Matikan motor DC sepenuhnya
      analogWrite(ENADC, LOW);         // PWM = 0
      digitalWrite(IN1DC, LOW);
      digitalWrite(IN2DC, LOW);
    
      // Buzzer menyala
      tone(buzzer, 1000);  // bunyi 1000 Hz
    
      // Servo dalam posisi terkunci
      servo.write(servoLock);
    
      // Tombol dinonaktifkan → tidak perlu dicek
    
      // Kirim notifikasi kembali ke MATLAB
      if (!intruderNotified) {
        Serial.println("I"); // Kirim notifikasi hanya sekali
        intruderNotified = true; // tandai sudah dikirim
      }
      break;

    case STATE_RESET:
      currentState = STATE_DEFAULT;
      break;
  }

  delay(50); // hindari overread
}

void sendSensorData() {
  if (millis() - lastSensorSend >= sensorInterval) {
    lastSensorSend = millis();

    // Baca LDR
    int ldrValue = analogRead(LDR_PIN);
    float voltage = ldrValue * (5.0 / 1023.0); // Ubah ke volt

    // Hitung resistansi LDR (dari voltage divider)
    float rLDR = (5.0 - voltage) * R_FIXED / voltage;

    // Perkiraan lux berdasarkan rumus empiris LDR: LUX = 500 / (rLDR / 1000)^1.4
    float lux = 1000 / pow((rLDR / 1000.0), 1.4); // rLDR dalam kΩ

    // Atau balik nilai akhir lux jadi:
    lux = 1000.0 / lux;  // atau 500000.0 / lux, tergantung skala

    // Baca Ultrasonik
    long duration;
    float distance;
      
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
  
    duration = pulseIn(ECHO_PIN, HIGH);
    distance = duration * 0.034 / 2; // cm

    // Kirim ke MATLAB dalam format mudah dibaca
    Serial.print("LDR:");
    Serial.print(lux);
    Serial.print(",US:");
    Serial.println(distance);

    delay(500);
  }
}

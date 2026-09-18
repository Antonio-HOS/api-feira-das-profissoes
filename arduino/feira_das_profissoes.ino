//=================> Hardware
//================= ENA-    --> GND Arduino
//================= ENA+    --> Pino 8 Arduino
//================= DIR-    --> GND Arduino
//================= DIR+    --> Pino 2 Arduino
//================= PUL-    --> GND Arduino
//================= PUL+    --> Pino 5 Arduino
//================= GND     --> GND da Fonte Externa
//================= VCC     --> Fonte Externa
//
//=================> Botões
//================= Botão Relé          --> Pino 9
//================= Botão 20 graus      --> Pino 10
//================= Botão 20 posições   --> Pino 11
//================= Outro terminal dos botões --> GND


//=================> Declarações
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

const int stepPin = 5;
const int dirPin = 2;
const int enPin = 8;
const int ledPin = 3;

// Botões
const int botaoRele = 9;
const int botao20Graus = 10;
const int botao20Vezes = 11;

const int stepsPerRevolution = 1600;
const float grausPorRevolucao = 360.0;

float grausAcumulados = 0.0;

const int delayPasso = 1000;


//=================> SETUP

void setup() {
  //LCD
  lcd.init();
  lcd.backlight();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Sistema pronto");

  lcd.setCursor(0, 1);
  lcd.print("Total: 0.00 graus");
//LCD




  pinMode(stepPin, OUTPUT);
  pinMode(dirPin, OUTPUT);
  pinMode(enPin, OUTPUT);
  pinMode(ledPin, OUTPUT);

  // Botões usando resistor interno do Arduino
  pinMode(botaoRele, INPUT_PULLUP);
  pinMode(botao20Graus, INPUT_PULLUP);
  pinMode(botao20Vezes, INPUT_PULLUP);

  // Habilita o driver
  digitalWrite(enPin, LOW);

  // Relé inicialmente desligado
  digitalWrite(ledPin, HIGH);

  Serial.begin(9600);

  Serial.println("Sistema pronto.");
  Serial.println("Botoes:");
  Serial.println("Botao 1 = Liga rele");
  Serial.println("Botao 2 = Gira 20 graus");
  Serial.println("Botao 3 = Executa 20 movimentos de 18 graus");
}


//=================> GIRAR GRAUS

void girarGraus(float graus) {

  if (graus <= 0 || (grausAcumulados + graus) > grausPorRevolucao) {

    Serial.println("Movimento invalido, excede 360 graus ou e zero.");

    return;
  }

  int passos = (int)((graus / grausPorRevolucao) * stepsPerRevolution);

  Serial.print("Girando ");
  Serial.print(graus, 2);
  Serial.println(" graus.");

  digitalWrite(dirPin, HIGH);

  for (int x = 0; x < passos; x++) {

    digitalWrite(stepPin, HIGH);
    delayMicroseconds(delayPasso);

    digitalWrite(stepPin, LOW);
    delayMicroseconds(delayPasso);
  }

  grausAcumulados += graus;

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Giro: ");
  lcd.print(graus, 2);
  lcd.print(" graus");

  lcd.setCursor(0, 1);
  lcd.print("Total: ");
  lcd.print(grausAcumulados, 2);

  Serial.print("Total girado: ");
  Serial.print(grausAcumulados, 2);
  Serial.println(" graus.");

  if (abs(grausAcumulados - 360.0) < 0.01) {

    grausAcumulados = 0.0;

    Serial.println("Total de graus igual a 360. Contador zerado!");
  }
}


//=================> GIRAR 20 VEZES

void girar20Vezes() {

  grausAcumulados = 0.0;

  Serial.println("Iniciando sequencia de 20 posicoes de 18 graus.");

  for (int i = 0; i < 20; i++) {

    Serial.print("Posicao ");
    Serial.print(i + 1);
    Serial.println(" de 20");

    girarGraus(18.0);

    delay(1000);
    disparaFonte();
    delay(1000);
  }

  grausAcumulados = 0.0;

  Serial.println("Sequencia concluida: 20 posicoes / 360 graus.");
}


//=================> ACIONAR RELÉ

void disparaFonte() {

  digitalWrite(ledPin, LOW);

  Serial.println("Rele ligado.");

  delay(1000);

  digitalWrite(ledPin, HIGH);

  Serial.println("Rele desligado.");
}


//=================> VOLTA COMPLETA

void girandoVolta() {

  grausAcumulados = 0.0;

  for (int i = 0; i < 200; i++) {

    girarGraus(1.8);
  }
}


//=================> RESET CONTADOR

void reiniciarContador() {

  grausAcumulados = 0.0;

  Serial.println("Contador de graus zerado!");

  Serial.print("Total girado: ");
  Serial.print(grausAcumulados, 2);
  Serial.println(" graus.");
}


//=================> LOOP

void loop() {

  //==================================================
  // BOTAO 1 - LIGA O RELE
  //==================================================

  if (digitalRead(botaoRele) == LOW) {

    // Evita acionamentos repetidos enquanto o botao estiver pressionado
    delay(50);

    if (digitalRead(botaoRele) == LOW) {

      disparaFonte();

      // Espera o botao ser solto
      while (digitalRead(botaoRele) == LOW) {
        delay(10);
      }
    }
  }


  //==================================================
  // BOTAO 2 - GIRA 20 GRAUS
  //==================================================

  if (digitalRead(botao20Graus) == LOW) {

    delay(50);

    if (digitalRead(botao20Graus) == LOW) {

      girarGraus(20.0);

      // Espera o botao ser solto
      while (digitalRead(botao20Graus) == LOW) {
        delay(10);
      }
    }
  }


  //==================================================
  // BOTAO 3 - EXECUTA 20 POSICOES
  //==================================================

  if (digitalRead(botao20Vezes) == LOW) {

    delay(50);

    if (digitalRead(botao20Vezes) == LOW) {

      girar20Vezes();

      // Espera o botao ser solto
      while (digitalRead(botao20Vezes) == LOW) {
        delay(10);
      }
    }
  }


  //==================================================
  // COMANDOS PELO SERIAL
  //==================================================

  if (Serial.available()) {

    String comando = Serial.readStringUntil('\n');

    comando.trim();

    if (comando == "1") {

      while (Serial.available() == 0);

      float graus = Serial.readStringUntil('\n').toFloat();

      girarGraus(graus);

    }

    else if (comando == "2") {

      disparaFonte();

    }

    else if (comando == "3") {

      girandoVolta();

    }

    else if (comando == "4") {

      reiniciarContador();

    }

    else if (comando == "5") {

      girar20Vezes();

    }

    else {

      Serial.println("Comando invalido.");
    }
  }
}


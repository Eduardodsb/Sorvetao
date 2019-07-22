#include <Ultrasonic.h>
#include <CmdMessenger.h>
#include <Wire.h>
#include <Servo.h>

//Pinos
#define trigger_sensor_frontal 7 //Porta do Arduino 7 = PD7
#define echo_sensor_frontal 5 //Porta do Arduino 5 = PD5
#define trigger_sensor_traseiroD 7 //Porta do Arduino 7 = PD7
#define echo1_sensor_traseiroD 6 //Porta do Arduino 6 = PD6
#define trigger_sensor_traseiroE 7 //Porta do Arduino 7 = PD7
#define echo1_sensor_traseiroE 8 //Porta do Arduino 8 = PB0
#define CH1 2 //Porta do Arduino 2 = PD2/INT0
#define CH2 3 //Porta do Arduino 3 = PD3/INT1/OC2B
#define CH3 4 //Porta do Arduino 4 = PD4 //Botão para ativar o modo autonomo e desativar.
#define CH4 PB7 //Arduino não possui equivalente => PB7 //Botão para ativar a arma. 
#define Bateria A0 //Porta do arduino A0 = PC0
#define Rx 0
#define Tx 1
#define Drive1 9 //Porta do arduino 9 = PB1 //Motor 1 roda da esquerda
#define Drive2 10 //Porta do arduino 10 = PB2 //Motor 2 roda da direita
#define Drive3 PB6 //Arduino não possui equivalente => PB6 //Motor 3 - arma 


//Macros gerais
#define MINIMUM_VOLTAGE   0
#define MAXIMUM_VOLTAGE   21
#define MINIMUM_REGISTER  0
#define MAXIMUM_REGISTER  1023


bool modo_Autonomo = false;
bool fail_Safe = false;
bool status_arma = false; //Ativada = true, desativada = false
float time_failsafe;
int aux = 0; //Variável de controle para a função failsafe
unsigned voltage = 0; //Voltagem da bateria

//Cálculo de largura de pulso
unsigned volatile long ultimaMedida_CH1;
unsigned volatile long tempoDeSubida_CH1;
float largura_CH1;

unsigned volatile long ultimaMedida_CH2;
unsigned volatile long tempoDeSubida_CH2;
float largura_CH2;

//Inicialização dos sensores HC-SR04
Ultrasonic ultrasonic_frontal(trigger_sensor_frontal, echo_sensor_frontal);
Ultrasonic ultrasonic_traseiroD(trigger_sensor_traseiroD, echo1_sensor_traseiroD);
Ultrasonic ultrasonic_traseiroE(trigger_sensor_traseiroE, echo1_sensor_traseiroE);

//Inicialização do bluetooth
CmdMessenger cmdMessenger(Serial);

//Endereço I2C do MPU6050
const int MPU = 0x68;

//Variáveis para armazenar valores dos sensores.
int AcX, AcY, AcZ, Tmp, GyX, GyY, GyZ;

//Variáveis Motores.
Servo meuServo1;
Servo meuServo2;
Servo meuServo3;

void Subida_CH1(){
  attachInterrupt(digitalPinToInterrupt(CH1), Descida_CH1, FALLING);
  tempoDeSubida_CH1 = micros();
}

void Subida_CH2(){
  attachInterrupt(digitalPinToInterrupt(CH2), Descida_CH2, FALLING);
  tempoDeSubida_CH2 = micros();
}

void Descida_CH1(){
  attachInterrupt(digitalPinToInterrupt(CH1),Subida_CH1, RISING);
  largura_CH1 = micros() - tempoDeSubida_CH1;
  ultimaMedida_CH1 = micros();
}

void Descida_CH2(){
  attachInterrupt(digitalPinToInterrupt(CH2),Subida_CH2, RISING);
  largura_CH2 = micros() - tempoDeSubida_CH2;
  ultimaMedida_CH2 = micros();
}

void controleRC(){//Função responsável por interpretar os comandos do controle.

  float canal1;
  float canal2;

  if(digitalRead(CH4) == 1 && !modo_Autonomo && !fail_Safe){
    Arma(); //Ativa ou desativa dependendo do estado atual da mesma. 
    delay(50);
    return;
  }
  
  if(digitalRead(CH3) == 1 && !modo_Autonomo && !fail_Safe){
    Serial.println("autonomo ativado");
    modo_Autonomo =  true;
    delay(150);
    return;
  }
  
  if(digitalRead(CH3) == 1 && modo_Autonomo && !fail_Safe){
    Serial.println("autonomo desativado");
    modo_Autonomo =  false;
    delay(150);
    return;
  }

  if(!modo_Autonomo && !fail_Safe){
    
    Serial.println("\n");
    Serial.println("Largura do sinal do canal 1");
    Serial.println(largura_CH1);
    Serial.println("Largura do sinal do canal 2");
    Serial.println(largura_CH2);
  
//Canal 1 - ir para frente e para trás.
    
    if(largura_CH1 < 1480.0){ //Ré
      canal1 = map(largura_CH1, 1000, 1500, 0, 127);
      Serial.println("RE - (Motor1 e Motor2):"); //Motor1 roda da esquerda e Motor2 roda da direita.
      Serial.println(canal1);
      //Enviar sinal para o Drive de motor
      meuServo1.writeMicroseconds(canal1);
      meuServo2.writeMicroseconds(canal1);
    }
  
      if(largura_CH1 > 1520.0){ //Acelerar
      canal1 = map(largura_CH1, 1500, 2000, 127, 255);
      Serial.println("Acelera - (Motor1 e Motor2):"); //Motor1 roda da esquerda e Motor2 roda da direita.
      Serial.println(canal1);
      //Enviar sinal para o Drive de motor
      meuServo1.writeMicroseconds(canal1);
      meuServo2.writeMicroseconds(canal1);
    }
    
//Canal 2 - girar para direita e esquerda.
    
    if(largura_CH2 < 1480.0){
      canal2 = map(largura_CH2, 1000, 1500, 127, 255);
      Serial.println("Virar para esquerda:");
      Serial.print("Motor 1: ");//Motor1 roda da esquerda
      Serial.println(map(canal2, 127, 255, 0, 127));
      meuServo1.writeMicroseconds(map(canal2, 127, 255, 0, 127));
      Serial.print("Motor 2: ");//Motor2 roda da direita
      Serial.println(canal2);
      meuServo2.writeMicroseconds(canal2);
    }
  
      if(largura_CH2 > 1520.0){
      canal2 = map(largura_CH2, 1500, 2000, 127, 255);
      Serial.println("Virar para direita:");
      Serial.print("Motor 1: ");//Motor1 roda da esquerda
      Serial.println(canal2);
      meuServo1.writeMicroseconds(canal2);
      Serial.print("Motor 2: ");//Motor2 roda da direita
      Serial.println(map(canal2, 127, 255, 0, 127));
      meuServo2.writeMicroseconds(map(canal2, 127, 255, 0, 127));
    }
  }
}

//Função autômona do robô
void autonomo() {

  float cmMsec_frontal, cmMsec_traseiroD, cmMsec_traseiroE;
  long microsec_frontal = ultrasonic_frontal.timing();
  long microsec_traseiroD = ultrasonic_traseiroD.timing();
  long microsec_traseiroE = ultrasonic_traseiroE.timing();
  
  //Le as informacoes do sensor em cm
  cmMsec_frontal = ultrasonic_frontal.convert(microsec_frontal, Ultrasonic::CM);
  cmMsec_traseiroD = ultrasonic_traseiroD.convert(microsec_traseiroD, Ultrasonic::CM);
  cmMsec_traseiroE = ultrasonic_traseiroE.convert(microsec_traseiroE, Ultrasonic::CM);

  
  // Se o robô detectar o adversário entre 2 a 3 cm de distância, ele liga a arma na maior potência e pára de avançar.
  if (cmMsec_frontal > 2 && cmMsec_frontal < 3){
    status_arma = false; //Garantir que ela seja ligada ao ser chamada.
    Arma();
    //Desativa os motores
    meuServo1.writeMicroseconds(127);
    meuServo2.writeMicroseconds(127);
    
  }

  //Se os sensores não captarem o adversário, o robô desliga a arma e gira até encontrá-lo
  else if (cmMsec_frontal >= 350 && cmMsec_traseiroD >= 350 && cmMsec_traseiroE >= 350){
    int potencia = map(cmMsec_frontal, 3, 350, 127, 255);
    status_arma = true; //Garantir que ela seja desligada ao ser chamada.
    Arma();
    //Gira para a direita
    meuServo1.writeMicroseconds(potencia);
    meuServo2.writeMicroseconds(map(cmMsec_frontal, 3, 350, 0, 127));
  }

  //Robô detectou o adversário na sua frente, regula a potência da arma e anda frontalmente  
  else if (cmMsec_frontal < 350 && cmMsec_traseiroD >= 350 && cmMsec_traseiroE >= 350){
    int potencia = map(cmMsec_frontal, 3, 350, 127, 255);
    status_arma = false; //Garantir que ela seja ligada ao ser chamada.
    Arma();
    meuServo1.writeMicroseconds(potencia);
    meuServo2.writeMicroseconds(potencia);
  }
  
  //Robô detectou o adversário na sua traseira direita, regula a potência da arma e anda para a direita
  else if (cmMsec_traseiroD < 350 && cmMsec_frontal >= 350 && cmMsec_traseiroE >= 350){
    int potencia = map(cmMsec_traseiroD, 3, 350, 127, 255);
    status_arma = false; //Garantir que ela seja ligada ao ser chamada.
    Arma();
    //Gira para a direita
    meuServo1.writeMicroseconds(potencia);
    meuServo2.writeMicroseconds(map(cmMsec_frontal, 3, 350, 0, 127));
  }

    //Robô detectou o adversário na sua traseira esquerda, regula a potência da arma e anda para a esquerda
  else if (cmMsec_traseiroE < 350 && cmMsec_frontal >= 350 && cmMsec_traseiroD >= 350){
    int potencia = map(cmMsec_traseiroE, 3, 350, 0, 100);
    status_arma = false; //Garantir que ela seja ligada ao ser chamada.
    Arma();
    //Gira para a esquerda
    meuServo1.writeMicroseconds(map(cmMsec_frontal, 3, 350, 0, 127));
    meuServo2.writeMicroseconds(potencia);
  }

}

void Debugger(){
//Tensão bateria
  voltage = map(analogRead(Bateria), MINIMUM_REGISTER, MAXIMUM_REGISTER, MINIMUM_VOLTAGE, MAXIMUM_VOLTAGE);

//Dados MPU
  Wire.beginTransmission(MPU);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU, 14, true);
  AcX = Wire.read()<<8|Wire.read();
  AcY = Wire.read()<<8|Wire.read();
  AcZ = Wire.read()<<8|Wire.read();
  Tmp = Wire.read()<<8|Wire.read();
  GyX = Wire.read()<<8|Wire.read();
  GyY = Wire.read()<<8|Wire.read();
  GyZ = Wire.read()<<8|Wire.read();

  cmdMessenger.sendCmdStart("Tensão:");
  cmdMessenger.sendCmdArg(voltage);
  cmdMessenger.sendCmdArg("\n");

  cmdMessenger.sendCmdArg("AcX: ");
  cmdMessenger.sendCmdArg(AcX);
  cmdMessenger.sendCmdArg("\n");
  
  cmdMessenger.sendCmdArg("AcY: ");
  cmdMessenger.sendCmdArg(AcY);
  cmdMessenger.sendCmdArg("\n");
  
  cmdMessenger.sendCmdArg("AcZ: ");
  cmdMessenger.sendCmdArg(AcZ);
  cmdMessenger.sendCmdArg("\n");
  
  cmdMessenger.sendCmdArg("AcZ: ");
  cmdMessenger.sendCmdArg(AcZ);
  cmdMessenger.sendCmdArg("\n");
  
  cmdMessenger.sendCmdArg("Tmp: ");
  cmdMessenger.sendCmdArg(Tmp/340.00+36.53);
  cmdMessenger.sendCmdArg("\n");

  cmdMessenger.sendCmdArg("GyX: ");
  cmdMessenger.sendCmdArg(GyX);
  cmdMessenger.sendCmdArg("\n");
  
  cmdMessenger.sendCmdArg("GyY: ");
  cmdMessenger.sendCmdArg(GyY);
  cmdMessenger.sendCmdArg("\n");
  
  cmdMessenger.sendCmdArg("GyZ: ");
  cmdMessenger.sendCmdArg(GyZ);
  cmdMessenger.sendCmdArg("\n");
  
  cmdMessenger.sendCmdArg("AcZ: ");
  cmdMessenger.sendCmdArg(AcZ);
  cmdMessenger.sendCmdArg("\n");

  cmdMessenger.sendCmdEnd();
}



void Arma(){
  if(status_arma){
    status_arma = false;
    //Desativar o motor da arma.
    meuServo3.writeMicroseconds(127);
  }else{
    status_arma = true;
    //Ativar o motor da arma.
    meuServo3.writeMicroseconds(255);
  }  
}


void failSafe(){ //Verifica se houve perda de sinal com o controle
  if(largura_CH2 <= 1000 || largura_CH1 <= 1000 ){
    if(aux == 0){
      time_failsafe = millis();
    }
    aux = 1;
    if(millis() - time_failsafe > 10000.0){
      Serial.println("Failsafe ativado");
      fail_Safe = true;
      //Desativar todos os motores.
      meuServo1.writeMicroseconds(127);
      meuServo2.writeMicroseconds(127);
      meuServo3.writeMicroseconds(127);
      exit(0);
    }
  }else{
    aux = 0;
  }

}

void setup(){
  Serial.begin(9600);
  //Inicializando as entradas
  pinMode(CH1, INPUT);
  pinMode(CH2, INPUT);
  pinMode(CH3, INPUT);
  pinMode(CH4, INPUT);
  pinMode(Bateria, INPUT);
  pinMode(Rx,INPUT);
  pinMode(Tx,OUTPUT);
  
  attachInterrupt(digitalPinToInterrupt(CH1), Subida_CH1, RISING);
  attachInterrupt(digitalPinToInterrupt(CH2), Subida_CH2, RISING);

  cmdMessenger.attach(0,Arma);
  cmdMessenger.attach(1,Debugger);

  Wire.begin();
  Wire.beginTransmission(MPU);
  Wire.write(0x6B);

  //Inicializa o MPU-6050
  Wire.write(0);
  Wire.endTransmission(true);

  //Inicializa Motores
  meuServo1.attach(Drive1);
  meuServo2.attach(Drive2);
  meuServo3.attach(Drive3);
  
}
void loop() {
  
failSafe();
controleRC();
if(modo_Autonomo && !failSafe){
  autonomo();
}
cmdMessenger.feedinSerialData();

}

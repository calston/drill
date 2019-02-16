#include "LiquidCrystal595.h"

#define rpmSense 2

#define in_a1 7
#define in_a2 8
#define en_a 9

#define sr_data 5
#define sr_latch 4
#define sr_clock 3

#define green 6
#define blue 10
#define enc_a 11
#define enc_b 12 

#define boxRatio 37



byte encoder_n = 0;
byte encoder_j = 0;

unsigned long rpmCount = 0;
int rpmCur = 0;
int rpmSet = 2500;
int pwmSet = 50;

unsigned int rpmLast = 0;

unsigned long timer_1 = 0;
unsigned long timer_2 = 0;
const long inter_1 = 100;
const long inter_2 = 500;

int turnsLast = 0;
int turnsSet = 1;
int turns = 0;
int qturns = 0;

unsigned long rotateTime = 0;

bool gr_last = 0;
bool bl_last = 0;

bool b_motor = 0;
bool b_direction = 0;
int  run_mode = 1;
int  current_screen = 0;

LiquidCrystal595 lcd(sr_data, sr_latch, sr_clock);

void drawRpm() {
  lcd.setCursor(0, 1);
  lcd.print("RPM: ");
  lcd.print(rpmCur/37);
  lcd.print("/");
  lcd.print(rpmSet/37);
  lcd.print("  ");
}

void drawTurns() {
  lcd.setCursor(0, 1);
  lcd.print("Turns: ");
  lcd.print(turns);
  lcd.print("/");
  lcd.print(turnsSet);
  lcd.print("  ");
}

void showScreen(int num) {
  current_screen = num;
  if (current_screen == 0) {
    // Main screen
    lcd.clear();

    lcd.setCursor(0, 0);

    lcd.print("Motor: ");

    if (b_motor) {
      lcd.print("Run ");
    } else {
      lcd.print("Stop");    
    }
    
    lcd.setCursor(13, 0);
    if (b_direction) {  
      lcd.print("REV");
    }
    else {
      lcd.print("FWD");
    }

    if (run_mode == 0) {
      drawRpm();
    } 
    else if (run_mode == 1) {
      drawTurns();
    }
  }
  else if (current_screen == 1) {
    // Config screen
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Mode: ");
    if (run_mode == 0){
      lcd.print("Set RPM");
    }
    if (run_mode == 1){
      lcd.print("Turns");
    }
  }
  else if (current_screen == 2){
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Direction: ");
    if (b_direction) {  
      lcd.print("REV");
    }
    else {
      lcd.print("FWD");
    }
  }
}

void encoderLeft(){
  if (current_screen == 0){
    if (run_mode == 0) {
      // Main screen
      rpmSet -= 185;
      if (rpmSet < 0) rpmSet = 0;
      drawRpm();
    }
    else if (run_mode == 1) {
      turnsSet -= 1;
      if (turnsSet < 1) turnsSet = 1;
      drawTurns();
    }
  }
  else if (current_screen == 1){
    // Mode switch
    run_mode -= 1;
    if (run_mode < 0) run_mode = 0;
    showScreen(1);
  }
  else if (current_screen = 2) {
    b_direction = true;
    showScreen(2);
  }
}

void encoderRight(){
  if (current_screen == 0){
    if (run_mode == 0) {
      if (rpmSet < 14800) {
        rpmSet += 185;
        drawRpm();
      }
    }
    else if (run_mode == 1) {
      turnsSet += 1;
      if (turnsSet > 10000) turnsSet = 10000;
      drawTurns();
    }
  }
  else if (current_screen == 1){
    // Mode switch
    run_mode += 1;
    if (run_mode > 1) run_mode = 1;
    showScreen(1);
  }
  else if (current_screen = 2) {
    b_direction = false;
    showScreen(2);
  }
}

void greenButton(){
  if (current_screen == 0) {
    b_motor = !b_motor;
    if (!b_motor) {
      motorStop();
    }
    else if (run_mode == 1){
      // Reset turns when motor start
      qturns = 0;
      turns = 0;
    }
  }
  if (current_screen > 0) {
    current_screen = 0;
  }
  
  showScreen(current_screen);
}

void blueButton(){
  if (current_screen == 0){
    showScreen(1);
  }
  else if (current_screen == 1) {
    showScreen(2);
  }
  else if (current_screen == 2) {
    showScreen(0);
  }
}

void setup() {
  //TCCR1B = TCCR1B & 0b11111000 | 0x01;

  lcd.begin(16,2);

  //digitalWrite(sr_latch, LOW);
  //shiftOut(sr_data, sr_clock, MSBFIRST, B11111111);
  //digitalWrite(sr_latch, HIGH);

  Serial.begin(9600);
  pinMode(rpmSense, INPUT);
  attachInterrupt(digitalPinToInterrupt(rpmSense), quadTick, FALLING);

  pinMode(in_a1, OUTPUT);
  pinMode(in_a2, OUTPUT);
  pinMode(en_a, OUTPUT);

  pinMode(enc_a, INPUT_PULLUP);
  pinMode(enc_b, INPUT_PULLUP);
  pinMode(blue, INPUT_PULLUP);
  pinMode(green, INPUT_PULLUP);

  digitalWrite(in_a1, LOW);
  digitalWrite(in_a2, LOW);

  Serial.println("Start");
  showScreen(0);
}

void quadTick() {
  // Interrupt for motor optical encoder
  
  if (b_motor){
    rpmCount ++;
  
    if (run_mode == 1) {
      // Set number of turns
      qturns += 1;
  
      if (qturns >= 146) {
        qturns = 0;
        turns += 1;
        if (turns >= turnsSet) {
          b_motor = false;
          motorStop();
          showScreen(0);
        }
      }
    }
  }
}

void loop() {
  unsigned long currentMillis = millis();

  bool bl_cur = digitalRead(blue);
  if (bl_cur != bl_last){
    bl_last = bl_cur;
    if (bl_cur) {
      blueButton();
      delay(50);
    }
  }

  bool gr_cur = digitalRead(green);
  if (gr_cur != gr_last){
    gr_last = gr_cur;
    if (gr_cur) {
      greenButton();
      delay(50);
    }
  }

  if ((digitalRead(enc_a) != encoder_j) || (digitalRead(enc_b) != encoder_n)) {
    // Check encoder
    if ((digitalRead(enc_a) == HIGH) && (encoder_j == LOW)) {
      if (encoder_n == LOW) // CCW
        encoderLeft();
      if (encoder_n == HIGH) // CW
        encoderRight();
    }
    
    encoder_j = digitalRead(enc_a);
    encoder_n = digitalRead(enc_b);
  }
  
  if (currentMillis - timer_1 >= inter_1) {
    timer_1 = currentMillis;
    loop1();
  }

  if (currentMillis - timer_2 >= inter_2){
    timer_2 = currentMillis;
    loop2();
  }
}

void motorForward(int pwm) {
  digitalWrite(in_a1, LOW);
  digitalWrite(in_a2, HIGH);
  if (pwm >= 254){
    digitalWrite(en_a, HIGH);
  }
  else {
    analogWrite(en_a, pwm);
  }
}

void motorReverse(int pwm) {
  digitalWrite(in_a1, HIGH);
  digitalWrite(in_a2, LOW);
  if (pwm >= 254){
    digitalWrite(en_a, HIGH);
  }
  else {
    analogWrite(en_a, pwm);
  }
}

void motorRun(int pwm) {
  if (b_direction) {
    motorReverse(pwm);
  }
  else {
    motorForward(pwm); 
  }
}

void motorStop() {
  digitalWrite(en_a, LOW);
  digitalWrite(in_a1, LOW);
  digitalWrite(in_a2, LOW);
}

void updateMotorSpeed(){
  if (rpmCur == 0) {
    // Kick the motor
    motorRun(254);
  }
  else if (rpmCur < rpmSet) {
    pwmSet += 1;
    if (pwmSet > 254) {
       pwmSet = 254;
    }
    motorRun(pwmSet);
  }
  else if (rpmCur > rpmSet) {
    pwmSet -= 1;
    if (pwmSet < 1) {
       pwmSet = 1;
    }
    motorRun(pwmSet);
  }
}
void loop1(){
  // 100ms loop
  
  if (rpmCount > 4) {
    rpmCur = rpmCount * 150;
    rpmCount = 0;
  }
  else {
    rpmCur = 0;
  }

  if (b_motor) {
    updateMotorSpeed();
  } 
  else {
    // Stop the motor
    motorStop();
  }
}

void loop2(){
  // 1 second loop

  if ((current_screen == 0) && (run_mode == 0) && (rpmCur != rpmLast)) {
    rpmLast = rpmCur;
    drawRpm();
  }

  if ((current_screen == 0) && (run_mode == 1) && (turns != turnsLast)) {
    turnsLast = turns;
    drawTurns();
  }
}


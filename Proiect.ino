#include <Keypad.h>
#include <Wire.h>
#include <Servo.h>
#include <LiquidCrystal_I2C.h>

#define S0 9
#define S1 10
#define S2 11
#define S3 12
#define SENSOR_OUT 13
#define SERVO_MONEY_PIN 5
#define SERVO_RESET_PIN 4

const int ROW_NUM = 4; //four rows
const int COLUMN_NUM = 3; //three columns

//pozitiile de start a servomotoarelor
#define SERVO_MONEY_START_POS 125
#define SERVO_RESET_START_POS 100

Servo servo_money;
Servo servo_reset;
LiquidCrystal_I2C lcd(0x27, 16, 2);

char keys[ROW_NUM][COLUMN_NUM] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};

byte pin_rows[ROW_NUM] = {8, 7, 6, A3}; //connect to the row pinouts of the keypad
byte pin_column[COLUMN_NUM] = {A2, A1, A0}; //connect to the column pinouts of the keypad

Keypad keypad = Keypad( makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM );

const String reset_password = "0000"; // change your password here
const String activate_password = "1234";
String input_password;

//variabilele folosite in program
int in_servo = SERVO_MONEY_START_POS; //pozitia de start a servomotorului unde se vor afla intensitatea bancnotelor
int out_servo = SERVO_RESET_START_POS; //pozitia de start a servomotorului unde se vor depozita bancnotele
int activate_sensor = 0; //activeaza senzorul de culoare
int reset = 0; //activeaza procesul de reset
int move_money = 0; //activeaza servomotorul pentru a deplasa bancnota
int move_reset = 0; //activeaza servomotorul pentru a oferi acces la bancnote
int direction_money = 0; //schimba directia de miscare a servomotorului
int direction_reset = 0; //schimba directia de miscare a servomotorului
int red_freq = 0; //frecventa pe R
int green_freq = 0; //frecventa pe G
int blue_freq = 0; //frecventa pe B
int money_value = 0; //valoarea bancnotei
int sum = 0; //suma totala

void setup(){
  Serial.begin(9600);
  input_password.reserve(32); // maximum input characters is 33, change if needed

  //servomotor setup
  servo_money.attach(SERVO_MONEY_PIN);
  servo_money.write(0);
  servo_reset.attach(SERVO_RESET_PIN);
  servo_reset.write(0);

  //pun servomotoarele in pozitiile initiale
  delay(150);
  servo_money.write(in_servo);
  servo_reset.write(out_servo);

  //color sensor setup
  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);

  pinMode(SENSOR_OUT, INPUT);

  digitalWrite(S0, HIGH);
  digitalWrite(S1, LOW);

  lcd.init();
  lcd.begin(16,2);
  lcd.backlight();

  lcd.clear();
  lcd.print("Total: ");
  lcd.print(sum);
  lcd.setCursor(0, 1);
  lcd.print("Insert Bill");
}

void loop(){
  char key = keypad.getKey();

  servo_money.write(in_servo);
  servo_reset.write(out_servo);
  delay(50);

  if (key){
    Serial.println(key);

    if(key == '*') {
      input_password = ""; // clear input password
    } else if(key == '#') {
      if (reset_password == input_password) {
        Serial.println("reset password is correct");
        sum = 0;
        lcd.clear();
        lcd.print("Total: ");
        lcd.print(sum);  
        lcd.setCursor(0, 1);
        lcd.print("Reset");

        //incepe miscarea servomotorului care ofera acces la zona de depozitare
        move_reset = 1;
        reset = 0;
      } else if (activate_password == input_password){
        Serial.println("activate password is correct");
        lcd.clear();
        lcd.print("Total: ");
        lcd.print(sum);
        lcd.setCursor(0, 1);
        lcd.print("Please Wait");

        //scanez bancnota pentru a afla intensitatea RGB si apoi determin tipul
        red_freq = get_red_frequency();
        green_freq = get_green_frequency();
        blue_freq = get_blue_frequency();
        Serial.println("codul bancnotelor este R ");
        Serial.print(red_freq);
        Serial.println(" G ");
        Serial.print(green_freq);
        Serial.println(" B ");
        Serial.print(blue_freq);
        money_value = check_money_type();
        delay(1000);

        //actualizez suma si afisez mesajele corespunzatoare
        sum += money_value;
        lcd.clear();
        lcd.print("Total: ");
        lcd.print(sum);
        lcd.setCursor(0, 1);
        lcd.print("Please Wait");

        //incepe miscarea servomotorului care muta bancnota in zona de depozitare
        move_money = 1;
        activate_sensor = 0;
      } else {
        Serial.println("password is incorrect, try again");
      }

      input_password = ""; // clear input password
      } else  {
        input_password += key; // append new character to input password string
      }
    }

  if (move_money == 1) {
      //voi misca servomotorul treptat in pozitia finala, iar apoi va ajunge in pozitia initiala
      //dupa ce bancnota va ajunge in zona de depozitare

      if ((in_servo <=  SERVO_MONEY_START_POS) && (direction_money == 0)) {
        in_servo -= 10;
      }
      if ((in_servo <=  SERVO_MONEY_START_POS) && (direction_money == 1)) {
        in_servo += 10;
      }
  
      if (in_servo ==  SERVO_MONEY_START_POS) {
        //servomotorul a ajuns in pozitia initiala si bancnota in zona de depozitare
        move_money = 0;

        //afisez mesajele corespunzatoare pe lcd
        lcd.clear();
        lcd.print("Total: ");
        lcd.print(sum);
        lcd.setCursor(0, 1);
        lcd.print("Insert Bill");
      
        direction_money = 0;
      }
      if (in_servo <= 5) {
        //schimb directia de miscare dupa ce a ajuns in pozitia finala
        direction_money = 1;
        delay(100);
      }
  }

  if (move_reset == 1) {
    //voi misca servomotorul treptat in pozitia finala, iar apoi va ajunge in pozitia initiala
    //dupa ce bancnotele vor fi extrase din zona de depozitare
    if ((out_servo >=  SERVO_RESET_START_POS) && (direction_reset == 0)) {
        out_servo += 10;
      }
      if ((out_servo >=  SERVO_RESET_START_POS) && (direction_reset == 1)) {
        out_servo -= 10;
      }
  
      if (out_servo ==  SERVO_RESET_START_POS) {
        //servomotorul a ajuns in pozitia initiala si bancnotele nu mai sunt in zona de depozitare
        move_reset = 0;

        //afisez mesajele corespunzatoare pe lcd
        lcd.clear();
        lcd.print("Total: ");
        lcd.print(sum);
        lcd.setCursor(0, 1);
        lcd.print("Insert Bill");
        
        direction_reset = 0;
      }
      if (out_servo >= 175) {
        //schimb directia de miscare dupa ce a ajuns in pozitia finala
        direction_reset = 1;
        delay(15000); //astept 15 sec
      }
  }
}

int get_red_frequency() { //aflu frecventa pe R, facand media a 10 valori
  int freq = 0;
  
  digitalWrite(S2, LOW);
  digitalWrite(S3, LOW);
  
  for (int i = 0; i < 10; i++) {
    freq += pulseIn(SENSOR_OUT, LOW);
  }
  
  return freq / 10;
}

int get_green_frequency() { //aflu frecventa pe G, facand media a 10 valori
  int freq = 0;
  
  digitalWrite(S2, HIGH);
  digitalWrite(S3, HIGH);
  
  for (int i = 0; i < 10; i++) {
    freq += pulseIn(SENSOR_OUT, LOW);
  }
  
  return freq / 10;
}

int get_blue_frequency() { //aflu frecventa pe B, facand media a 10 valori
  int freq = 0;
  
  digitalWrite(S2, LOW);
  digitalWrite(S3, HIGH);
  for (int i = 0; i < 10; i++) {
    freq += pulseIn(SENSOR_OUT, LOW);
  }
  
  return freq / 10;
}

int check_money_type(){
  //raza de valori este determinata experimental pentru fiecare bancnota in toate pozitiile in care o bancnota poate fi introdusa
  int money_type = 0;
  
  if ((red_freq >= 105) && (red_freq <= 108) && (green_freq >= 100) && (green_freq <= 105) && (blue_freq >= 78) && (blue_freq <= 81)) {
    money_type = 1;
  }
  if ((red_freq >= 96) && (red_freq <= 100) && (green_freq >= 91) && (green_freq <= 97) && (blue_freq >= 71) && (blue_freq <= 76)) {
    money_type = 1;
  }
  if ((red_freq >= 105) && (red_freq <= 109) && (green_freq >= 100) && (green_freq <= 106) && (blue_freq >= 80) && (blue_freq <= 84)) {
    money_type = 1;
  }
  if ((red_freq >= 91) && (red_freq <= 98) && (green_freq >= 89) && (green_freq <= 96) && (blue_freq >= 70) && (blue_freq <= 75)) {
    money_type = 1;
  }
  
  if ((red_freq >= 104) && (red_freq <= 107) && (green_freq >= 103) && (green_freq <= 106) && (blue_freq >= 75) && (blue_freq <= 79)) {
    money_type = 5;
  }
  if ((red_freq >= 97) && (red_freq <= 99) && (green_freq >= 93) && (green_freq <= 98) && (blue_freq >= 70) && (blue_freq <= 74)) {
    money_type = 5;
  }
  if ((red_freq >= 104) && (red_freq <= 106) && (green_freq >= 101) && (green_freq <= 104) && (blue_freq >= 75) && (blue_freq <= 78)) {
    money_type = 5;
  }
  if ((red_freq >= 96) && (red_freq <= 100) && (green_freq >= 95) && (green_freq <= 98) && (blue_freq >= 72) && (blue_freq <= 74)) {
    money_type = 5;
  }

  if ((red_freq >= 104) && (red_freq <= 109) && (green_freq >= 100) && (green_freq <= 106) && (blue_freq >= 77) && (blue_freq <= 81)) {
    money_type = 10;
  }
  if ((red_freq >= 90) && (red_freq <= 95) && (green_freq >= 90) && (green_freq <= 96) && (blue_freq >= 70) && (blue_freq <= 74)) {
    money_type = 10;
  }
  if ((red_freq >= 106) && (red_freq <= 109) && (green_freq >= 104) && (green_freq <= 109) && (blue_freq >=78) && (blue_freq <= 82)) {
    money_type = 10;
  }
  if ((red_freq >= 92) && (red_freq <= 96) && (green_freq >= 92) && (green_freq <= 97) && (blue_freq >= 71) && (blue_freq <= 74)) {
    money_type = 10;
  }

  if ((red_freq >= 111) && (red_freq <= 113) && (green_freq >= 105) && (green_freq <= 107) && (blue_freq >= 78) && (blue_freq <= 80)) {
    money_type = 20;
  }
  if ((red_freq >= 105) && (red_freq <= 108) && (green_freq >= 99) && (green_freq <= 102) && (blue_freq >= 74) && (blue_freq <= 76)) {
    money_type = 20;
  }
  if ((red_freq >= 112) && (red_freq <= 114) && (green_freq >= 106) && (green_freq <= 108) && (blue_freq >= 78) && (blue_freq <= 81)) {
    money_type = 20;
  }
  if ((red_freq >= 107) && (red_freq <= 109) && (green_freq >= 101) && (green_freq <= 103) && (blue_freq >= 74) && (blue_freq <= 77)) {
    money_type = 20;
  }

  if ((red_freq >= 108) && (red_freq <= 110) && (green_freq >= 105) && (green_freq <= 107) && (blue_freq >= 80) && (blue_freq <= 82)) {
    money_type = 50;
  }
  if ((red_freq >= 93) && (red_freq <= 97) && (green_freq >= 93) && (green_freq <= 97) && (blue_freq >= 73) && (blue_freq <= 77)) {
    money_type = 50;
  }
  if ((red_freq >= 104) && (red_freq <= 107) && (green_freq >= 102) && (green_freq <= 105) && (blue_freq >= 80) && (blue_freq <= 83)) {
    money_type = 50;
  }
  if ((red_freq >= 94) && (red_freq <= 96) && (green_freq >= 92) && (green_freq <= 96) && (blue_freq >= 72) && (blue_freq <= 74)) {
    money_type = 50;
  } 

  if ((red_freq >= 114) && (red_freq <= 118) && (green_freq >= 108) && (green_freq <= 112) && (blue_freq >= 80) && (blue_freq <= 82)) {
    money_type = 100;
  }
  if ((red_freq >= 101) && (red_freq <= 105) && (green_freq >= 95) && (green_freq <= 98) && (blue_freq >= 70) && (blue_freq <= 73)) {
    money_type = 100;
  }
  if ((red_freq >= 111) && (red_freq <= 114) && (green_freq >= 106) && (green_freq <= 111) && (blue_freq >= 81) && (blue_freq <= 84)) {
    money_type = 100;
  }
  if ((red_freq >= 98) && (red_freq <= 101) && (green_freq >= 93) && (green_freq <= 95) && (blue_freq >= 69) && (blue_freq <= 72)) {
    money_type = 100;
  }
  if ((red_freq >= 200) && (red_freq <= 300) && (green_freq >= 200) && (green_freq <= 350) && (blue_freq >= 200) && (blue_freq <= 250)) {
    money_type = 100;
  }

  if ((red_freq >= 110) && (red_freq <= 113) && (green_freq >= 108) && (green_freq <= 111) && (blue_freq >= 83) && (blue_freq <= 85)) {
    money_type = 200;
  }
  if ((red_freq >= 94) && (red_freq <= 97) && (green_freq >= 95) && (green_freq <= 97) && (blue_freq >= 74) && (blue_freq <= 76)) {
    money_type = 200;
  }
  if ((red_freq >= 110) && (red_freq <= 113) && (green_freq >= 110) && (green_freq <= 114) && (blue_freq >= 84) && (blue_freq <= 87)) {
    money_type = 200;
  }
  if ((red_freq >= 90) && (red_freq <= 92) && (green_freq >= 90) && (green_freq <= 93) && (blue_freq >= 71) && (blue_freq <= 73)) {
    money_type = 200;
  }

  if ((red_freq >= 111) && (red_freq <= 114) && (green_freq >= 106) && (green_freq <= 109) && (blue_freq >= 79) && (blue_freq <= 80)) {
    money_type = 500;
  }
  if ((red_freq >= 90) && (red_freq <= 92) && (green_freq >= 86) && (green_freq <= 88) && (blue_freq >= 67) && (blue_freq <= 68)) {
    money_type = 500;
  }
  if ((red_freq >= 104) && (red_freq <= 106) && (green_freq >= 96) && (green_freq <= 98) && (blue_freq >= 71) && (blue_freq <= 73)) {
    money_type = 500;
  }
  if ((red_freq >= 90) && (red_freq <= 92) && (green_freq >= 86) && (green_freq <= 89) && (blue_freq >= 67) && (blue_freq <= 68)) {
    money_type = 500;
  }

  return money_type;
}
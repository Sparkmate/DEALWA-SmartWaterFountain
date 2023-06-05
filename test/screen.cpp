/*************************************************** 
  This is an example for our Adafruit 16-channel PWM & Servo driver
  PWM test - this will drive 16 PWMs in a "wave"

  Pick one up today in the adafruit shop!
  ------> http://www.adafruit.com/products/815

  These drivers use I2C to communicate, 2 pins are required to  
  interface.

  Adafruit invests time and resources providing this open source code, 
  please support Adafruit and open-source hardware by purchasing 
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.  
  BSD license, all text above must be included in any redistribution
 ****************************************************/

#include <Wire.h>
#include <TimeLib.h>
#include <Adafruit_NeoPixel.h>

#include <iostream>  
#include <string>

#include <Arduino_GFX_Library.h>

#define num_compartments  8
#define light_pin GPIO_NUM_34 //Will need to check this
#define num_leds 100

#define TFT_SCK    18
#define TFT_MOSI   23
#define TFT_MISO   19
#define TFT_CS     15
#define TFT_DC     2
#define TFT_RESET  17

#define screen_width 330
#define screen_height 240

// called this way, it uses the default address 0x40
Adafruit_NeoPixel pixels(num_leds, light_pin, NEO_GRB + NEO_KHZ800);

Arduino_ILI9341 display = Arduino_ILI9341(&bus, TFT_RESET);

// you can also call it with a different address you want
//Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x41);
// you can also call it with a different address and I2C interface
//Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40, Wire);

TaskHandle_t user_input;
TaskHandle_t system_control;
TaskHandle_t update_display;

/*
Notes:
PWM 0 - 7 -> Fan Driver
PWM 8 - 15 -> Locks
Will need a light controller
Humidity Readings from Multiplexer
Relay for compressor - Probably not code
*/

int max_pwm = 4095;
int value = 0;
int desired_humidity = 80;
float temperature = 0;
float humidity = 0;
bool locks[num_compartments] = {false};
int desired_temp_lower[num_compartments] = {20, 20, 20, 20, 20, 20, 20, 20};
int desired_temp_upper[num_compartments] = {22, 22, 22, 22, 22, 22, 22, 22};
float current_temp[num_compartments] = {};
float current_humidity[num_compartments] = {};
bool humidity_low[num_compartments] = {false};
int time_delay = 0;
int time_increase = 60;

int r = 0;
int g = 0;
int b = 0;

void send_message(){
  Serial.println("What action would you like to perform?");
  Serial.print("Lock Compartment : 0");
  Serial.print("\t Change Lights : 1");
  Serial.print("\t Change Desired Compartment Temperature : 2");
  Serial.print("\t Change Desired Humidity : 3");
  Serial.println();
}

void select_compartment(int compartment_number){
  Wire.beginTransmission(0x70);
  Wire.write(1 << compartment_number);
  Wire.endTransmission();
}

void fan_speed(int compartment, float temperature){
  if (temperature + 80<= desired_temp_lower[compartment] || !locks[compartment]){
    pwm.setPWM(compartment, 0, 0); //OFF
  }
  else if (temperature + 80>= desired_temp_upper[compartment] && locks[compartment]){
    Serial.println("Fan on");
    pwm.setPWM(compartment, 0, max_pwm); // Fully on
  }
}

void check_humidity(int compartment, float humidity){
  if (humidity < desired_humidity){
    humidity_low[compartment] = true;
  }
}

bool check_true(bool array[]){
  for (int i = 0; i < num_compartments; i++){
    if (!array[i]){
      return false;
    }
  }
  return true;
}

void increase_humidity(){
  if (check_true(humidity_low) && now() > time_delay && check_true(locks)){
    Serial.println("Would inrease humidity"); // Replace this
    time_delay = now() + time_increase;
  }
}

void lock(int compartment, bool action){
  int output = 0;
  if (action){
    Serial.println("Locked");
    output = pwm.setPWM(compartment + num_compartments, 0, max_pwm);
  }
  else{
    Serial.println("Unlocked");
    output = pwm.setPWM(compartment + num_compartments, 0, 0);
  }
  locks[compartment] = action;
  Serial.println(output);
}


int wait_for_response(){
  while (Serial.available() == 0){vTaskDelay(10);}
  String output = "";
  while (Serial.available() != 0){
    int value = (int) Serial.read();
    output += String(value - 48);
    delay(10);
  }

  int result = output.toInt();
  return result ;
}

void draw_data(int compartment){
  int start_x = 0;
  int start_y = (compartment%3)*screen_height/3;
  
  if (compartment == num_compartments){
    start_x = screen_width/3;
    start_y = screen_height/3;

    display.setCursor(10 + start_x, 10 + start_y);
    display.print("Desired Hum: ");
    display.print(desired_humidity);
    display.setCursor(10 + start_x, 20 + start_y);
    display.print("Red: ");
    display.print(r);
    display.setCursor(10 + start_x, 30 + start_y);
    display.print("Green: ");
    display.print(g);
    display.setCursor(10 + start_x, 40 + start_y);
    display.print("Blue: ");
    display.print(b);
  }

  else {
    float temperature = current_temp[compartment];
    float humidity = current_humidity[compartment];
    int low_temp = desired_temp_lower[compartment];
    int high_temp = desired_temp_upper[compartment];
    bool lock = locks[compartment];

    if (compartment > 2 && compartment <= 4){
      start_x = screen_width/3;
      if (compartment == 4){
        start_y = 2*screen_height/3;
      }
    }
    else if (compartment > 4){
      start_x = 2 * screen_width/3;
      start_y = (compartment-5)%3*screen_height/3;
    }

    display.setCursor(10 + start_x, 10 + start_y);
    display.print("Temp: ");
    display.print(temperature);
    display.setCursor(10 + start_x, 20 + start_y);
    display.print("Hum: ");
    display.print(humidity);
    display.setCursor(10 + start_x, 30 + start_y);
    display.print("Low Temp: ");
    display.print(low_temp);
    display.setCursor(10 + start_x, 40 + start_y);
    display.print("High Temp: ");
    display.print(high_temp);
    display.setCursor(10 + start_x, 50 + start_y);
    display.print("Lock: ");
    display.print(lock);
  }
}

void draw_screen(){
  display.fillScreen(WHITE);
  delay(1000);
  display.drawRect(0, 0, screen_width/3, screen_height/3, BLUE);
  display.drawRect(screen_width/3, 0, screen_width/3, screen_height/3, BLUE);
  display.drawRect(2*screen_width/3, 0, screen_width/3, screen_height/3, BLUE);
  display.drawRect(0, screen_height/3, screen_width/3, screen_height/3, BLUE);
  display.drawRect(screen_width/3, screen_height/3, screen_width/3, screen_height/3, BLUE);
  display.drawRect(2*screen_width/3, screen_height/3, screen_width/3, screen_height/3, BLUE);
  display.drawRect(0, 2*screen_height/3, screen_width/3, screen_height/3, BLUE);
  display.drawRect(screen_width/3, 2*screen_height/3, screen_width/3, screen_height/3, BLUE);
  display.drawRect(2*screen_width/3, 2*screen_height/3, screen_width/3, screen_height/3, BLUE);

  for (int i = 0; i < num_compartments + 1; i++){
    draw_data(i);
  }
}

void run_message_code(void * parameter){
  for(;;){
    check_message();
    Serial.println("Checking for input");
    vTaskDelay(1000);
  }
}

void run_display_update(void * paramter){
  for(;;){
    Serial.println("Update Screen");
    draw_screen();
    vTaskDelay(5000);
  }
}

void setup() {
  Serial.begin(115200);
  pixels.begin();

  display.begin();
  display.fillScreen(WHITE);
  display.setRotation(3);
  display.setTextSize(1);
  display.setTextColor(BLUE);

  draw_screen();
}

void loop() {
}

/*******************************************************************************
 * Start of Arduino_GFX setting
 *
 * Arduino_GFX try to find the settings depends on selected board in Arduino IDE
 * Or you can define the display dev kit not in the board list
 * Defalult pin list for non display dev kit:
 * Arduino Nano, Micro and more: CS:  9, DC:  8, RST:  7, BL:  6, SCK: 13, MOSI: 11, MISO: 12
 * ESP32 various dev board     : CS:  5, DC: 27, RST: 33, BL: 22, SCK: 18, MOSI: 23, MISO: nil
 * ESP32-C3 various dev board  : CS:  7, DC:  2, RST:  1, BL:  3, SCK:  4, MOSI:  6, MISO: nil
 * ESP32-S2 various dev board  : CS: 34, DC: 38, RST: 33, BL: 21, SCK: 36, MOSI: 35, MISO: nil
 * ESP32-S3 various dev board  : CS: 40, DC: 41, RST: 42, BL: 48, SCK: 36, MOSI: 35, MISO: nil
 * ESP8266 various dev board   : CS: 15, DC:  4, RST:  2, BL:  5, SCK: 14, MOSI: 13, MISO: 12
 * Raspberry Pi Pico dev board : CS: 17, DC: 27, RST: 26, BL: 28, SCK: 18, MOSI: 19, MISO: 16
 * RTL8720 BW16 old patch core : CS: 18, DC: 17, RST:  2, BL: 23, SCK: 19, MOSI: 21, MISO: 20
 * RTL8720_BW16 Official core  : CS:  9, DC:  8, RST:  6, BL:  3, SCK: 10, MOSI: 12, MISO: 11
 * RTL8722 dev board           : CS: 18, DC: 17, RST: 22, BL: 23, SCK: 13, MOSI: 11, MISO: 12
 * RTL8722_mini dev board      : CS: 12, DC: 14, RST: 15, BL: 13, SCK: 11, MOSI:  9, MISO: 10
 * Seeeduino XIAO dev board    : CS:  3, DC:  2, RST:  1, BL:  0, SCK:  8, MOSI: 10, MISO:  9
 * Teensy 4.1 dev board        : CS: 39, DC: 41, RST: 40, BL: 22, SCK: 13, MOSI: 11, MISO: 12
 ******************************************************************************/
#include <Arduino_GFX_Library.h>

#define GFX_BL DF_GFX_BL // default backlight pin, you may replace DF_GFX_BL to actual backlight pin

/* More dev device declaration: https://github.com/moononournation/Arduino_GFX/wiki/Dev-Device-Declaration */
#if defined(DISPLAY_DEV_KIT)
Arduino_GFX *gfx = create_default_Arduino_GFX();
#else /* !defined(DISPLAY_DEV_KIT) */

/* More data bus class: https://github.com/moononournation/Arduino_GFX/wiki/Data-Bus-Class */
Arduino_DataBus *bus = create_default_Arduino_DataBus();

/* More display class: https://github.com/moononournation/Arduino_GFX/wiki/Display-Class */
Arduino_GFX *gfx = new Arduino_ILI9341(bus, DF_GFX_RST, 0 /* rotation */, false /* IPS */);

#endif /* !defined(DISPLAY_DEV_KIT) */
/*******************************************************************************
 * End of Arduino_GFX setting
 ******************************************************************************/

void setup(void)
{
#ifdef GFX_EXTRA_PRE_INIT
  GFX_EXTRA_PRE_INIT();
#endif

  // Init Display
  if (!gfx->begin())
  {
    Serial.println("gfx->begin() failed!");
  }
  gfx->fillScreen(BLACK);

#ifdef GFX_BL
  pinMode(GFX_BL, OUTPUT);
  digitalWrite(GFX_BL, HIGH);
#endif

  gfx->setCursor(10, 10);
  gfx->setTextColor(RED);
  gfx->println("Hello World!");

  delay(5000); // 5 seconds
}

void loop()
{
  gfx->setCursor(random(gfx->width()), random(gfx->height()));
  gfx->setTextColor(random(0xffff), random(0xffff));
  gfx->setTextSize(random(6) /* x scale */, random(6) /* y scale */, random(2) /* pixel_margin */);
  gfx->println("Hello World!");

  delay(1000); // 1 second
}
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

float rof = 0;
long rof_t0 = 0;
long rof_tprev = 0;
int rof_count = 0;
float avg_fps = 0;
float shot_num = 0;
int disp_rot = 0;
long g1_time = 0;
long g2_time = 0;
long g1_slow_time = 0;
long g2_slow_time = 0;
long now = 0;
long display_time = 0;
bool g1_trip = false;
bool g2_trip = false;
bool g1_trip0 = false;
bool  g2_trip0 = false;
bool g1_trip_latch = false;
bool g2_trip_latch = false;
int g1_persist = 0;
int g2_persist = 0;
int g1_pin = 2;
int g2_pin = 3; // This is our input pin
int latch_persist = 2;
float gate_dist = 51.0; //distance in millimeters
float fps = 0;
String fpstr = "";
//oled screen dimensions.
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup() {
  pinMode(10,OUTPUT);
  // put your setup code here, to run once:
  Serial.begin(1000000);
  pinMode(2, INPUT);
  pinMode(3, INPUT);
  attachInterrupt(0,gate1,RISING);
  attachInterrupt(1,gate2,RISING);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  display.display();
  delay(3000); // Pause for 2 seconds
  display.clearDisplay();
  display.setRotation(0);
  display.setTextSize(1);             // Normal 1:1 pixel scale
  display.setTextColor(WHITE);        // Draw white text
  display.setCursor(0,0);
  if(disp_rot){
    display_vert();
  }
  else{
    display_horz();
  }
}

void loop() {
  timer();
  if((now-display_time) > 50000){
    display_time = now;
    g1_trip0 = g1_trip;
    g2_trip0 = g2_trip;
    g1_trip = !digitalRead(2);
    g2_trip = !digitalRead(3);
    
    g1_persist += g1_trip*latch_persist - !g1_trip;
    g1_persist = constrain(g1_persist,0,latch_persist);
    
    g2_persist += g2_trip*latch_persist - !g2_trip;
    g2_persist = constrain(g2_persist,0,latch_persist);

    if((g2_trip) && (!g2_trip0)){
      g2_slow_time = millis();
    }
    
    if((!g2_trip) && (g2_trip0)){
      if( (now/1000 - g2_slow_time) >500){
        disp_rot++;
        disp_rot = disp_rot%2;
        g1_persist = 0;
        g2_persist = 0;
      }
    }
    if(disp_rot){
      display_vert();
    }
    else{
      display_horz();
    }
  }

  
  g1_persist += g1_trip_latch*latch_persist;
  g2_persist += g2_trip_latch*latch_persist;
  g1_trip_latch = false;
  
  if(g1_persist){
    if(g2_trip_latch){
      Serial.println("MEASURING");
      fps = 3281*gate_dist/(g2_time-g1_time);
      if (fps<0){
        fpstr = "ERR1";
      }
      else if (fps>420){
        fpstr = "ERR2";
      }
      else{
        fpstr = String(int(fps));
        avg_fps = (avg_fps*shot_num+fps)/(shot_num+1);
        shot_num++;
        if((g2_time-rof_tprev)>2000000){
          Serial.println("restarting count");
          rof_t0 = g2_time;
          rof_tprev = g2_time;
          rof_count = 0;
          rof = 0.0;
        }
        else{
          Serial.println("continuing count");
          rof_tprev = g2_time;
          rof_count++;
          rof = float(rof_count)*1000000/float(g2_time-rof_t0);
        }
      }
      
      Serial.println(fps);
      Serial.println(g2_time-g1_time);
      g1_persist = 0;
      g2_persist = 0;
      if(disp_rot){
        display_vert();
      }
      else{
        display_horz();
      }
    }
  }
  g2_trip_latch = false;
}

void gate1(){
  g1_time = micros();
  g1_trip_latch = true;
}

void gate2(){
  g2_time = micros();
  g2_trip_latch = true;
}

void timer(){
  now = micros();
}

void display_vert(){
  display.setRotation(1);
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(12,10);
  display.print("FPS: ");
  display.setCursor(12,30);
  display.print(fpstr);
  display.setTextSize(1);
  display.setCursor(10,50);
  display.print("SHT#: ");
  display.print(int(shot_num));
  display.setCursor(10,60);
  display.print("AVGF: ");
  display.print(int(avg_fps));
  display.setCursor(10,70);
  display.print("RPS: ");
  char buffer[5];
  String srof = dtostrf(rof, 2, 1, buffer);
  display.print(srof);
  if(g1_trip){
    display.setCursor(15,105);
    display.print("GATE 1");
  }
  if(g2_trip){
    display.setCursor(15,115);
    display.print("GATE 2");
  }
  display.display();
}

void display_horz(){
  display.setRotation(0);
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(12,12);
  display.print("FPS: ");
  display.print(fpstr);
  display.setTextSize(1);
  display.setCursor(54,45);
  display.print("SHOT#: ");
  display.print(int(shot_num));
  display.setCursor(60,35);
  display.print("AVGF: ");
  display.print(int(avg_fps));
  display.setCursor(66,55);
  display.print("RPS: ");
  char buffer[5];
  String srof = dtostrf(rof, 2, 1, buffer);
  display.print(srof);
  if(g1_trip){
    display.setCursor(2,40);
    display.print("GATE 1");
  }
  if(g2_trip){
    display.setCursor(2,53);
    display.print("GATE 2");
  }
  display.display();
}

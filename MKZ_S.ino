/*
 * Copyright (c) 2015, Majenko Technologies
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 * 
 * * Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 * 
 * * Neither the name of Majenko Technologies nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* Create a WiFi access point and provide a web server on it. */

#include <ESP8266WiFi.h>
#include <WiFiClient.h> 
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <FS.h>

/* Set these to your desired credentials. */
//const char *ssid = "MKZ-S_2400**"; 起動時にSWをB->Dに2秒してBに戻すと、SSID同じブラウザ画面で変更可能とした
const char *password = "";
//const char command_offset[6]={'o','f','f','s','e','t'};　offsetはソフト設定でなく物理的なボリュームでセンターをリアルタイムに変更可能とした
const char command_left[4]={'l','e','f','t'};  // 左右微調整は別途MKZ4WK等で通信で設定必要
const char command_right[5]={'r','i','g','h','t'};


//left+space+角度[ENTER]　//初期値：65
//left 60 //左ステアリング角度を深くする場合
//left 70 //左ステアリング角度を浅くする場合

//right+space+角度[ENTER]　//初期値：110
//right 105 //右ステアリング角度を浅くする場合
//right 115 //右ステアリング角度を深くする場合


ESP8266WebServer server(80);
ESP8266WebServer server_8080(8080);

//
// モード切り替えピン
const int MODE_PIN = 0; // GPIO0 通常動作モードとSSID設定モード切替に設定

// SSID設定保存ファイル
const char* settings = "/ssid_settings.txt";

// サーバモードでのパスワード
const String pass = "";
//

//DRV8835 port assign
#define AIN1 4
#define AIN2 14
//#define BIN1 13　モータードライバ2ch目用
//#define BIN2 5　モータードライバ2ch目用
#define WAIT 1
#define WAITS 10
#define WAIT2 100 //300


unsigned int pwm[]={0,10,50,100};  //大きい方が遅い
unsigned char s = 0;
unsigned char drive_mode = 1;


#define command_start  0
#define command_stop   1
#define command_back  2

char state = command_stop;

#define LED_H       (digitalWrite( 12, HIGH ))
#define LED_L       (digitalWrite( 12, LOW ))

#define eeprom_size   (128)
#define offset_address (0) 
#define left_address   (1)
#define right_address  (2) 


int offset = 0;
int servo_left=65; 
int servo_right=110; 
int val=0;

/**
  ssid設定画面
 */
void handleRootGet() {
  String html = "";
  html += "<h1>SSID Settings</h1>";
  html += "<form method='post'>";
  html += "  <input type='text' name='ssid' placeholder='ssid'><br>";
  html += "  <input type='submit'><br>";
  html += "</form>";
  server.send(200, "text/html", html);
}

void handleRootPost() {
  String ssid = server.arg("ssid");
  //String pass = server.arg("pass");

  File f = SPIFFS.open(settings, "w");
  f.println(ssid);
  f.close();

  String html = "";
  html += "<h1>ssid Settings</h1>";
  html += ssid + "<br>";
  server.send(200, "text/html", html);
}

/**
 * 初期化(SSID設定時のサーバモード)
 */
void setup_server() {

  File f = SPIFFS.open(settings, "r");
  String ssid = f.readStringUntil('\n');
  f.close();

  ssid.trim();

  Serial.println("SSID: " + ssid);


  /* You can remove the password parameter if you want the AP to be open. */
  WiFi.softAP(ssid.c_str(), pass.c_str());

  server.on("/", HTTP_GET, handleRootGet);
  server.on("/", HTTP_POST, handleRootPost);
  server.begin();
  Serial.println("HTTP server started.");
}


String form ="<html>"
"<head>"
"<meta name=\"viewport\" content=\"width=device-width, initial-scale=1, minimum-scale=1, maximum-scale=1\">"
"<style>"
"* { padding: 0; margin: 0; }"
"body { background-color: #0097C1; }"
"</style>"
"</head>"
"<body>"
"<div style=\"position:fixed;top: 50pt; text-align:center; width: 100%; color:white; font-size:300%; font-weight:bold; text-transform:uppercase; font-family:sans-serif\" id=\"value\">connected</div>"
"<form action=\"\" target=\"tif\" id=\"form\">"
"<iframe src=\"javascript: false;\" style=\"display: none;\" name=\"tif\" id=\"tif\"></iframe>"
"</form>"
"<script>"
"var offset = 50;"
"document.body.style.height = document.body.clientHeight + offset + \'px\';"
"document.body.style.width = document.body.clientWidth + offset + \'px\';"
"document.getElementsByTagName(\"html\")[0].style.height = document.body.style.height + \'px\';"
"document.getElementsByTagName(\"html\")[0].style.width = document.body.style.width + \'px\';"
"var moveHomePosition = function() {"
    "document.body.scrollTop = offset / 2;"
    "document.body.scrollLeft = offset / 2;"
"};"
"setTimeout(moveHomePosition, 500);"
"var startX = 0;var startY = 0;var command =\'/stop\';"
"var threshold = 40;"
"var esp_port = \'http://192.168.4.1:8080\';"
"var el_form = document.getElementById(\'form\');"
"document.body.ontouchstart = function(event) {"
    "startX = event.touches[0].clientX;"
    "startY = event.touches[0].clientY;"
"};"
"document.body.ontouchmove = function(event) {"
    "var x = parseInt(event.touches[0].clientX - startX);"
    "var y = parseInt(event.touches[0].clientY - startY);"
    "var url = null;"
    "if (x < (threshold * -1)) {"
       "if (y < (threshold * -1)){"
          "url = \'/left<br>forward\';"
       "} else if (y > threshold) {"
          "url = \'/left<br>back\';"
       "}else {"
        "url = \'/left\';"
       "}"
    "} else if (x > threshold) {"
       "if (y < (threshold * -1)) {"
          "url = \'/right<br>forward\';"
      "} else if (y > threshold) {"
          "url = \'/right<br>back\';"
      "}else{"
          "url = \'/right\';"
      "}"
     "} else {"
        "if (y < (threshold * -1)) {"
            "url = \'/forward\';"
        " } else if (y > threshold) {"
            "url = \'/back\';"
        "}"
     "}"
    "if (command != url) {"
      "if (url) {"
          "el_form.action = esp_port + url.replace(\"<br>\",\"\");"
          "el_form.submit();"
          "document.getElementById(\'value\').innerHTML = url.replace(\"/\",\"\");"
      "}"
    "}"
    "command = url;"
"};"
"document.body.ontouchend = function(event) {"
    "el_form.action = esp_port + \'/stop\';"
    "el_form.submit();"
    "setTimeout(moveHomePosition, 50);"
   "document.getElementById(\'value\').innerHTML = \'stop\';"
"};"
"</script>"
"</body>"
"</html>";

/* Just a little test message.  Go to http://192.168.4.1 in a web browser
 * connected to this access point to see it.
 */

//Motor DriverDRV8835 Control
/*
    drive mode0 : stop
    drive mode1 : forward
    drive mode2 : back
*/
void DRV8835_Control(){
    if (drive_mode == 0){
      
      analogWrite(AIN1,1023); 
      analogWrite(AIN2,1023);
      delay(100);
    }
    if (drive_mode == 1){
     
      for (int i=255; i <pwm[s]; i=i-10){
      analogWrite(AIN1,1023);
      analogWrite(AIN2,i);
      delay(WAIT); 
      
      }            
      analogWrite(AIN1,1023);
      analogWrite(AIN2,pwm[s]);
      delay(WAIT2);
    
      drive_mode = 0;
    
    }
    if (drive_mode == 2){
     
      for (int i=255; i <pwm[s]; i=i-10){
      analogWrite(AIN1,i);
      analogWrite(AIN2,1023);
      delay(WAIT);

      }
     
      analogWrite(AIN1,pwm[s]);
      analogWrite(AIN2,1023);
      delay(WAIT2);
    
      drive_mode = 0;

    }


 if (WiFi.status() != WL_CONNECTED) { 
    drive_mode = 0;

    }

}


void setup() {
  
  delay(2000);

  // ファイルシステム初期化、SSIDファイル読み込み
  SPIFFS.begin();

  File f = SPIFFS.open(settings, "r");
  String ssid = f.readStringUntil('\n');
  f.close();

  ssid.trim();
  //Bで起動後2秒間の間にDだったら、SSID設定モードに遷移
  pinMode(MODE_PIN, INPUT);
  if (digitalRead(MODE_PIN) == 0) {
    setup_server();
  } 

  Serial.begin(115200);
  Serial.println();
  Serial.print("Configuring access point...");

  EEPROM.begin(eeprom_size);
  /* You can remove the password parameter if you want the AP to be open. */
  WiFi.softAP(ssid, password);

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
 
  server.on("/", handleRoot);
  server_8080.on("/stop", handle_stop);
  server_8080.on("/forward", handle_forward);
  server_8080.on("/back", handle_back);
  server_8080.on("/left", handle_left);
  server_8080.on("/right", handle_right);
  server_8080.on("/leftforward", handle_f_left);
  server_8080.on("/rightforward", handle_f_right);
  server_8080.on("/leftback", handle_b_left);
  server_8080.on("/rightback", handle_b_right);
 
  server.begin();
  server_8080.begin();
  
  Serial.println("HTTP server started");
  pinMode(16,OUTPUT);
  pinMode(12,OUTPUT);

//AIN1 AIN2
  pinMode(AIN1,OUTPUT);
  pinMode(AIN2,OUTPUT);
  digitalWrite(AIN1, HIGH);
  digitalWrite(AIN2, HIGH);
    
  LED_H;

  vr_calib();
  delay(100);

/*
  if((EEPROM.read(offset_address) != 0)
   && (EEPROM.read(offset_address) != 0xff)){
     offset =(int8)( EEPROM.read(offset_address));
   }

  Serial.print(offset);
  Serial.println();
*/
  if((EEPROM.read(left_address) != 0)
   && (EEPROM.read(left_address) != 0xff)){
    servo_left = EEPROM.read(left_address);
  }
  Serial.print("servo_left:");  
  Serial.print(servo_left);
  Serial.println();
  
  if((EEPROM.read(right_address) != 0)
   && (EEPROM.read(right_address) != 0xff)){
    servo_right = EEPROM.read(right_address);
  }
 Serial.print("servo_right:");   
  Serial.print(servo_right);
  Serial.println();
}

void loop() {
  server.handleClient();
  server_8080.handleClient();



}


void handleRoot() {
  server.send(200, "text/html", form);
}


void handle_stop() {
  Serial.print("stop\r\n");
  
  read_calib_data(); //test
    
  LED_L;
    drive_mode = 0;
    DRV8835_Control();
    state = command_stop;
  LED_H;
  server_8080.send(200, "text/html", "");

}

void vr_calib() {
    int val = analogRead(A0);
    offset=val/16-32;
   // Serial.print("val:");   ボリュームの読み値を調整為たいときは表示
   // Serial.println(val);     
    
    Serial.print("VR offset:");   
    Serial.println(offset);     
}

void handle_forward() {
 vr_calib();

 Serial.print("forward\r\n");

  servo_control(90);  //90+offsetが適用される,先に角度指定してから駆動輪回した方がいい
  server_8080.send(200, "text/html", "");

  drive_mode = 1;
  DRV8835_Control(); 
      drive_mode = 0;
}

void handle_back() {
//  vr_calib(); 
  Serial.print("back\r\n");

  servo_control(90);
  server_8080.send(200, "text/html", "");

  drive_mode = 2;
  DRV8835_Control();
      drive_mode = 0;    
}

void handle_left(){
      drive_mode = 0;
      DRV8835_Control();         
  vr_calib();
  Serial.print("left\r\n");
  servo_control(servo_left);
  server_8080.send(200, "text/html", "");
    Serial.print("offset:"); 
    Serial.println(offset);  
}

void handle_right(){
      drive_mode = 0;
      DRV8835_Control();       
  vr_calib();
  Serial.print("right\r\n");
  servo_control(servo_right);
  server_8080.send(200, "text/html", "");
    Serial.print("offset:"); 
    Serial.println(offset);  
}

void handle_f_left(){
  Serial.print("f_left\r\n");

  servo_control(servo_left);
  server_8080.send(200, "text/html", "");

  drive_mode = 1;
  DRV8835_Control();  
    drive_mode = 0;
}

void handle_f_right(){
  Serial.print("f_right\r\n");

  servo_control(servo_right);
  server_8080.send(200, "text/html", "");

  drive_mode = 1;
  DRV8835_Control();  
    drive_mode = 0;  
}

void handle_b_left(){
  Serial.print("b_left\r\n");

  servo_control(servo_left );
  server_8080.send(200, "text/html", "");

  drive_mode = 2;
  DRV8835_Control(); 
    drive_mode = 0;    
}


void handle_b_right(){
  Serial.print("b_right\r\n");

  servo_control(servo_right);
  server_8080.send(200, "text/html", "");

  drive_mode = 2;
  DRV8835_Control(); 
    drive_mode = 0;  
}



void servo_control(int angle){
int microsec,i;
      LED_L;
      microsec = (5*(angle+offset))+ 1000;
       
      for(i=0; i<20 ;i++){
        digitalWrite( 16, HIGH );
        delayMicroseconds( microsec ); 
        digitalWrite( 16, LOW );
        delayMicroseconds( 10000 - microsec ); 
      }
      LED_H;
}

void read_calib_data(){

  static char buf[128] = {0};
  size_t len = 0;
  static int index = 0;
  
  while(Serial.available()){
      len = Serial.available();
      index += Serial.readBytes(&buf[index],len);
      if(buf[index-2] ==0x0d && buf[index-1] == 0xa){
        parse_command(buf);
        index = 0;
        memset(buf,0,sizeof(buf));  
      }
    }
}

void parse_command(char *buf){
  uint8 data = 0;
/*  
  //offset
  if(strncmp(command_offset,buf,sizeof(command_offset))==0){
    data = atoi(&buf[sizeof(command_offset)+1]);
    offset =(int8)data;
    EEPROM.write(offset_address, data);
    EEPROM.commit();
    Serial.print("save offset\t");
    Serial.print((int8)data);
    Serial.println();
  }
*/
  //left
  if(strncmp(command_left,buf,sizeof(command_left))==0){
    data = atoi(&buf[sizeof(command_left)+1]);
    servo_left = data;
    EEPROM.write(left_address, data);
    EEPROM.commit();
    Serial.print("save left\t");
    Serial.print(data);
    Serial.println();
  }

    //right
  if(strncmp(command_right,buf,sizeof(command_right))==0){
    data = atoi(&buf[sizeof(command_right)+1]);
    servo_right = data;
    EEPROM.write(right_address, data);
    EEPROM.commit();
    Serial.print("save right\t");
    Serial.print(data);
    Serial.println();
  }
}

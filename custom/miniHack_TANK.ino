#include <ESP8266WiFi.h>  
#include <WiFiClient.h> 
#include <ESP8266WebServer.h>

/* Set these to your desired credentials. */
const char *ssid = "miniHack_TANK";
const char *password = "";

ESP8266WebServer server(80);
ESP8266WebServer server_8080(8080);

#define command_start  0
#define command_stop   1
#define command_back  2

#define ANG 45


#define LED_H       (digitalWrite( 12, HIGH ))
#define LED_L       (digitalWrite( 12, LOW ))

//DRV8835 port assign
#define AIN1 4
#define AIN2 14
#define BIN1 13
#define BIN2 5
#define WAIT 1
#define WAIT2 100

//unsigned char drive_mode = 0;
unsigned int pwm[]={0,32,63,192};  //大きい方が遅い
unsigned char s = 2 ;
unsigned char drive_mode = 0;


char state = command_stop;

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

//Motor DriverDRV8835 Control
/*
    drive mode0 : stop
    drive mode1 : forward
    drive mode2 : back
    drive mode3 : Rapid left turn
    drive mode4 : Rapid right turn
    drive mode5 : back left turn
    drive mode6 : back right turn
    drive mode7 : forword left turn
    drive mode8 : forword right turn
*/
void DRV8835_Control(){
    if (drive_mode == 0){
      analogWrite(AIN1,1023);
      analogWrite(AIN2,1023);
      analogWrite(BIN1,1023);
      analogWrite(BIN2,1023);
      delay(100);
    }
    if (drive_mode == 1){

      for (int i=255; i <pwm[s]; i=i-10){
      analogWrite(AIN1,i);
      analogWrite(AIN2,1023);
      analogWrite(BIN1,i);
      analogWrite(BIN2,1023);
      delay(WAIT);
      }      
      analogWrite(AIN1,pwm[s]);
      analogWrite(AIN2,1023);
      analogWrite(BIN1,pwm[s]);
      analogWrite(BIN2,1023);
      delay(WAIT2);
        drive_mode = 0;
    }
    if (drive_mode == 2){
      for (int i=255; i <pwm[s]; i=i-10){
      analogWrite(AIN1,0);
      analogWrite(AIN2,i);
      analogWrite(BIN1,0);
      analogWrite(BIN2,i);
      delay(WAIT);
      }
      analogWrite(AIN1,1023);
      analogWrite(AIN2,pwm[s]);
      analogWrite(BIN1,1023);
      analogWrite(BIN2,pwm[s]);
      delay(WAIT2);
     drive_mode = 0;     
    }
    if (drive_mode == 3){
      for (int i=255; i <pwm[s]; i=i-10){
      analogWrite(AIN1,1023);
      analogWrite(AIN2,i);
      analogWrite(BIN1,i);
      analogWrite(BIN2,1023);
      delay(WAIT);
      }      
      analogWrite(AIN1,1023);
      analogWrite(AIN2,pwm[s]);
      analogWrite(BIN1,pwm[s]);
      analogWrite(BIN2,1023);
      delay(WAIT2);
      drive_mode = 0; 

    }
    if (drive_mode == 4){
      for (int i=255; i <pwm[s]; i=i-10){
      analogWrite(AIN1,i);
      analogWrite(AIN2,1023);
      analogWrite(BIN1,1023);
      analogWrite(BIN2,i);
      delay(WAIT);
      }      
      analogWrite(AIN1,pwm[s]);
      analogWrite(AIN2,1023);
      analogWrite(BIN1,1023);
      analogWrite(BIN2,pwm[s]);
      delay(WAIT2);      
       drive_mode = 0; 
    }


      if (drive_mode == 5){
      for (int i=255; i <pwm[s]; i=i-10){
      analogWrite(AIN1,1023);
      analogWrite(AIN2,i);
      analogWrite(BIN1,1023);
      analogWrite(BIN2,1023);
      delay(WAIT);

      }
      analogWrite(AIN1,1023);
      analogWrite(AIN2,pwm[s]);
      analogWrite(BIN1,1023);
      analogWrite(BIN2,1023);
      delay(WAIT2);
      drive_mode = 0;
    }      
    if (drive_mode == 6){
      for (int i=255; i <pwm[s]; i=i-10){
      analogWrite(AIN1,1023);
      analogWrite(AIN2,1023);
      analogWrite(BIN1,1023);
      analogWrite(BIN2,i);
      delay(WAIT);
      }
      analogWrite(AIN1,1023);
      analogWrite(AIN2,1023);
      analogWrite(BIN1,1023);
      analogWrite(BIN2,pwm[s]);
      delay(WAIT2);  
      drive_mode = 0;  
        
    }

    if (drive_mode == 7){
      for (int i=255; i <pwm[s]; i=i-10){
      analogWrite(AIN1,1023);
      analogWrite(AIN2,1023);
      analogWrite(BIN1,i);
      analogWrite(BIN2,1023);
      delay(WAIT);
      }
      analogWrite(AIN1,1023);
      analogWrite(AIN2,1023);
      analogWrite(BIN1,pwm[s]);
      analogWrite(BIN2,1023);
      delay(WAIT2);    
      drive_mode = 0;  
     
    }
    if (drive_mode == 8){
      for (int i=255; i <pwm[s]; i=i-10){
      analogWrite(AIN1,i);
      analogWrite(AIN2,1023);
      analogWrite(BIN1,1023);
      analogWrite(BIN2,1023);
      delay(WAIT);

      }
      analogWrite(AIN1,pwm[s]);
      analogWrite(AIN2,1023);
      analogWrite(BIN1,1023);
      analogWrite(BIN2,1023);
      delay(WAIT2);
      drive_mode = 0;
    }
 if (WiFi.status() != WL_CONNECTED) { 
    drive_mode = 0;
    }    
}



void setup() {
  delay(1000);
  Serial.begin(115200);
  Serial.println();
  Serial.print("Configuring access point...");

  
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
  //Servo
  pinMode(12,OUTPUT);

  LED_L;
  delay(1000);
  LED_H;  
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
  LED_L;
    servo_control(ANG);

    drive_mode = 0;
    DRV8835_Control();
    state = command_stop;
  LED_H;
  server_8080.send(200, "text/html", "");
}

void handle_forward() {
 Serial.print("forward\r\n");
    servo_control(ANG);    
    drive_mode = 1;
    DRV8835_Control();  
  server_8080.send(200, "text/html", "");
        drive_mode = 0;
}

void handle_back() {
  Serial.print("back\r\n");
    servo_control(ANG);
    drive_mode = 2;
    DRV8835_Control();  
  server_8080.send(200, "text/html", "");
        drive_mode = 0;
}

void handle_left(){
  Serial.print("left\r\n");

  drive_mode = 3;
  DRV8835_Control();
  
  server_8080.send(200, "text/html", "");
        drive_mode = 0;
}

void handle_right(){
  Serial.print("right\r\n");
  drive_mode = 4;
  DRV8835_Control();
  
  server_8080.send(200, "text/html", "");
        drive_mode = 0;
}

void handle_f_left(){
  Serial.print("f_left\r\n");
    servo_control(ANG+70);
    drive_mode = 7;
    DRV8835_Control();

  server_8080.send(200, "text/html", "");
        drive_mode = 0;
}

void handle_f_right(){
  Serial.print("f_right\r\n");
    servo_control(ANG-70);
    drive_mode = 8;
    DRV8835_Control();
      
 server_8080.send(200, "text/html", "");
       drive_mode = 0;
}

void handle_b_left(){
  Serial.print("b_left\r\n");
    servo_control(ANG+70);
    drive_mode = 6;
    DRV8835_Control();

  server_8080.send(200, "text/html", "");
        drive_mode = 0;
}


void handle_b_right(){
  Serial.print("b_right\r\n");
    servo_control(ANG-70);
    drive_mode = 5;
    DRV8835_Control();

  server_8080.send(200, "text/html", "");
        drive_mode = 0;
}

void servo_control(int angle){
int microsec,i;
      LED_L;
      microsec = (5*(angle))+ 1000;
       
      for(i=0; i<20 ;i++){
        digitalWrite( 16, HIGH );
        delayMicroseconds( microsec ); 
        digitalWrite( 16, LOW );
        delayMicroseconds( 10000 - microsec ); 
      }
      LED_H;
}


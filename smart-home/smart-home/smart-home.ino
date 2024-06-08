//library
#include <Servo.h>
#include <WiFi.h>
#include <WiFiAP.h>
#include <ESPAsyncWebServer.h>

//pins
#define gas 35
#define buzzer 14
#define IRSensor 33
#define servoMotor 13
#define lightSensor 39
#define PIRSensor 12
#define flame 34
int relayGPIOs[4] = {15, 4, 5, 18};

//variables

Servo myservo;

const char* ssid      ="INTELLHOME";
const char* password  ="smarthome";

const char* PARAM_INPUT_1 = "relay";  
const char* PARAM_INPUT_2 = "state";

unsigned long previousMillis=0;
const long interval=10000;

AsyncWebServer server(80);

const char home_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
<title>INTELLHOME</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    h2 {font-size: 3.0rem;}
    p {font-size: 3.0rem;}
    body {max-width: 600px; margin:0px auto; padding-bottom: 25px;}
    .switch {position: relative; display: inline-block; width: 120px; height: 68px} 
    .switch input {display: none}
    .slider {position: absolute; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; border-radius: 34px}
    .slider:before {position: absolute; content: ""; height: 52px; width: 52px; left: 8px; bottom: 8px; background-color: #fff; -webkit-transition: .4s; transition: .4s; border-radius: 68px}
    input:checked+.slider {background-color: #2196F3}
    input:checked+.slider:before {-webkit-transform: translateX(52px); -ms-transform: translateX(52px); transform: translateX(52px)}
    .labels{
    font-size: 1.5rem;
    vertical-align:middle;
    padding-bottom: 15px;}
    
  </style>
</head>
<body>
  <h2>light</h2>
  <h4>Room 1</h4><label class="switch"><input type="checkbox" onchange="toggleLight(this)" id="1"><span class="slider"></span></label>
  <h4>Room 2</h4><label class="switch"><input type="checkbox" onchange="toggleLight(this)" id="2"><span class="slider"></span></label>
  <h4>Room 3</h4><label class="switch"><input type="checkbox" onchange="toggleLight(this)" id="3"><span class="slider"></span></label>
  <h4>Room 4</h4><label class="switch"><input type="checkbox" onchange="toggleLight(this)" id="4"><span class="slider"></span></label>
  <p>
    <i style="color:#059e8a;"></i> 
    <span class="labels">GAS :-</span> 
    <span id="gas"></span>
    <span class="perc">%</span>
  </p>
  
<script>function toggleLight(element) {
  var xhr = new XMLHttpRequest();
  if(element.checked){ xhr.open("GET", "/update?relay="+element.id+"&state=1", true); }
  else { xhr.open("GET", "/update?relay="+element.id+"&state=0", true); }
  xhr.send();}
  setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("gas").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/gas", true);
  xhttp.send();
}, 2000 ) ;
</script>
</body>
</html>
)rawliteral";

String ReadGas(){
  int g=analogRead(gas);
  g=map(g,0,4095,0,100);
  if(isnan(g)){
    return "--";
  }
  else{
    return String(g);
  }
}

String Light(){
  int lightsens=digitalRead(lightSensor);  //light sensor
  if(lightsens==0){
   return String(lightsens);
  }
  else{
    return String(lightsens);
  }
}


void setup() {
  
  //serial monitor setup
  Serial.begin(115200);

  //pins setup
  pinMode(gas,INPUT);
  pinMode(buzzer,OUTPUT);
  pinMode(IRSensor,INPUT);
  pinMode(lightSensor,INPUT);
  pinMode(PIRSensor,INPUT);
  pinMode(flame,INPUT);
  myservo.attach(servoMotor);
  for(int i=1; i<=4; i++){
    pinMode(relayGPIOs[i-1], OUTPUT);
    digitalWrite(relayGPIOs[i-1], 1);
  }
  
  //wifi setup
  delay(10);
  if (!WiFi.softAP(ssid, password)) {
    Serial.println("Soft AP creation failed.");
    while(1);
  }
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());
  server.begin();
  Serial.println("Server started");

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", home_html);
  });

  server.on("/gas",HTTP_GET,[](AsyncWebServerRequest *request){
    request->send_P(200,"text/plain",ReadGas().c_str());
  });

  server.on("/auto",HTTP_GET,[](AsyncWebServerRequest *request){
    request->send_P(200,"text/plain",Light().c_str());
    String inputparameter;
    if(request->hasParam("state")){
      inputparameter=request->getParam("state")->value();
      for(int i=0;i<=4;i++){
        digitalWrite(relayGPIOs[i], !inputparameter.toInt());
      }
    }
  });
  server.on("/update", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    String inputParam;
    String inputMessage2;
    String inputParam2;
    if (request->hasParam(PARAM_INPUT_1) & request->hasParam(PARAM_INPUT_2)) {
      inputMessage = request->getParam(PARAM_INPUT_1)->value();
      inputParam = PARAM_INPUT_1;
      inputMessage2 = request->getParam(PARAM_INPUT_2)->value();
      inputParam2 = PARAM_INPUT_2;
      digitalWrite(relayGPIOs[inputMessage.toInt()-1], !inputMessage2.toInt());
     }
    else {
      inputMessage = "No message sent";
      inputParam = "none";
    }
    request->send(200, "text/plain", "OK");
  });
  // Start server
  server.begin();
}

void loop() {
  //gas sensor
  int gasSens =digitalRead(gas);     
  if(gasSens==1){
    for(int i=0;i<=4;i++){
    digitalWrite(buzzer,1);
    delay(500);
    digitalWrite(buzzer,0);
    delay(500);
    }
  }
  else{
    digitalWrite(buzzer,0);
  }                                   
  //IR sensor
  int distance =digitalRead(IRSensor); 
  if(distance==1){
    myservo.write(180);
    delay(30);
    delay(7500);
  }
  else{
    myservo.write(0);
    delay(30);
  }                                   
  //flame sensor
  int flamesens=digitalRead(flame);   
  if(flame==1){
    for(int i=0;i<=3;i++){
    digitalWrite(buzzer,1);
    delay(1000);
    digitalWrite(buzzer,0);
    delay(500);
    }
  }
  else{
    digitalWrite(buzzer,0);
  }                                   
  //PIR sensor
  unsigned long currentMillis=millis();
  if(currentMillis-previousMillis>=interval){
    previousMillis=currentMillis;
    for(int i=0;i<=2;i++){
    digitalWrite(buzzer,1);
    delay(1500);
    digitalWrite(buzzer,0);
    delay(500);
  }}
  else{
    digitalWrite(buzzer,0);
  }
}
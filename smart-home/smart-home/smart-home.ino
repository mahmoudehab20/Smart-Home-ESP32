//library
#include <Servo.h>
#include <WiFi.h>
#include <WiFiAP.h>
#include <ESPAsyncWebServer.h>

//Define pins
#define D0Gas 35
#define A0Gas 32
#define buzzer 14
#define IRSensor 33
#define servoMotor 13
#define lightSensor 39
#define PIRSensor 12
#define flame 34
int relayGPIOs[4] = {15, 4, 5, 18};

//variables
//Servo object
Servo myservo;
//ESP32 WiFi name and password
const char* ssid      ="INTELLHOME";
const char* password  ="smarthome";
//parameters for relay module API
const char* PARAM_INPUT_1 = "relay";  
const char* PARAM_INPUT_2 = "state";
//PIR sensor variables
unsigned long previousMillis=0;
const long interval=5000;
//Server port
AsyncWebServer server(80);
//HTML for web server(optional)
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
  <h4>Auto</h4><label class="switch"><input type="checkbox" onchange="toggleAuto(this)" id="4"><span class="slider"></span></label>
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
function toggleAuto(element){
setInterval(function(){
  var x=new XMLHttpRequest();
  if(element.checked){
    x.open("GET","/auto",true);}
  x.send();
  },2000);
  } 
</script>
</body>
</html>
)rawliteral";
//method to get gas reading to the API
String ReadGas(){
  int g=analogRead(A0Gas);
  g=map(g,0,4095,0,100);
  if(isnan(g)){
    return "--";
  }
  else{
    return String(g);
  }
}
//things that run one time at the startprogram only
void setup() {
  
  //serial monitor setup
  Serial.begin(115200);

  //pins Mode(INPUT/OUTPUT)
  pinMode(D0Gas,INPUT);
  pinMode(A0Gas,INPUT);
  pinMode(buzzer,OUTPUT);
  pinMode(IRSensor,INPUT);
  pinMode(lightSensor,INPUT);
  pinMode(PIRSensor,INPUT_PULLUP);
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
  //API for web server(optional)
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", home_html);
  });
  //API to get gas reading
  server.on("/gas",HTTP_GET,[](AsyncWebServerRequest *request){
    request->send_P(200,"text/plain",ReadGas().c_str());
  });
  //API to control light automatically 
  server.on("/auto",HTTP_GET,[](AsyncWebServerRequest *request){
    int light=digitalRead(lightSensor);
    for(int i=0;i<=4;i++){
      digitalWrite(relayGPIOs[i],!light);
    }
    request->send_P(200,"text/plain","DONE");
  });
  //API for PIR sensor
  server.on("/pir",HTTP_GET,[](AsyncWebServerRequest *request){
    //PIR sensor
    long currentMillis=millis();
    if(currentMillis-previousMillis>=interval && digitalRead(PIRSensor)==1){
      previousMillis=currentMillis;
      for(int i=0;i<=2;i++){
      digitalWrite(buzzer,1);
      delay(1500);
      digitalWrite(buzzer,0);
      delay(500);
      request->send_P(200,"text/plain","1");
    }}
    else{
      digitalWrite(buzzer,0);
      request->send_P(200,"text/plain","0");
    }
  });
  //API to control light
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
  int gasSens =digitalRead(D0Gas);     
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
}

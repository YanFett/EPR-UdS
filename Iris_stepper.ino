#include <SPI.h>
#include <Ethernet.h>

 

#define dirPin 2
#define stepPin 3
#define buttonPin 4
#define buttonPower 5
#define switchPin 6

 

#define stepsPerRevolution 6400
#define delayshort 5
#define delaylong 100

 

// set current position as 0
long pos = 0;
// set state to safe
bool unsafe = false;
// set switch position to off
bool switchpos = false;
// setup server options
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192,168,1,129);//modifying according your own IP
EthernetServer server(80);

 

void setup() {
  // Declare motor step pins as output, declare button as pullup input
  pinMode(stepPin, OUTPUT);
  pinMode(dirPin, OUTPUT);
  pinMode(buttonPower, OUTPUT);
  pinMode(switchPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);
  digitalWrite(buttonPower, LOW);
  digitalWrite(switchPin, LOW);
  //start network and debug in/outputs
  Serial.begin(9600);
  Ethernet.begin(mac, ip);
  //start server
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());
}

 

void loop(){
  // listen for incoming clients
  String readString = "";
  EthernetClient client = server.available();  
  if (client) {

 

    Serial.println("new client");
    while (client.connected()) {
      //Serial.println("start");
      if (client.available()) {
        while(true){
          // read what client wants
          char c = client.read();
          int number = int(c);
          if(number == -1){
             //Serial.println(readString);
             break;
          }
          readString += c;
        }
        String reqtype = readString.substring(0, 4);
        //Serial.println(reqtype);
        //get start of message
        //if get --> print website
        //if post --> handle request
        //if neither something has gone wrong
        if(reqtype == "GET "){
          handle_get(client);
        }else if (reqtype == "POST"){
          handle_post(readString, client);    
        }else{
          handle_kaputt(readString,client);
        }
        
      }
      //after every get/post/other interaction the connection is closed
      client.stop();
    }
  }
}

 

void handle_get(EthernetClient client){
  //print the website
  Serial.println("[server] 200 response send");
  double posstring = double(pos)/double(stepsPerRevolution);
  client.println( ("HTTP/1.0 200 OK\r\n"
    "Content-Type: text/html\r\n" 
    "\r\n" // a blank line to split HTTP header and HTTP body
    "<!doctype html>\n" // the HTTP Page itself
    "<html lang='en'>\n"
    "<head>\n"
    "<meta charset='utf-8'>\n"
    "<meta name='viewport' content='width=device-width'>\n"
    "<title>Iris stepper</title>\n"
    "</head>\n"
    "<body style='font-family:Helvetica, sans-serif'>\n" 
    "<h1>Iris stepper</h1>\n"
    "<p>The current Turn position is "  ));
    client.println(posstring,8);
    client.println(("</p>\n"
    "<p>Zero is the minimum and represents the stop point in the resonator if initialized.</p>\n"
    "<p>Otherwise it is the screw position where the Arduino was started. Unsafe option can override the stop safeguard temporarily.</p>\n"
    "<p>Select number of Turns from current position. Negative numbers go in, positive out of the Resonator.</p>\n"
    "<form method='post' action='/' name='pins'>" 
    "<label for=\"turns\">Turns:</label><br>" // double quotes have to be escaped via /"
    "<input type=\"number\" id=\"turns\" name=\"turns\" value=\"0\" step=\"0.00015625\"><br>" // that number is the smallest stepsize (1/6400)
    "<p> Chooses if the Arduino shall do nothing, initialize, go to unsafe mode for the following turn, flip the VNA/SA switch or return to position 0 before the turn instruction.</p>\n"
    "<input type=\"radio\" id=\"none\" name=\"state\" value=\"none\" checked>"
    "<label for=\"none\">Standard</label><br>"
    "<input type=\"radio\" id=\"init\" name=\"state\" value=\"init\">"
    "<label for=\"init\">Initialize</label><br>"
    "<input type=\"radio\" id=\"unsafe\" name=\"state\" value=\"unsafe\">"
    "<label for=\"unsafe\">Unsafe</label> <br>"
    "<input type=\"radio\" id=\"switch\" name=\"state\" value=\"switch\">"
    "<label for=\"switch\">Switch</label> <br>"
    "<input type=\"radio\" id=\"home\" name=\"state\" value=\"home\">"
    "<label for=\"home\">Return to pos 0</label> <br>"
    "<input type=\"submit\" value=\"Submit\"><br>"
    "</form>\n"
    "</body>\n</html>"));
  return;
}

 

void handle_post(String req, EthernetClient client){
  //mainly parsing the input for turns and state token. Rest is done elsewhere
  int buffsize = 50;
  int i = 0;
  int req_len = req.length()+1;
  char char_req[req_len];
  req.toCharArray(char_req,req_len);
  String found;
  Serial.println(req); //print request
  // split request into lines
  char *ptr[buffsize];
  char separator[] = "\n";
  char *token = strtok(char_req, separator);
  while(token != NULL){
    ptr[i] = token;
    i++;
    token = strtok(NULL, separator);
  }
  ptr[i] = token;
  char *vals = ptr[i-1]; // second last line is the one we want
  char separator1[] = "&"; // now split this line by the dividing &
  token = strtok(vals, separator1);
  char *tokenz[buffsize];
  i = 0;
  while(token != NULL){
    tokenz[i] = token;
    i++;
    token = strtok(NULL, separator1);
  }
  tokenz[i] = token;
  //Serial.println();
  //Serial.println(tokenz[0]);
  //Serial.println(tokenz[1]);
  //Serial.println();

 

  char separator2[] = "="; // now split the results by = and put them into an array
  token = strtok(tokenz[0], separator2);
  char *results[buffsize];
  i = 0;
  while(token != NULL){
    results[i] = token;
    i++;
    token = strtok(NULL, separator2);
  }

 

  token = strtok(tokenz[1], separator2);
  while(token != NULL){
    results[i] = token;
    i++;
    token = strtok(NULL, separator2);
  }
  //Serial.println();
  //Serial.println(results[0]); //should be turns
  //Serial.println(results[1]); //should be the number of turns
  //Serial.println(results[2]); //should be state
  //Serial.println(results[3]); //should be the value of state
  //Serial.println();
  double turnnum;
  String state;
  if(String(results[0]) == "turns"){ // if clause to account for mixup in order
    turnnum = String(results[1]).toDouble();
    state = String(results[3]);
  }else if(String(results[0]) == "state"){
    turnnum = String(results[3]).toDouble();
    state = String(results[1]);
  }
  Serial.println();
  Serial.println();
  //process the requests. State always happens before the turn.
  handle_state(state);
  handle_turn(turnnum);
  //send the page again, so that the browser doesnt cry.
  handle_get(client);
  return;
}

 

void handle_state(String state){
  if(state == "none"){
    Serial.println("state none");    
  }
  if(state == "init"){
    Serial.println("state init"); 
    do_init();
    pos = 0;
  }
  if(state == "unsafe"){
    Serial.println("state unsafe");
    unsafe = true;    
  }
  if(state == "switch"){
    Serial.println("state switch");    
    do_switch();
  }
  if(state == "home"){
    Serial.println("state home");    
    do_home();
  }
  return;
}

 

void handle_turn(double turnnum){
  Serial.print("Turns to do: ");
  Serial.println(turnnum,8);
  long stepnum = round(turnnum * stepsPerRevolution);
  Serial.print("steps to do: ");
  Serial.println(stepnum);
  if(turnnum ==0){
    unsafe = false;
    Serial.print("current position : ");
    Serial.println(pos);
    return;
  }
  long stepcount = abs(stepnum);
  int diffposition;
  if(stepnum > 0){
    digitalWrite(dirPin, LOW);
    diffposition = 1;  
  }else{
    digitalWrite(dirPin, HIGH);
    diffposition = -1;
  }
  long i = 0;
  while(i<stepcount){
    if((stepnum > 0 || (pos > 0 && digitalRead(buttonPin) == LOW))||unsafe){
      //Serial.println("makes step");
      pos = pos + diffposition;
      //Serial.println(pos);
      step(delayshort);
      i++;
    }else{
      Serial.println("pos 0 reached");
      break;  
    }
    
  }
  unsafe = false;
  Serial.print("current position : ");
  Serial.println(pos);
  return;
}

 

void do_switch(){
  if(switchpos){
    digitalWrite(switchPin, LOW);
    switchpos = false;
  }else{
    digitalWrite(switchPin, HIGH);
    switchpos = true;  
  }
}

 

void do_init(){
  Serial.println("perform init");
  digitalWrite(dirPin, HIGH);
  digitalWrite(buttonPower, HIGH);
  while(digitalRead(buttonPin) == LOW){
    step(delaylong);
    delayMicroseconds(delaylong);  
  }
  digitalWrite(buttonPower, LOW);
  Serial.println("init successful"); 
  return;
}

 

void do_home(){
    handle_turn(round(-1*pos/stepsPerRevolution));
    return;
}

 

void step(int dur){
  //Serial.println("i makes step");
  delayMicroseconds(dur);
  digitalWrite(stepPin, HIGH);
  delayMicroseconds(dur);
  digitalWrite(stepPin, LOW); 
  return;
}

 

void handle_kaputt(String req, EthernetClient client){
  Serial.println("kaputt");
  return;
}

#include <WiFi.h> //Connect to WiFi Network
#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <SPI.h> //Used in support of TFT Display
#include <string.h>  //used for some string handling and processing.

const uint8_t IDLE = 0;
const uint8_t START_GAME = 1;
const uint8_t PLAY_GAME = 2;
const uint8_t GET_QUESTION = 3;
const uint8_t POST = 4;
const uint8_t TRUE = 7;
const uint8_t FALSE = 8;

uint8_t state;

TFT_eSPI tft = TFT_eSPI();

const int RESPONSE_TIMEOUT = 6000; //ms to wait for response from host
const int GETTING_PERIOD = 2000; //periodicity of getting a number fact.
const int BUTTON_TIMEOUT = 1000; //button timeout in milliseconds
const uint16_t IN_BUFFER_SIZE = 1000; //size of buffer to hold HTTP request
const uint16_t OUT_BUFFER_SIZE = 1000; //size of buffer to hold HTTP response
char request_buffer[IN_BUFFER_SIZE]; //char array buffer to hold HTTP request
char response_buffer[OUT_BUFFER_SIZE]; //char array buffer to hold HTTP response
char host[] = "608dev-2.net";

char network[] = "MIT";
char password[] = "";
uint8_t scanning = 0;

uint8_t channel = 1;
byte bssid[] = {0x04, 0x95, 0xE6, 0xAE, 0xDB, 0x41};

const int BUTTON1 = 45; //pin connected to button 
const int BUTTON2 = 39;
const int BUTTON3 = 38;
const int BUTTON4 = 34;


void print_IDLE_screen(){
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0,0,1);
  tft.println("WELCOME TO TRIVIA :)");
  tft.println("----------------");
  tft.println("Port 45 to begin");
}

void setup(){
  tft.init();  //init screen
  tft.setRotation(3); //adjust rotation
  tft.setTextSize(1); //default font size
  tft.fillScreen(TFT_BLACK); //fill background
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  Serial.begin(115200); //begin serial comms
  if (scanning){
    int n = WiFi.scanNetworks();
    Serial.println("scan done");
    if (n == 0) {
      Serial.println("no networks found");
    } else {
      Serial.print(n);
      Serial.println(" networks found");
      for (int i = 0; i < n; ++i) {
        Serial.printf("%d: %s, Ch:%d (%ddBm) %s ", i + 1, WiFi.SSID(i).c_str(), WiFi.channel(i), WiFi.RSSI(i), WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "open" : "");
        uint8_t* cc = WiFi.BSSID(i);
        for (int k = 0; k < 6; k++) {
          Serial.print(*cc, HEX);
          if (k != 5) Serial.print(":");
          cc++;
        }
        Serial.println("");
      }
    }
  }
  delay(100); //wait a bit (100 ms)

  //if using regular connection use line below:
  WiFi.begin(network, password);
  //if using channel/mac specification for crowded bands use the following:
  //WiFi.begin(network, password, channel, bssid);
  uint8_t count = 0; //count used for Wifi check times
  Serial.print("Attempting to connect to ");
  Serial.println(network);
  while (WiFi.status() != WL_CONNECTED && count<6) {
    delay(500);
    Serial.print(".");
    count++;
  }
  delay(2000);
  if (WiFi.isConnected()) { //if we connected then print our IP, Mac, and SSID we're on
    Serial.println("CONNECTED!");
    Serial.printf("%d:%d:%d:%d (%s) (%s)\n",WiFi.localIP()[3],WiFi.localIP()[2],
                                            WiFi.localIP()[1],WiFi.localIP()[0], 
                                          WiFi.macAddress().c_str() ,WiFi.SSID().c_str());
    delay(500);
  } else { //if we failed to connect just Try again.
    Serial.println("Failed to Connect :/  Going to restart");
    Serial.println(WiFi.status());
    ESP.restart(); // restart the ESP (proper way)
  }
  pinMode(BUTTON1, INPUT_PULLUP); //set input pin as an input!
  pinMode(BUTTON2, INPUT_PULLUP); //set input pin as an input!
  pinMode(BUTTON3, INPUT_PULLUP); //set input pin as an input!
  pinMode(BUTTON4, INPUT_PULLUP); //set input pin as an input!
  state = IDLE;
  print_IDLE_screen();
}

uint8_t char_append(char* buff, char c, uint16_t buff_size) {
        int len = strlen(buff);
        if (len>buff_size) return false;
        buff[len] = c;
        buff[len+1] = '\0';
        return true;
}

void do_http_GET(char* host, char* request, char* response, uint16_t response_size, uint16_t response_timeout, uint8_t serial){
  WiFiClient client; //instantiate a client object
  if (client.connect(host, 80)) { //try to connect to host on port 80
    if (serial) Serial.print(request);//Can do one-line if statements in C without curly braces
    client.print(request);
    memset(response, 0, response_size); //Null out (0 is the value of the null terminator '\0') entire buffer
    uint32_t count = millis();
    while (client.connected()) { //while we remain connected read out data coming back
      client.readBytesUntil('\n',response,response_size);
      //if (serial) Serial.println(response);
      if (strcmp(response,"\r")==0) { //found a blank line!
        break;
      }
      memset(response, 0, response_size);
      if (millis()-count>response_timeout) break;
    }
    memset(response, 0, response_size); 
    count = millis();
    while (client.available()) { //read out remaining text (body of response)
      char_append(response,client.read(),OUT_BUFFER_SIZE);
    }
    //if (serial) Serial.println(response);
    client.stop();
    //if (serial) Serial.println("-----------"); 
  }else{
    //if (serial) Serial.println("connection failed :/");
    //if (serial) Serial.println("wait 0.5 sec...");
    client.stop();
  }
}

char question[1000];
int index_of_and = 0;
char correct_answer[100];

void get_question(){
  memset(correct_answer, 0, 100);
  memset(question, 0, 1000);
  index_of_and = 0;

  sprintf(request_buffer, "GET http://608dev-2.net/sandbox/sc/shengh/trivia_game/trivia_file.py HTTP/1.1\r\n");
  strcat(request_buffer, "Host: 608dev-2.net\r\n"); //add more to the end
  strcat(request_buffer, "\r\n"); //add blank line!
  do_http_GET(host, request_buffer, response_buffer, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, 1);
  //Serial.println(response_buffer); //print to serial monitor

  for (int i=0; response_buffer[i] != '\0'; i++){
    if (response_buffer[i] == '&'){
      index_of_and = i;
    }
  }

  for (int i=0; i<index_of_and; i++){
    question[i] = response_buffer[i];
  }

  Serial.println(question);

  for (int i=index_of_and+1; response_buffer[i] != '\0'; i++){
    correct_answer[i-index_of_and-1] = response_buffer[i];
  }
  Serial.println(correct_answer);  

}

//IF YOU WANT YOU CAN CHANGE THE USER NAME. THE DATABASE CHECKS FOR DUPLICATES :)
char user_name[] = "Berry Allen";
int score = 0;
int correct_answers = 0;
int incorrect_answers = 0;

void post_score() {
  sprintf(request_buffer,"POST http://608dev-2.net/sandbox/sc/shengh/trivia_game/trivia_file.py?user=%s&score=%d&correct=%d&incorrect=%d HTTP/1.1\r\n", user_name, score, correct_answers, incorrect_answers);
  strcat(request_buffer, "Host: 608dev-2.net\r\n"); //add more to the end
  strcat(request_buffer, "\r\n"); //add blank line!
  do_http_GET(host, request_buffer, response_buffer, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, 1);
  Serial.println(response_buffer); //print to serial monitor
 }

char score_string[100];
char correct_string[100];
char incorrect_string[100];

int and_index = 0;
int index_of_money = 0;
int index_of_at = 0;

void get_scores(){
  and_index = 0;
  index_of_money = 0;
  index_of_at = 0;
  memset(score_string, 0, 100);
  memset(correct_string, 0, 100);
  memset(incorrect_string,0, 100);

  sprintf(request_buffer,"GET http://608dev-2.net/sandbox/sc/shengh/trivia_game/trivia_file.py?user=%s HTTP/1.1\r\n", user_name);
  strcat(request_buffer, "Host: 608dev-2.net\r\n"); //add more to the end
  strcat(request_buffer, "\r\n"); //add blank line!
  do_http_GET(host, request_buffer, response_buffer, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, 1);
  Serial.println(response_buffer); //print to serial monitor

  for (int i=0; response_buffer[i] != '\0'; i++){
    if (response_buffer[i] == '&'){
      and_index = i;      
    }
    else if (response_buffer[i] == '$'){
      index_of_money = i;
    }
    else if (response_buffer[i] == '@'){
      index_of_at = i;
    }
  }

  for (int i=0; i < and_index; i++){
    score_string[i] = response_buffer[i];
  }

  for (int i=and_index+1; i< index_of_money; i++){
    correct_string[i-and_index-1] = response_buffer[i];
  }
  

  for (int i=index_of_money+1; i< index_of_at; i++){
    incorrect_string[i-index_of_money-1] = response_buffer[i];
  }
  

  Serial.println(score_string);
  Serial.println(correct_string);
  Serial.println(incorrect_string);

  score = atoi(score_string);
  correct_answers = atoi(correct_string);
  incorrect_answers = atoi(incorrect_string);
  
}

void print_PLAY_GAME_screen(){
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0,0,1);
  tft.println(user_name);
  tft.println("-------------------");

  char total_score[100];
  sprintf(total_score, "Total Score: %d", score);

  char total_correct_answers[100];
  sprintf(total_correct_answers, "Correct: %d", correct_answers);

  char total_incorrect_answers[100];
  sprintf(total_incorrect_answers, "Incorrect: %d", incorrect_answers);  
    
  tft.println(total_score);
  tft.println(total_correct_answers);
  tft.println(total_incorrect_answers);
  tft.println("-------------------");
  tft.println(question);
  tft.println("-------------------");
  tft.println("Port 45 for T");
  tft.println("Port 39 for F");
  tft.println("Port 38 for Post");
}


void loop(){
  uint8_t button_1 = digitalRead(BUTTON1); // 45
  uint8_t button_2 = digitalRead(BUTTON2); // 39
  uint8_t button_3 = digitalRead(BUTTON3); //38
  uint8_t button_4 = digitalRead(BUTTON4); //34

  switch(state){
    case IDLE:
      if (button_1 == 0){ 
        state = START_GAME;
      }      
    break;

    case START_GAME:
      if(button_1 == 1){
        get_scores();
        get_question();
        print_PLAY_GAME_screen();
        state = PLAY_GAME;
      }
    break;

    case PLAY_GAME:
      Serial.println("IN PLAY_GAME");
      // can't use 34 in this state
      if (button_1 == 0){
        state = TRUE;
      }

      if (button_2 == 0){
        state = FALSE;
      }

      if (button_3 == 0){
        Serial.println("GOING TO POST");
        state = POST;
      }
    break;

    case POST:
      if (button_3 == 1){
        
        post_score();
        print_IDLE_screen();
        state = IDLE;
      }
    break;

    case TRUE:
      if(button_1 == 1){
               
        if (correct_answer[0] == 'T'){
          //Serial.println("IN THIS TRUE LOOP");
          score += 1;
          correct_answers +=1;
          get_question();
          print_PLAY_GAME_screen();
          state = PLAY_GAME; 
        }
        else{
          //Serial.println("IN OTHER TRUE LOOP");
          if (score > 0){
            score -=1;
          }
          incorrect_answers += 1;
          get_question();
          print_PLAY_GAME_screen();
          state = PLAY_GAME; 
        }
               
      }      
    break;

    case FALSE:
      if(button_2 == 1){  
        if (correct_answer[0] == 'F'){
          //Serial.println("IN THIS FALSE LOOP");
          score += 1;
          correct_answers += 1;
          get_question();
          print_PLAY_GAME_screen();
          state = PLAY_GAME;
        }
        else{
          //Serial.println("IN THIS OTHER FALSE LOOP");
          if (score > 0){
            score -=1;
          }
          incorrect_answers +=1;
          get_question();
          print_PLAY_GAME_screen();
          state = PLAY_GAME;
        }
        
      }      
    break;
  }

}


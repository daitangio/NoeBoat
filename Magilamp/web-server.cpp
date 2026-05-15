#include "web-server.h"
#include "magilamp_config.h"
#include <utility>
#include <vector>

#include <string>


int status = WL_IDLE_STATUS;
WiFiServer server(80);

void noRoute(void* pvParameters);

// typedef void (* TaskFunction_t)( void * );

// A set of routing actions
// Refer to https://www.educative.io/answers/how-to-use-stdpair-in-cpp

// Regex is very huge, see https://en.cppreference.com/w/cpp/regex
// consider the NickGammon's Regexp library but it is very old
#include <regex>
// std::regex self_regex("REGULAR EXPRESSIONS",std::regex_constants::basic | std::regex_constants::icase);

std::vector<std::pair<std::string, TaskFunction_t>> router = {
  {"/sys", noRoute },
  {"/phantom", noRoute} // Phantom cycling sequence

}; 

// Routing functions

void noRoute(void* pvParameters)
{ 
  Serial.println("No Route");
}


void TaskWebServer(void* pvParameters){
  // Directly taken from from WiFiWebServer with exttra logs
  printWifiStatus();
  server.begin();
  Serial.println("Listening to port 80"); 
  
  for(;;){ 
    // listen for incoming clients
    WiFiClient client = server.available();
    if (client) {
      // New client
      // TODO: Signal it to the LED Martix
      // an HTTP request ends with a blank line
      boolean currentLineIsBlank = true;
      while (client.connected()) {
        if (client.available()) {
          char c = client.read();
          if(DEBUG_MODE){
            Serial.write(c);
          }

          // if you've gotten to the end of the line (received a newline
          // character) and the line is blank, the HTTP request has ended,
          // so you can send a reply
          if (c == '\n' && currentLineIsBlank) {
            // send a standard HTTP response header
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: text/html");
            client.println("Connection: close");  // the connection will be closed after completion of the response
            client.println("Refresh: 5");  // refresh the page automatically every 5 sec
            client.println();
            client.println("<!DOCTYPE HTML>");
            client.println("<html>");
            // output the value of each analog input pin
            for (int analogChannel = 0; analogChannel < 6; analogChannel++) {
              int sensorReading = analogRead(analogChannel);
              client.print("analog input ");
              client.print(analogChannel);
              client.print(" is ");
              client.print(sensorReading);
              client.println("<br />");
            }
            client.println("</html>");
            break;
          }
          if (c == '\n') {
            // you're starting a new line
            currentLineIsBlank = true;
          } else if (c != '\r') {
            // you've gotten a character on the current line
            currentLineIsBlank = false;
          }
        }
      }
      // give the web browser the time to receive the data
      // GG hum this seems not necessary
      // delay(portTICK_PERIOD_MS);      
      client.stop(); // client disconnected
    }else{
      vTaskDelay(  portTICK_PERIOD_MS ); 
    }
  } //for ever
}
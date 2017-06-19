/**Imported from Brandenhall github library
 * Imported libraries mentioned below
 */
//Libraries to be used
#include <SPI.h> 
#include <Ethernet.h>
#include <WebSocketClient.h>
#include <Time.h>

/**Global Variables mentioned below
 */

int periodVar;
int period = 0;
int shortPeriod = 4;
bool clientDataSent;
bool serverDataReceived;
bool serverConfirm;
bool timerStarted;
int currentTime = 0;
int previousTime = 0;
int currentShortTime = 0;
int previousShortTime = 0;
int i = 0;
int j = 0;
#define LED_GREEN 22
#define LED_RED 23
char path[] = "/echo";
char host[] = "133.11.236.253:1235";
char server[] = "133.11.236.253";
EthernetClient client;
WebSocketClient WS_Client;

/**Actual MAC Address of given Arduino Device and the
 * static IP address if needed by it
 */
byte mac[] = { 0xB0, 0x12, 0x66, 0x01, 0x02, 0x17 };
IPAddress ip(192, 168, 0, 177);

/**Handshake Setup:
 * It starts the Serial Monitor, defines inputs and outputs,
 * initiates ethernet connection by controlling its
 * ethernet shield and also initiates handshake
 */
void setup() {
  /**Start Serial Monitor and wait for it
   */
  Serial.begin(9600);
  while (!Serial) {}

  /**Define inputs and outputs
   */
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(A0, OUTPUT);

  /**Connect to ethernet shield and get IP address using static
   * or DHCP
   */
  Serial.println("Setting ethernet");
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to set IP Address with DHCP");
    Ethernet.begin(mac, ip); //Set to default static IP address
  }
  
  /**Delay for setting up ethernet connection
   */
  delay(1000);
  Serial.println("Connecting...");

  /**Initiate connection to the server address and port
   * provided via ethernet
   */
  if (client.connect(server, 1235)) {
    /**Upgrade the current socket to a websocket and
     * after setting websocket host address and path
     * send an handshake initialization to the server
     */
    Serial.println("Connection Done!");
    WS_Client.path=path;
    WS_Client.host=host;
    if (WS_Client.handshake(client)) {
      Serial.println("Handshake successful");  
    }
    else {
      Serial.println("Handshake Failed!");  
      /**If connection fails, hold on to prevent infinite looping
       */
      while (1) {}
    }
  }
  else {
    Serial.println("Client, disconnected");

    /**If connection fails, hold on to prevent infinite looping
     */
    while(1) {}
}

   /** Initialize various variables needed in the algorithm
    *  and send Introductory data, especially ID of the client
    */
   WS_Client.sendData("1");
   period = 10;
   clientDataSent = false;
   serverDataReceived = false;
   serverConfirm = false;
   timerStarted = false;
   currentTime = now();
   previousTime = now();
   currentShortTime = now();
   previousShortTime = now();
   periodVar = period;
}

/**Main Loop:
 * Handles sending and receving of messages while 
 * also managing stay awake for the client side, 
 * ACKS and reconnection setup incase of TCP connection failure
 */
void loop() {

  /**If the client is connected and the there is a positive length 
   * message from the server then we check the message and see which category it falls into
   */
  String recv;
  if (client.connected()) {
    recv = WS_Client.getData();
    if (recv.length() > 0) {
      Serial.print("Recv: ");
      Serial.println(recv);
      if (recv.substring(0,1) == "R") {

        /**If message is for LED Trigger, then the 
         * approproiate LED is turned on for a fixed amount of time
         */
        digitalWrite(A0, HIGH);
        Serial.println("R=ON, G=OFF"); 
        Serial.println(j);
        j++;
        digitalWrite(LED_RED, HIGH);  
        delay(100);                    
        digitalWrite(LED_RED, LOW);   
        delay(100);
      }
      else if (recv.substring(0,3) == "ACK") {
        /**If its an ACK then the appropriate variable's state is changed
         */
        serverConfirm = true;
      }
      else if (recv.substring(0,3) == "PTI") {
        /**If its a periodic interval message then an ACK is delivered back 
         * to the server and an alert is sounded at the client
         */
        String ACK_Send = "ACK " + String(j);
        WS_Client.sendData(ACK_Send);
        digitalWrite(LED_GREEN, HIGH);
        delay(100);
        digitalWrite(LED_GREEN, LOW);
        delay(100);
        serverDataReceived = true;
      }
      else {
        /**Else the message is ignored
         */
        Serial.println("Wrong Option!");
      }
    } 

    /**Starts timer when atleast one of them has been received
     */
    if ((serverConfirm || serverDataReceived) && !timerStarted) {
      previousShortTime = now();  
      timerStarted = true;
    }

    currentShortTime = now();
    if (serverConfirm && serverDataReceived) {
      /**If both are on then we just have to know that awake message 
      * was received successfully
      */
      period+=10;
      Serial.println("Period");
      Serial.println(period);
      serverConfirm = false;
      serverDataReceived = false;
      clientDataSent = false;
      timerStarted = false;
      Serial.println("Adding");
    }
    else if (((currentShortTime - previousShortTime) > shortPeriod) && timerStarted) {
      /**If both have not arrived in the 2 seconds duration, we assume
       * that the messages have not been exchanged and so we need to 
       * make the period shorter
       */
      period/=2;
      Serial.println("Period");
      Serial.println(period);
      serverConfirm = false;
      serverDataReceived = false;
      timerStarted = false;
      clientDataSent = false;
      Serial.println("Backing off");
    }
  
    /**At every periodic interval, the client sends an periodic message
     * to keep the connection awake
     */
    currentTime = now();
    if ((currentTime - previousTime) > periodVar) {
      WS_Client.sendData("PTI ALERT CLIENT");
      previousTime = currentTime;
      clientDataSent = true;
    }
  }
  else {
    /**If the client is disconnected from the server, then it 
     * stops the connection and restarts the handshake again
     * but while using the newly updated periodic interval value
     * and the current time
     */
    period/=2;
    Serial.println("Client Disconnected!");
    delay(3000);
    client.stop();
    delay(2000);    

    /**The code below is the exact same as that used above in the
     * Arduino setup part
     */
    if (client.connect(server, 1235)) {
      Serial.println("Connection Done!");
      WS_Client.path=path;
      WS_Client.host=host;
      if (WS_Client.handshake(client)) {
        Serial.println("Handshake successful");  
      }
      else {
        Serial.println("Handshake Failed!");  
        /**If connection fails, hold on to prevent infinite looping
        */
        while (1) {}
      }
    }
    else {
      Serial.println("Client, disconnected");
      /**If connection fails, hold on to prevent infinite looping
      */
      while(1) {}
    }

    /**Send introductory data again to server
     */
    WS_Client.sendData("1");
  }
}

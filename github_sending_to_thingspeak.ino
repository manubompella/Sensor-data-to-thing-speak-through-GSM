
/*
  Sensor data sending to thingspeak sketch for Arduino using SIM900 and DHT11
  Manohar, thingTronics
  
Copyright (c) 2015 thingTronics Limited

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

version 0.1
*/

#include <GPRS_Shield_Arduino.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include <dht.h>
#include "SoftReset.h"

//dht sensor pin
#define DHT11_PIN 11
//gsm module pins to arduino MEGA 2560
#define PIN_TX    50
#define PIN_RX    51
//Baudrate for sim900
#define BAUDRATE  9600
#define DELAY_TIME  1000

//Function definations
void resetfn(void);
void InitSetup(void);
void big_delay(int );
void gprs_Init_Setup(void);
void Check_Sensor_Data(int );
void Make_TCP_Connection(void);

//Create an instaneous DHT library
dht DHT;

//Create an instaneous GPRS library
GPRS gprs(PIN_TX, PIN_RX, BAUDRATE);

//global variables
char dateTime[24];
char buffer[250];
char apn[]="internet";	//this is for idea
//your thingspeak API_key
String API_key="xxxxxxxxxxxxxx";

void setup(){
	//initial setup for gsm
	InitSetup();
	
	//gprs initial setup
	gprs_Init_Setup();

  } 
  
void loop(){

	char t_buffer[170];
	char h_buffer[10];
	
	//read sensor values
	int chk = DHT.read11(DHT11_PIN);
	
	//Check sensor data valid or not
	Check_Sensor_Data(chk);
	
	//read temperature and humidity values 
	//and convert them into string
	float t=(DHT.temperature);
	String temp=dtostrf(t,0,5,t_buffer);
	Serial.print("Temp = ");
	Serial.println(t);
	float h=(DHT.humidity);
	Serial.print("Humidity = ");
	Serial.println(h);
	String humid=dtostrf(h,0,5,h_buffer);

	String st = "GET /update?api_key="+API_key+"&field1="+temp+"&field2="+humid+" HTTP/1.0\r\n\r\n";
	
	//convert string to char array
	st.toCharArray(t_buffer,170);
	Serial.println(t_buffer);
 
	// successful DHCP. get IP address
	Serial.print("IP Address is ");
	Serial.println(gprs.getIPAddress());
	
	//establish TCP connection
    Make_TCP_Connection();
	  delay(DELAY_TIME); 
	//send sensor data into thingspeak
    gprs.send(t_buffer, sizeof(t_buffer)-1);
    delay(500);       
	
	//wait for response
	while (true)
	{
		int ret = gprs.recv(buffer, sizeof(buffer)-1);
		if (ret <= 0){
			Serial.println("SENDING over...");
			break; 
		}
		buffer[ret] = '\0';
		Serial.print("Recv: ");
		Serial.print(ret);
		Serial.print(" bytes: ");
		Serial.println(buffer);
	}
  delay(DELAY_TIME);
	 gprs.close();
	// delay to get required frequency 
	big_delay(6);

}

void resetfn(void)
{
    Serial.println("Restarting..........");
    delay(DELAY_TIME);
	//restart gsm module
    sim900_check_with_cmd("AT+CFUN=1,1\r\n", "OK\r\n", CMD);
    delay(8*DELAY_TIME);
	//restart arduino
    soft_restart();
}
void big_delay(int x )   
{
	//delay x seconds
	while(x!=0)
	{
		delay(DELAY_TIME);
		x--;
    }      
}

void InitSetup(void)
{
	//set baudrate for serial communication
	Serial.begin(9600);
	int d=0;
    delay(10*DELAY_TIME);
    while(!gprs.init()) 
	{
		delay(DELAY_TIME);
		Serial.print("init error\r\n");
		if(d==6)
		{
			delay(DELAY_TIME);
			resetfn();
			break;
		}
        d++;
    }
	// Check for signal strength
	int rssi, ber;
	// sent AT+CSQ
	gprs.getSignalStrength(&rssi);
#ifdef DEBUG
	Serial.print(F("Signal Strength is "));
	Serial.println(rssi);
#endif
	// Check for SIM registration
	// Send AT+CREG?
	int networkType;
	gprs.getSIMRegistration(&networkType);
	if( networkType == 1 || networkType == 5) {
		Serial.println(F("SIM is registered"));
		if(networkType == 1) {
#ifdef DEBUG
			Serial.println(F("NETWORK type is LOCAL (1)"));
#endif
		}
		else if(networkType == 5) {
#ifdef DEBUG
			Serial.println(F("NETWORK type is ROAMING (5)"));
#endif
		}
	}
	else 
	{
		Serial.println(F("SIM is not registered"));
		resetfn();
	}
	// Check for Service Provider's Name
	// AT+CSPN?
	char providerName[10];
	gprs.getProviderName(providerName);
	Serial.println("-------------------------");
	Serial.print("PROVIDER NAME :");
	Serial.println(providerName);
	Serial.println("-------------------------");
	// Send AT+CLIP=1
	sim900_check_with_cmd(F("AT+CLIP=1\r\n"),"OK\r\n",CMD);
	delay(DELAY_TIME);
	
}

void gprs_Init_Setup(void)
{
	int c=0;
	delay(DELAY_TIME);
	while(!gprs.join(apn)) 
	{
        Serial.println("gprs join network error");
        delay(4*DELAY_TIME);
		c++;
		if(c==6)
		{
			delay(DELAY_TIME);
			resetfn();
			break;
		}
	} 
	Serial.println("GPRS initial Setup Ready");
	delay(3*DELAY_TIME);   
}

void Check_Sensor_Data(int chk )

{	//check sensor data valid or not
	switch (chk)
	{
		case DHTLIB_OK:  
		Serial.println("Sensor data Valid\t"); 
		break;
		case DHTLIB_ERROR_CHECKSUM: 
		Serial.println("Checksum error\t"); 
		break;
		case DHTLIB_ERROR_TIMEOUT: 
		Serial.println("Time out error\t"); 
		break;
		case DHTLIB_ERROR_CONNECT:
        Serial.println("Connect error\t");
        break;
		case DHTLIB_ERROR_ACK_L:
        Serial.println("Ack Low error,\t");
        break;
		case DHTLIB_ERROR_ACK_H:
        Serial.println("Ack High error,\t");
        break;
		default: 
		Serial.println("Unknown error,\t"); 
		break;
    }
}
void Make_TCP_Connection(void)
{
	//make TCP connection
	int k=0;
	while(!gprs.connect(TCP,"api.thingspeak.com", 80)) {
		delay(DELAY_TIME*3);
		Serial.println("TCP connect error");
		if(k==5){
            delay(DELAY_TIME);
            resetfn();
            break;
        }
	k++;
	}
	delay(DELAY_TIME);
	Serial.println("connect api.thingspeak.com success");
  delay(DELAY_TIME);
	Serial.println("waiting to SEND...");
}

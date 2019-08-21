/*
    Name:       MQTT_esp8266_client_python_server.ino
    Created:	7/19/2019 3:20:42 PM
    Author:     DESKTOP-D4E53N5\liors
*/


#include <Wire.h>
#include <ESP8266WiFi.h> 
#include <Ticker.h>
#include <PubSubClient.h>

// use onboard LED for convenience 
#define LED (2)
// maximum received message length 
#define MAX_MSG_LEN (128)

#define MPU 0x68 // MPU6050 I2C address

// Wifi configuration
const char* ssid = "Liornet";
const char* password = "0544988409";
bool runData = false;
bool state = false;

// MQTT Configuration
// if you have a hostname set for the MQTT server, you can use it here
//const char *serverHostname = "your MQTT server hostname";
// otherwise you can use an IP address like this
const IPAddress serverIPAddress(10, 100, 102, 3);
// the topic we want to use
const char* topic = "Odin/HeadGearSensor";
WiFiClient espClient;
PubSubClient client(espClient);

double AccX, AccY, AccZ, AccNorm;
double GyroX, GyroY, GyroZ;
int16_t ax, ay, az;
int16_t gx, gy, gz;

Ticker flipper;

void setup()
{
	Serial.begin(115200);
	while (!Serial);
	pinMode(LED, OUTPUT);
	digitalWrite(LED, LOW);
	// Configure serial port for debugging
	
	// Initialise wifi connection - this will wait until connected
	connectWifi();
	// connect to MQTT server  
	client.setServer(serverIPAddress, 1883);
	client.setCallback(callback);
	MPU6050_setup();
	flipper.attach_ms(10,sendData);
}
void loop()
{
	char* temp;
	if (!client.connected()) {
		connectMQTT();
	}
	// this is ESSENTIAL!
	client.loop();
	// idle


}
void connectWifi()
{
	delay(10);
	// Connecting to a WiFi network
	Serial.printf("\nConnecting to %s\n", ssid);
	WiFi.begin(ssid, password);
	while (WiFi.status() != WL_CONNECTED) {
		delay(250);
		Serial.print(".");
	}
	Serial.println("");
	Serial.print("WiFi connected on IP address ");
	Serial.println(WiFi.localIP());
}
// connect to MQTT server
void connectMQTT() {
	// Wait until we're connected
	while (!client.connected()) {
		// Create a random client ID
		String clientId = "ESP8266-";
		clientId += String(random(0xffff), HEX);
		Serial.printf("MQTT connecting as client %s...\n", clientId.c_str());
		// Attempt to connect
		if (client.connect(clientId.c_str())) {
			Serial.println("MQTT connected");
			// Once connected, publish an announcement...
			client.publish(topic, "hello from ESP8266");
			// ... and resubscribe
			client.subscribe(topic);
		}
		else {
			Serial.printf("MQTT failed, state %s, retrying...\n", client.state());
			// Wait before retrying
			delay(2500);
		}
	}
}
void callback(char* msgTopic, byte* msgPayload, unsigned int msgLength)
{
	// copy payload to a static string
	static char message[MAX_MSG_LEN + 1];
	if (msgLength > MAX_MSG_LEN)
	{
		msgLength = MAX_MSG_LEN;
	}
	strncpy(message, (char*)msgPayload, msgLength);
	message[msgLength] = '\0';

	//Serial.printf("topic %s, message received: %s\n", topic, message);
	// decode message
	if (strcmp(message, "off") == 0) {
		runData = false;
	}
	else if (strcmp(message, "on") == 0) {
		runData = true;
	}
}
void MPU6050_setup()
{
	Wire.begin();                      // Initialize comunication
	Wire.beginTransmission(MPU);       // Start communication with MPU6050 // MPU=0x68
	Wire.write(0x6B);                  // Talk to the register 6B
	Wire.write(B00000000);             // Make reset - place a 0 into the 6B register
	Wire.endTransmission(true);        //end the transmission

	Wire.begin();                      // Initialize comunication
	Wire.beginTransmission(MPU);       // Start communication with MPU6050 // MPU=0x68
	Wire.write(0x1B);                  // Talk to the register 6B
	Wire.write(B00011000);             // Make reset - place a 0 into the 6B register
	Wire.endTransmission(true);        //end the transmission

	Wire.begin();                      // Initialize comunication
	Wire.beginTransmission(MPU);       // Start communication with MPU6050 // MPU=0x68
	Wire.write(0x1C);                  // Talk to the register 6B
	Wire.write(B00011000);             // Make reset - place a 0 into the 6B register
	Wire.endTransmission(true);        //end the transmission
}
void MPU6050_getAcc()
{
	Wire.beginTransmission(MPU);
	Wire.write(0x3B); // Start with register 0x3B (ACCEL_XOUT_H)
	Wire.endTransmission(false);
	Wire.requestFrom(MPU, 6);
	//For a range of +-2g, we need to divide the raw values by 16384, according to the datasheet

	ax = (Wire.read() << 8 | Wire.read()); // X-axis value
	ay = (Wire.read() << 8 | Wire.read()); // Y-axis value
	az = (Wire.read() << 8 | Wire.read()); // Z-axis value

	//ax >= pow(2, 15) ? ax = ax - pow(2, 16) : ax = ax;
	//ay >= pow(2, 15) ? ay = ay - pow(2, 16) : ay = ay;
	//az >= pow(2, 15) ? az = az - pow(2, 16) : az = az;

	ax = (int16_t)((double)ax * (1000 / (16384.0 / 8)));
	ay = (int16_t)((double)ay * (1000 / (16384.0 / 8)));
	az = (int16_t)((double)az * (1000 / (16384.0 / 8)));
}
void MPU6050_getOmega()
{
	Wire.beginTransmission(MPU);
	Wire.write(0x43); // Start with register 0x3B (ACCEL_XOUT_H)
	Wire.endTransmission(false);
	Wire.requestFrom(MPU, 6);
	//For a range of +-2g, we need to divide the raw values by 16384, according to the datasheet

	gx = (Wire.read() << 8 | Wire.read()); // X-axis value
	gy = (Wire.read() << 8 | Wire.read()); // Y-axis value
	gz = (Wire.read() << 8 | Wire.read()); // Z-axis value

	//ax >= pow(2, 15) ? ax = ax - pow(2, 16) : ax = ax;
	//ay >= pow(2, 15) ? ay = ay - pow(2, 16) : ay = ay;
	//az >= pow(2, 15) ? az = az - pow(2, 16) : az = az;

	gx = (int16_t)((double)gx * (1/ (16384.0 / 1000)));
	gy = (int16_t)((double)gy * (1 / (16384.0 / 1000)));
	gz = (int16_t)((double)gz * (1 / (16384.0 / 1000)));
}
void sendData()
{
	//unsigned long int t1 = micros();
	MPU6050_getAcc();
	MPU6050_getOmega();
	int val1 = ax;
	int val2 = ay;
	int val3 = az;
	int val4 = gx;
	int val5 = gy;
	int val6 = gz;
	String msg = " ";
	msg = msg + val1 + "," + val2 + "," + val3 + "," + val4 + "," + val5 + "," + val6;
	char message[50];
	msg.toCharArray(message, 50);
	client.publish(topic, message);
	//unsigned long int t2 = micros();
	//Serial.println(t2 - t1);
}
void sendDataOnCommand()
{
	unsigned long int counter;
	if (runData == true)
	{
		unsigned long counter = 0;
		unsigned long int t1 = millis();
		for (int i = 0; i < 20000; i++)
		{
			MPU6050_getAcc();
			int val1 = AccX;
			int val2 = AccY;
			int val3 = AccZ;
			int val4 = AccNorm;
			String msg = " ";
			msg = msg + val1 + " , " + val2 + " , " + val3 + " , " + val4;
			char message[58];
			msg.toCharArray(message, 58);

			client.publish("test", message);
			digitalWrite(2, state);
			state = !state;
			counter++;
		}
		client.publish("test", "counter:");
		String msg = " ";
		msg = msg + counter + " , " + (int)(millis() - t1);
		char message[58];
		msg.toCharArray(message, 58);
		client.publish("test", message);

		runData = false;
		state = HIGH;
		digitalWrite(2, state);
	}
	else {}
}
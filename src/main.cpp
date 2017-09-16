#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include "privatefile.h" // Remove this line by commenting on it

// Expose Espressif SDK functionality - wrapped in ifdef so that it still
// compiles on other platforms
#ifdef ESP8266
extern "C" {
#include "user_interface.h"
}
#endif

#define MYHOSTNAME "RoomSLight1"

const char* ssid = SSID;//Put  yuor ssid here
const char* password = SSIDPASSWORD;//Put  yuor ssid password here
const char* host = MYHOSTNAME;

int led_pin1 = 12;
int led_pin2 = 13;
int led_pin3 = 15;

int sw_pin = 4;

bool LampSwitch = true;

void callback(char* topic, byte* payload, unsigned int length);

const char* mqtt_server = "192.168.1.39";
WiFiClient espClient;
PubSubClient client(mqtt_server, 1883, callback, espClient);

const byte msg_size = 100;
char msg[msg_size];
char topic[msg_size];


unsigned long previousMillis = 0;
unsigned long previousMillisSend = 0;
unsigned long previousMillisLamp = 0;
unsigned long chkMillisLamp = 3000;


void SetLampSwitch(bool sw)
{
	if (LampSwitch != sw)
	{
		if (sw)
			chkMillisLamp = 3000;
		else
			chkMillisLamp = 500;
		LampSwitch = sw;
		previousMillisLamp = 0;
	}
}



///mqtt callback
void callback(char* topic, byte* payload, unsigned int length) {

	Serial.println("");
	Serial.println("-------");
	Serial.println("New callback of MQTT-broker");
	Serial.setDebugOutput(true);

	//преобразуем тему(topic) и значение (payload) в строку
	payload[length] = '\0';
	String strTopic = String(topic);
	String strPayload = String((char*)payload);
	Serial.printf("strTopic %s ~ strPayload %s \n", topic, payload);

	if (!strTopic.startsWith(MYHOSTNAME)) return;

	if (strTopic.endsWith("/switch"))
	{
		Serial.printf("switch topic!\n");
		SetLampSwitch(strPayload.equalsIgnoreCase("true"));
	}
	//Исследуем что "прилетело" от сервера по подписке:
	//Изменение интервала опроса
}

void SendParam()
{
	Serial.println("SendParam");

	snprintf(msg, msg_size, "%ld", ESP.getFreeHeap());
	snprintf(topic, msg_size, "%s/freemem", host);
	client.publish(topic, msg);

	snprintf(topic, msg_size, "%s/wanip", host);
	WiFi.localIP().toString().toCharArray(msg, msg_size);
	client.publish(topic, msg);

	snprintf(topic, msg_size, "%s/mac", host);
	WiFi.macAddress().toCharArray(msg, msg_size);
	client.publish(topic, msg);

	snprintf(topic, msg_size, "%s/rssi", host);
	snprintf(msg, msg_size, "%ld", WiFi.RSSI());
	client.publish(topic, msg);

	snprintf(topic, msg_size, "%s/switch", host);
	snprintf(msg, msg_size, "%s", LampSwitch ? "true" : "fase");
	client.publish(topic, msg);


	snprintf(topic, msg_size, "%s/uptime", host);
	snprintf(msg, msg_size, "%ld", millis());
	client.publish(topic, msg);

}

void SendParam1()
{
	if (!client.connected()) return;

	snprintf(topic, msg_size, "%s/uptime", host);
	snprintf(msg, msg_size, "%ld", millis());
	client.publish(topic, msg, true);

}

void LampCheck()
{
	Serial.printf("Lamp check %ld %ld\n", millis(), chkMillisLamp);

	bool l = digitalRead(led_pin1);
	if (LampSwitch != l)
	{
		Serial.println("lamp led_pin1 sw");
		digitalWrite(led_pin1, LampSwitch ? HIGH : LOW);
		return;
	}

	l = digitalRead(led_pin2);
	if (LampSwitch != l)
	{
		Serial.println("lamp led_pin2 sw");
		digitalWrite(led_pin2, LampSwitch ? HIGH : LOW);
		return;
	}

	l = digitalRead(led_pin3);
	if (LampSwitch != l)
	{
		Serial.println("lamp led_pin3 sw");
		digitalWrite(led_pin3, LampSwitch ? HIGH : LOW);
		return;
	}
	chkMillisLamp = 6000;
}

void setup() {
	Serial.begin(115200);
	Serial.println("Booting");

	/* switch on led */
	pinMode(led_pin1, OUTPUT);
	digitalWrite(led_pin1, HIGH);

	pinMode(led_pin2, OUTPUT);
	digitalWrite(led_pin2, LOW);

	pinMode(led_pin3, OUTPUT);
	digitalWrite(led_pin3, LOW);

	//pinMode(sw_pin, INPUT);

	WiFi.hostname(host);
	WiFi.mode(WIFI_STA);
	WiFi.begin(ssid, password);
	while (WiFi.waitForConnectResult() != WL_CONNECTED) {
		Serial.println("Connection Failed! Rebooting...");
		delay(5000);
		ESP.restart();
	}

	// Port defaults to 8266
	// ArduinoOTA.setPort(8266);

	// Hostname defaults to esp8266-[ChipID]
	// ArduinoOTA.setHostname("myesp8266");

	// No authentication by default
	//ArduinoOTA.setPassword((const char *)"123");


	//================================================================================================
	// OTA auto update


	ArduinoOTA.onStart([]() {
		ESP.wdtDisable();
		Serial.println("Start");

		digitalWrite(led_pin1, HIGH);
		digitalWrite(led_pin2, HIGH);
		digitalWrite(led_pin3, HIGH);
	});


	ArduinoOTA.onEnd([]() {
		Serial.println("\nEnd");
	});


	ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
		Serial.printf("Progress: %u%%\r\n", (progress / (total / 100)));
	});


	ArduinoOTA.onError([](ota_error_t error) {
		Serial.printf("Error[%u]: ", error);
		if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
		else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
		else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
		else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
		else if (error == OTA_END_ERROR) Serial.println("End Failed");
	});


	//================================================================================================

	ArduinoOTA.setHostname(host);
	ArduinoOTA.begin();
	Serial.println("Ready");
	Serial.print("IP address: ");
	Serial.println(WiFi.localIP());


	uint32_t realSize = ESP.getFlashChipRealSize();
	uint32_t ideSize = ESP.getFlashChipSize();
	FlashMode_t ideMode = ESP.getFlashChipMode();

	Serial.printf("Flash real id:   %08X\n", ESP.getFlashChipId());
	Serial.printf("Flash real size: %u\n\n", realSize);

	Serial.printf("Flash ide  size: %u\n", ideSize);
	Serial.printf("Flash ide speed: %u\n", ESP.getFlashChipSpeed());
	Serial.printf("Flash ide mode:  %s\n", (ideMode == FM_QIO ? "QIO" : ideMode == FM_QOUT ? "QOUT" : ideMode == FM_DIO ? "DIO" : ideMode == FM_DOUT ? "DOUT" : "UNKNOWN"));


	if (ideSize != realSize) {
		Serial.println("Flash Chip configuration wrong!\n");
	}
	else {
		Serial.println("Flash Chip configuration ok.\n");
	}
	ESP.wdtEnable(WDTO_4S);
}


void loop() {
	ArduinoOTA.handle();
	//digitalRead(sw_pin);
	ESP.wdtFeed();

	if (millis() - previousMillisSend >= 60000) {
		previousMillisSend = millis();
		SendParam1();
	}

	if (millis() - previousMillisLamp >= chkMillisLamp) {
		previousMillisLamp = millis();
		LampCheck();
	}


	if (!client.connected()) {
		Serial.println("Connect to MQTT-boker...  ");
		snprintf(msg, msg_size, "%s", host);

		if (client.connect(msg, "q1", "q1")) {
			Serial.println("publish an announcement");
			// Once connected, publish an announcement...
			snprintf(msg, msg_size, "%ld", ESP.getFreeHeap());
			snprintf(topic, msg_size, "%s/freemem", host);
			client.publish(topic, msg);

			Serial.println("1");

			snprintf(topic, msg_size, "%s/wanip", host);
			WiFi.localIP().toString().toCharArray(msg, msg_size);
			client.publish(topic, msg);

			Serial.println("2");

			snprintf(topic, msg_size, "%s/mac", host);
			WiFi.macAddress().toCharArray(msg, msg_size);
			client.publish(topic, msg);

			Serial.println("3");

			snprintf(topic, msg_size, "%s/rssi", host);
			snprintf(msg, msg_size, "%ld", WiFi.RSSI());
			client.publish(topic, msg);

			Serial.println("4");


			snprintf(topic, msg_size, "%s/switch", host);
			client.subscribe(topic);

			Serial.println("5");

			snprintf(topic, msg_size, "%s/uptime", host);
			snprintf(msg, msg_size, "%s", "3");
			client.publish(topic, msg);

			Serial.println("6");

			SendParam();
		}
		else
		{
			Serial.println("Delay");
			delay(1000);
		}
	}




	client.loop();
}




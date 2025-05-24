#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

// Wi-Fi 설정
const char* ssid = "";
const char* password = "";

// MQTT 브로커 설정
const char* mqtt_server = "broker.emqx.io";

// 센서 설정
#define DHTPIN 15           // DHT 데이터 핀
#define DHTTYPE DHT11       // DHT11 사용
#define LIGHT_SENSOR_PIN 34 // 조도 센서 핀

DHT dht(DHTPIN, DHTTYPE);

// MQTT 클라이언트
WiFiClient espClient;
PubSubClient client(espClient);

long lastMsg = 0;
float temperature = 0;
float humidity = 0;

void setup_wifi() {
  delay(10);

  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void callback(char* topic, byte* message, unsigned int length) {
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("")) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  dht.begin();
  pinMode(LIGHT_SENSOR_PIN, INPUT);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long now = millis();
  if (now - lastMsg > 5000) {
    lastMsg = now;

    // 온습도 측정
    temperature = dht.readTemperature();
    humidity = dht.readHumidity();

    if (isnan(temperature) || isnan(humidity)) {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }

    char tempString[8];
    dtostrf(temperature, 1, 2, tempString);
    Serial.print("Temperature: ");
    Serial.println(tempString);
    client.publish("yonsei/gsi/smart-desk/temperature", tempString);

    char humString[8];
    dtostrf(humidity, 1, 2, humString);
    Serial.print("Humidity: ");
    Serial.println(humString);
    client.publish("yonsei/gsi/smart-desk/humidity", humString);

    // 조도 측정
    int lightValue = analogRead(LIGHT_SENSOR_PIN);

    if (isnan(temperature)) {
      Serial.println("Failed to read from Light sensor!");
      return;
    }

    char lightString[8];
    itoa(lightValue, lightString, 10);
    Serial.print("Light: ");
    Serial.println(lightString);
    client.publish("yonsei/gsi/smart-desk/light", lightString);
  }
}

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

#define WIDTH 64
#define HEIGHT 32
#define TOTAL_PIXELS 2048

// WiFi credentials
// const char* ssid = "NachoWiFi";
// const char* password = "bubbleboy11";
const char* ssid = "this one";
const char* password = "Rockhall1";

// MQTT server info
const char* mqtt_server = "broker.mqttdashboard.com";
const char* mqtt_topic = "testtopic/Chase/mqtt-test";
const int mqtt_port = 1883;

WiFiClient espClient;
PubSubClient mqtt_client(espClient);

struct RGB {
    char r;
    char g;
    char b;
};

MatrixPanel_I2S_DMA* Matrix;

RGB INPUT_PIXELS[WIDTH*HEIGHT] = {0, 0, 0};


void connect_WifI() {
  WiFi.mode(WIFI_STA);
  Serial.print("Connecting to WiFi ..");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
}


void initMatrix() {

  // init LED pannel
  HUB75_I2S_CFG::i2s_pins _pins={25, 26, 27, 14, 12, 13, 23, 19, 5, 17, 32, 4, 15, 16};
  HUB75_I2S_CFG mxconfig(WIDTH, HEIGHT, 1, _pins);
  Matrix = new MatrixPanel_I2S_DMA(mxconfig);

  // let's adjust default brightness to about 75%
  Matrix->setBrightness8(100);    // range is 0-255, 0 - 0%, 255 - 100%

  // Allocate memory and start DMA display
  if( not Matrix->begin() )
    Serial.println("****** !KABOOM! I2S memory allocation failed ***********");

  Serial.println("Fill screen: RED");
  Matrix->fillScreenRGB888(255, 0, 0);
  delay(300);

  Serial.println("Fill screen: GREEN");
  Matrix->fillScreenRGB888(0, 255, 0);
  delay(300);

  Serial.println("Fill screen: BLUE");
  Matrix->fillScreenRGB888(0, 0, 255);
  delay(300);

  Serial.println("Fill screen: Neutral White");
  Matrix->fillScreenRGB888(64, 64, 64);
  delay(300);

  Serial.println("Fill screen: black");
  Matrix->fillScreenRGB888(0, 0, 0);
}


void updateDisplay(byte* newPixels) {


  // 32 x 64 = 2048 total pixels
  // Currently with all white at '255 255 255 ' = 12 chars
  // 12 chars * 2048 = ~24574 total bytes
  //
  // new idea, individual char for each r, g, b
  // 2048 * 3 = 6144 bytes always.
  // 74% smaller!!!!!

  /*
    Compare Speeds:

    Test_2: Dont Save Pixels in arr. Set pixel directly in loop
      Scenario_1: Best of Test_1
      Speed_1:
      Scenario_2: Set pixel directly in loop
      Speed_2:
    Test_3:If Test_1 is faster, test 3 arrsys vs array struct like we currently have
    Test_4:Might Be able to use different int type for len
  */

  // check if faster to keep pixelIndex or just do ((bytesIndex / 3) - 1)
  for (uint_fast16_t pixelIndex = 0; pixelIndex < TOTAL_PIXELS; pixelIndex++) {

    char r = *newPixels;
    newPixels++;
    char g = *newPixels;
    newPixels++;
    char b = *newPixels;
    newPixels++;

    //Serial.printf("R: %d G: %d B: %d\n", r, g, b);

    INPUT_PIXELS[pixelIndex] = {r, g, b};
	}

  uint_fast16_t pixelIndex = 0;
  for (uint_fast8_t y = 0; y < HEIGHT; y++) {
    for (uint_fast8_t x = 0; x < WIDTH; x++) {
      RGB pixel = INPUT_PIXELS[pixelIndex++];
      Matrix->drawPixel(x, y, Matrix->color565(pixel.r, pixel.g, pixel.b));
      // Matrix->drawPixel(x, y, Matrix->color333(pixel.r, pixel.g, pixel.b));
      // Matrix->drawPixel(x, y, Matrix->color444(pixel.r, pixel.g, pixel.b));
      // Matrix->drawPixel(x, y, Matrix->color565to888(pixel.r, pixel.g, pixel.b));
      //Matrix->drawPixelRGB888(x, y, pixel.r, pixel.g, pixel.b);
    }
  }
}




void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  Serial.println("Message arrived");
  Serial.println("Processing data");
  updateDisplay(payload);
  Serial.println("Done Processing data");
}

void reconnect() {
  // Loop until we're reconnected
  while (!mqtt_client.connected()) {
    Serial.print("Attempting MQTT connection...");
      
    // Create a random client ID
    String clientId = "ESP32Client-" + String(random(0xffff), HEX);
      
    // Attempt to connect
    if (mqtt_client.connect(clientId.c_str())) {
      Serial.println("connected");
      mqtt_client.subscribe(mqtt_topic);
    } 
    // attempt reconnect after 5 seconds
    else {
      Serial.print("failed, rc=");
      Serial.print(mqtt_client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  connect_WifI();

  initMatrix();

  // MQTT client settings
  mqtt_client.setBufferSize(7000);
  mqtt_client.setKeepAlive(10);

  mqtt_client.setServer(mqtt_server, mqtt_port);
  mqtt_client.setCallback(mqtt_callback);
}

void loop() {
  if (!mqtt_client.connected()) {
    reconnect();
  }
  mqtt_client.loop();
}
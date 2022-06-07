#include "esp_camera.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <MQTTClient.h>

#include "soc/soc.h"           // Disable brownour problems
#include "soc/rtc_cntl_reg.h"  // Disable brownour problems
#include "driver/rtc_io.h"

#define CAMERA_MODEL_AI_THINKER //definir el tipo de placa

const char* ESP32CAM_PUBLISH_TOPIC = "camara/puerta/sala";
const int bufferSize = 1024*32;
const char* ssid = "0.0.0.0";
const char* password = "!Hell@.TP!";
const char* mqtt_userName = "liz";
const char* mqtt_passKey = "Sistema_123";  

WiFiClient net;
MQTTClient client = MQTTClient(bufferSize);

void init_wifi(){
  int i;
  WiFi.begin(ssid, password);
  Serial.println("Conectando \n");
  while (WiFi.status() != WL_CONNECTED) {
    delay(50);
    Serial.print(".");
    i++;
    if(i==70){
        ESP.restart();
    }
  }
  Serial.println("");
  Serial.println("WiFi connected");
}


void reconnect() {
  client.begin("192.168.100.21", net);
  Serial.println("Conectando la Raspberry PI");  
  while (!client.connected()){
    Serial.print("Esperando conexión con MQTT...");
    // Creación de una ID aleatoria 
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    // Intentando conectar
    Serial.print(".");
    delay(10);
    if (client.connect(clientId.c_str(),mqtt_userName,mqtt_passKey)) {
      Serial.println("Conectado");
      //clientSubscribe("led01");
    } else {
      Serial.print("Fallo en la conexion");
      Serial.println("se intentará otra vez en 5 segundos");
      delay(10);
    }
  }
  Serial.println("Conectado al servidor MQTT");
}

void grab_image(){
  camera_fb_t * fb = esp_camera_fb_get();
  Serial.println("\n Movimiento detectado, cargando imagen");
  Serial.println("Capturando imagen");
  digitalWrite(LEDC_CHANNEL_0, HIGH);
  if(fb != NULL && fb->format == PIXFORMAT_JPEG && fb->len < bufferSize){
    Serial.print("Image Length: ");
    Serial.print(fb->len);
    Serial.print("\t Publish Image: ");
    bool result = false;
    result = client.publish(ESP32CAM_PUBLISH_TOPIC,(const char*)fb->buf, fb->len);
    if(!result){
      Serial.println("Captura fallida");
      grab_image();
    }
    Serial.print(" Con exito: ");
  }
  esp_camera_fb_return(fb);
  delay(1);
}
void sleep_esp(){
  pinMode(4, OUTPUT);  //GPIO for LED flash
  digitalWrite(4, LOW);  //turn OFF flash LED
  rtc_gpio_hold_en(GPIO_NUM_4);  //make sure flash is held LOW in sleep

  esp_sleep_enable_ext0_wakeup(GPIO_NUM_13, 0);
 
  Serial.println("Going to sleep now");
  delay(50);
  esp_deep_sleep_start();
  Serial.println("Now in Deep Sleep Mode");
  delay(50);
}

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  camara_init();
  init_wifi();
  reconnect();
  pinMode(4, INPUT);
  digitalWrite(4, LOW);
  rtc_gpio_hold_dis(GPIO_NUM_4);
  Serial.println("Motion detected!"); 
  Serial.println("Se envia imagen!"); 
  grab_image();
  sleep_esp();
}

void loop() {
  client.loop(); 
}


void camara_init(){
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;
  config.pin_d0       = 5;
  config.pin_d1       = 18;
  config.pin_d2       = 19;
  config.pin_d3       = 21;
  config.pin_d4       = 36;
  config.pin_d5       = 39;
  config.pin_d6       = 34;
  config.pin_d7       = 35;
  config.pin_xclk     = 0;
  config.pin_pclk     = 22;
  config.pin_vsync    = 25;
  config.pin_href     = 23;
  config.pin_sscb_sda = 26;
  config.pin_sscb_scl = 27;
  config.pin_pwdn     = 32;
  config.pin_reset    = -1;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  
  config.frame_size   = FRAMESIZE_QQVGA; // establece el tamaño de la imagen
  config.jpeg_quality = 10;            // calidad de la salida JPEG. 0-63 menor significa mayor calidad
  config.fb_count     = 2;
  
  // Inicialización de la cámara
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("La inicializacion de la cámara ha fallado con un error 0x%x", err);
    Serial.println(err);
    return;
  }
  Serial.println("Camara iniciada con exito");
}

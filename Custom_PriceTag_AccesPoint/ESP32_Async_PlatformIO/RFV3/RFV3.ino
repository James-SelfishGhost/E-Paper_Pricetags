#include <Arduino.h>
#include <SPI.h>
#include "RFV3.h"
#include "main_variables.h"
#include "cc1101_spi.h"
#include "cc1101.h"
#include "class.h"
#include "compression.h"
#include "interval_timer.h"
#include "settings.h"
#include "web.h"
#include "utils.h"
#include "mode_sync.h"
#include "mode_full_sync.h"
#include "mode_idle.h"
#include "mode_trans.h"
#include "mode_wu.h"
#include "mode_wu_reset.h"
#include "mode_wu_activation.h"
#include "mode_wun_activation.h"
#include "mode_activation.h"
#include "esp_task_wdt.h"
#include "trans_assist.h"
#include "settings.h"
#include <PubSubClient.h>

// #if defined(ARDUINO_ESP32S3_DEV)
//   #include <TFT_eSPI.h>
// #endif


WiFiClient espClient;
PubSubClient mqttClient(espClient);

const char* mqttServer = "mqtt.selfishghost.net";
const int mqttPort = 1883; // Port for SSL/TLS
const char* mqttUser = "SelfishMQTT";
const char* mqttPassword = "";


const char* commandTopic = "esp32/command";
const char* commandTopicSend = "esp32/command/send";
const char* commandTopicMode = "esp32/command/mode";
const char* responseTopic = "esp32/response";
const char* statusTopic = "esp32/status";
const char* modeTopic = "esp32/mode";
const char* fileUploadTopicBlack = "esp32/fileupload/black";
const char* fileUploadTopicRed = "esp32/fileupload/red";
const char* tagIdTopic = "esp32/tagid";
const char* sendStatusCommandTopic = "esp32/command/sendStatus";
const char* commandTopicWake = "esp32/command/wake";


class ModePlaceholder : public mode_class
{
};
ModePlaceholder modePlaceholder;

mode_class *currentMode = &modePlaceholder;
mode_class *tempMode = &modeIdle;

volatile int interrupt_counter = 0;
int no_count_counter = 0;
int tagid = 0;


volatile int int_fired = 0;
void IRAM_ATTR GDO2_interrupt()
{
  interrupt_counter++;
  int_fired++;
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  if (strcmp(topic, fileUploadTopicBlack) == 0) {
    // Create a unique filename based on the current timestamp
    String filename = "/black_layer.bmp";

    // Open the file for writing
    File file = SPIFFS.open(filename, "w");
    if (!file) {
      Serial.println("Failed to open file for writing");
      publishResponse("Failed to upload black layer");
      return;
    }

    // Write the received data to the file
    if (file.write(payload, length) != length) {
      Serial.println("Failed to write file");
      publishResponse("Failed to upload black layer");
    } else {
      Serial.println("File saved successfully");
      publishResponse("Black layer uploaded");
    }

    // Close the file
    file.close();
  } else if (strcmp(topic, fileUploadTopicRed) == 0) {
    // Create a unique filename based on the current timestamp
    String filename = "/red_layer.bmp";

    // Open the file for writing
    File file = SPIFFS.open(filename, "w");
    if (!file) {
      Serial.println("Failed to open file for writing");
      publishResponse("Failed to upload red layer");
      return;
    }

    // Write the received data to the file
    if (file.write(payload, length) != length) {
      Serial.println("Failed to write file");
      publishResponse("Failed to upload red layer");
    } else {
      Serial.println("File saved successfully");
      publishResponse("Red layer uploaded");
    }

    // Close the file
    file.close();
  } else if (strcmp(topic, sendStatusCommandTopic) == 0) {
    // Handle the "send status" command
    String status = get_mode_string();
    publishStatus(status.c_str());
  } else if (strcmp(topic, tagIdTopic) == 0) {
    // Handle the tag ID
    String tagId = String((char*)payload).substring(0, length);
    tagid=tagId.toInt();
  } else if (strcmp(topic, commandTopicSend) == 0) {
    // Handle the black and red image files
    String filename = "/black_layer.bmp";
    String filenameColor = "/red_layer.bmp";
    int id = tagid;
    Serial.println("MQTT COMMAND SEND ID: " + String(id));
    char buffer[50];
    sprintf(buffer, "MQTT COMMAND SEND ID: %d", id);
    publishResponse(buffer);

    if (!SPIFFS.exists(filename)) {
      Serial.println("Error opening file");
      publishStatus("Error opening file");
      return;
    }

    if (filenameColor != "" && !SPIFFS.exists(filenameColor)) {
      Serial.println("Error opening color file");
      publishStatus("Error opening color file");
      return;
    }

    int compressedLen = load_img_to_bufer(filename, filenameColor, 0);

    if (compressedLen) {
      set_is_data_waiting(id);
      Serial.println("OK cmd to display " + String(id) + " File: " + filename + " Len: " + String(compressedLen));
     // publishStatus("OK cmd to display " + String(id) + " File: " + filename + " Len: " + String(compressedLen));
    } else {
      Serial.println("Something wrong with the file");
      publishStatus("Something wrong with the file");
    }
  }  else if (strcmp(topic, commandTopicMode) == 0) {
    publishMode();
  } else if (strcmp(topic, commandTopicWake) == 0) {
    set_is_data_waiting(0);
    set_mode_full_sync();
  }

}

void publishMode() {
  String actiStatus = "";
  String sendStatus = "";

  switch (get_last_activation_status()) {
    case 0:
      actiStatus = "not started";
      break;
    case 1:
      actiStatus = "started";
      break;
    case 2:
      actiStatus = "timeout";
      break;
    case 3:
      actiStatus = "successful";
      break;
    default:
      actiStatus = "Error";
      break;
  }

  switch (get_last_send_status()) {
    case 0:
      sendStatus = "nothing send";
      break;
    case 1:
      sendStatus = "in sending";
      break;
    case 2:
      sendStatus = "timeout";
      break;
    case 3:
      sendStatus = "successful";
      break;
    default:
      sendStatus = "Error";
      break;
  }

  String modeData = "Send: " + sendStatus +
                    " , waiting: " + String(get_is_data_waiting_raw()) +
                    "\nActivation: " + actiStatus +
                    "\nNetID " + String(get_network_id()) +
                    " freq " + String(get_freq()) +
                    " slot " + String(get_slot_address()) +
                    " bytes left: " + String(get_still_to_send()) +
                    " Open: " + String(get_trans_file_open()) +
                    "\nlast answer: " + get_last_receive_string() +
                    "\nmode " + get_mode_string();

  mqttClient.publish(modeTopic, modeData.c_str());
}

void publishStatus(const char* status) {
  mqttClient.publish(statusTopic, status);
}

void publishResponse(const char* response) {
  mqttClient.publish(responseTopic, response);
}
void reconnectMQTT() {
  // Loop until we're reconnected
  #ifdef HAS_TFT
    extern void TFTLog(String text);
    TFTLog("Attempting MQTT connection...");
  #endif
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    log("Attempting MQTT connection...");
    // Create a unique client ID
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);

    // Attempt to connect
    if (mqttClient.connect(clientId.c_str(), mqttUser, mqttPassword)) {
      Serial.println("connected");
      mqttClient.subscribe(commandTopic);
      mqttClient.subscribe(fileUploadTopicBlack);
      mqttClient.subscribe(fileUploadTopicRed);
      mqttClient.subscribe(tagIdTopic);
      mqttClient.subscribe(statusTopic);
      mqttClient.subscribe(responseTopic);
      mqttClient.subscribe(commandTopicSend);
      mqttClient.subscribe(commandTopicMode);
      mqttClient.subscribe(commandTopicWake);
      log("MQTT reconnected");

      // Publish a message to indicate successful reconnection
      mqttClient.publish(statusTopic, "ESP32 reconnected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      log("Failed to connect to MQTT broker, rc=" + String(mqttClient.state()));


      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void init_interrupt()
{
  pinMode(GDO2, INPUT);
  attachInterrupt(GDO2, GDO2_interrupt, FALLING);
}

void log(String message)
{
  Serial.print(millis());
  Serial.println(" : " + message);
  // #ifdef HAS_TFT
  //   extern void TFTLog(String text);
  //   TFTLog(message);
  // #endif
}
void logTFT(String message)
{
  #ifdef HAS_TFT
    extern void TFTLog(String text);
    TFTLog(message);
  #endif
}

void setup()
{
  Serial.begin(500000);
  Serial.setDebugOutput(true);
  SPIFFS.begin(true);
  esp_task_wdt_init(30, true); // Set timeout to 30 seconds
  #ifdef HAS_TFT
    extern void yellow_ap_display_init(void);
    yellow_ap_display_init();
  #endif

  init_spi();
  uint8_t radio_status;
  while ((radio_status=init_radio()))
  {
    if (radio_status == 1)
      Serial.println("Radio not working!!! ERROR");
      logTFT("Radio not working!!! ERROR");
    if (radio_status == 2)
      Serial.println("GPIO2 Interrupt input not working");
      logTFT("GPIO2 Interrupt input not working");
    delay(1000);
  }
  read_boot_settings();
  init_interrupt();
  init_timer();
  init_web();
  mqttClient.setServer(mqttServer, mqttPort);
  mqttClient.setCallback(mqttCallback);

  // Set the maximum MQTT packet size
  mqttClient.setBufferSize(1024 * 5); // Adjust the buffer size as needed

  // Connect to MQTT broker
  while (!mqttClient.connected()) {
    if (mqttClient.connect("ESP32Client", mqttUser, mqttPassword)) {
      Serial.println("Connected to MQTT broker");
      mqttClient.subscribe(commandTopic);
      mqttClient.subscribe(fileUploadTopicBlack);
      mqttClient.subscribe(fileUploadTopicRed);
      mqttClient.subscribe(tagIdTopic);
      mqttClient.subscribe(statusTopic);
      mqttClient.subscribe(responseTopic);
      mqttClient.subscribe(commandTopicSend);
      mqttClient.subscribe(commandTopicMode);
      mqttClient.subscribe(commandTopicWake);
      log("MQTT connected");
      #ifdef HAS_TFT
        extern void TFTLog(String text);
        TFTLog("MQTT Connected");
      #endif
    } else {
      Serial.print("Failed to connect to MQTT broker, rc=" + String(mqttClient.state()));
      log("Failed to connect to MQTT broker, rc=" + String(mqttClient.state()));
      logTFT("Failed to connect to MQTT broker, rc=" + String(mqttClient.state()));
      Serial.println(" Retrying in 5 seconds...");
      delay(5000);
    }
  }
}
void loop()
{
  if (int_fired)
  {
    int_fired--;
    currentMode->interrupt();
  }
  if (currentMode != tempMode)
  {
    currentMode->post();
    currentMode = tempMode;
    log("Mode changed to " + currentMode->get_name());
    currentMode->pre();
  }
  currentMode->main();
  if (check_new_interval())
  {
    log("Count: " + String(interrupt_counter));
    if (interrupt_counter == 0)
    {
      if (no_count_counter++ > 5)
      {
        no_count_counter = 0;
        log("no interrupts anymore, something is broken, trying to fix it now");
        if (get_trans_mode())
        {
          set_trans_mode(0);
          restore_current_settings();
        }
        set_last_activation_status(0);
        if (currentMode == &modeIdle)
          set_mode_full_sync();
        else if (currentMode == &modeFullSync || currentMode == &modeWu || currentMode == &modeWuAct || currentMode == &modeWunAct || currentMode == &modeWuReset || currentMode == &modeActivation)
        {
          currentMode->pre();
        }
        else
          set_mode_idle();
      }
    }
    else
    {
      no_count_counter = 0;
    }
    interrupt_counter = 0;
    currentMode->new_interval();
  }
  #ifdef HAS_TFT
    extern void yellow_ap_display_loop(void);
    yellow_ap_display_loop();
    static unsigned long lastTFTUpdateTime = 0;
    if (millis() - lastTFTUpdateTime >= 1000) {
      lastTFTUpdateTime = millis();
      extern void publishModeTFT();
      publishModeTFT();
    }
  #endif
  
  esp_task_wdt_reset();
  if (!mqttClient.connected()) {
    reconnectMQTT();
  }
  mqttClient.loop();
}

void set_mode_idle()
{
  tempMode = &modeIdle;
}

void set_mode_sync()
{
  tempMode = &modeSync;
}

void set_mode_full_sync()
{
  tempMode = &modeFullSync;
}

void set_mode_trans()
{
  tempMode = &modeTrans;
}

void set_mode_wu()
{
  tempMode = &modeWu;
}

void set_mode_wu_reset()
{
  tempMode = &modeWuReset;
}

void set_mode_wu_activation()
{
  tempMode = &modeWuAct;
}

void set_mode_wun_activation()
{
  tempMode = &modeWunAct;
}

void set_mode_activation()
{
  tempMode = &modeActivation;
}

String get_mode_string()
{
  return currentMode->get_name();
}

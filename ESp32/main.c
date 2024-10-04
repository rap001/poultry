#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <freertos/semphr.h>

// Define your WiFi credentials and MQTT broker details
const char* ssid = "GNXS-2.4G-C68521";
const char* password = "Greenlabel@qwerty";
const char* mqtt_server = "192.168.1.40";
const char* coordinates = "esp/coordinates";
const char* validate = "esp/object";
String response;

SemaphoreHandle_t semaphore;
SemaphoreHandle_t paintSemaphore; // Semaphore for controlling paintingTask execution

typedef struct YourMessageType {
  int chickenId;  // Stores the unique ID of the chicken
  int x;          // X coordinate of the chicken's location
  int y;          // Y coordinate of the chicken's location
} YourMessageType;

WiFiClient espClient;
PubSubClient client(espClient);
QueueHandle_t messageQueue; // Declare the message queue handle

// Function prototypes for tasks and MQTT callback
void processTask(void *pvParameters);
void paintingTask(void *pvParameters);
void callback(char* topic, byte* payload, unsigned int length);
void checkAndProcessData(YourMessageType data); // Declaration of checkAndProcessData function

void setup() {
  Serial.begin(115200);

  // Initialize WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to the WiFi network");

  // Initialize MQTT client
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  response = "esp/response" + WiFi.macAddress();

  // Create semaphores
  semaphore = xSemaphoreCreateBinary();
  paintSemaphore = xSemaphoreCreateBinary(); // Create paint semaphore

  // Create the message queue
  messageQueue = xQueueCreate(10, sizeof(YourMessageType)); // Adjust queue size and message type

  // Check if semaphores were created successfully
  if (semaphore != NULL && paintSemaphore != NULL) {
    // Give the semaphore to start with
    xSemaphoreGive(semaphore);

    // Create FreeRTOS tasks
    xTaskCreate(processTask, "ProcessTask", 2048, NULL, 1, NULL);
    xTaskCreate(paintingTask, "PaintingTask", 2048, NULL, 1, NULL); // Create painting task
  } else {
    // Semaphore creation failed
    Serial.println("Semaphore creation failed!");
    while (1);
  }
}

void loop() {
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    if (client.connect("ESP32Client1")) {
      Serial.println("connected");
      // Subscribe to your topic here
      client.subscribe(coordinates);
      client.subscribe(response.c_str()); // Subscribe to the response topic
    } else {
      Serial.print("failed with state ");
      Serial.print(client.state());
    
      delay(1000);
    }
  }
}

// Implement the callback function to handle incoming messages
void callback(char* topic, byte* payload, unsigned int length) {
  YourMessageType message; // Define your message structure
  payload[length] = '\0';
  String jsonString = String((char*)payload);
  DynamicJsonDocument doc(1024); // Allocate memory for the JSON object
  deserializeJson(doc, jsonString);
  JsonObject obj = doc.as<JsonObject>();

  Serial.print("Received message on topic: ");
  Serial.println(topic); // Print received topic for debugging

  if (strcmp(topic, coordinates) == 0) {
    message.chickenId = obj["id"];
    message.x = obj["x"];
    message.y = obj["y"];
    Serial.print("x: ");
    checkAndProcessData(message);
    xSemaphoreGive(semaphore);
  } else if (strcmp(topic, response.c_str()) == 0) {
      xSemaphoreGive(paintSemaphore); // Give paint semaphore when painting is required
      
  }
}

// Implement the processing task
void processTask(void *pvParameters) {
  Serial.println("Starting processTask...");
  
  YourMessageType message;
  
  while (true) {
    // Wait for a message to be available in the message queue
    Serial.println("executing Process task");
    if (xSemaphoreTake(semaphore, portMAX_DELAY) == pdTRUE) {
      if (xQueueReceive(messageQueue, &message, portMAX_DELAY) == pdPASS) {
        // Create a JSON document and populate it with message data
        StaticJsonDocument<200> doc;
        doc["host"] = WiFi.macAddress();
        doc["id"] = message.chickenId;
        doc["x"] = message.x;
        doc["y"] = message.y;
        
        // Serialize the JSON document to a string
        String jsonString;
        serializeJson(doc, jsonString);
        
        // Publish the JSON string to the MQTT topic
        client.publish(validate, jsonString.c_str());
        
        // Print debug information
        Serial.println("Published message:");
        Serial.println(jsonString);
      } else {
        Serial.println("Failed to receive message from queue!");
      }
      xSemaphoreGive(semaphore); // Release semaphore
    } else {
      Serial.println("Failed to take semaphore!");
    }

    // Delay to avoid busy-waiting
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
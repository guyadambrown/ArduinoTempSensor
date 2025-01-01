#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include "WiFiS3.h"
#include "Arduino_LED_Matrix.h"
#include "ArduinoGraphics.h"

ArduinoLEDMatrix matrix;

#define DHT_pin 2
#define DHT_type DHT11
#define red_pin 9
#define green_pin 10
#define blue_pin 11


DHT dht(DHT_pin, DHT_type);



struct TempHumidity {
    float temperature{};
    float humidity{};
    String status;
};

float manualTemperature = NAN; // Initialize to NAN

TempHumidity readTemperatureAndHumidity() {
    TempHumidity data;
    data.humidity = dht.readHumidity();
    data.temperature = isnan(manualTemperature) ? dht.readTemperature() : manualTemperature; // If the manual temp is not set, read from the sensor

    // Check if any reading failed
    if (isnan(data.humidity) || isnan(data.temperature)) {
        data.humidity = -1;     // Use -1 to indicate an error
        data.temperature = -1;  // Use -1 to indicate an error
        data.status = "ERROR";
    }

    if (data.temperature < 18)
    {
        // Low temp
        data.status = "LOW";
    }else if (data.temperature > 18.0 && data.temperature < 27.0)
    {
        // Good temp
        data.status = "OK";
    } else if (data.temperature > 27.0)
    {
        // High temp
        data.status = "HIGH";
    }

    return data;
}

// Function to convert float to byte array
void floatToBytes(const float value, uint8_t *bytes) {
    memcpy(bytes, &value, sizeof(value));
}

void displayText(const char* text) {
    matrix.clear();
    matrix.beginDraw();
    matrix.stroke(0xFFFFFFFF);
    matrix.textFont(Font_5x7);
    matrix.beginText(0, 1, 0xFFFFFF);
    matrix.println(text);
    matrix.endText();

    matrix.endDraw();
}

void handleSerialInput()
{
    if (Serial.available() > 0) {
        if (const String input = Serial.readStringUntil('\n'); input.startsWith("temp")) {
            if (input.substring(5)) {
                manualTemperature = input.substring(5).toFloat();
            }
        } else if (input.startsWith("DHT")) {
            manualTemperature = NAN;

        }
    }
}

void setup() {
    Serial.begin(9600);
    matrix.begin();
    matrix.beginDraw();
    pinMode(red_pin, OUTPUT);
    pinMode(green_pin, OUTPUT);
    pinMode(blue_pin, OUTPUT);
    dht.begin();
    // Sleep for 2 seconds to allow the sensor to stabilize
    delay(2000);
}

void loop() {
    handleSerialInput();


    if (auto [temperature, humidity, status] = readTemperatureAndHumidity(); humidity == -1 || temperature == -1) {
        Serial.println("{'temperature': 'fail', 'humidity': 'fail', 'status': 'ERROR'}");

    } else {
        const String tempString = "{\"temperature\":" + String(temperature) + ", \"humidity\":" + String(humidity) + R"(, "status":")" + String(status) + "\"}\n";
        Serial.println(tempString);

        if (status == "LOW")
        {
            // Blue
            analogWrite(red_pin, 255);
            analogWrite(green_pin, 255);
            analogWrite(blue_pin, 0);
        } else if (status == "OK")
        {
            // Green
            analogWrite(red_pin, 255);
            analogWrite(green_pin, 0);
            analogWrite(blue_pin, 255);
        } else if (status == "HIGH" || status == "ERROR")
        {
            // Red
            analogWrite(red_pin, 0);
            analogWrite(green_pin, 255);
            analogWrite(blue_pin, 255);
        }

        char tempBuffer[8];
        dtostrf(temperature, 0, 0, tempBuffer);

        displayText(tempBuffer);

    }

    delay(10); // Wait 10 milliseconds
}

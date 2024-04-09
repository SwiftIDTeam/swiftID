#include <Arduino.h>
#include <SPI.h>

// put function declarations here:
//int myFunction(int, int);

#define GPIO_INT_PIN 000000 //******** REPLACE THIS WITH PIN WE CONNECT BUTTON TO*******

// USING VSPI pins (Could also use HSPI miso= 12, mosi= 13, ss= 15 , sck= 14)

// Define the SPI pins for ESP32 - VSPI
#define RFID_SS_PIN   5   // SS pin for RFID reader
#define RFID_RST_PIN  4   // Reset pin for RFID reader

// Define the VSPI pins
#define VSPI_SCK  18  // SCK pin for VSPI
#define VSPI_MISO 19  // MISO pin for VSPI
#define VSPI_MOSI 23  // MOSI pin for VSPI

// Create an instance of the SPI class for VSPI
SPIClass RFID_SPI(VSPI);

volatile bool scanRequested = false;

void IRAM_ATTR handleInterrupt() { //ISR routine (sets flag)
  scanRequested = true;
}

// Function to setup SPI communication
void setupSPI() {
  // Initialize SPI communication on VSPI
  RFID_SPI.begin();

  // Configure RFID reset pin
  pinMode(RFID_RST_PIN, OUTPUT);
  
  // Attach interrupt to the VSPI interrupt pin (not used in VSPI, but placeholder)
  attachInterrupt(digitalPinToInterrupt(GPIO_INT_PIN), handleInterrupt, FALLING);

  // Reset the RFID reader
  digitalWrite(RFID_RST_PIN, HIGH);
  delay(100);
  digitalWrite(RFID_RST_PIN, LOW);
  delay(100);
  digitalWrite(RFID_RST_PIN, HIGH);
  delay(100);

  // Set SS pin to high (deselect)
  digitalWrite(RFID_SS_PIN, HIGH);

  // Configure RFID reader settings here
}

void setup() {
  // Initialize Serial Monitor
  Serial.begin(9600);

  // Set LED to output
  pinMode(LED_BUILTIN, OUTPUT);

  // Call setupSPI function to initialize SPI communication
  setupSPI();

}

void loop() {
  if (scanRequested) {
    scanRequested = false;
    //turn on LED
    digitalWrite(LED_BUILTIN, HIGH);
    // Select RFID reader
    digitalWrite(RFID_SS_PIN, LOW);

    // Send command to start scanning
    // Example: RFID_SPI.transfer(0x01); // Assuming 0x01 is the command to start scanning

    // Deselect RFID reader
    digitalWrite(RFID_SS_PIN, HIGH);

    // Wait for scan to complete (we can probably set some interrupt for this but if not a delay() function will work)
    delay(1000); // Adjust delay as needed

    // Read RFID data
    // Example: int data = RFID_SPI.transfer(0x00); // Assuming 0x00 is the command to read data

    // Process RFID data
    // Example:
    // Serial.println(data);

    //turn off LED
    digitalWrite(LED_BUILTIN, LOW);
  }
}

// put function definitions here:
//int myFunction(int x, int y) {
  //return x + y;
//}

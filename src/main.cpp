#include <Arduino.h>
#include <SPI.h>
#include <RFID.h>


#define GPIO_BUTTON_PIN 000000 //******** REPLACE THIS WITH PIN WE CONNECT BUTTON TO*******

// USING VSPI pins (Could also use HSPI miso= 12, mosi= 13, ss= 15 , sck= 14)

// Define the SPI pins for ESP32 - VSPI
#define RFID_SS_PIN   5   // SS pin for RFID reader
#define RFID_RST_PIN  4   // Reset pin for RFID reader (we would need to add a button to do this)

// Define the VSPI pins
#define VSPI_SCK  18  // SCK pin for VSPI
#define VSPI_MISO 19  // MISO pin for VSPI
#define VSPI_MOSI 23  // MOSI pin for VSPI

// Create an instance of the SPI class for VSPI
SPIClass RFID_SPI(VSPI);

// Create an instance of the RFID class
RFID rfid(RFID_SS_PIN, RFID_RST_PIN);

volatile bool scanRequested = false;

int serNum[5]; // stores scanned RFID

void IRAM_ATTR buttonISR() { //ISR routine (sets flag)
  scanRequested = true;
}

// Function to setup SPI communication
void Init_SPI() {
  // Initialize SPI communication on VSPI
  RFID_SPI.begin();

  // Configure RFID reset pin
  pinMode(RFID_RST_PIN, OUTPUT);
  
  // Attach interrupt to the VSPI interrupt pin (not used in VSPI, but placeholder)
  attachInterrupt(digitalPinToInterrupt(GPIO_BUTTON_PIN), buttonISR, FALLING);

  // Reset the RFID reader
  rfid.reset();
  //******************* OLD RESET CODE *************
  /*
  digitalWrite(RFID_RST_PIN, HIGH);
  delay(100);
  digitalWrite(RFID_RST_PIN, LOW);
  delay(100);
  digitalWrite(RFID_RST_PIN, HIGH);
  delay(100);
  */

  // Set SS pin to high (deselect)
  digitalWrite(RFID_SS_PIN, HIGH);

  // Configure RFID reader settings here
}

void setup() {
  // Initialize Serial Monitor
  Serial.begin(9600);

  // initialize RFID
  rfid.init();

  // Set LED to output
  pinMode(LED_BUILTIN, OUTPUT);
  // Set button gpio pin to input ******************************** NEED TO GET BUTTON AND FIGURE OUT WHAT PIN TO CONNECT TO
  //pinMode(GPIO_INT_PIN, INPUT);

  // Call init_SPI function to initialize SPI communication
  Init_SPI();

}

void loop() {
  if (scanRequested) {
    scanRequested = false;
    //turn on LED
    digitalWrite(LED_BUILTIN, HIGH);
    // Select RFID reader
    digitalWrite(RFID_SS_PIN, LOW);
    
    if(rfid.isCard()){ // Checks if a card is near the scanner

      if(rfid.readCardSerial()){ //reads the number from the card

        // Wait for scan to complete (we can probably set some interrupt for this but if not a delay() function will work)
        delay(100); // Adjust delay as needed

        // move into our serNum[] variable outside the rfid class so we can do more processing and send via bluetooth
        for (int i = 0; i < 5; i++) {
          serNum[i] = rfid.serNum[i];
        }

        //print card number
        Serial.print("Detected Card: ");
        for (int i = 0; i < 5; i++) {
          Serial.print(serNum[i]);
          Serial.print(" ");
        }
        Serial.println("");
      }
      else{
        Serial.print("Error: Unable To Read Card Serial Number");
      }

    }
    else{
      Serial.print("Error: No Card Detected");
    }

    // Deselect RFID reader
    digitalWrite(RFID_SS_PIN, HIGH);

    //turn off LED
    digitalWrite(LED_BUILTIN, LOW);

    rfid.halt(); //stop RFID reader

    // From here we can implement some varification or something using the Serial Number 
    // For instance, if we had access to database we would check that its in the schools database, etc...

    // Process RFID data
    // Example:
    // Serial.println(data);

  }
}


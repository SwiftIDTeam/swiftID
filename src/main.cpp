#include <Arduino.h>
#include <SPI.h>
#include <RFID.h>
#include <BluetoothSerial.h>


#define GPIO_BUTTON_READ_PIN 32 // Pin connected to button used to trigger scan (initiates software interrupt)

// USING VSPI pins (Could also use HSPI miso= 12, mosi= 13, ss= 15 , sck= 14)

// Define the SPI pins for ESP32 - VSPI
#define RFID_SS_PIN   5   // SS pin for RFID reader
#define RFID_RST_PIN  4   // Reset pin for RFID reader (we would need to add a button to do this)

// Define the VSPI pins
#define VSPI_SCK  18  // SCK pin for VSPI
#define VSPI_MISO 19  // MISO pin for VSPI
#define VSPI_MOSI 23  // MOSI pin for VSPI

//the size of the array of integers that will contain the serial number form a card
#define SerNum_Array_Size 5 // ** this can change with different cards **

// Create an instance of the SPI class for VSPI
SPIClass RFID_SPI(VSPI);

// Create an instance of the RFID class
RFID rfid(RFID_SS_PIN, RFID_RST_PIN);

//create an instance of the bluetooth class
BluetoothSerial ESP32_BT;

volatile bool scanRequested = false;

int serNum[SerNum_Array_Size]; // stores scanned RFID

unsigned long lastDebounceTime; //stores last time button was pressed

unsigned long debounceDelay = 50; //50 millisecond delay

void IRAM_ATTR read_buttonISR() { //ISR routine (sets flag)
  if ((millis() - lastDebounceTime) > debounceDelay) { //if there was more than 50 milliseconds between presses its a valid press (avoids mechanical bounce affect)
  scanRequested = true;
  lastDebounceTime = millis(); //save last time button was pressed
  }
}

//function to handle sending RFID via bluetooth
void Send_RFID_BT(int array[], int length){
  /*
  Im not sure what the endianess of our phone is but the esp32 is little endian so it sends data in little endian
  if we need to change to big endian we can use this for loop

  // Convert array to big-endian byte order
  for (int i = 0; i < length; i++) {
    array[i] = htonl(array[i]); // htonl function converts to big-endian
  }
  */

  // Send the raw binary data over Bluetooth
  ESP32_BT.write((uint8_t*)array, length * sizeof(int));
}

// Function to setup SPI communication
void Init_SPI() {
  // Initialize SPI communication on VSPI
  RFID_SPI.begin();

  // Configure RFID reset pin
  pinMode(RFID_RST_PIN, OUTPUT);
  
  // Attach interrupt to the VSPI interrupt pin (not used in VSPI, but placeholder)
  //read interrupt
  attachInterrupt(digitalPinToInterrupt(GPIO_BUTTON_READ_PIN), read_buttonISR, FALLING);

  // Reset the RFID reader
  rfid.reset();

  // Set SS pin to high (deselect)
  digitalWrite(RFID_SS_PIN, HIGH);

  // Configure RFID reader settings here
}

void setup() {
  // Initialize Serial Monitor
  Serial.begin(9600);

  // initialize RFID
  rfid.init();

  //initialize Bluetooth
  ESP32_BT.begin("ESP32_RFID_Module");

  // Set LED to output
  pinMode(LED_BUILTIN, OUTPUT);
  // Set button gpio pin to input
  pinMode(GPIO_BUTTON_READ_PIN, INPUT_PULLUP);//read

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
        for (int i = 0; i < SerNum_Array_Size; i++) {
          serNum[i] = rfid.serNum[i];
        }

        //print card number
        Serial.print("Detected Card: ");
        for (int i = 0; i < SerNum_Array_Size; i++) {
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

    Send_RFID_BT(serNum, SerNum_Array_Size); //send the scanned serial number to phone via bluetooth

    //*** Implement some error handling ***

    // Process RFID data
    // Example:
    // Serial.println(data);

  }

}


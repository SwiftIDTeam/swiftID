#include <Arduino.h>
#include <SPI.h>
#include <RFID.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>


#define GPIO_BUTTON_READ_PIN 21 // Pin connected to button used to trigger scan (initiates software interrupt)
#define LED_PIN 2 //built in blue LED

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

// Create an instance of the RFID class
RFID rfid(RFID_SS_PIN, RFID_RST_PIN);

volatile bool scanRequested = false;

int serNum[SerNum_Array_Size]; // stores scanned RFID

unsigned long lastDebounceTime; //stores last time button was pressed

unsigned long debounceDelay = 500; //500 millisecond delay

void IRAM_ATTR read_buttonISR() { //ISR routine (sets flag)
  if ((millis() - lastDebounceTime) > debounceDelay) { //if there was more than 500 milliseconds between presses its a valid press (avoids mechanical bounce affect)
  scanRequested = true;
  lastDebounceTime = millis(); //save last time button was pressed
  }
}

//setting up BLE params
BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
BLEDescriptor *pDescr;
BLE2902 *pBLE2902;

bool deviceConnected = false;
bool oldDeviceConnected = false;
String value = "";

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

class MyServerCallbacks: public BLEServerCallbacks { //callback to change flag when device connects
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) { //callback to change flag when device disconnects
      deviceConnected = false;
    }
};

// Function to setup SPI communication
void Init_SPI() {
  // Initialize SPI communication on VSPI
  SPI.begin();

  // Configure RFID reset pin
  pinMode(RFID_RST_PIN, OUTPUT);
  
  // Attach interrupt to the interrupt pin
  //read interrupt
  attachInterrupt(digitalPinToInterrupt(GPIO_BUTTON_READ_PIN), read_buttonISR, FALLING); //initiates when we tansition from high to low

  // Reset the RFID reader
  rfid.reset();

  // Set SS pin to high (deselect)
  digitalWrite(RFID_SS_PIN, HIGH);

  // Configure RFID reader settings here
}

void Init_BLE(){
   // Create the BLE Device
  BLEDevice::init("ESP32_RFID");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_NOTIFY);

  // Create a BLE Descriptor
  pDescr = new BLEDescriptor((uint16_t)0x2901);
  pDescr->setValue("A very interesting variable");
  pCharacteristic->addDescriptor(pDescr);
  
  pBLE2902 = new BLE2902();
  pBLE2902->setNotifications(true);
  pCharacteristic->addDescriptor(pBLE2902);

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  Serial.println("Waiting a client connection to notify...");
}

void setup() {
  // Initialize Serial Monitor
  Serial.begin(9600);

  // Call init_SPI function to initialize SPI communication
  Init_SPI(); //*** must do this before initializing rfid module ***

  // initialize RFID
  rfid.init();;

  // Set LED to output
  pinMode(LED_PIN, OUTPUT);
  // Set button gpio pin to input
  pinMode(GPIO_BUTTON_READ_PIN, INPUT_PULLUP);//read

  digitalWrite(LED_PIN, LOW);

  //BLE initialization
  Init_BLE();

}

void loop() {
  if (scanRequested) {
    //turn on LED
    digitalWrite(LED_PIN, HIGH);
    // Select RFID reader
    digitalWrite(RFID_SS_PIN, LOW);

    if(rfid.isCard()){ // Checks if a card is near the scanner
      //Serial.println("Step 1");

      if(rfid.readCardSerial()){ //reads the number from the card
        //Serial.println("Step 2");

        // move into our serNum[] variable outside the rfid class so we can do more processing and send via bluetooth
        for (int i = 0; i < SerNum_Array_Size; i++) {
          //Serial.println(i);
          serNum[i] = rfid.serNum[i];
        }

        //print card number
        Serial.println("Detected Card: ");
        for (int i = 0; i < SerNum_Array_Size; i++) {
          Serial.print(serNum[i]);
          Serial.print(" ");
        }
        Serial.println("");
      }
      else{
        Serial.println(" Error: Unable To Read Card Serial Number");
      }

    }
    else{
      Serial.println("Error: No Card Detected");
    }

    // Deselect RFID reader
    digitalWrite(RFID_SS_PIN, HIGH);

    //turn off LED
    digitalWrite(LED_PIN, LOW);

    //Send_RFID_BT(serNum, SerNum_Array_Size); //send the scanned serial number to phone via bluetooth

    if (deviceConnected) {
      for (int i = 0; i < SerNum_Array_Size; i++) {
        pCharacteristic->setValue(serNum[i]);
        pCharacteristic->notify();
        delay(1000);
        }
    }

    scanRequested = false;

    rfid.halt(); //stop RFID reader

  }

}
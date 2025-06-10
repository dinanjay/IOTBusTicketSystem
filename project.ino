#include <math.h>
#include <SPI.h>
#include <MFRC522.h>

// Define pins for RC522 RFID module (adjust if needed for your ESP32-S3)
#define SS_PIN 5   // SDA pin, ensure this GPIO is free
#define RST_PIN 4  // Reset pin, ensure this GPIO is free
// Custom SPI pins (uncomment and adjust if defaults don't work)
#define SCK_PIN 18
#define MISO_PIN 19
#define MOSI_PIN 21

MFRC522 mfrc522(SS_PIN, RST_PIN);

// Passenger data structure
struct Passenger {
  String uid;          // RFID card UID
  float balance;       // Passenger's balance
  bool isOnBus;        // Tracks if passenger is on the bus
  double boardingLat;  // Boarding latitude
  double boardingLon;  // Boarding longitude
};

// Predefined passengers (local storage, easily adaptable to Firebase)
const int numPassengers = 2;
Passenger passengers[numPassengers] = {
  { "d73a6304", 200.0, false, 0.0, 0.0 },  // Example UID with initial balance
  { "e90f3684", 400.0, false, 0.0, 0.0 }   // Another example UID 
};

// Base coordinates (e.g., Colombo, Sri Lanka)
const double baseLat = 6.9271;
const double baseLon = 79.8612;
const double R = 6371.0;  // Earth's radius in km

// Convert degrees to radians
double toRadians(double degree) {
  return degree * (M_PI / 180.0);
}

// Haversine formula to calculate distance between two coordinates
double calculateDistance(double lat1, double lon1, double lat2, double lon2) {
  double dLat = toRadians(lat2 - lat1);
  double dLon = toRadians(lon2 - lon1);
  double a = sin(dLat / 2) * sin(dLat / 2) + cos(toRadians(lat1)) * cos(toRadians(lat2)) * sin(dLon / 2) * sin(dLon / 2);
  double c = 2 * atan2(sqrt(a), sqrt(1 - a));
  return R * c;
}

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  while (!Serial) {
    delay(10);  // Wait for serial port to connect
  }

  // Initialize SPI and RFID module
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SS_PIN);  // Explicitly set SPI pins
  mfrc522.PCD_Init();

  // Seed random number generator
  randomSeed(millis());

  Serial.println("System initialized. Tap RFID card to begin.");
}

void loop() {
  // Check for RFID card
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    // Read card UID
    String uid = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      if (mfrc522.uid.uidByte[i] < 0x10) {
        uid += "0";  // Pad single digits with leading zero
      }
      uid += String(mfrc522.uid.uidByte[i], HEX);
    }
    uid.toLowerCase();  // Ensure consistent lowercase for comparison

    // Find passenger with matching UID
    bool found = false;
    for (int i = 0; i < numPassengers; i++) {
      if (passengers[i].uid == uid) {
        found = true;
        if (!passengers[i].isOnBus) {
          // Boarding process

          if (passengers[i].balance > 0) {
            passengers[i].isOnBus = true;
            passengers[i].boardingLat = baseLat + (random(-500, 500) / 10000.0);
            passengers[i].boardingLon = baseLon + (random(-500, 500) / 10000.0);
            Serial.println("Passenger " + uid + " boarded at lat: " + String(passengers[i].boardingLat, 6) + ", lon: " + String(passengers[i].boardingLon, 6));
            rgbLedWrite(RGB_BUILTIN, 0, 0, 255); 
            delay(1000);                       
            rgbLedWrite(RGB_BUILTIN, 0, 0, 0);
          } else {
            Serial.println("Cannot board: negative balance (" + String(passengers[i].balance, 2) + ")");
            rgbLedWrite(RGB_BUILTIN, 255, 0, 0); 
            delay(200);                           
            rgbLedWrite(RGB_BUILTIN, 0, 0, 0);
            delay(200);                           
            rgbLedWrite(RGB_BUILTIN, 255, 0, 0);  
            delay(200);                         
            rgbLedWrite(RGB_BUILTIN, 0, 0, 0);
            delay(200);                           
            rgbLedWrite(RGB_BUILTIN, 255, 0, 0); 
            delay(200);                          
            rgbLedWrite(RGB_BUILTIN, 0, 0, 0);
          }

        } else {

          double alightingLat = baseLat + (random(-500, 500) / 10000.0);
          double alightingLon = baseLon + (random(-500, 500) / 10000.0);
          double distance = calculateDistance(passengers[i].boardingLat, passengers[i].boardingLon, alightingLat, alightingLon);
          float fare = distance * 10.0;  // 10 units per km

          passengers[i].balance -= fare;  // Deduct fare, can go negative
          Serial.println("Passenger " + uid + " alighted at lat: " + String(alightingLat, 6) + ", lon: " + String(alightingLon, 6));
          Serial.println("Distance: " + String(distance, 2) + " km");
          Serial.println("Fare: " + String(fare, 2) + " units");
          Serial.println("New balance: " + String(passengers[i].balance, 2) + " units");
          rgbLedWrite(RGB_BUILTIN, 0, 255, 0); 
          delay(1000);                      
          rgbLedWrite(RGB_BUILTIN, 0, 0, 0); 
          if (passengers[i].balance < 0) {
            Serial.println("Warning: Balance is negative. Please top up before next trip.");
          }
          passengers[i].isOnBus = false;
        }
        break;
      }
    }

    // Handle unknown card
    if (!found) {
      Serial.println("Unknown card: " + uid);
      rgbLedWrite(RGB_BUILTIN, 255, 0, 0); 
      delay(1000);                          
      rgbLedWrite(RGB_BUILTIN, 0, 0, 0);    
    }

    // Halt RFID card to allow next read
    mfrc522.PICC_HaltA();
    delay(100);  // Small delay to prevent multiple reads
  }
}

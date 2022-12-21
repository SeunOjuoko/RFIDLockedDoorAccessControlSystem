#include <Adafruit_GFX.h>                 //Library for OLED to display on the hardware
#include <Adafruit_SSD1306.h>             //Library for the 4pin OLED for monochrome 128x32 pixels
#include <MFRC522.h>                      //Library for MFRC522 sensing module 
#include <SPI.h>                          //Library for the Serial Peripheral Interface (SPI) bus
#include <Servo.h>                        //Library for Servo Motor
#include <SoftwareSerial.h>               //Library for Sim900 GSM Module

//Display Settings
#define OLED_RESET 4                      
Adafruit_SSD1306 display(OLED_RESET);     //Initialises  OLED reset signal to Pin A4
Servo servoMotor;                         //Creates the instance for the Servo Motor 

#define Rx              2                 //Initialises Rx signal to Pin 2
#define Tx              3                 //Initialises Tx signal to Pin 3
#define buzzer          5                 //Initialises buzzer signal to Pin 5
#define greenLED        6                 //Initialises Green LED signal to Pin 6
#define redLED          7                 //Initialises Red LED signal to Pin 7
#define RST_PIN         8                 //Initialises MFRC522 Reset signal to Pin 8
#define SS_PIN          10                //Initialises MFRC522 Serial data signal to Pin 10
        
MFRC522 mfrc522(SS_PIN, RST_PIN);         //Creates instance for the MFRC522
MFRC522::MIFARE_Key key;                  //Creates instance Key for card authentication 
SoftwareSerial SIM900(Rx, Tx);            //Creates instance for the Sim900 GSM Module

String UID;                               //String that stores the RFID Tag's UID
String Name;                              //String that stores the RFID Tag's Name
String Role;                              //String that stores the RFID Tag's Role

byte sector         = 12;                 //Sector 12 holds the desired data
byte nameBlock      = 48;                 //Block 48 stores the name as Hexadecimal bytes  
byte roleBlock      = 49;                 //Block 49 stores the role as Hexadecimal bytes
byte trailerBlock   = 51;                 //Sector railer block 51

byte buffer[18];                          //Defined the buffer variable
byte size = sizeof(buffer);               //Defines the size of the buffer

void setup() {
   //This Initialies serial communications with the PC
  Serial.begin(9600);   
  // Does nothing if no serial port is opened
  while (!Serial);    
  //Initlialises the Serial Peripheral Interface (SPI) bus
  SPI.begin();      
  //Initialises the MFRC522
  mfrc522.PCD_Init();   
  
  //Key A defined for card authentication
  //Password: FF FF FF FF AA 13
  key.keyByte[0] = 0xFF ; 
  key.keyByte[1] = 0xFF ; 
  key.keyByte[2] = 0xFF ; 
  key.keyByte[3] = 0xFF ; 
  key.keyByte[4] = 0xAA ;
  key.keyByte[5] = 0x13 ;
  
  //Initialises the OLED 
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  //Begins with "SCAN CARD" displayed on screen
  Default();
  
  //Red LED begins on
  digitalWrite(redLED, HIGH);
  
  //Attaches Servo Motor to pin 9(PWM signal)
  servoMotor.attach(9);
  //Door lock begins locked
  servoMotor.write(0);

  //Sets up the baud rate for the GSM Module  
  SIM900.begin(9600); 
  delay(100);
  ///Sets the GSM Module in Text Mode
  SIM900.println("AT+CMGF=1");   
  //Delay for 1 second
  delay(1000);    
}

void loop() {
  //This resets the loop if no new cards were present on the MFRC522 reader (saves the entire process when idle).
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    return;
  }
 
  // Selects one of the tags
  if ( ! mfrc522.PICC_ReadCardSerial()) { 
    return;
  }

//Converts the UID to Hexadecimal
  String content= "";
  byte letter;
  for (byte i = 0; i < mfrc522.uid.size; i++) 
  {
     Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
     Serial.print(mfrc522.uid.uidByte[i], HEX);
     content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
     content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  
    //Declares the variable to compare MFRC522 to the status 
    MFRC522::StatusCode status;
    //Displays the PICC type data of the card 
    dump_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size);
    //MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);

  content.toUpperCase();
  //If the MFRC522 recieves a UID from an RFID tag this conditional statement commences... 
  if(content.substring(1)!=UID){
    //Erases the content in the OLED
     erase();
     //Stores the UID from the tag to the string
     UID=content.substring(1); 
     //Authenticates the PICC data from the scanned card by the comparing the sector trailer using key A
     status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
     //If the data presented an invalid status then the tag is presetended as invalid 
    if (status != MFRC522::STATUS_OK) {
      //OLED presents the tag's UID and the text INVALID 
      Invalid();
      delay(2000);
      //OLED presents Indentity Theft 
      IdentityTheft();
      //SIM900 GSM calls the phone number to alert the guest
      CallGuest();
      delay(2000);
    }else{
      //If the data presented an valid status then the OLED presents Valid  
      ValidUID();
      delay(2000);
      //Distinguishes whehter the tag is the Guest, Stranger, Housekeeping and Room Service based on the UID 
       if(UID == "D4 7C 93 29") {
            //Grants Access to the "Guest" tag and presents this on the OLED for 2 seconds 
            AccessGranted();
            //SIM900 GSM sends a welcome message when entering or to notify whether the guest's ID tag was stolen 
            SendMessage("Welcome Back");
            delay(2000);
            //Unlocks the door for the guest to enter their room
            OpenDoor();
      }else if(UID == "F3 08 E9 94") {
            //Denies Access to the "Stranger" tag and presents this on the OLED for 2 seconds
            AccessDenied();
            //SIM900 GSM sends a message to inform the guest on their phone
            SendMessage("A stranger accidentally tried to enter you room.");
            delay(2000);
            //Reads the written data blocks within sector 12 from the Stranger ID tag
            ReadData();
            delay(5000);
      }else if (UID =="C3 60 B1 12") {
            //Presents "Please wait..." on the OLED for 2 seconds to send a message to the guest's phone
            PleaseWait();
            //SIM900 GSM sends a message to inform the guest on their phone
            SendMessage("Housekeeping is cleaning the room");
            delay(2000);
            //Grants Access to the "Housekeeeping" tag and presents this on the OLED for 2 seconds
            AccessGranted();
            delay(2000);
            //Unlocks the door for Housekeeping to clean their room
            OpenDoor();
      }else if (UID =="93 59 66 12") {
            //Presents "Please wait..." on the OLED for 2 seconds to send a message to the guest's phone
            PleaseWait();
            //SIM900 GSM sends a message to inform the guest on their phone
            SendMessage("Room Service delievered food to the room!");
            delay(2000);
            //Waits 2 seconds before Granting Access to the "Room Service" tag
            AccessGranted();
            delay(2000);
            //Unlocks the door for Room Service to deliver the food to their room
            OpenDoor();
      }
    } 
  } 
  //Each scanned ID tags eventually reverts back to the original "SCAN CARD" display on the OLED
  Default();
}

//Receieves the Hexadecimal from the selected block 
  void dump_byte_array(byte *buffer, byte bufferSize) 
  {
    for (byte i = 0; i < bufferSize; i++) {
        String stringOne = String(buffer[i],HEX);     
    }
  }

  //Converts the Hexadecimal from the selected block to ASCII value 
  String dump_byte_array_string(byte *buffer, byte bufferSize) {
    String stringOne;
    for (byte i = 0; i < bufferSize; i++) {
         stringOne.concat((char)buffer[i]); 
    }
    return stringOne;
  }

//Reads the written data blocks within sector 12 from a Mifare Classic 1K Card 
  void ReadData() {
    //Receives the UID in block 0 to be presented on the OLED
    printUID();
    //Receives the Hex to ASCII databytes from block 48 (Name) to be presented on the OLED
    getName();
    //Receives the Hex to ASCII databyes from block 49 (Role) to be presented on OLED
    getRole();
  }

  void getName()
  {
    //This reads the bytes within the name block (sector 48)  
    mfrc522.MIFARE_Read(nameBlock, buffer, &size);
    //Stores the ASCII value into "Name" as a string variable from the function 
    Name = dump_byte_array_string(buffer, 16);
    //Calls the function that presents the name on the OLED
    printName();
  }

  void getRole()
  {
    //This reads the bytes within the role block (sector 49)  
    mfrc522.MIFARE_Read(roleBlock, buffer, &size);
    //Stores the ASCII value into "Role" as a string variable from the function 
    Role = dump_byte_array_string(buffer, 16);
     //Calls the function that presents the "Role" on the OLED
    printRole();
  }

  
  void OpenDoor() 
  {
    //Servo Motor opens (unlocked)
    servoMotor.write(90);
    //Reads the written data blocks within sector 12 from the "Guest", "Housekeeping" or "Room Service" ID tag
    ReadData();
    //Character has a five seconds window to access the room 
    delay(5000);
    //Servo Motor closes (locked)
    servoMotor.write(0);
    //Red LED on and Green LED off
    digitalWrite(redLED, HIGH);
    digitalWrite(greenLED, LOW);
  }

  //Presents "Access Granted" on the OLED
  void AccessGranted() 
  {
     display.clearDisplay();
    display.display();
    display.setTextColor(WHITE);
    display.setTextSize(2);
    display.setCursor(30,0); 
    display.print("ACCESS");
    display.setTextSize(2); 
    display.setCursor(24,15); 
    display.print("GRANTED");
    display.display();
    //This follows the green LED  turning on
    digitalWrite(redLED, LOW);
    digitalWrite(greenLED, HIGH);
  }

  //Presents "Access Denied" on the OLED
  void AccessDenied() 
  {
    display.clearDisplay();
    display.display();
    display.setTextColor(WHITE);
    display.setTextSize(2);
    display.setCursor(0,0); 
    display.print("ACCESS");
    display.setCursor(0,15); 
    display.print("DENIED");
    display.display();
    //Follows the negative buzzer sound for "Access Denied"
    tone(buzzer, 300);
    delay(100);
    noTone(buzzer);
  }

  //Presents "Please wait..." on the OLED
  void PleaseWait()
  {
    display.clearDisplay();
    display.display();
    display.setTextColor(WHITE);
    display.setTextSize(2);
    display.setCursor(0,0); 
    display.print("Please"); 
    display.setCursor(0,15); 
    display.print("wait...");
    display.display();
  }

  //Function sends any message within the string
  void SendMessage(String message)
  {
    //Sends SMS messages to this number 
    SIM900.println("AT+CMGS=\"+447521654658\"\r"); 
    delay(1000);
    //This SMS text you want to send
    SIM900.println(message);// The SMS text you want to send
    delay(100);
    // ASCII code of CTRL+Z
    SIM900.println((char)26);
    delay(1000);
  }

  //Function calls the Guest
  void CallGuest()
   {
    //SIM900 calls this number
    SIM900.println("ATD+447521654658;");
    delay(100);
   }

  //Function presents "Identity Theft" on the OLEDS
  void IdentityTheft() 
  {
    display.clearDisplay();
    display.display();
    display.setTextColor(WHITE);
    display.setTextSize(2);
    display.setCursor(15,0); 
    display.print("IDENTITY"); 
    display.setCursor(30,15); 
    display.print("THEFT");
    display.display();
  }

  void ValidUID(){
    //Function presents "ValidUID" on the OLED 
    display.clearDisplay();
    display.display();
    display.setTextColor(WHITE);
    display.setTextSize(2);
    display.setCursor(0,10); 
    display.print("ValidUID");
    display.display();
    //Follows the "Valid" buzzer sound for a valid tag
    tone(buzzer, 500);
    delay(100);
    noTone(buzzer);
  }  
  
  void Invalid(){   
    //Prints the UID on the OLED
    printUID();
    //Displays the text "Invalid" on the OLED
    display.clearDisplay();
    display.display();
    display.setTextColor(WHITE);
    display.setTextSize(2);
    display.setCursor(0,10); 
    display.print("Invalid");
    display.display();
    //Follows the "invalid" buzzer sound for an Invalid tag
    tone(buzzer, 300);
    delay(1000);
    noTone(buzzer);
  }

    //Displays Original Text "SCAN CARD" 
  void Default() 
  {
    display.clearDisplay();
    display.display();
    display.setTextColor(WHITE);
    display.setTextSize(2);
    display.setCursor(10,0); 
    display.print("SCAN CARD");
    display.display();
  }

  //Function prints the block 0 (UID) on the OLED
  void printUID()
  {
    //Displays the text "UID: "
    display.setTextColor(WHITE); 
    display.setTextSize(1);
    display.setCursor(0,0); 
    display.print("UID: ");
    //Presents the recieved "UID" as a string
    display.setCursor(30,0); 
    display.print(UID);
    display.display();
  }

  //Function prints the Name string
  void printName()
  {
    //Displays the text "Name: "
    display.setTextColor(WHITE); 
    display.setTextSize(1);
    display.setCursor(0,10); 
    display.print("Name: ");
    //Presents the recieved "name" as a string
    display.setCursor(30,10); 
    display.print(Name);
    display.display();
  }

  //Function prints the Hex to ASCII databyes from block 49 (Role) the on OLED
   void printRole()
  {
   //Displays the text "Role: "
    display.setTextColor(WHITE);
    display.setTextSize(1);
    display.setCursor(0,20); 
    display.print("Role: ");
    //Presents the recieved "role" as a string
    display.setCursor(30,20); 
    display.print(Role);
    display.display();
  }

  //Function erases all the contents in the OLED
  void erase()
  {
    display.clearDisplay();
    display.setTextColor(BLACK); 
    display.setTextSize(1);
    display.setCursor(30,20); 
    //Erases the UID string
    display.print(UID);
    //Erases the "Name" string
    display.print(Name);
    //Erases the "Role" string
    display.print(Role);
    display.display();
  }

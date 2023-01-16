#include <Adafruit_GFX.h>                 //Library for OLED to display on the hardware
#include <Adafruit_SSD1306.h>             //Library for the 4pin OLED for monochrome 128x32 pixels
#include <MFRC522.h>                      //Library for MFRC522 sensing module 
#include <SPI.h>                          //Library for the Serial Peripheral Interface (SPI) bus

//Display Settings
#define OLED_RESET 4                      
Adafruit_SSD1306 display(OLED_RESET);     //Intailises OLED reset signal to Pin A4

#define buzzer          5                 //Intailises buzzer signal to Pin 5
#define greenLED        6                 //Intailises Green LED signal to Pin 6
#define redLED          7                 //Intailises Red LED signal to Pin 7
#define RST_PIN         8                 //Intailises MFRC522 Reset signal to Pin 8
#define SS_PIN          10                //Intailises MFRC522 Serial data signal to Pin 10

MFRC522 mfrc522(SS_PIN, RST_PIN);         //Creates instance for the MFRC522
MFRC522::MIFARE_Key key;                  //Creates instance Key for card authentication

String UID;                               //String that stores the RFID Tag's UID
String Name;                              //String that stores the RFID Tag's Name
String Role;                              //String that stores the RFID Tag's Role

byte sector         = 12;                 //Sector 12 holds the desired data
byte nameBlock      = 48;                 //Block 48 stores the name as Hexadecimal bytes  
byte roleBlock      = 49;                 //Block 49 stores the role as Hexadecimal bytes
byte trailerBlock   = 51;                 //Trailer block 51 needed for authenitication 

byte buffer[15];                          //Defined the buffer variable
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
 
  //Initialises the OLED 
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.display();
  //Begins with "SCAN CARD" displayed on screen
  Default();
  
  //Key A defined for card authentication
  //Password: FF FF FF FF AA 13
  key.keyByte[0] = 0xFF ; 
  key.keyByte[1] = 0xFF ; 
  key.keyByte[2] = 0xFF ; 
  key.keyByte[3] = 0xFF ; 
  key.keyByte[4] = 0xAA ;
  key.keyByte[5] = 0x13 ;
    
}

void loop() {
	//This resets the loop if no new cards were present on the MFRC522 reader (saves the entire process when idle).
	if ( ! mfrc522.PICC_IsNewCardPresent()) {
		return;
	}
 
	// Select one of the tags
	if ( ! mfrc522.PICC_ReadCardSerial()) { 
		return;
	}

//Converting UID to Hex
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
     //Authenticates the PICC data from the scanned card by the comparing the sector trailor using key A
     status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
     //If the data presented an invalid status then the tag is presetended as invalid 
    if (status != MFRC522::STATUS_OK) {
      //OLED presents the tag's UID and the text INVALID 
      Invalid();
    }else{
      //If the data presented an valid status then the OLED presents Valid  
      ValidUID();
      delay(2000);
      //Distinguishes whehter the tag is the Guest, Stranger, Housekeeping and Room Service based on the UID 
      if(UID == "D4 7C 93 29" || UID =="F3 08 E9 94"||UID =="C3 60 B1 12"||UID =="93 59 66 12") {
        //Reads the written data blocks within sector 12 for each valid tag
        ReadData();
      }
    }
  } 
  //Each scanned ID tags eventually reverts back to the original "SCAN CARD" display on the OLED
  Default();
}

//Reads the data from the Mifare Classic 1K Card
  void ReadData() {
    //Recieves the UID in block 0 to be presented on the OLED
    printUID();
    //Receives the Hex to ASCII databytes from block 48 (Name) to be presented on the OLED
    getName();
    //Recieves the Hex to ASCII databyes from block 49 (Role) to be presented on OLED
    getRole();
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

 void ValidUID(){
    //Signals the tag is Valid with a green LED (turns off red LED)
    digitalWrite(redLED, LOW);
    digitalWrite(greenLED, HIGH);
    //Function presents "ValidUID" on the OLED  
    display.clearDisplay();
    display.display();
    display.setTextColor(WHITE);
    display.setTextSize(2);
    display.setCursor(0,10); 
    display.print("ValidUID");
    display.display();
    //Follows the "ValidUID" buzzer sound for a valid tag
    tone(buzzer, 500);
    delay(100);
    noTone(buzzer);
  }  
  
  void Invalid(){
    //Signals the tag is invalid with the red LED (turns off green LED)
    digitalWrite(redLED, HIGH);
    digitalWrite(greenLED, LOW);
    //Prints the UID on the OLED
    printUID();
    //Displays the text "InValidUID" on the OLED
    display.clearDisplay();
    display.display();
    display.setTextColor(WHITE);
    display.setTextSize(2);
    display.setCursor(0,10); 
    display.print("InvalidUID");
    display.display();
    //Follows the "invalid" buzzer sound for an Invalid tag
    tone(buzzer, 300);
    delay(1000);
    noTone(buzzer);
  }

  //Displays the original Text "SCAN CARD" 
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

  //Function prints the Hex to ASCII databyes from block 48 (Name) on the OLED
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

  

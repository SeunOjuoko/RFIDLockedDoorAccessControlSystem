#include <Adafruit_GFX.h>                 //Library for OLED to display on the harware
#include <Adafruit_SSD1306.h>             //Library for the 4pin OLED for monochrome 128x32 pixels
#include <MFRC522.h>                      //Library for MFRC522 sensing module 
#include <SPI.h>                          //Library for the Serial Perhiperal Interface (SPI) bus
    
//Display Settings
#define OLED_RESET 4                      
Adafruit_SSD1306 display(OLED_RESET);     //Intailises OLED reset signal to Pin A4

#define RST_PIN         8                 //Intailises MFRC522 Reset signal to Pin 8
#define SS_PIN          10                //Intailises MFRC522 Serial data signal to Pin 10

MFRC522 mfrc522(SS_PIN, RST_PIN);         //Creates instance for the MFRC522

String UID;                               //String that stores the RFID Tag's UID
String Name;                              //String that stores the RFID Tag's Name
String Role;                              //String that stores the RFID Tag's Role

byte sector         = 12;                 //Sector 12 holds the desired data
byte nameBlock      = 48;                 //Block 48 stores the name as Hexadecimal bytes  
byte roleBlock      = 49;                 //Block 49 stores the role as Hexadecimal bytes
byte trailerBlock   = 51;                 //Sector trailer block 51

byte blocks[15];                          //Defined the blocks variable
byte size = sizeof(blocks);               //Defines the size of the blocks
  
void setup() {
  //Initialises serial communications with the PC
	Serial.begin(9600);		
	// Does nothing if no serial port is opened
	while (!Serial);		
	//Initialises the Serial Peripheral Interface (SPI) bus
	SPI.begin();			
	//Initialises the MFRC522
	mfrc522.PCD_Init();		
 
  //Initialises the OLED 
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.display();
  //Begins with "SCAN CARD" displayed on screen
  display.setTextColor(WHITE);
  display.setTextSize(2);
  display.setCursor(10,0); 
  display.print("SCAN CARD");
  display.display();
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

  //Retrieves the UID to be stored as a string
  String content= "";
  //Store each hexadecimal data byte as a byte 
  byte letter;
  //Iterates through the block 0 of the memory layout
  for (byte i = 0; i < mfrc522.uid.size; i++) 
  {
     //Hexadecimal (Base 16)  
     content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
     content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }

  //Displays the PICC type data of the card
  dump_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size);
    
  //Displaying UID to LCD
  content.toUpperCase();
   //If the MFRC522 recieves a UID from an RFID tag this conditional statement commences... 
  if(content.substring(1)!=UID){
    //Erases the content in the OLED
     erase();
     //Stores the UID from the tag to the string
     UID=content.substring(1); 
     ////Distinguishes whehter the tag is the Guest, Stranger, Housekeeping and Room Service based on the UID 
     if(UID == "D4 7C 93 29" || UID =="F3 08 E9 94"||UID =="C3 60 B1 12"||UID =="93 59 66 12") {
      //Reads the written data blocks within sector 12 for each valid tag
      ReadData();
       }else{
      //Function for the UNKNOWN tag with a different UID to be displayed the OLED
      unknownUID(); 
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
  void dump_byte_array(byte *blocks, byte blocksSize) 
  {
    for (byte i = 0; i < blocksSize; i++) {
        String stringOne = String(blocks[i],HEX);     
    }
  }

  //Converts the Hexadecimal from the selected block to ASCII value 
  String dump_byte_array_string(byte *blocks, byte blocksSize) {
    String stringOne;
    for (byte i = 0; i < blocksSize; i++) {
         stringOne.concat((char)blocks[i]); 
    }
    return stringOne;
  }

  void getName()
  {
    //This reads the bytes within the name block (sector 48)  
    mfrc522.MIFARE_Read(nameBlock, blocks, &size);
    //Stores the ASCII value into "Name" as a string variable from the function 
    Name = dump_byte_array_string(blocks, 16);
    //Calls the function that presents the name on the OLED
    printName();
  }

  void getRole()
  {
    //This reads the bytes within the role block (sector 49)  
    mfrc522.MIFARE_Read(roleBlock, blocks, &size);
    //Stores the ASCII value into "Role" as a string variable from the function 
    Role = dump_byte_array_string(blocks, 16);
     //Calls the function that presents the "Role" on the OLED
    printRole();
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

  //Function displays the unknown UID tag on the OLED
   void unknownUID(){
    //Prints the UID on the OLED
    printUID();
    //Displays the text "UNKNOWN" on the OLED
    display.clearDisplay();
    display.display();
    display.setTextColor(WHITE);
    display.setTextSize(2);
    display.setCursor(0,10); 
    display.print("UNKNOWN");
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


  

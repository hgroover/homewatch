// Basic house monitor
// Hardware: SeeedStudio GPRS shield v2.0
// Get a USMobile account and a plan with voice and text
// Currently only text supported. Cost is about $5/month for
// minimum voice and 250 text messages.
// SIM directory entry 1 is used for the state of unarmed /
// perimeter watch / armed. This prevents loss of set state
// on power cycle
// SIM directory entry 2 is used for the disarm PIN.
// Creates a 
// serial link between the computer and the GPRS Shield
// at 19200 bps 8-N-1
// Computer is connected to Hardware UART
// GPRS Shield is connected to the Software 
// Using digital 6, 5, 4, 3, 2, 10, 11, 12
// Using i2c expander at 0x38 (default address)
// d6: front door
// d5: back door
// d4: motion detector
// d3: loft door
// d2: garage door
// d12: line power
// i2c-0:
// i2c-1:
// i2c-2: Fam 1
// i2c-3: Fam 2,3
// i2c-4: Study/dining
// i2c-5: BR4
// i2c-6: Ofc
// i2c-7: Breakfast

// USMobile data connection parameters:
// APN: pwg
// MCC: 310
// MNC: 260

// Use bogus AT+CLPB=<int> to reflect integer
// Unsolicited +CMTI: "SM",2
// may apppear when we have a text.
// Use AT+CMGL to list, or AT+CMGR=<n> to retrieve:
//+CMGR: "REC READ","+17605551212","","16/09/10,06:49:53-20"
//Disregard
// AT+CMGD=<n> to delete. This particular device seems to
// be unable to list after an earlier message has been deleted
// Result from init:
// +CPBR: 1,"*72",177,"ST"

#include <SoftwareSerial.h>
#include <Wire.h>

// Set jumpers on I2c expander boards for slave address.
// Default is all to - side for 0x20, move A0 to + side for 0x21
#define I2CEXP_IN  0x20
#define I2CEXP2_IN  0x21

SoftwareSerial GPRS(7, 8);
unsigned char buffer[65]; // buffer array for data recieve over serial port
int count=0;     // counter for buffer array
byte val=0;
byte caret=0;
String notify = "+17605551212";
byte powerState = 1; // Assume on
//byte expData = 0;
//byte lastExp = 0;
// Sensor note: total 14 inputs
// Mounting note: 3.25" spacing
// Pin identifiers for digital pins monitored for faults
// These will map to the low-order 8 bits of fault
int digPins[8] = { 2, 3, 4, 5, 6, 10, 11, 12 };
//                 bit0 ....             msb
// List other pins here
//int GPRS_data1 = 7;
//int GPRS_data2 = 8;
int GPRS_powerPin = 9;
int ledPin = 13;
unsigned long fault = 0;
// xor'ed with fault - 0 for test mode, mask of all
// connected inputs for live mode
unsigned long connectedMask = 0x3f1f;
unsigned long lastFault = 0;
unsigned long lastFaultTime = 0;
unsigned long curTime;
unsigned long elapsedSinceLastFault;
int initState = 0;
int bytesReceived = 0;
String modemInit[5] = {"AT\r","ATE0Q0\r", "AT+CMGF=1\r", "AT+CPBS=\"SM\"\r", "AT+CPBR=1\r"};
int modemInitLen = 5;
int armedState = 0; // 0 = unarmed; 1 = perimeter watch; 2 = armed (away); 3 = alarm
int saveArmedState = 0; // Save to SMS
unsigned long waitTarget = 0;
int modemBytesReceived = 0;
int gotLastArmedState = 0;
int solicitArmedStateAttemptCount = 0;
// bit 0 of blinkAction means on
// short short long - initializing
// short short short - getting armed state
// long - disarmed
// long long - perimeter
// long long long - armed
// short long - alarm
int blinkAction[10] = {251,750,251,750,1501,500,0,0,0,0};
int blinkActionLen = 6;
int blinkIndex = 0;
unsigned long blinkIndexStart;

void setup()
{
  Wire.begin();
  int n;
  for (n = 0; n < 8; n++)
    if (digPins[n]) pinMode(digPins[n], INPUT_PULLUP);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, 0);
  GPRS.begin(19200);               // the GPRS baud rate   
  Serial.begin(19200);             // the Serial port of Arduino baud rate.
  //powerUp();
  waitTarget = millis() + 1000;
}

void loop()
{
  handleSensors();
  curTime = millis();
  handleBlink();
  handleFault();
  
  if (!handleModemResponse()) return;

  if (!handleModemInit()) return;  

  if (!handleSerialInput()) return;
  
} // loop()

void clearBufferArray()              // function to clear buffer array
{
  for (int i=0; i<count;i++)
    { buffer[i]=NULL;}                  // clear all index of array with command NULL
}

void powerUp()
{
  pinMode(GPRS_powerPin, OUTPUT);
  digitalWrite(GPRS_powerPin,HIGH);
  //delay(1000);
  //digitalWrite(9,HIGH);
  //delay(2000);
  //digitalWrite(9,LOW);
  //delay(3000);
}

void powerDown()
{
  pinMode(GPRS_powerPin, OUTPUT);
  digitalWrite(GPRS_powerPin, LOW);
}

// Process sensor inputs, debounce and take action on detected faults
void handleFault()
{
  elapsedSinceLastFault = curTime - lastFaultTime;
  // Debounce by requiring minimum 100ms
  if (fault != lastFault && elapsedSinceLastFault >= 100)
  {
    //if (fault) digitalWrite(ledPin, 1);
    //else digitalWrite(ledPin, 0);
    lastFault = fault;
    lastFaultTime = curTime;

    Serial.print("F ");
    Serial.print(fault);
    Serial.print(" t ");
    Serial.print(elapsedSinceLastFault);
    Serial.print("\r\n");
    switch (armedState)
    {
      case 0:
        break;
      case 1:
        Serial.print("PERIM\r\n");
        break;
      case 2:
        Serial.print("ALARM\r\n");
        GPRS.print("AT + CMGS = \"" + notify + "\"\r" );
        delay(500);
        GPRS.print("ALARM ");
        GPRS.print(fault);
        GPRS.write((char)26);
        break;
    }
  }
}

// Read sensors and build current fault bitmap
void handleSensors()
{
  Wire.requestFrom(I2CEXP_IN, 1);
  fault = 0;
  byte expData = 0;
  byte expData2 = 0;
  if (Wire.available())
  {
    expData = Wire.read();
    // P0 = 254 (0xfe)
    // P1 = 253 (0xfd)
    fault |= (((expData ^ 0xff) & 0xff) << 8);
    // Bit 15 = P0, bit 14 = P1, bit 13 = P2, bit 12 = P3, etc.
  }
  /***
  Wire.requestFrom(I2CEXP2_IN, 1);
  if (Wire.available())
  {
    expData2 = Wire.read();
    // P0 = 254 (0xfe)
    // P1 = 253 (0xfd)
    fault |= (((expData2 ^ 0xff) & 0xff) << 16);
    // Bit 23 = P0, bit 22 = P1, bit 21 = P2, bit 20 = P3, etc.
  }
  ***/
  int n;
  byte setBit = 0x01;
  for (n = 0; n < 8; n++, setBit <<= 1)
  {
    if (digitalRead(digPins[n]) == 0) fault |= setBit;
  }
  // If test mode, closures are pseudo-triggers,
  // otherwise open is a fault
  fault = fault ^ connectedMask;
}

// curTime is set
void handleBlink()
{
  if (curTime < blinkIndexStart || curTime - blinkIndexStart > blinkAction[blinkIndex])
  {
    blinkIndex = (blinkIndex + 1) % blinkActionLen;
    blinkIndexStart = curTime;
    if (blinkAction[blinkIndex] & 0x01)
    {
      // LED on
      digitalWrite(ledPin, 1);
    }
    else
    {
      digitalWrite(ledPin, 0);
    }
  }
}

// Set blink pattern according to armed state
void setBlink()
{
// long - disarmed
// long long - perimeter
// long long long - armed
// short long - alarm
  switch (armedState)
  {
    case 0:
      blinkActionLen = 2;
      blinkAction[0] = 1501;
      blinkAction[1] = 500;
      blinkIndex = blinkActionLen - 1;
      break;
    case 1:
      blinkActionLen = 4;
      blinkAction[0] = 1501;
      blinkAction[1] = 500;
      blinkAction[2] = 1501;
      blinkAction[3] = 2500;
      blinkIndex = blinkActionLen - 1;
      break;
    case 2:
      if (fault)
      {
        blinkActionLen = 4;
        blinkAction[0] = 501;
        blinkAction[1] = 500;
        blinkAction[2] = 1501;
        blinkAction[3] = 1500;
        blinkIndex = blinkActionLen - 1;
      }
      else
      {
        blinkActionLen = 4;
        blinkAction[0] = 1501;
        blinkAction[1] = 500;
        blinkAction[2] = 1501;
        blinkAction[3] = 500;
        blinkAction[4] = 1501;
        blinkAction[5] = 2500;
        blinkIndex = blinkActionLen - 1;
      }
      break;
  }
}

boolean handleModemResponse()
{
  bytesReceived = 0;
  if (GPRS.available())              // if date is comming from softwareserial port ==> data is comming from gprs shield
  {
    while(GPRS.available())          // reading data into char array
    {
      buffer[count++]=GPRS.read();     // writing data into array
      if(count == 64)break;
    }
    Serial.write(buffer,count);            // if no data transmission ends, write buffer to hardware serial port
    bytesReceived = count;
    modemBytesReceived += count;
    buffer[count] = 0; // null-terminate
    String s = String((char *)buffer);
    int i = s.indexOf("+CPBR: 1,\"*7");
    if (i >= 0)
    {
      gotLastArmedState = 1;
      armedState = s[i+12] - '0';
      Serial.print("Found entry 1 state ");
      Serial.print(armedState); 
      setBlink();
    }
    clearBufferArray();              // call clearBufferArray function to clear the storaged data from the array
    count = 0;                       // set counter of while loop to zero


  }
  
  return true;
  
} // handleModemResponse()

// Process init sequence. Returns true to continue, false to exit loop()
boolean handleModemInit()
{
  // Send modem init string if quiet
  if (initState >= 0 && bytesReceived == 0 && (waitTarget == 0 || curTime > waitTarget))
  {
    // If we've sent initial part, wait for response
    if (initState == 1 && modemBytesReceived == 0)
    {
      Serial.print("Attempting powerup\r\n");
      powerUp();
      waitTarget = curTime + 4000;
      return false;
    }
    if (initState > 1 && modemBytesReceived == 0)
    {
      waitTarget = curTime + 500;
      return false;
    }
    Serial.print("Init: ");
    Serial.print(initState);
    Serial.print("\r\n");
    GPRS.print(modemInit[initState]);
    initState++;
    if (initState >= modemInitLen) 
    {
      initState = -1;
      Serial.print("Init complete\r\n");
      blinkActionLen = 6;
      blinkAction[0] = 251;
      blinkAction[1] = 500;
      blinkAction[2] = 251;
      blinkAction[3] = 500;
      blinkAction[4] = 251;
      blinkAction[5] = 1500;
      waitTarget = curTime + 2000;
    }
    else
    {
      waitTarget = curTime + 500;
      modemBytesReceived = 0;
    }
    // Final init string should give us results of AT+CPBR=1
    // something like
    // +CPBR: 1,"*70",177,"ST"
    // We'll update using
    // AT+CPBW=1,"*71",177,"ST"
    // where *7 is ignored
  }

  if (gotLastArmedState == 0 && bytesReceived == 0 && (waitTarget == 0 || curTime > waitTarget))
  {
    // limit attempts
    if (solicitArmedStateAttemptCount < 5)
    {
      GPRS.print("AT+CPBR=1\r");
      solicitArmedStateAttemptCount++;
      waitTarget = curTime + 1000;
    }
  }

  return true;
}

// Process serial input. Returns true to continue, false to exit loop()
boolean handleSerialInput()
{
  if (Serial.available())            // if data is available on hardwareserial port ==> data is comming from PC or notebook
  {
    // RPi will send commands to set armed state
    val = Serial.read();
    if (caret)
    {
      if (val >= 0x40)
      {
        val -= 0x40;
      }
      else 
      {
        saveArmedState = 0;
        switch (val)
        {
        case '0': // disarm
          if (armedState == 0)
            Serial.print("Already disarmed\r\n");
          else
          {
            Serial.print("Disarmed from fault ");
            Serial.print(lastFault);
            Serial.print("\r\n");
            saveArmedState = 1;
          }
          armedState = 0;
          break;
        case '1': // perimeter watch
          if (armedState == 1)
            Serial.print("Already perimeter watch\r\n");
          else
          {
            Serial.print("Perimeter watch set, fault ");
            Serial.print(lastFault);
            Serial.print("\r\n");
            saveArmedState = 1;
            armedState = 1;
            lastFault = 0;
          }
          break;
        case '2': // arm
          if (armedState == 2)
            Serial.print("Already armed\r\n");
          else if (lastFault != 0)
          {
            Serial.print("ERROR: cannot arm, fault ");
            Serial.print(lastFault);
            Serial.print("\r\n");
          }
          else
          {
            Serial.print("Arming...\r\n");
            armedState = 2;
            saveArmedState = 1;
          }
          break;
        default:
          break;
        }
        if (saveArmedState)
        {
          GPRS.print("AT+CPBW=1,\"*7");
          GPRS.print(armedState);
          GPRS.print("\",177,\"ST\"\r");
          setBlink();
        }
      }
      caret = 0;
      return false;
    }
    else if (val == '^')
    {
      caret = 1;
    }
    if (caret == 0)
    {
      GPRS.write(val);       // write it to the GPRS shield
    }
  }
  return true;
}


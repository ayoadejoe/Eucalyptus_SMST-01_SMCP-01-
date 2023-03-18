
// Copyright (C)2016 Ayoade Adetunji. All right reserved

// web: http://www.litconcept.net
//]75]100]
#include <DS1302.h>
#include <SoftwareSerial.h> 
#include <avr/sleep.h>  
#include <avr/power.h>
#include <EEPROM.h>

SoftwareSerial mySerial(12, 5);   //RX/TX

#define power100 8
#define power75 4

#define solenoid100 A2
#define solenoid75 A1
#define pump A3

#define bigGauge A4
#define smallGauge A5

#define grayEcho 2
#define grayTrig 3

#define blueEcho 6
#define blueTrig 7

#define wifiReset A0

// Init the DS1302                    
DS1302 rtc(9,10, 11);                                    // DS1302:  CE pin    -> Arduino Digital 2          - Reset
                                                        //          I/O pin   -> Arduino Digital 3          - Data Line
                                                        //          SCLK pin  -> Arduino Digital 4          - Sync Clock

boolean bigStatus = false;
boolean smallStatus = false;

boolean fillBig = false;
boolean fillSmall = false;

boolean state100=false;
boolean state75=false;

boolean hook100 = false;
boolean hook75 = false;

long timer100 = 0, oldtime100 = 0, t100;
long timer75 = 0, oldtime75 = 0, t75;

String finalOut="Empty";
String words= "";
String status75 = " ";
String status100 = " ";
String fill75Time=" ";         
String fill100Time=" "; 

int bigFull = 320;
int smallFull = 290;

int bigMinimum = 100;
int smallMinimum = 125;

int count=0;

unsigned long previousMillis = 0;        // will store last time LED was updated

// constants won't change :
const long interval = 5000;           // interval at which to blink (milliseconds)
void setup()
{
  Serial.begin(9600);
  mySerial.begin(115200);
  EEPROM.begin();
  
  pinMode(wifiReset, OUTPUT);
  digitalWrite(wifiReset, LOW);
 
  Serial.println("Starting");
  pinMode(bigGauge, INPUT);
  pinMode(smallGauge, INPUT);

  pinMode(power100, INPUT);
  pinMode(power75, INPUT);

  pinMode(solenoid100, OUTPUT);
  pinMode(solenoid75, OUTPUT);
  pinMode(pump, OUTPUT);

  pinMode(blueTrig, OUTPUT);
  pinMode(blueEcho, INPUT);
  
  pinMode(grayTrig, OUTPUT);
  pinMode(grayEcho, INPUT);

  // Set the clock to run-mode, and disable the write protection
  rtc.halt(false);
  rtc.writeProtect(false);
   // The following lines can be commented out to use the values already stored in the DS1302
  //rtc.setDOW(FRIDAY);        // Set Day-of-Week to FRIDAY
 // rtc.setTime(4, 13, 0);     // Set the time to 12:00:00 (24hr format)
  //rtc.setDate(16, 9, 2016);   // Set the date to August 6th, 2010

   /* if(EEPROM.read(10)!=255 && EEPROM.read(10)!=0)bigFull = EEPROM.read(10);
    if(EEPROM.read(11)!=255 && EEPROM.read(11)!=0)smallFull = EEPROM.read(11);

    if(EEPROM.read(12)!=255 && EEPROM.read(12)!=0)bigMinimum = EEPROM.read(12);
    if(EEPROM.read(13)!=255 && EEPROM.read(13)!=0)smallMinimum = EEPROM.read(13);
    */

    timer75 = getOldtime(0);
    timer100 = getOldtime(4);

    Serial.println("Max Full for 100:");
    Serial.println(bigFull, DEC);
    Serial.println("Minimum for 100:");
    Serial.println(bigMinimum, DEC);
    
    Serial.println("Max Full for 75:");
    Serial.println(smallFull, DEC);
    Serial.println("Minimum for 75:");
    Serial.println(smallMinimum, DEC);
}

String ssid, password, ip;
void loop(){
if(digitalRead(wifiReset)==HIGH){
  Serial.println(Serial.readString());
  Serial.println("some came in");
  delay(1000);
  digitalWrite(wifiReset, LOW);
      count = 0;
}
while(!mySerial.available()){                                 //have to break in event that state of power

   while(Serial.available())  {
    
      Serial.println("Loop activated");
      words = Serial.readString();
      
     while(words.equals("internet") || words.equals("error")){
      Serial.println("Follow the prompts:");
      words = getNewAddress();
      delay(1000);
     }
     Serial.println(words);
     int code = words.substring(0).toInt();
     Serial.println(code);
     processWords(words, code);
  }

  
  unsigned long currentMillis = millis();
  
  if(currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;    

    count++;
    if(count == 900)processWords("Z", 0);                         //software reset esp, by taking A0 low after so long without call from client


  int levelGray = analyseStorage(grayTrig, grayEcho);
  int levelBlue = analyseStorage(blueTrig, blueEcho);
  String l1 = String(levelBlue);
  String l2 = String(levelGray);
  
  int gen100 = 1024 - analogResistance100();
  int gen75  = 1024 - analogResistance75();
  
  String g1 = String(gen100);
  String g2 = String(gen75);

  boolean d = digitalRead(power100);
  if(!d){
    status100 = String("100OFF:");
  }else{
    status100 = String("100ON:");
  }

  /*
  if(d!=bigStatus){
      if(d) status100 = String("100ON:");  //this stores the last time, accessing loop only if there is change, that was when i wanted time but now, it is affecting memory - Strings issue
      if(!d) status100 = String("100OFF:");
      //Serial.print("Big Status Change at: ");
      //Serial.println(status100);
      bigStatus = d;
  }*/

  boolean r = digitalRead(power75);
  if(!r){
    status75 = String("75OFF:");
    }else{status75 = String("75ON:");}
    
  /*if(r!=smallStatus){
      if(r) status75 = String("75ON:");
      if(!r) status75 = String("75OFF:");
      //Serial.print("Small Status Change at: ");
      //Serial.println(status75);
      smallStatus = r;
  }*/

 //range should be minim 60- maxim 300 for the slim gauge on bigGauge for 100kVA*
                                                    //objective is to allow to reach 100 before it refills, anything between 100 - 190 leaves the STATE as it is

/*  int bigFull = 115;
int bigMinimum = 260;
 */

  if(gen100<=bigMinimum ) {
    Serial.println("Diesel Level in 100kVA is low:");
    fill100Time = String("B_LOW");
  }

  if(gen100<=bigMinimum && d==false) {
    state100=true;        //quantity below defined must be refilled - too low
  }
  
  if(gen100>=bigFull){
    state100=false;                    //quantity above 250 must not be refilled or stop refill-full
    fill100Time = String("B_FULL");
  }
  if(d && hook100){                                     //Truncation in case Gen goes on by mistake
        stopFillBig();
        Serial.println ("Refill100 truncated. Gen on in transit ");
        fill100Time = String("B_REF");
        hook100 = false;
  }
  if(state100 != fillBig){                          //if false = false, loop is not accessed at all, true = false, access loop and refills because fillBig becomes true
    Serial.print ("Fuel level alert: ");
    Serial.println(gen100);
    fillBig = state100;                             //true = true, is only possible if fillBig becomes true, loop becomes inaccessible allowing other things to proceed
      if(fillBig){                                  //if false = true, loop is accessed once again but stops fill because fillBig becomes false, so false = false result
        refillBig();
        Serial.println ("Refilling 100");
        hook100 = true;                            //keep breaking the while not available loop, tank is empty
      }else{
        stopFillBig();
        Serial.println ("Refill100 Completed ");
         fill100Time = String("B_FULL");
        hook100 = false;                             //lock the while loop back, tank is full
      }
    }


/*int smallFull = 260;
int smallMinimum = 115;*/

  if(gen75<=smallMinimum ) {
    fill75Time = String("S_LOW");
  }

  if(gen75<=smallMinimum && r==false) {
    state75=true;             //quantity below 100 must be refilled
  }
  if(gen75>=smallFull){
    fill75Time = String("S_FULL");
    state75=false;                        //quantity above 190 must not be refilled or stop refill
  }
   if(r && hook75){                                     //Truncation in case Gen goes on by mistake
        stopFillSmall();
        Serial.println ("Refill 75 truncated. Gen on in transit ");
        fill75Time = String("S_REF");
        hook75 = false;
  }
 // Serial.print ("Here now 75 at ");
  //Serial.println(gen75);
  if(state75 != fillSmall){                           //if false = false, loop is not accessed at all, true = false, access loop and refills because fillBig becomes true
    fillSmall = state75;                              //true = true, is only possible if fillBig becomes true, loop becomes inaccessible allowing other things to proceed
      if(fillSmall){                                  //if false = true, loop is accessed once again but stops fill because fillBig becomes false, so false = false result
        refillSmall();
        Serial.println ("Refilling 75 ");
        hook75 = true;
      }else{
        stopFillSmall();
        Serial.println ("Refill 75 Completed ");
        fill75Time = String("S_FULL");
        hook75 = false;
      }
    }
    digitalWrite(13, HIGH);
    delay(500);
    String finally = String(l1+"]"+l2+"]"+g1+"]"+g2+"]"+status100+"]"+status75+"]"+fill100Time+"]"+fill75Time+"]");

    finalOut = finally;
    Serial.println("Present Values: ");
    Serial.println(finalOut);
    digitalWrite(13, LOW);
    delay(500);
  }
  }

  //Serial.println("Final to send out: ");
  //Serial.println(finalOut);
  while(mySerial.available() > 0){
    int x = mySerial.read();
    
    if(x==46){
      Serial.println(x);
      Serial.println("Breaking... Network issues, please get new SSID & Password");
      break;
    }
    digitalWrite(13, HIGH);
     processWords(mySerial.readString(), 10);
    count = 0;                          //reset counter of duration
  }

}


String getNewAddress(){
  Serial.println("Please Enter SSID>");
      while (Serial.available()<1){
        Serial.println("Please Enter SSID>");
        delay(5000);
      }
      if(Serial.available()>0){
        ssid = Serial.readString();
      }

      if(ssid.equals("X"))return "error";             //error handler

      Serial.println("Please Enter Password>");
      while (Serial.available()<1){
        Serial.println("Please Enter Password>");
        delay(5000);
      }
      if(Serial.available()>0){
        password = Serial.readString();
      }

      if(password.equals("X"))return "error";             //error handler
      
      Serial.println("Please Enter IP>");
      while (Serial.available()<1){
        Serial.println("Please Enter IP>");
        delay(5000);
      }
      if(Serial.available()>0){
        ip = Serial.readString();
      }
      if(ip.equals("X"))return "error";             //error handler
      
      return "2done";
}

void processWords(String wood, int code){
 int len = wood.length();
 boolean internet = false;

     if(code == 2){                      //wifi re-address code
      ssid.concat("#");
      if(ssid.length()<20){
        while(ssid.length()<20){
          ssid+="*";
        }
      }
    Serial.println(ssid);

      password.concat("#");
      if(password.length()<20){
        while(password.length()<20){
          password+="*";
        }
      }
    Serial.println(password);

    ip.concat("#");
      if(ip.length()<20){
        while(ip.length()<20){
          ip+="*";
        }
      }
    Serial.println(ip);

    String pro = "newAd";
    pro.concat(ssid);
    pro.concat(password);
    pro.concat(ip);

   Serial.println(pro);
   mySerial.println(pro);
   delay(5);
     }

    if(code==10){                       //wifi data code
     int xlocator=0;  
     int x=0;
     Serial.print("words in loop:");
     Serial.println(wood);
     Serial.print("lenght:");
     Serial.println(len);
     String xvalue ="";
     
    while(x<51){
        xlocator = wood.indexOf('X', x);
        x=x+7;
        Serial.print("xlocator+2:");
        Serial.println(xlocator+2);
        Serial.print("xlocator+8:");
        Serial.println(xlocator+9);
        xvalue = wood.substring(xlocator+2, xlocator+8);
        Serial.print("xvalue:");
        Serial.println(xvalue);
    
        if(xvalue.equals("STP075")){
          code = 11;
          mySerial.println("STOPPED 75kVA" );
          delay(5);
          break;
        }else if(xvalue.equals("Mquery")){
          mySerial.println(finalOut);
          delay(5);
          break;
        }else if(xvalue.equals("STP100")){
          code = 12;
          mySerial.println("STOPPED 100kVA" );
          delay(5);
          break;
        }else if(xvalue.equals("STT075")){
          code = 13;
          mySerial.println("START 75kVA" );
          delay(5);
          break;
        }else if(xvalue.equals("STT100")){
          code = 14;
          mySerial.println("START 100kVA" );
          delay(5);
          break;
        }else if(xvalue.equals("SHUTDN")){
          code = 15;
          break;
        }else if(xvalue.equals("RESETT")){
          code = 16;
          break;
        }else if(xvalue.equals("SETESP")){
          code =  0;
          break;
        }else if(xvalue.equals("CAL100")){
          code =  17;
          break;
        }else if(xvalue.equals("CAL075")){
          code =  18;
          break;
        }else if(xvalue.equals("RETIME")){
          code =  1;
          internet = true;
          break;
        }
      }
      Serial.print("xvalue:");
      Serial.println(xvalue);
      }
      
      

  switch(code){
    case 0:
      Serial.println("resetting esp");
      digitalWrite(wifiReset, HIGH);
      delay(1000);
      digitalWrite(wifiReset, LOW);
      count = 0;
    break;

    case 1:                                                     //1;04:dd,MM,yyyy:hh,mm,ss:   Date entry syntax
     if(internet){
          String wor = wood.substring(26,(len-1));
           Serial.print("Internet Time:");
           Serial.println(wor);
           resetTime(wor);
        }else{
            resetTime(wood);
        }
    break;

    case 11:
      stopFillSmall();
    break;

    case 12:
      stopFillBig();
    break;

    case 13:
      refillSmall();
    break;

    case 14:
      refillBig();
    break;

    case 15:
    initiateShutdown(true);
    break;

    case 16:
    //resetArduino();
    break;

    default:
    break;
  }

  if(code==17){
          Serial.print("code 17 100 wood:");
          Serial.println(wood);
          Serial.print("MAX:"); 
          Serial.println(wood.substring(33,40)); 
          Serial.print("MIN:"); 
          Serial.println(wood.substring(42,48)); 
          int max100  = wood.substring(33,40).toInt();
          Serial.print("Max100:"); 
          Serial.println(max100); 
          int min100  = wood.substring(42,48).toInt();
          Serial.print("Min100:"); 
          Serial.println(min100); 
          
          calibrateFullGenTank(10, max100);
          calibrateMinimumGenTank(12, min100);
          mySerial.println("100 CALIBRATED");
  }

  if(code==18){
          Serial.print("code 18 75 wood:");
          Serial.println(wood);
          Serial.print("MAX:"); 
          Serial.println(wood.substring(33,40)); 
          Serial.print("MIN:"); 
          Serial.println(wood.substring(42,48)); 
          int max75  = wood.substring(33,40).toInt();
          Serial.print("Max75:"); //next character is comma, so skip it using this
          Serial.println(max75); 
          int min75  = wood.substring(42,48).toInt();
          Serial.print("Min75"); //next character is comma, so skip it using this
          Serial.println(min75); 
          
          calibrateFullGenTank(11, max75);
          calibrateMinimumGenTank(13, min75);
          mySerial.println("75 CALIBRATED");
  }
}

void call100(String wor){
   //CAL100X CAL100X CAL100X CAL100X 000160X 000300X    the lesser, the fuller, the greater - the emptier
   //CAL100X CAL100X CAL100X CAL100X MAXIMUM MINIMUM
         
}

void call75(String wor){
         
}

void calibrateFullGenTank(int genMem, int level){
  if(level>255) level = 250;
  EEPROM.write(genMem, level);
}

void calibrateMinimumGenTank(int genMem, int level){
  if(level>255) level = 250;
  EEPROM.write(genMem, level);
}


void wakeUpNow() {  
      digitalWrite(wifiReset, HIGH);
}  

int ds = 0;
void initiateShutdown(boolean initO){
  while(initO){
  while(Serial.available()){
    Serial.println(Serial.read(), DEC);
  }
  
  Serial.println(ds++);
  delay(1000);
  if(ds>10){
    ds=0;
    arduinoSleep();
    break;
  }
  }
}

void arduinoSleep(){
  set_sleep_mode(SLEEP_MODE_IDLE);   // sleep mode is set here

  sleep_enable();          // enables the sleep bit in the mcucr register
                             // so sleep is possible. just a safety pin 
  
  power_adc_disable();
  power_spi_disable();
  power_timer0_disable();
  power_timer1_disable();
  power_timer2_disable();
  power_twi_disable();
  
  
  sleep_mode();            // here the device is actually put to sleep!!
 
  wakeUpNow();              // THE PROGRAM CONTINUES FROM HERE AFTER WAKING UP
  sleep_disable();         // first thing after waking from sleep:
                            // disable sleep...
  power_all_enable();
}

void resetTime(String wood){
        //04:14,10,2016:16,50,00:
        Serial.print("Time Received:");
        Serial.println(wood);
        int dayOfWeek  = wood.substring(2,4).toInt();
        Serial.read(); //next character is comma, so skip it using this
        Serial.print(dayOfWeek);   Serial.print(":");
        
        int dd = wood.substring(5,7).toInt();
        Serial.read();
        Serial.print(dd); Serial.print(",");
        
        int MM = wood.substring(8,10).toInt();
        Serial.read();
        Serial.print(MM); Serial.print(",");
        
        int yyyy = wood.substring(11,15).toInt();
        Serial.read();
        Serial.print(yyyy); Serial.print(":");
        
        int hh  = wood.substring(16,18).toInt();
        Serial.read();
        Serial.print(hh); Serial.print(",");
        
        int mm  = wood.substring(19,21).toInt();
        Serial.read();
        Serial.print(mm);  Serial.print(",");
        
        int ss  = wood.substring(22,24).toInt();
        Serial.read();
        Serial.print(ss);  Serial.println(":");
        Serial.println(wood.substring(2,4));
         rtc.setDOW(dayOfWeek);
         rtc.setTime(hh, mm, ss);
         rtc.setDate(dd, MM, yyyy);
}

int analogResistance100(){
  int outV = analogRead(bigGauge);          //range should be minim 60- maxim 300 for the slim gauge on bigGauge for 100kVA
  int resistance = (5.0*outV);
  return outV;
}


int analogResistance75(){
  int outV = analogRead(smallGauge);          //range should be minim 0- maxim 250 for the thick gauge on smallGauge for 75kVA
  int resistance = (5.0*outV);
  return outV;
}


String getDateTime(){
      String da8 = String(rtc.getDateStr());
      String tim = String(rtc.getTimeStr());
      String daTim = String(da8+"_"+tim);
      return daTim;
}

boolean refillBig(){
  digitalWrite(solenoid100, HIGH);
  delay(1500);
  digitalWrite(pump, HIGH);
  return true;
}

boolean stopFillBig(){
  digitalWrite(pump, LOW);
  delay(1500);
  digitalWrite(solenoid100, LOW);
  return true;
}

boolean refillSmall(){
  digitalWrite(solenoid75, HIGH);
  delay(1500);
  digitalWrite(pump, HIGH);
  return true;
}

boolean stopFillSmall(){
  digitalWrite(pump, LOW);
  delay(1500);
  digitalWrite(solenoid75, LOW);
  return true;
}

int analyseStorage(int trigPin, int echoPin){
  long duration, distance;  
  int cum = 0; int ave = 0;
  for(int count = 0; count<10; count++){
  digitalWrite(trigPin, LOW);  // Added this line
  delayMicroseconds(2); // Added this line
  digitalWrite(trigPin, HIGH);
  //delayMicroseconds(1000); - Removed this line
  delayMicroseconds(10); // Added this line
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  distance = (duration/2) / 2.91;
  cum+=(distance/10);
  if(count == 2){
    ave = cum/3;
    return ave;
  }
  }
}


long getOldtime(long address)
      {
      //Read the 4 bytes from the eeprom memory.
      long four = EEPROM.read(address);
      long three = EEPROM.read(address + 1);
      long two = EEPROM.read(address + 2);
      long one = EEPROM.read(address + 3);

      //Return the recomposed long by using bitshift.
      return ((four << 0) & 0xFF) + ((three << 8) & 0xFFFF) + ((two << 16) & 0xFFFFFF) + ((one << 24) & 0xFFFFFFFF);
      }

      
//This function will write a 4 byte (32bit) long to the eeprom at
//the specified address to address + 3.
void updateTime(int address, long value)
      {
      //Decomposition from a long to 4 bytes by using bitshift.
      //One = Most significant -> Four = Least significant byte
      byte four = (value & 0xFF);
      byte three = ((value >> 8) & 0xFF);
      byte two = ((value >> 16) & 0xFF);
      byte one = ((value >> 24) & 0xFF);

      //Write the 4 bytes into the eeprom memory.
      EEPROM.update(address, four);
      EEPROM.update(address + 1, three);
      EEPROM.update(address + 2, two);
      EEPROM.update(address + 3, one);
      }


// Board:             DOIT ESP32 (Node ESP?)
// Documentation URL: asdasd

#include "RTClib.h"
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>

// RTC
RTC_DS3231 rtc;

// DISPLAY
LiquidCrystal_I2C lcd(0x27, 16, 2); 
long nextClear = 0;

// OTHERS
int activeShift = 0;
  // 0 - 8am-8pm  break:  10:00 - 10:15
  //                      12:00 - 12:30
  //                      3:00 - 3:15
  //                      6:00 - 6:15
  //
  // 1 - 8pm-8am  break:  10:00 - 10:15
  //                      12:00 - 12:30
  //                      3:00 - 3:15
  //                      6:00 - 6:15
  //
  // 2 - 6am-2pm  break:  8:00 - 8:15
  //                      10:00 - 10:30
  //                      12:00 - 12:15
  //
  // 3 - 2pm-10pm break:  4:00 - 4:15
  //                      6:00 - 6:30
  //                      8:00 - 8:15
  //
  // 4 - 10pm-6am break:  12:00 - 12:15
  //                      2:00 - 2:30
  //                      4:00 - 4:15
  // 8am - 4pm            10 - 10:15
   //                     12 1230
   //                     3  3:15
   
bool isBreak = false;
bool isLate = false;
bool isPass = false;        // Reset Pass
bool isPassSwitch = false;  // Shift Pass
float lateCounter = 0;
String breakMessage = "";

// pass reset
String passkey = "1111";
String passType = "";
float idleCounter = 0;

// pass switch
String passKeySwitch = "2323";
int switchType = 0;         // 1 - up
                            // 2 - down

// towerlight
int ledRed = 33; 
int ledGreen = 32;
int ledBuzzer = 14;

// button
int btnUp = 27;
int btnDown = 13;
int btnOK = 12;
int debounce = 10; // 5 trigger        

// keypad
const byte ROWS = 4;
const byte COLS = 3;
char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};
byte rowPins[ROWS] = {2, 4, 16, 17};  //connect to the row pinouts of the keypad
byte colPins[COLS] = {5, 18, 19};     //connect to the column pinouts of the keypad
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );
//char key = keypad.getKey();// Read the key


// ============================
// Start
// ============================
void setup() {
  // Serial
  Serial.begin(9600);

  // rtc
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1) delay(10);
  }
  
  // init lcd
  lcd.init(); //initialize the lcd
  lcd.backlight(); //open the backlight 
  lcd.clear();

  // towerlight
  pinMode(ledRed, OUTPUT);
  pinMode(ledGreen, OUTPUT);
  pinMode(ledBuzzer, OUTPUT);
  digitalWrite(ledRed, LOW);
  digitalWrite(ledGreen, LOW);
  digitalWrite(ledBuzzer, LOW);

  // button
  pinMode(btnUp, INPUT_PULLUP);
  pinMode(btnDown, INPUT_PULLUP);
  pinMode(btnOK, INPUT_PULLUP);
}


// ============================
// Loop
// ============================
void loop() {
  DateTime now = rtc.now();

  int btnUpVal = digitalRead(btnUp);
  int btnDownVal = digitalRead(btnDown);
  int btnOKVal = digitalRead(btnOK);

  char key = keypad.getKey();
  
  if (key){
    Serial.println(key);
  }
  
  // button
  if (!btnUpVal || key == '#')
  {
    if (isBreak)
    { 
      return;
    }

    if (isPass)
    {
      return;
    }

    if (isPassSwitch)
    {
      return;
    }

    isPassSwitch = true;
    idleCounter = 0;
    switchType = 1;

    /*
    // trigger
    if (debounce > 3)
    {
      debounce = 10;
      if (activeShift >= 5)
      {
        activeShift = 0;
      }
      else
      {
        activeShift++;
      }
    }
    else
    {
      debounce++;
    }
    */
  }

  // button
  if (!btnDownVal || key == '*')
  {
    if (isBreak)
    { 
      return;
    }

    if (isPass)
    {
      return;
    }

    if (isPassSwitch)
    {
      return;
    }

    isPassSwitch = true;
    idleCounter = 0;
    switchType = 2;

    /*
    // trigger
    if (debounce > 3)
    {
      debounce = 10;
      if (activeShift <= 0)
      {
        activeShift = 5;
      }
      else
      {
        activeShift--;
      }
    }
    else
    {
      debounce++;
    }
    */
  }

  // button
  if (!btnOKVal || key == '0')
  {
    if (isPass)
    {
      return;
    }
    
    if (isPassSwitch)
    {
      return;
    }

    if (isBreak)
    {
      isPass = true;
      idleCounter = 0;
    }
  }

  // pass reset?
  if (isPass && (key == '1' || key == '2' || key == '3' || key == '4' || key == '5' || key == '6' || key == '7' || key == '8' || key == '9'))
  {
    idleCounter = 0;

    // typing?
    if (passType.length() < 4)
    {
      passType = passType + key;
    }

    // correct?
    if (passType.length() == 4)
    {
      // correct
      if (passType == passkey)
      {
        isPass = false;
        isBreak = false;
        isLate = false;
        lateCounter = 0;
        passType = "";
      }

      // wrong
      else
      {
        passType = "";
      }
    }
  }

  // pass switch?
  if (isPassSwitch && (key == '1' || key == '2' || key == '3' || key == '4' || key == '5' || key == '6' || key == '7' || key == '8' || key == '9'))
  {
    idleCounter = 0;

    // typing?
    if (passType.length() < 4)
    {
      passType = passType + key;
    }

    // correct?
    if (passType.length() == 4)
    {
      // correct
      if (passType == passKeySwitch)
      {
        // up?
        if (switchType == 1)
        {
          if (activeShift >= 5)
          {
            activeShift = 0;
          }
          else
          {
            activeShift++;
          }
        }

        // down?
        if (switchType == 2)
        {
          if (activeShift <= 0)
          {
            activeShift = 5;
          }
          else
          {
            activeShift--;
          }
        }

        // 
        isPass = false;
        isPassSwitch = false;
        switchType = 0;
        passType = "";
      }

      // wrong
      else
      {
        passType = "";
      }
    }
  }

  // testing?
  if (!isPass && !isPassSwitch && key == '5')
  {
    breakMessage = "Testing...";
    isBreak = true;
    isLate = true;
  }

  // process
  {
    // 1st shift
    if (activeShift == 0)
    {
      // check break
      if (now.hour() == 10 && now.minute() >= 0 && now.hour() == 10 && now.minute() <= 16)
      {
        if (!isBreak)
        {
          breakMessage = "10:00AM-10:15AM";
          isBreak = true;
        }

        // late
        if (now.minute() >= 16)
        {
          isLate = true;
        }
      }

      // check break
      if (now.hour() == 12 && now.minute() >= 0 && now.hour() == 12 && now.minute() <= 31)
      {
        if (!isBreak)
        {
          breakMessage = "12:00PM-12:30PM";
          isBreak = true;
        }

        // late
        if (now.minute() >= 31)
        {
          isLate = true;
        }
      }

      // check break
      if (now.hour() == 15 && now.minute() >= 0 && now.hour() == 15 && now.minute() <= 16)
      {
        if (!isBreak)
        {
          breakMessage = "03:00PM-03:15PM";
          isBreak = true;
        }

        // late
        if (now.minute() >= 16)
        {
          isLate = true;
        }
      }

      // check break
      if (now.hour() == 18 && now.minute() >= 0 && now.hour() == 18 && now.minute() <= 16)
      {
        if (!isBreak)
        {
          breakMessage = "06:00PM-06:15PM";
          isBreak = true;
        }

        // late
        if (now.minute() >= 16)
        {
          isLate = true;
        }
      }
    }

    // 2nd shift
    if (activeShift == 1)
    {
      // check break
      if (now.hour() == 22 && now.minute() >= 0 && now.hour() == 22 && now.minute() <= 16)
      {
        if (!isBreak)
        {
          breakMessage = "10:00PM-10:15PM";
          isBreak = true;
        }

        // late
        if (now.minute() >= 16)
        {
          isLate = true;
        }
      }

      // check break
      if (now.hour() == 0 && now.minute() >= 0 && now.hour() == 0 && now.minute() <= 31)
      {
        if (!isBreak)
        {
          breakMessage = "12:00AM-12:30AM";
          isBreak = true;
        }

        // late
        if (now.minute() >= 31)
        {
          isLate = true;
        }
      }

      // check break
      if (now.hour() == 3 && now.minute() >= 0 && now.hour() == 3 && now.minute() <= 16)
      {
        if (!isBreak)
        {
          breakMessage = "03:00AM-03:15AM";
          isBreak = true;
        }

        // late
        if (now.minute() >= 16)
        {
          isLate = true;
        }
      }

      // check break
      if (now.hour() == 6 && now.minute() >= 0 && now.hour() == 6 && now.minute() <= 16)
      {
        if (!isBreak)
        {
          breakMessage = "06:00AM-06:15AM";
          isBreak = true;
        }

        // late
        if (now.minute() >= 16)
        {
          isLate = true;
        }
      }
    }

    // 3rd shift
    if (activeShift == 2)
    {
      // check break
      if (now.hour() == 8 && now.minute() >= 0 && now.hour() == 8 && now.minute() <= 16)
      {
        if (!isBreak)
        {
          breakMessage = "08:00AM-08:15AM";
          isBreak = true;
        }

        // late
        if (now.minute() >= 16)
        {
          isLate = true;
        }
      }

      // check break
      if (now.hour() == 10 && now.minute() >= 0 && now.hour() == 10 && now.minute() <= 31)
      {
        if (!isBreak)
        {
          breakMessage = "10:00AM-10:30AM";
          isBreak = true;
        }

        // late
        if (now.minute() >= 31)
        {
          isLate = true;
        }
      }

      // check break
      if (now.hour() == 12 && now.minute() >= 0 && now.hour() == 12 && now.minute() <= 16)
      {
        if (!isBreak)
        {
          breakMessage = "12:00PM-12:15PM";
          isBreak = true;
        }

        // late
        if (now.minute() >= 16)
        {
          isLate = true;
        }
      }
    }

    // 4th shift
    if (activeShift == 3)
    {
      // check break
      if (now.hour() == 16 && now.minute() >= 0 && now.hour() == 16 && now.minute() <= 16)
      {
        if (!isBreak)
        {
          breakMessage = "04:00PM-04:15PM";
          isBreak = true;
        }

        // late
        if (now.minute() >= 16)
        {
          isLate = true;
        }
      }

      // check break
      if (now.hour() == 18 && now.minute() >= 0 && now.hour() == 18 && now.minute() <= 31)
      {
        if (!isBreak)
        {
          breakMessage = "06:00PM-06:30PM";
          isBreak = true;
        }

        // late
        if (now.minute() >= 31)
        {
          isLate = true;
        }
      }

      // check break
      if (now.hour() == 20 && now.minute() >= 0 && now.hour() == 20 && now.minute() <= 16)
      {
        if (!isBreak)
        {
          breakMessage = "08:00PM-08:15PM";
          isBreak = true;
        }

        // late
        if (now.minute() >= 16)
        {
          isLate = true;
        }
      }
    }

     // 5th shift
    if (activeShift == 4)
    {
      // check break
      if (now.hour() == 0 && now.minute() >= 0 && now.hour() == 24 && now.minute() <= 16)
      {
        if (!isBreak)
        {
          breakMessage = "12:00AM-12:15AM";
          isBreak = true;
        }

        // late
        if (now.minute() >= 16)
        {
          isLate = true;
        }
      }

      // check break
      if (now.hour() == 2 && now.minute() >= 0 && now.hour() == 2 && now.minute() <= 31)
      {
        if (!isBreak)
        {
          breakMessage = "02:00AM-02:30AM";
          isBreak = true;
        }

        // late
        if (now.minute() >= 31)
        {
          isLate = true;
        }
      }

      // check break
      if (now.hour() == 4 && now.minute() >= 0 && now.hour() == 4 && now.minute() <= 16)
      {
        if (!isBreak)
        {
          breakMessage = "04:00AM-04:15AM";
          isBreak = true;
        }

        // late
        if (now.minute() >= 16)
        {
          isLate = true;
        }
      }
    }

    // 6th shift
    if (activeShift == 5)
    {
      // check break
      if (now.hour() == 10 && now.minute() >= 0 && now.hour() == 10 && now.minute() <= 16)
      {
        if (!isBreak)
        {
          breakMessage = "10:00AM-10:15AM";
          isBreak = true;
        }

        // late
        if (now.minute() >= 16)
        {
          isLate = true;
        }
      }

      // check break
      if (now.hour() == 12 && now.minute() >= 0 && now.hour() == 12 && now.minute() <= 31)
      {
        if (!isBreak)
        {
          breakMessage = "12:00PM-12:30PM";
          isBreak = true;
        }

        // late
        if (now.minute() >= 31)
        {
          isLate = true;
        }
      }

      // check break
      if (now.hour() == 15 && now.minute() >= 0 && now.hour() == 15 && now.minute() <= 16)
      {
        if (!isBreak)
        {
          breakMessage = "03:00PM-03:15PM";
          isBreak = true;
        }

        // late
        if (now.minute() >= 16)
        {
          isLate = true;
        }
      }
    }

    // pass idle?
    if (idleCounter > 10)
    {
      isPass = false;
      isPassSwitch = false;
    }
  }

  // indicator
  {
    if (isBreak)
    {
      // late
      if (isLate)
      {
        digitalWrite(ledRed, HIGH);
        digitalWrite(ledGreen, LOW);
        digitalWrite(ledBuzzer, HIGH);
        //Serial.println("late");
      }
      else
      {
        digitalWrite(ledRed, HIGH);
        digitalWrite(ledGreen, LOW);
        digitalWrite(ledBuzzer, LOW);
        //Serial.println("break");
      } 
    }
    else
    {
      digitalWrite(ledRed, LOW);
      digitalWrite(ledGreen, HIGH);
      digitalWrite(ledBuzzer, LOW);
      //Serial.println("ok");
    }
  }

  // lcd 
  if (nextClear <= millis())
  {
    // display
    nextClear = millis() + 500;
    lcd.clear(); 
    
    // Active
    if (activeShift == 0)
    {
      if (!isBreak)
      {
        lcd.setCursor(0, 0);        
        lcd.print("Active Shift"); 
        lcd.setCursor(0, 1);        
        lcd.print("08:00AM-08:00PM");
      } 
    }

    // Active
    if (activeShift == 1)
    {
      if (!isBreak)
      {
        lcd.setCursor(0, 0);        
        lcd.print("Active Shift"); 
        lcd.setCursor(0, 1);        
        lcd.print("08:00PM-08:00AM"); 
      } 
    }

    // Active
    if (activeShift == 2)
    {
      if (!isBreak)
      {
        lcd.setCursor(0, 0);        
        lcd.print("Active Shift"); 
        lcd.setCursor(0, 1);        
        lcd.print("06:00AM-02:00PM"); 
      } 
    }

    // Active
    if (activeShift == 3)
    {
      if (!isBreak)
      {
        lcd.setCursor(0, 0);        
        lcd.print("Active Shift"); 
        lcd.setCursor(0, 1);        
        lcd.print("02:00PM-10:00PM"); 
      } 
    }

    // Active
    if (activeShift == 4)
    {
      if (!isBreak)
      {
        lcd.setCursor(0, 0);        
        lcd.print("Active Shift"); 
        lcd.setCursor(0, 1);        
        lcd.print("10:00PM-06:00AM"); 
      } 
    }

    // Active
    if (activeShift == 5)
    {
      if (!isBreak)
      {
        lcd.setCursor(0, 0);        
        lcd.print("Active Shift"); 
        lcd.setCursor(0, 1);        
        lcd.print("08:00AM-04:00PM"); 
      } 
    }

    // Break
    if (isBreak)
    {
      lcd.setCursor(0, 0);        
      lcd.print("   On Break!!   "); 
      lcd.setCursor(0, 1);        
      lcd.print(breakMessage); 
    }

    // Late
    if (isLate)
    {
      lcd.setCursor(0, 0);        
      lcd.print("  Unattended!!  "); 
      lcd.setCursor(0, 1);        
      lcd.print(String((int)lateCounter) + " seconds ago"); 
      lateCounter += 0.5;
    }

    // Pass?
    if (isPassSwitch || isPass)
    {
      lcd.clear(); 
      lcd.setCursor(0, 0);        
      lcd.print("   Enter Code   "); 
      lcd.setCursor(0, 1);        
      lcd.print("      " + passType + "      "); 
      idleCounter += 0.5;
    }
  }
}

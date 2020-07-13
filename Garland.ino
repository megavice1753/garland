#include <Adafruit_NeoPixel.h>
#include <SoftwareSerial.h>
#define RX 4
#define TX 2
SoftwareSerial BTSerial(RX, TX);//bluetooth rx ->arduino tx, bt tx-> arduino rx  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

#define DEBUG 2

#define PIXEL_PIN 6
#define PIXEL_COUNT 192//28///47+21 //300 =5m

#define CHP 1 //ch plus
#define CHM 2 //ch minus

#define PREV 5 //<<
#define NEXT 4 //>>
//#define PAUSE 0;

#define BRIGHTP 7
#define BRIGHTM 8

#define BASE64 9

#define MAX_PRGM 7

int WAIT = 90;
int WAIT_STEP = 10;
byte BRIGHT = 100; 
byte PRGM = 4;

#define BUF_LENGTH = 10;

char base64Content[4*PIXEL_COUNT + 4];

Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
    strip.begin();
    for (int i = 0; i < PIXEL_COUNT; ++i) {
      strip.setPixelColor(i, strip.Color(0, 0, 0));
    }
    strip.setBrightness(BRIGHT);
    strip.show();

    //Serial.begin(9600);
    BTSerial.begin(9600);
    if (DEBUG > 0) {
        //Serial.print("delay ");
        //Serial.print(WAIT);
        //Serial.print(" program ");
        //Serial.println(PRGM);
    }
}

void loop() {
    if (PRGM == 0) {
        pause();
    } else if (PRGM == 1) {
        singleColorIteration();
    } else if (PRGM == 2) {
        myPartRainbow();//theaterPartRainbow();
    } else if (PRGM == 3) {
        myChaseRainbow();//theaterChaseRainbow();
    } else if (PRGM == 4) {
        fullColor(strip.Color(255, 255, 255));
    } else if (PRGM == 5){
        fullColor(strip.Color(0, 0, 0));
    } else if (PRGM == 6) {
        runningLight(strip.Color(255, 255, 0));
    } else if (PRGM == 7) {
        programmableColor();
    }
}
 
 void programmableColor() {
   byte header[3] = {0, 0, 0};
   getContent(0, base64Content, header);
   byte comand = header[0] >> 4;
   int firstPix = ((header[0] & 0xf) << 6) | (header[1] >> 2);
   int secondPix = ((header[1] & 0x3) << 8) | header[2];
   if(comand == 0) {//отобразить полученный результат
     strip.show();
   } else if (comand == 1) {//заполнить переданный участок гирлянды
     for(int i = 0; firstPix+i <= secondPix; ++i) {
       getContent(1 + i, base64Content, header);
        strip.setPixelColor(firstPix+i, strip.Color(header[0], header[1], header[2]));
     }
   } else if(comand == 2) {//clear
     for (int i = 0; i < PIXEL_COUNT; ++i) {
        strip.setPixelColor(i, strip.Color(0, 0, 0));//rgb
     }
     strip.show();
   }
   delay(WAIT);
   serialHandler();
 }
 
void fullColor(uint32_t color) {
  for (int i = 0; i < PIXEL_COUNT; ++i) {
      strip.setPixelColor(i, color);//rgb
  }
  strip.show();
  delay(WAIT);
  serialHandler();
}

void singleColorIteration() {
    for (int j = 0; j < 256; ++j) {
        uint32_t color = wheel(j);
        for (int i = 0; i < PIXEL_COUNT; ++i) {
            strip.setPixelColor(i, color);
        }
        strip.show();
        delay(WAIT);
        if (serialHandler() == 1) {
            return;
        }
    }
}

void pause() {
   delay(WAIT);
   serialHandler();
}

void runningLight(uint32_t color) {
  for (int i = 0; i < PIXEL_COUNT; ++i) {
      strip.setPixelColor(i, strip.Color(0, 0, 0));//rgb
  }
  for(int i = 0; i < PIXEL_COUNT; ++i) {
    for(int j = 0; j < PIXEL_COUNT - i; ++j) {
      strip.setPixelColor(PIXEL_COUNT - j, strip.Color(0, 0, 0));//rgb
      strip.setPixelColor(PIXEL_COUNT - j - 1, color);//rgb
      strip.show();
      delay(WAIT*4);
      if (serialHandler() == 1) {
         return;
      }
    }
  }
}

//выводит радугу как есть, сколько влезет в размер гирлянды
void myPartRainbow() {
  //1536 цветов слишком много, даже на 150 пикселах кажется, что они все одного цвета
  //сократим в n раз
  int n = 6;
  for(int j = 0; j < 1536; j += 1) {
    for(int i = 0; i < PIXEL_COUNT; ++i) {
      int colorIndx = (j + i * n) % 1536;
      uint32_t color = getRGB(colorIndx);
      strip.setPixelColor(i, color);
    }
    strip.show();
    delay(WAIT);
    if (serialHandler() == 1) {
        return;
    }
  }
}

//ужимает радугу до размера гирлянды
void myChaseRainbow() {
  for(int j = 0; j < 1536; ++j) {
    for(int i = 0; i < PIXEL_COUNT; ++i) {
      int colorIndx = to1536(i + j, PIXEL_COUNT);
      uint32_t color = getRGB(colorIndx);
      strip.setPixelColor(i, color);
    }
    strip.show();
    delay(WAIT);
    if (serialHandler() == 1) {
        return;
    }
  }
}


//ужимает радугу до размера гирлянды
void theaterChaseRainbow() {
    for (int j = 0; j < PIXEL_COUNT; ++j) {
        for (int i = 0; i < PIXEL_COUNT; ++i) {
            strip.setPixelColor((i + j) % PIXEL_COUNT, wheel(i * 255 / PIXEL_COUNT));
        }
        strip.show();
        delay(WAIT);
        if (serialHandler() == 1) {
            return;
        }
    }
}

//выводит радугу как есть, сколько влезет в размер гирлянды
void theaterPartRainbow() {
    for (int j = 0; j < 256; ++j) {
        for (int i = 0; i < PIXEL_COUNT; ++i) {
            strip.setPixelColor(i, wheel((j + i) % 255));
        }
        strip.show();
        delay(WAIT);
        if (serialHandler() == 1) {
            return;
        }
    }
}

int signalCatcher() {
    //0  1  2  3  4  5  6  7  8  9  
    //48 49 50 51 52 53 54 55 56 57
    //int count = 0;
    int result = -1;
    if (BTSerial.available()) {
        String command = "";
        while (BTSerial.available()) {
          command += (char)BTSerial.read();
          delay(10);
        }
        if (DEBUG == 2) {
            BTSerial.write("!!");
            for(int j = 0; j < command.length();++j) {
              BTSerial.write(command[j]);
            }
            BTSerial.write("!!\r\n");
        }
        if(command == "CHP" || command == "chp") {
          result = CHP;
        } else if (command == "CHM" || command == "chm") {
          result = CHM;
        } else if (command == "PRV" || command == "prv") {
          result = PREV;
        } else if (command == "NXT" || command == "nxt") {
          result = NEXT;
        } else if(command == "BRP" || command == "brp") {
          result = BRIGHTP;
        } else if (command == "BRM" || command == "brm") {
          result = BRIGHTM;
        } else if(command.length() > 0 && command.length() % 4 == 0){
          result = BASE64;
          for(int i = 0; i < command.length(); ++i) {
            base64Content[i] = command[i];
          }
        }
    }
    return result;
}

int serialHandler() {
    int button = signalCatcher();
    if (button == PREV) { //prev == slow down
        WAIT += WAIT_STEP;
        if (DEBUG > 0) {
            BTSerial.write("delay changed to ");
            BTSerial.write(String(WAIT).c_str());
            BTSerial.write("\r\n");
        }
    } else if (button == NEXT) { //next == fast up
        if (WAIT >= 2 * WAIT_STEP) {
            WAIT -= WAIT_STEP;
            if (DEBUG > 0) {
                BTSerial.write("delay changed to ");
                BTSerial.write(String(WAIT).c_str());
                BTSerial.write("\r\n");
            }
        }
    } else if (button == CHP) {
        if (PRGM < MAX_PRGM) {
            ++PRGM;
            if (DEBUG > 0) {
                BTSerial.write("program changed to ");
                BTSerial.write(String(PRGM).c_str());
                BTSerial.write("\r\n");
            }
            return 1;
        }
    } else if (button == CHM) {
        if (PRGM > 0) {
            --PRGM;
            if (DEBUG > 0) {
                BTSerial.write("program changed to ");
                BTSerial.write(String(PRGM).c_str());
                BTSerial.write("\r\n");
            }
            return 1;
        }
    } else if (button == BRIGHTP) {
        BRIGHT = BRIGHT + 10 < 256 ? BRIGHT + 10 : 255;
        strip.setBrightness(BRIGHT);
        if (DEBUG > 0) {
            BTSerial.write("bright changed to ");
            BTSerial.write(String(BRIGHT).c_str());
            BTSerial.write("\r\n");
        }
    } else if (button == BRIGHTM) {
        BRIGHT = BRIGHT - 10 > -1 ? BRIGHT - 10 : 0;
        strip.setBrightness(BRIGHT);
        if (DEBUG > 0) {
            BTSerial.write("bright changed to ");
            BTSerial.write(String(BRIGHT).c_str());
            BTSerial.write("\r\n");
        }
    } else if (button == BASE64) {
      if (DEBUG > 0) {
            BTSerial.write("base64 ");
            byte header[3] = {0, 0, 0};
            getContent(0, base64Content, header);
            byte comand = header[0] >> 4;
            BTSerial.write(String(comand).c_str());
            BTSerial.write(" ");
            int firstPix = ((header[0] & 0xf) << 6) | (header[1] >> 2);
            BTSerial.write(String(firstPix).c_str());
            BTSerial.write(" ");
            int secondPix = ((header[1] & 0x3) << 8) | header[2];
            BTSerial.write(String(secondPix).c_str());
            BTSerial.write("\r\n");
        }
    }
    return 0;
}

const char* alphaBet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

byte posCalc(char c) {
  for(byte i = 0; i < 64; ++i) {
    if(alphaBet[i] == c) {
      return i;
    }
  }
  return 0;
}

void getContent(int pos, char* str, byte* result) {
  byte a = posCalc(str[0 + pos * 4]);
  byte b = posCalc(str[1 + pos * 4]);
  byte c = posCalc(str[2 + pos * 4]);
  byte d = posCalc(str[3 + pos * 4]);
  result[0] = (a << 2) | (b >> 4);
  result[1] = ((b & 0xf) << 4) | c >> 2;
  result[2] = ((c & 0x3) << 6) | d;
}

uint32_t wheel(byte wheelPos) {
    wheelPos = 255 - wheelPos;
    if (wheelPos < 85) {
        return strip.Color(255 - wheelPos * 3, 0, wheelPos * 3);
    }
    if (wheelPos < 170) {
        wheelPos -= 85;
        return strip.Color(0, wheelPos * 3, 255 - wheelPos * 3);
    }
    wheelPos -= 170;
    return strip.Color(wheelPos * 3, 255 - wheelPos * 3, 0);
}

//   0-255   256-511   512-767   768-1023   1024-1279   1280-1535
//---------------------------------------------------------------
//r   255     255-0       0         0         0-255        255
//g  0-255     255       255      255-0         0           0
//b    0        0       0-255      255         255        255-0
uint32_t getRGB(int val) {
        byte res[3] = {0, 0, 0};
	if(val < 256) {
		res[0] = 255;
		res[1] = val % 256;
	} else if(val < 512) {
		res[0] = 255 - val % 256;
		res[1] = 255;
	} else if(val < 768) {
		res[1] = 255;
		res[2] = val % 256;
	} else if(val < 1024) {
		res[1] = 255 - val % 256;
		res[2] = 255;
	} else if(val < 1280) {
		res[0] = val % 256;
		res[2] = 255;
	} else if(val < 1536) {
		res[0] = 255;
		res[2] = 255 - val % 256;
	} else {
		res[0] = 255;
		res[1] = 255;
		res[2] = 255;
	}
	return strip.Color(res[0], res[1], res[2]);
}

int to1536(int position, int total) {
        position = position % total;
	float f = 1536;
	return (f * position) / total;
}

/*base64 method doc
1111                          1111 |  1111 11            11  |  1111 1111
тип команды 4 бита
                          номер 1ого в пакете
                          от 0 до 1023 (10 бит)
                                                         номер последнего пиксела (включительно)
                                                         в пакете от 0 до 1023  (10 бит)


тип команд:
1 - заполнить гирлядну значениями
0 - отрисовать (show) гирлянду (передается) только заголовок из 3 байт
2 - очистить гирлянду




15 пикс - 45 байт 60 символов

*/

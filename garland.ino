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
#define PAUSE 6
#define BRIGHTP 7
#define BRIGHTM 8
#define BASE64 9

#define MAX_PRGM 8

unsigned int WAIT = 90;
#define WAIT_STEP 10
byte BRIGHT = 100; 
byte PRGM = 5;

String base64Content;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, NEO_GRB + NEO_KHZ800);
uint32_t globalColor = strip.Color(255, 255, 255);

void setup() {
	strip.begin();
	strip.setBrightness(BRIGHT);
	BTSerial.begin(9600);
}

void loop() {
	if (PRGM == 2) {
		singleColorIteration();
	} else if (PRGM == 3) {
		myPartRainbow();
	} else if (PRGM == 4) {
		myChaseRainbow();
	} else if (PRGM == 5) {
		uint32_t tmp = strip.Color(255, 255, 255);
		fullColor(tmp);
	} else if (PRGM == 6) {
		uint32_t tmp = strip.Color(0, 0, 0);
		fullColor(tmp);
	} else if (PRGM == 7) {
		runningLight();
	} else if (PRGM == 8) {
		fullColor(globalColor);
	} else {
		doNothing();
	}
}

int base64Handler() {
	int result = 0;
	if (base64Content.length() > 0) {
		byte header[3] = {0, 0, 0};
		getContent(0, base64Content.c_str(), header);
		byte comand = getComand(header);
		//предполагается несколько вызовов comand==1, затем вызов comand==0
		//command == 0 более не нужен, отображение осуществляется сразу при заполнении
		
		if (comand == 0) {//отобразить полученный результат
			strip.show();
			result = 1;
			PRGM = 0;//doNothing
		} else if (comand == 1) {//заполнить переданный участок гирлянды
			//этот кусок теперь не работает, надо думать, где хранить временную информацию о цветах перед отрисовкой
			//либо использовать сочетание очистки + этот метод
			unsigned int position = getPosition(header);
			int secondPix = position + base64Content.length() / 4  - 1;
			for (int i = 0; position+i <= secondPix; ++i) {
				getContent(1 + i, base64Content.c_str(), header);
				strip.setPixelColor(position+i, strip.Color(header[0], header[1], header[2]));
			}
			strip.show();
			result = 1;
			PRGM = 0;//doNothing
		} else if (comand == 2) {//clear
			for (int i = 0; i < PIXEL_COUNT; ++i) {
				strip.setPixelColor(i, strip.Color(0, 0, 0));//rgb
			}
			strip.show();
			result = 1;
			PRGM = 0;//doNothing
		} else if (comand == 3) {//set params in any order manually (prgm {1, value, 0}, delay{2, byte2, byte1}, bright{3, value, 0}, color{4, 0, 0,   r, g, b})
			int size = base64Content.length() / 4 - 1;
			for (int i = 0; i < size; ++i) {
				getContent(i + 1, base64Content.c_str(), header);
				if (header[0] == 1) {//prgm
					result = 1;
					PRGM = header[1];
					if (DEBUG > 0) {
						BTSerial.write("set prgm ");
						BTSerial.write(String(PRGM).c_str());
						BTSerial.write("\r\n");
					}
				} else if (header[0] == 2) {//wait
					WAIT = (header[1] << 8) | header[2];
					if (DEBUG > 0) {
						BTSerial.write("set wait ");
						BTSerial.write(String(WAIT).c_str());
						BTSerial.write("\r\n");
					}
				} else if (header[0] == 3) {//bright
					BRIGHT = header[1];
					if (DEBUG > 0) {
						BTSerial.write("set bright ");
						BTSerial.write(String(BRIGHT).c_str());
						BTSerial.write("\r\n");
					}
				} else if (header[0] == 4) {//color
					++i;
					getContent(i + 1, base64Content.c_str(), header);
					globalColor = strip.Color(header[0], header[1], header[2]);
					if (DEBUG > 0) {
						BTSerial.write("set color ");
						BTSerial.write(String(header[0]).c_str());
						BTSerial.write(" ");
						BTSerial.write(String(header[1]).c_str());
						BTSerial.write(" ");
						BTSerial.write(String(header[2]).c_str());
						BTSerial.write("\r\n");
					}
				}
			}
		}
		base64Content = "";
	}
	return result;
}

int fullColor(uint32_t& color) {
	for (int i = 0; i < PIXEL_COUNT; ++i) {
		strip.setPixelColor(i, color);
	}
	strip.show();
	delay(WAIT);
	return serialHandler();
}

void singleColorIteration() {
	for (int j = 0; j < 1536; ++j) {
		uint32_t color = getRGB(j);
		if (fullColor(color) == 1) {
			return;
		}
	}
}

void doNothing() {
	delay(WAIT);
	serialHandler();
}

void runningLight() {
	for (int i = 0; i < PIXEL_COUNT; ++i) {
		strip.setPixelColor(i, strip.Color(0, 0, 0));//rgb
	}
	for (int i = 0; i < PIXEL_COUNT; ++i) {
		for (int j = 0; j < PIXEL_COUNT - i; ++j) {
			strip.setPixelColor(PIXEL_COUNT - j, strip.Color(0, 0, 0));//rgb
			strip.setPixelColor(PIXEL_COUNT - j - 1, globalColor);//rgb
			strip.show();
			delay(WAIT);
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
	for (int j = 0; j < 1536; j += 1) {
		for (int i = 0; i < PIXEL_COUNT; ++i) {
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
	for (int j = 0; j < 1536; ++j) {
		for (int i = 0; i < PIXEL_COUNT; ++i) {
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

int signalCatcher() {
	//0  1  2  3  4  5  6  7  8  9  
	//48 49 50 51 52 53 54 55 56 57
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
		if (command == "CHP" || command == "chp") {
			result = CHP;
		} else if (command == "CHM" || command == "chm") {
			result = CHM;
		} else if (command == "PRV" || command == "prv") {
			result = PREV;
		} else if (command == "NXT" || command == "nxt") {
			result = NEXT;
		} else if (command == "BRP" || command == "brp") {
			result = BRIGHTP;
		} else if (command == "BRM" || command == "brm") {
			result = BRIGHTM;
		} else if (command.length() > 0 && command.length() % 4 == 0) {
			result = BASE64;
			base64Content = command;
		} else if (command == "PAUSE" || command == "pause") {
			result = PAUSE;
		}
	}
	return result;
}

int serialHandler() {
	return serialHandler(0);
}
//0 - обычное поведение, продолжать выполнять текущую задачу
//1 - прервать выполнение текущей задачи
//btn != 0  - состояние после паузы, нужно обработать последнюю команду
int serialHandler(int btn) {
	int button = btn == 0 ? signalCatcher() : btn;
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
		if (PRGM > 2) {
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
			getContent(0, base64Content.c_str(), header);
			byte comand = getComand(header);
			BTSerial.write(String(comand).c_str());
			BTSerial.write(", position:");
			unsigned int position = getPosition(header);
			BTSerial.write(String(position).c_str());
			BTSerial.write(", size:");
			unsigned int size = base64Content.length() / 4 - 1;
			BTSerial.write(String(size).c_str());
			BTSerial.write("\r\n");
		}
		return base64Handler();
	} else if (button == PAUSE && btn == 0) {
		do {
			delay(WAIT);
			button = signalCatcher();
		} while (button == -1);
		return serialHandler(button);
	}
	return 0;
}

const char* alphaBet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

byte posCalc(char c) {
	for (byte i = 0; i < 64; ++i) {
		if (alphaBet[i] == c) {
			return i;
		}
	}
	return 0;
}

void getContent(int pos, const char* base64Content, byte* result) {
	byte a = posCalc(base64Content[0 + pos * 4]);
	byte b = posCalc(base64Content[1 + pos * 4]);
	byte c = posCalc(base64Content[2 + pos * 4]);
	byte d = posCalc(base64Content[3 + pos * 4]);
	result[0] = (a << 2) | (b >> 4);
	result[1] = ((b & 0xf) << 4) | c >> 2;
	result[2] = ((c & 0x3) << 6) | d;
}

byte getComand(byte* header) {
	return header[0];
}

unsigned int getPosition(byte* header) {
	return (header[1] << 8) | header[2];
}

//   0-255   256-511   512-767   768-1023   1024-1279   1280-1535
//---------------------------------------------------------------
//r   255     255-0       0         0         0-255        255
//g  0-255     255       255      255-0         0           0
//b    0        0       0-255      255         255        255-0
uint32_t getRGB(int val) {
	byte res[3] = {0, 0, 0};
	if (val < 256) {
		res[0] = 255;
		res[1] = val % 256;
	} else if (val < 512) {
		res[0] = 255 - val % 256;
		res[1] = 255;
	} else if (val < 768) {
		res[1] = 255;
		res[2] = val % 256;
	} else if (val < 1024) {
		res[1] = 255 - val % 256;
		res[2] = 255;
	} else if (val < 1280) {
		res[0] = val % 256;
		res[2] = 255;
	} else if (val < 1536) {
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
11111111                 |  1111 1111  1111 1111
тип команды 8 бит
                          номер позиции 0ого пиксела из команды во всей гирлянде
                          от 0 до 65535 (16 бит)


тип команд:
0 - отрисовать (show) гирлянду (передается только заголовок из 3 байт)
1 - заполнить гирлядну значениями
2 - очистить гирлянду (передается только заголовок из 3 байт)
3 - задать глобальные параметры: цвет, программу, яркость, частоту



15 пикс - 45 байт 60 символов
1 пикс = 1 группа из 4х символов base64
*/

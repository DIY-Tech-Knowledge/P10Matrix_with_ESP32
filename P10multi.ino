#include <WiFi.h>
#include <WebServer.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <Fonts/FreeSansBold9pt7b.h>

/* ================= WIFI ================= */
#define WIFI_SSID "DIY Tech Knowledge"
#define WIFI_PASS "Diy@123499"

/* ================= MATRIX PINS ================= */
#define R1_PIN 25
#define G1_PIN 26
#define B1_PIN 27
#define R2_PIN 14
#define G2_PIN 12
#define B2_PIN 13

#define A_PIN 23
#define B_PIN 19
#define C_PIN 5
#define D_PIN -1
#define E_PIN -1

#define LAT_PIN 4
#define OE_PIN 15
#define CLK_PIN 16

#define MATRIX_WIDTH 64
#define MATRIX_HEIGHT 16

/* ================= MATRIX ================= */
HUB75_I2S_CFG mxconfig(
  MATRIX_WIDTH,
  MATRIX_HEIGHT,
  1);

HUB75_I2S_CFG::i2s_pins pins = {
  R1_PIN, G1_PIN, B1_PIN,
  R2_PIN, G2_PIN, B2_PIN,
  A_PIN, B_PIN, C_PIN, D_PIN, E_PIN,
  LAT_PIN, OE_PIN, CLK_PIN
};

MatrixPanel_I2S_DMA *display;

/* ================= WEB SERVER ================= */
WebServer server(80);

/* ================= MODES ================= */
enum DisplayMode {
  MODE_BOOT_IP,
  MODE_RANDOM_LETTER,
  MODE_ALPHA_SCROLL,
  MODE_NUMBER_SCROLL,
  MODE_RANDOM_NUMBER,
  MODE_CUSTOM_TEXT
};

DisplayMode currentMode = MODE_BOOT_IP;

/* ================= HELPERS ================= */
String numberText = "";
String customText = "HELLO SWASTIKA";
int scrollSpeed = 30;     // ms delay (lower = faster)
bool staticText = false;  // scroll or static
unsigned long bootStart = 0;
unsigned long lastLetterChange = 0;
unsigned long lastNumberChange = 0;

char randomLetter() {
  return 'A' + random(26);
}

uint16_t randomColor() {
  return display->color565(random(255), random(255), random(255));
}
/* -------- Spin Animation -------- */
void spinAnimation() {
  for (int i = 0; i < 6; i++) {
    display->fillScreen(0);
    for (int x = 0; x < MATRIX_WIDTH; x += 4) {
      for (int y = 0; y < MATRIX_HEIGHT; y += 4) {
        display->drawPixel(x, y, randomColor());
      }
    }
    delay(70);
  }
  display->fillScreen(0);
}
/* ================= MATRIX FUNCTIONS ================= */
void showStaticText() {
  display->fillScreen(0);
  display->setFont(&FreeSansBold9pt7b);
  display->setTextColor(randomColor());

  int16_t x1, y1;
  uint16_t w, h;
  display->getTextBounds(customText, 0, 0, &x1, &y1, &w, &h);

  int x = (MATRIX_WIDTH - w) / 2;
  int y = 14;

  display->setCursor(x, y);
  display->print(customText);
}

void scrollCustomText() {
  static int x = MATRIX_WIDTH;
  static unsigned long lastScroll = 0;

  if (millis() - lastScroll >= scrollSpeed) {
    display->fillScreen(0);
    display->setFont(&FreeSansBold9pt7b);
    display->setTextColor(randomColor());
    display->setCursor(x, 13);
    display->print(customText);

    int16_t x1, y1;
    uint16_t w, h;
    display->getTextBounds(customText, 0, 0, &x1, &y1, &w, &h);

    if (--x < -w) x = MATRIX_WIDTH;
    lastScroll = millis();
  }
}

void showLetter(char c) {
  display->fillScreen(0);
  display->setFont(&FreeSansBold9pt7b);
  display->setTextColor(randomColor());
  display->setCursor(22, 14);
  display->print(c);
}

void showRandomNumber() {
  int num = random(1, 101);  // 1 to 100

  display->fillScreen(0);
  display->setFont(&FreeSansBold9pt7b);
  display->setTextColor(randomColor());

  // Center approx
  display->setCursor(18, 14);
  display->print(num);
}

void scrollAlphabet() {
  static int x = MATRIX_WIDTH;
  static unsigned long lastScroll = 0;

  const char *text = "A B C D E F G H I J K L M N O P Q R S T U V W X Y Z";
  int textWidth = strlen(text) * 8 + 20;  // approximate width

  if (millis() - lastScroll >= 30) {
    display->fillScreen(0);
    display->setTextColor(randomColor());
    display->setCursor(x, 13);
    display->print(text);

    if (--x < -textWidth) {
      x = MATRIX_WIDTH;
    }

    lastScroll = millis();
  }
}

void buildNumberText() {
  numberText = "";
  for (int i = 1; i <= 100; i++) {
    numberText += String(i) + " ";
  }
}

void scrollNumbers() {
  static int x = MATRIX_WIDTH;
  static unsigned long lastScroll = 0;

  int textWidth = numberText.length() * 7;  // manual width

  if (millis() - lastScroll >= 30) {
    display->fillScreen(0);
    display->setTextColor(randomColor());
    display->setCursor(x, 13);
    display->print(numberText);

    if (--x < -textWidth) {
      x = MATRIX_WIDTH;
    }
    lastScroll = millis();
  }
}
void showIPAddress() {
  static int x = MATRIX_WIDTH;
  static unsigned long lastScroll = 0;
  static bool init = false;

  static String ip;  // 🔥 persistent string

  if (!init) {
    ip = WiFi.localIP().toString();
    x = MATRIX_WIDTH;
    init = true;
  }

  int textWidth = ip.length() * 8 + 10;  // approx width

  if (millis() - lastScroll >= 30) {
    display->fillScreen(0);
    display->setFont(&FreeSansBold9pt7b);
    display->setTextColor(randomColor());
    display->setCursor(x, 13);
    display->print(ip);

    if (--x < -textWidth) {
      x = MATRIX_WIDTH;
    }

    lastScroll = millis();
  }
}

/* ================= WEB PAGE ================= */
void handleRoot() {
  server.send(200, "text/html",
              "<!DOCTYPE html><html><head>"
              "<meta name='viewport' content='width=device-width, initial-scale=1'>"
              "<style>"
              "body{background:#111;color:white;font-family:Arial;text-align:center;}"
              "input,button{font-size:18px;padding:10px;margin:5px;}"
              "</style></head><body>"

              "<h2>ESP32 RGB Matrix</h2>"

              "<input id='txt' placeholder='Enter text'><br>"
              "<button onclick='sendText()'>Show Text</button><br>"

              "<b>Speed:</b> <input type='range' min='10' max='100' value='30' oninput='setSpeed(this.value)'><br>"

              "<button onclick='fetch(\"/scrollmode\")'>Scroll</button>"
              "<button onclick='fetch(\"/staticmode\")'>Static</button><br><br>"

              "<button onclick='fetch(\"/randomletter\")'>Random Letter</button>"
              "<button onclick='fetch(\"/alpha\")'>Alphabet</button>"
              "<button onclick='fetch(\"/number\")'>1-100 Numbers</button>"
              "<button onclick='fetch(\"/randnum\")'>Random Number</button>"

              "<script>"
              "function sendText(){"
              "let t=document.getElementById('txt').value;"
              "fetch('/settext?msg='+encodeURIComponent(t));}"
              "function setSpeed(v){fetch('/speed?val='+v);}"
              "</script>"

              "</body></html>");
}

void setCustomText() {
  if (server.hasArg("msg")) {
    customText = server.arg("msg");
    currentMode = MODE_CUSTOM_TEXT;
  }
  server.send(200, "text/plain", "OK");
}

void setSpeed() {
  if (server.hasArg("val")) {
    scrollSpeed = server.arg("val").toInt();
  }
  server.send(200, "text/plain", "Speed Set");
}

void setScrollMode() {
  staticText = false;
  currentMode = MODE_CUSTOM_TEXT;
  server.send(200, "text/plain", "Scroll Mode");
}

void setStaticMode() {
  staticText = true;
  currentMode = MODE_CUSTOM_TEXT;
  showStaticText();
  server.send(200, "text/plain", "Static Mode");
}

void setRandom() {
  currentMode = MODE_RANDOM_LETTER;
  server.send(200, "text/plain", "Random Mode");
}

void setAlphaScroll() {
  currentMode = MODE_ALPHA_SCROLL;
  server.send(200, "text/plain", "Alphabet Scroll");
}

void setNumberScroll() {
  currentMode = MODE_NUMBER_SCROLL;
  server.send(200, "text/plain", "Number Scroll");
}

void setRandomNumber() {
  currentMode = MODE_RANDOM_NUMBER;
  server.send(200, "text/plain", "Random Number Mode");
}


/* ================= SETUP ================= */
void setup() {
  Serial.begin(115200);
  randomSeed(esp_random());

  mxconfig.gpio = pins;
  mxconfig.driver = HUB75_I2S_CFG::FM6126A;
  mxconfig.clkphase = false;

  display = new MatrixPanel_I2S_DMA(mxconfig);
  display->begin();
  display->setBrightness8(120);
  display->clearScreen();

  buildNumberText();  // important

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConnected");
  Serial.print("Open browser: http://");
  Serial.println(WiFi.localIP());
  bootStart = millis();
  currentMode = MODE_BOOT_IP;

  server.on("/", handleRoot);
  server.on("/randomletter", setRandom);
  server.on("/alpha", setAlphaScroll);
  server.on("/number", setNumberScroll);
  server.on("/randnum", setRandomNumber);
  server.on("/settext", setCustomText);
  server.on("/speed", setSpeed);
  server.on("/scrollmode", setScrollMode);
  server.on("/staticmode", setStaticMode);
  server.begin();
}

/* ================= LOOP ================= */
void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    spinAnimation();
    delay(500);
    showLetter(randomLetter());
  } else {
    server.handleClient();
    yield();

    if (currentMode == MODE_BOOT_IP) {
      showIPAddress();

      if (millis() - bootStart > 5000) {  // 5 seconds
        currentMode = MODE_ALPHA_SCROLL;  // 🔥 SWITCH MODE
        display->clearScreen();
      }
    } else if (currentMode == MODE_RANDOM_LETTER) {
      // Show a new random letter every 5 seconds
      if (millis() - lastLetterChange >= 5000) {
        spinAnimation();
        delay(500);
        showLetter(randomLetter());
        lastLetterChange = millis();
      }
    } else if (currentMode == MODE_ALPHA_SCROLL) {
      scrollAlphabet();  // continuously scroll alphabet
    } else if (currentMode == MODE_NUMBER_SCROLL) {
      scrollNumbers();  // continuously scroll numbers 1-100
    } else if (currentMode == MODE_CUSTOM_TEXT) {
      if (staticText)
        showStaticText();  // display custom text static
      else
        scrollCustomText();  // scroll custom text
    } else if (currentMode == MODE_RANDOM_NUMBER) {
      // Show a random number every 5 seconds
      if (millis() - lastNumberChange >= 5000) {
        spinAnimation();
        delay(500);
        showRandomNumber();
        lastNumberChange = millis();
      }
    }
  }
}
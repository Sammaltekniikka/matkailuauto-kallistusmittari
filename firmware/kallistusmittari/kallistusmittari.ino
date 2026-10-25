// Kallistusmittari retkeilyautoon
// ESP32 (CH340) + MPU6050 (GY-521) + 2 x Waveshare 1.28inch LCD Module (GC9A01, 240x240)
//
// Näyttö 1 (CS GPIO5)  = PITUUS (pitch), auto sivulta
// Näyttö 2 (CS GPIO17) = SIVUTTAIN (roll), auto takaa
//
// Painike GPIO25 -> GND:
//   lyhyt painallus  = taustavalon kirkkaus 100 % / 40 % / 10 %
//   pitkä (2 s)      = nollaus (auto tasaisella), tallentuu muistiin
//
// Kirjastot: TFT_eSPI (Bodmer) ja TJpg_Decoder (Bodmer).
// Autokuvat ovat valmiiksi laskettuja JPEG-kehyksiä (frames_pitch.h, frames_roll.h),
// yksi kuva jokaiselle kuvan kallistusasteelle -30...+30.
// Arduino IDE: Tools > Partition Scheme > "Huge APP (3MB No OTA/1MB SPIFFS)".
// Lukeman fontti: FreeSansBold, jos User_Setup.h:ssa on LOAD_GFXFF, muuten fontti 7.
// Kuvat voi luoda uudelleen grafiikka-kansion työkaluilla (ks. grafiikka/LUEMINUT.txt).

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <TJpg_Decoder.h>
#include <Preferences.h>
#include <math.h>
#include "frames_pitch.h"
#include "frames_roll.h"

// ---------------- Pinnit ----------------
const int PIN_CS_PITCH = 5;
const int PIN_CS_ROLL  = 17;
const int PIN_BL       = 32;   // molempien näyttöjen BL. Aseta -1, jos BL on kytketty suoraan 3V3:een
const int PIN_BUTTON   = 25;
const int PIN_SDA      = 21;
const int PIN_SCL      = 22;

// ---------------- Anturin asennus ----------------
// Oletus: GY-521 komponenttipuoli ylöspäin, X-nuoli auton keulaan päin, Y-nuoli vasemmalle.
// Jos lukemat ovat väärin päin tai akselit vaihtuneet, korjaa nämä.
const bool SWAP_XY      = false;
const bool INVERT_PITCH = false;
const bool INVERT_ROLL  = false;

// ---------------- Näytön asetukset ----------------
const float VISUAL_GAIN  = 4.0f;    // auton kuvan kallistus liioiteltuna (lukema on aina todellinen)
const int   VISUAL_MAX   = 30;      // kuvan maksimikulma (kehyksiä -30...+30)
const float LEVEL_OK     = 0.5f;    // alle tämän vihreä
const float LEVEL_WARN   = 1.5f;    // alle tämän keltainen, muuten punainen
const float FILTER_ALPHA = 0.03f;   // pienempi = rauhallisempi lukema (näytteet 100 Hz)
const uint8_t BRIGHTNESS[] = {255, 100, 25};
const char* BRIGHTNESS_TXT[] = {"VALO 100%", "VALO 40%", "VALO 10%"};

const uint8_t MPU_ADDR = 0x68;

// Näyttö jaetaan kahteen osaan, jotka piirretään samalla 16-bittisellä spritellä:
// yläosa (auto + asteikko) 240x160 ja alaosa (lukema) 240x80.
const int TOP_H = 160;
const int GX = 120, GY = 132;       // asteikon keskipiste (maanpinta)

TFT_eSPI tft;
TFT_eSprite spr(&tft);           // 240x160, 16-bit = 77 kt
uint16_t BG;                     // taustaväri, sama kuin kehysten reunoilla
Preferences prefs;

float pitchF = 0, rollF = 0;        // suodatetut raakakulmat
float pitchOff = 0, rollOff = 0;    // nollauskulmat
bool filterInit = false;
bool sensorOk = false;
uint8_t brightIdx = 0;
unsigned long msgUntil = 0;
const char* msgText = "";

// ======================= MPU6050 =======================

bool mpuWrite(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(reg);
  Wire.write(val);
  return Wire.endTransmission() == 0;
}

bool mpuInit() {
  if (!mpuWrite(0x6B, 0x00)) return false;   // herätys unitilasta
  delay(50);
  mpuWrite(0x19, 0x09);   // näytetaajuus 100 Hz
  mpuWrite(0x1A, 0x06);   // sisäinen alipäästösuodin ~5 Hz
  mpuWrite(0x1C, 0x00);   // kiihtyvyysalue ±2 g (paras tarkkuus)
  return true;
}

bool mpuReadAccel(float &ax, float &ay, float &az) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((int)MPU_ADDR, 6) != 6) return false;
  uint8_t b[6];
  for (int i = 0; i < 6; i++) b[i] = Wire.read();
  ax = (int16_t)((b[0] << 8) | b[1]) / 16384.0f;
  ay = (int16_t)((b[2] << 8) | b[3]) / 16384.0f;
  az = (int16_t)((b[4] << 8) | b[5]) / 16384.0f;
  return true;
}

void sampleSensor() {
  static int failCount = 0;
  static unsigned long lastRetry = 0;

  float ax, ay, az;
  if (mpuReadAccel(ax, ay, az)) {
    if (SWAP_XY) { float t = ax; ax = ay; ay = t; }
    float p = atan2f(ax, sqrtf(ay * ay + az * az)) * RAD_TO_DEG;  // + = keula ylempänä
    float r = atan2f(ay, az) * RAD_TO_DEG;                         // + = vasen puoli ylempänä
    if (INVERT_PITCH) p = -p;
    if (INVERT_ROLL)  r = -r;

    if (!filterInit) { pitchF = p; rollF = r; filterInit = true; }
    else {
      pitchF += FILTER_ALPHA * (p - pitchF);
      rollF  += FILTER_ALPHA * (r - rollF);
    }
    failCount = 0;
    sensorOk = true;
  } else if (++failCount > 20) {
    sensorOk = false;
    if (millis() - lastRetry > 2000) {      // yritetään uudelleen, jos liitin on irronnut hetkeksi
      lastRetry = millis();
      Wire.end();
      Wire.begin(PIN_SDA, PIN_SCL);
      Wire.setClock(400000);
      if (mpuInit()) filterInit = false;
    }
  }
}

// ======================= Taustavalo ja painike =======================

void setBacklight(uint8_t v) {
  if (PIN_BL < 0) return;
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(PIN_BL, v);
#else
  ledcWrite(0, v);
#endif
}

void initBacklight() {
  if (PIN_BL < 0) return;
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttach(PIN_BL, 5000, 8);
#else
  ledcSetup(0, 5000, 8);
  ledcAttachPin(PIN_BL, 0);
#endif
}

void showMessage(const char* txt, unsigned long ms) {
  msgText = txt;
  msgUntil = millis() + ms;
}

void zeroOffsets() {
  pitchOff = pitchF;
  rollOff  = rollF;
  prefs.putFloat("p0", pitchOff);
  prefs.putFloat("r0", rollOff);
  showMessage("NOLLATTU", 2000);
}

void cycleBrightness() {
  brightIdx = (brightIdx + 1) % 3;
  setBacklight(BRIGHTNESS[brightIdx]);
  prefs.putUChar("bl", brightIdx);
  showMessage(BRIGHTNESS_TXT[brightIdx], 1200);
}

void handleButton() {   // kutsutaan 10 ms välein -> toimii samalla debouncena
  static bool last = HIGH;
  static unsigned long pressStart = 0;
  static bool longDone = false;

  bool st = digitalRead(PIN_BUTTON);
  unsigned long now = millis();

  if (st == LOW && last == HIGH) { pressStart = now; longDone = false; }
  if (st == LOW && !longDone && now - pressStart >= 2000) { zeroOffsets(); longDone = true; }
  if (st == HIGH && last == LOW && !longDone && now - pressStart >= 50) cycleBrightness();
  last = st;
}

// ======================= Piirto =======================

// TJpg_Decoder kutsuu tätä jokaiselle puretulle lohkolle -> kopioidaan spriteen
bool jpgToSprite(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bmp) {
  spr.pushImage(x, y, w, h, bmp);
  return true;
}

uint16_t levelColor(float a) {
  a = fabsf(a);
  if (a < LEVEL_OK)   return TFT_GREEN;
  if (a < LEVEL_WARN) return TFT_YELLOW;
  return TFT_RED;
}

// Reunarengas. cy = ympyrän keskipiste spriten koordinaateissa.
void drawRing(int cy, uint16_t col) {
  for (int r = 117; r <= 119; r++) spr.drawCircle(120, cy, r, col);
}

// Yläosa: valmis autokuva + osoitin asteikolla
void drawTop(bool isPitch, float angle, bool msgOn) {
  int vis = constrain((int)lroundf(angle * VISUAL_GAIN), -VISUAL_MAX, VISUAL_MAX);
  int k = vis + VISUAL_MAX;
  const uint8_t* jpg = isPitch ? (const uint8_t*)pgm_read_ptr(&PITCH_FRAMES[k])
                               : (const uint8_t*)pgm_read_ptr(&ROLL_FRAMES[k]);
  uint32_t len = isPitch ? pgm_read_dword(&PITCH_SIZES[k]) : pgm_read_dword(&ROLL_SIZES[k]);
  TJpgDec.drawJpg(0, 0, jpg, len);

  uint16_t col = levelColor(angle);
  // osoitin: sama kulma kuin auton kuvassa (pituus: keula ylös = vastapäivään)
  float rot = (isPitch ? -vis : vis) * DEG_TO_RAD;
  float sx = sinf(rot), cy = -cosf(rot);
  spr.drawWideLine(GX + 92 * sx, GY + 92 * cy, GX + 114 * sx, GY + 114 * cy, 5, col, BG);
  drawRing(120, col);

  if (msgOn) {
    spr.fillRoundRect(30, 70, 180, 44, 10, TFT_NAVY);
    spr.setTextColor(TFT_WHITE);
    spr.setTextDatum(MC_DATUM);
    spr.drawString(msgText, 120, 92, 4);
  }
}

// Alaosa: lukema ja otsikko. Spriten y=0 on näytön y=160.
void drawBottom(bool isPitch, float angle) {
  uint16_t col = levelColor(angle);
  spr.fillSprite(BG);
  drawRing(120 - TOP_H, col);

  char buf[12];
  if (fabsf(angle) < 0.05f) snprintf(buf, sizeof buf, "0.0");
  else                      snprintf(buf, sizeof buf, "%+.1f", angle);
  spr.setTextColor(col);
  spr.setTextDatum(TC_DATUM);
#ifdef LOAD_GFXFF
  // FreeFont käytössä (User_Setup.h: #define LOAD_GFXFF)
  spr.setFreeFont(&FreeSansBold24pt7b);
  spr.drawString(buf, 116, 8);
  int w = spr.textWidth(buf);
  spr.setTextFont(1);
#else
  // muuten TFT_eSPI:n sisäänrakennettu fontti 7 (7-segmenttinumerot)
  spr.drawString(buf, 116, 4, 7);
  int w = spr.textWidth(buf, 7);
#endif
  spr.fillCircle(116 + w / 2 + 8, 12, 5, col);   // asteen merkki
  spr.fillCircle(116 + w / 2 + 8, 12, 2, BG);

  spr.setTextColor(0x9DB8);                       // vaalea harmaansininen
  spr.drawString(isPitch ? "PITUUS" : "SIVUTTAIN", 120, 54, 2);
}

void drawError(bool top) {
  spr.fillSprite(BG);
  drawRing(top ? 120 : 120 - TOP_H, TFT_RED);
  if (top) {
    spr.setTextColor(TFT_RED);
    spr.setTextDatum(MC_DATUM);
    spr.drawString("ANTURI?", 120, 120, 4);
  }
}

void selectDisplay(int csPin) {
  digitalWrite(PIN_CS_PITCH, csPin == PIN_CS_PITCH ? LOW : HIGH);
  digitalWrite(PIN_CS_ROLL,  csPin == PIN_CS_ROLL  ? LOW : HIGH);
}

void deselectAll() {
  digitalWrite(PIN_CS_PITCH, HIGH);
  digitalWrite(PIN_CS_ROLL, HIGH);
}

void showScreen(int csPin, bool isPitch, float angle, bool msgOn) {
  // yläosa
  if (sensorOk) drawTop(isPitch, angle, msgOn); else drawError(true);
  selectDisplay(csPin);
  spr.pushSprite(0, 0);
  deselectAll();
  // alaosa (sprite piirretään kohtaan y=160, ylimenevä osa leikkautuu pois)
  if (sensorOk) drawBottom(isPitch, angle); else drawError(false);
  selectDisplay(csPin);
  spr.pushSprite(0, TOP_H);
  deselectAll();
}

void updateDisplays() {
  static int lastP = 99999, lastR = 99999;
  static bool lastMsgP = false, lastMsgR = false, lastOkP = false, lastOkR = false;

  int p10 = (int)lroundf((pitchF - pitchOff) * 10);
  int r10 = (int)lroundf((rollF - rollOff) * 10);
  bool msgOn = millis() < msgUntil;

  if (p10 != lastP || msgOn != lastMsgP || sensorOk != lastOkP || msgOn) {
    showScreen(PIN_CS_PITCH, true, p10 / 10.0f, msgOn);
    lastP = p10; lastMsgP = msgOn; lastOkP = sensorOk;
  }
  if (r10 != lastR || msgOn != lastMsgR || sensorOk != lastOkR || msgOn) {
    showScreen(PIN_CS_ROLL, false, r10 / 10.0f, msgOn);
    lastR = r10; lastMsgR = msgOn; lastOkR = sensorOk;
  }
}

// ======================= Setup & loop =======================

void setup() {
  Serial.begin(115200);

  // Molemmat näytöt valitaan alustuksen ajaksi -> alustetaan yhdellä kertaa
  pinMode(PIN_CS_PITCH, OUTPUT);
  pinMode(PIN_CS_ROLL, OUTPUT);
  digitalWrite(PIN_CS_PITCH, LOW);
  digitalWrite(PIN_CS_ROLL, LOW);
  tft.init();
  tft.setRotation(0);
  BG = tft.color565(11, 15, 20);
  tft.fillScreen(BG);
  deselectAll();

  pinMode(PIN_BUTTON, INPUT_PULLUP);

  prefs.begin("tilt", false);
  pitchOff  = prefs.getFloat("p0", 0);
  rollOff   = prefs.getFloat("r0", 0);
  brightIdx = prefs.getUChar("bl", 0);
  if (brightIdx > 2) brightIdx = 0;

  initBacklight();
  setBacklight(BRIGHTNESS[brightIdx]);

  spr.setColorDepth(16);                // täydet värit; 240x160 = 77 kt, mahtuu ilman PSRAMia
  if (!spr.createSprite(240, TOP_H)) Serial.println("Spriten luonti epäonnistui");

  TJpgDec.setJpgScale(1);
  TJpgDec.setSwapBytes(true);           // spriteen piirrettäessä tavujärjestys käännetään
  TJpgDec.setCallback(jpgToSprite);

  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(400000);
  sensorOk = mpuInit();
  Serial.println(sensorOk ? "MPU6050 OK" : "MPU6050 ei vastaa");
}

void loop() {
  static unsigned long lastSample = 0, lastDraw = 0, lastLog = 0;
  unsigned long now = millis();

  if (now - lastSample >= 10) {         // 100 Hz
    lastSample = now;
    sampleSensor();
    handleButton();
  }
  if (now - lastDraw >= 100) {          // näyttö max 10 Hz
    lastDraw = now;
    updateDisplays();
  }
  if (now - lastLog >= 500) {           // sarjaporttiin akselien tarkistusta varten
    lastLog = now;
    Serial.printf("pitch %.2f  roll %.2f  (raaka %.2f / %.2f)\n",
                  pitchF - pitchOff, rollF - rollOff, pitchF, rollF);
  }
}

# Firmware

ESP32:n ohjelmakoodi kallistusmittarille: [`kallistusmittari/`](kallistusmittari/)

| Tiedosto | Sisältö |
|---|---|
| `kallistusmittari.ino` | Pääohjelma: anturin luku, suodatus, nollaus, kirkkaus ja näyttöjen piirto |
| `frames_pitch.h` | 61 valmista JPEG-kuvaa autosta sivulta (kuvan kallistus −30°…+30°) |
| `frames_roll.h` | 61 valmista JPEG-kuvaa autosta takaa (kuvan kallistus −30°…+30°) |
| `grafiikka/` | Autokuvien lähdetiedostot (SVG) ja työkalut kuvien luomiseen uudelleen |

## Kirjastot

Asenna Arduino IDE:n Library Managerista:

- **TFT_eSPI** (Bodmer)
- **TJpg_Decoder** (Bodmer)

Wire ja Preferences tulevat ESP32-ytimen mukana.

### TFT_eSPI:n asetukset

TFT_eSPI-kirjaston `User_Setup.h`:ssa pitää olla GC9A01-ajuri ja kytkentäkaavion mukaiset SPI-pinnit, esimerkiksi:

```cpp
#define GC9A01_DRIVER
#define TFT_WIDTH  240
#define TFT_HEIGHT 240
#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_CS   -1   // CS-pinnejä (5 ja 17) ohjataan koodissa
#define TFT_DC   27
#define TFT_RST  26
```

Lukema piirretään FreeSansBold-fontilla, jos `User_Setup.h`:ssa on `#define LOAD_GFXFF`. Muuten käytetään sisäänrakennettua 7-segmenttifonttia (fontti 7).

## Lataaminen

1. Avaa `kallistusmittari/kallistusmittari.ino` Arduino IDE:ssä.
2. **Tools → Board:** ESP32 Dev Module
3. **Tools → Partition Scheme:** Huge APP (3MB No OTA/1MB SPIFFS) – autokuvat vievät noin 800 kt.
4. Valitse COM-portti ja paina **Upload**. Jos lataus jää kohtaan "Connecting…", pidä ESP32:n BOOT-painiketta pohjassa.

## Käyttö

- **Lyhyt painallus:** taustavalon kirkkaus 100 % / 40 % / 10 %
- **Pitkä painallus (2 s):** nollaus – pysäköi auto tasaiselle ja pidä painike pohjassa. Nollaus tallentuu pysyvään muistiin.
- Lukeman väri: vihreä alle 0,5°, keltainen alle 1,5°, muuten punainen.

## Säädettävät asetukset

Koodin alussa:

| Asetus | Merkitys |
|---|---|
| `SWAP_XY`, `INVERT_PITCH`, `INVERT_ROLL` | Anturin asennusasento, jos lukemat ovat väärin päin |
| `VISUAL_GAIN` | Kuinka paljon auton kuvan kallistusta liioitellaan (oletus 4×) |
| `LEVEL_OK`, `LEVEL_WARN` | Värirajat asteina |
| `FILTER_ALPHA` | Lukeman rauhoitus (pienempi = rauhallisempi) |

## Autokuvien muokkaaminen

Kuvat on piirretty SVG-muotoon (`grafiikka/side.svg` ja `grafiikka/rear.svg`). Kun niitä muokkaa, kehykset luodaan uudelleen työkaluilla – ohjeet: [`grafiikka/LUEMINUT.txt`](kallistusmittari/grafiikka/LUEMINUT.txt).

Idea valmiiksi lasketuista kuvista on peräisin upirin [Pitch and Roll indicator](https://github.com/upiir/pitch_roll_indicator) -projektista.

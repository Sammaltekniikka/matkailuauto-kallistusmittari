# Matkailuauton kallistusmittari

ESP32-pohjainen kallistusmittari matkailuautoon. Kaksi pyöreää näyttöä näyttävät auton kallistuksen pituus- ja leveyssuunnassa – auton kuva ja asteluku. Mittarin voi nollata painikkeella, ja nollaus tallentuu pysyvään muistiin, joten laite on siirrettävissä autosta toiseen ilman koodimuutoksia.

> 🎬 Projektista on tehty kolmiosainen videosarja. Videot löytyvät [SammalTekniikka-kanavalta](https://www.youtube.com/@sammaltekniikka).

## Ominaisuudet

- Pituuskallistus (pitch) ja sivukallistus (roll) omilla näytöillään
- Auton kuva kallistuu kulman mukana + asteluku (valmiiksi lasketut, pehmeäreunaiset kuvat)
- Nollauspainike: auton "suora" asento tallennetaan pysyvään muistiin
- Ei autokohtaisia asetuksia koodissa
- Samalla painikkeella lyhyellä painalluksella voidaan säätää näytön kirkkautta
## Komponentit

| Osa | Määrä | Huom. |
|---|---|---|
| ESP32 DevKit (CH340) | 1 | Pääohjain |
| MPU6050 (GY-521) | 1 | Kiihtyvyysanturi/gyro, I2C |
| Waveshare 1.28inch LCD Module (GC9A01, 240×240) | 2 | Pyöreät näytöt, SPI |
| Painike (kaksinapainen) | 1 | Nollaus |

## Kytkentäkaavio

![Kytkentäkaavio](hardware/Kallistusmittarin%20kytkent%C3%A4kaavio%20retkeilyautoon_johdotuspohja.jpg)

## Kytkennät

Näytöt jakavat SPI-väylän (DIN, CLK, DC, RST, BL), ja kummallakin on oma CS-pinni. MPU6050 on I2C-väylässä.

### MPU6050 → ESP32 (I2C)

| MPU6050 | ESP32 |
|---|---|
| VCC | 3V3 |
| GND | GND |
| SCL | GPIO22 (D22) |
| SDA | GPIO21 (D21) |
| XDA, XCL, AD0, INT | Ei kytketä |

### Näytöt → ESP32 (yhteiset SPI-johdot)

| Näyttö | ESP32 |
|---|---|
| VCC | 3V3 (molemmat näytöt) |
| GND | GND (molemmat näytöt) |
| DIN | GPIO23 (D23) |
| CLK | GPIO18 (D18) |
| DC | GPIO27 (D27) |
| RST | GPIO26 (D26) |
| BL | GPIO32 (D32) |
| CS, näyttö 1 – pituuskallistus (pitch) | GPIO5 (D5) |
| CS, näyttö 2 – leveyskallistus (roll) | GPIO17 (D17) |

### Nollauspainike → ESP32

| Painike | ESP32 |
|---|---|
| Liitin 1 | GPIO25 (D25) |
| Liitin 2 | GND |

### Virransyöttö

- ESP32 saa virran USB:stä (5 V, tietokone tai USB-laturi).
- Näytöt ja MPU6050 saavat virtansa ESP32:n 3,3 V:sta (kokonaisvirta noin 100 mA).
- Kaikki GND:t ovat yhteisiä.

## Hakemistorakenne

```
firmware/   ESP32:n ohjelmakoodi ja autokuvat
hardware/   Kytkentäkaavio, osaluettelo, kotelon tiedostot
docs/       Ohjeet ja kuvat
```

## Asennus

1. Kytke osat yllä olevan taulukon mukaan.
2. Lataa koodi `firmware/kallistusmittari/` ESP32:lle – kirjastot ja asetukset: [`firmware/README.md`](firmware/README.md).
3. Asenna mittari autoon, pysäköi auto tasaiselle alustalle ja paina nollauspainiketta.

## Videot

1. [Osa 1 – Rakennan kallistusmittarin retkeilyautoon ESP32:lla!](https://www.youtube.com/watch?v=94Pz2oVSIYc)
2. [Osa 2 – Kallistusmittarin rakentaminen – komponenttien juottaminen!](https://www.youtube.com/watch?v=AogkXMNzD_4)
3. Osa 3 – _tulossa_

## Tila

Toimiva versio valmis.

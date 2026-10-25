# Matkailuauton kallistusmittari

ESP32-pohjainen kallistusmittari matkailuautoon. Kaksi pyöreää näyttöä näyttävät auton kallistuksen pituus- ja leveyssuunnassa – auton kuva ja asteluku. Mittarin voi nollata painikkeella, ja nollaus tallentuu pysyvään muistiin, joten laite on siirrettävissä autosta toiseen ilman koodimuutoksia.

> 🎬 Projektista on tehty kolmiosainen videosarja. Linkit lisätään tähän, kun osat julkaistaan.

## Ominaisuudet

- Pituuskallistus (pitch) ja sivukallistus (roll) omilla näytöillään
- Auton kuva kallistuu kulman mukana + asteluku
- Nollauspainike: auton "suora" asento tallennetaan pysyvään muistiin
- Ei autokohtaisia asetuksia koodissa

## Komponentit

| Osa | Määrä | Huom. |
|---|---|---|
| ESP32 DevKit (CH340) | 1 | Pääohjain |
| MPU6050 | 1 | Kiihtyvyysanturi/gyro, I2C |
| Waveshare 1.28inch LCD Module (GC9A01, 240×240) | 2 | Pyöreät näytöt, SPI |
| Painike | 1 | Nollaus |

## Kytkennät

Näytöt jakavat SPI-väylän, ja kummallakin on oma CS-pinni. MPU6050 on I2C-väylässä.

| Signaali | ESP32 GPIO |
|---|---|
| MPU6050 SDA | _täydennä_ |
| MPU6050 SCL | _täydennä_ |
| Näyttö SCK (jaettu) | _täydennä_ |
| Näyttö MOSI/DIN (jaettu) | _täydennä_ |
| Näyttö DC (jaettu) | _täydennä_ |
| Näyttö RST (jaettu) | _täydennä_ |
| Näyttö BL (taustavalo) | GPIO32 |
| Näyttö 1 CS (pituus) | _täydennä_ |
| Näyttö 2 CS (sivu) | _täydennä_ |
| Nollauspainike | _täydennä_ |

Tarkempi kytkentäkaavio: [`hardware/`](hardware/)

## Hakemistorakenne

```
firmware/   ESP32:n ohjelmakoodi
hardware/   Kytkentäkaavio, osaluettelo, kotelon tiedostot
docs/       Ohjeet ja kuvat
```

## Asennus

1. Kytke osat yllä olevan taulukon mukaan.
2. Avaa `firmware/` ja lataa koodi ESP32:lle (ohjeet: [`firmware/README.md`](firmware/README.md)).
3. Asenna mittari autoon, pysäköi auto tasaiselle alustalle ja paina nollauspainiketta.

## Videot

1. Osa 1 – _tulossa_
2. Osa 2 – _tulossa_
3. Osa 3 – _tulossa_

## Tila

🚧 Kehitysvaiheessa – nykyinen versio on demo.

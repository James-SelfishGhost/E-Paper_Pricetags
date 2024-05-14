#include <Arduino.h>
#include <SPI.h>
#include "RFV3.h"
#include "cc1101_spi.h"

SPIClass *spi_bus = nullptr;

void init_spi()
{
  log("SPI init");

  pinMode(SS_PIN, OUTPUT);

#if defined(ARDUINO_ESP32_DEV)
  // Code for ESP32
  spi_bus = new SPIClass(VSPI);
  spi_bus->begin(CLK_PIN, MISO_PIN, MOSI_PIN);

#elif defined(ARDUINO_ESP32S3_DEV)
  // Code for ESP32-S3
  spi_bus = new SPIClass(FSPI);
  spi_bus->begin(CLK_PIN, MISO_PIN, MOSI_PIN);

#else
  #error "Unsupported board"
#endif

  log("SPI init done");
}
void spi_start()
{
  digitalWrite(SS_PIN, LOW);
  spi_bus->beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
}

void spi_end()
{
  spi_bus->endTransaction();
  digitalWrite(SS_PIN, HIGH);
}

uint8_t spi_putc(uint8_t data)
{
  return spi_bus->transfer(data);
}

void spi_write_strobe(uint8_t spi_instr)
{
  spi_start();
  spi_putc(spi_instr);
  spi_end();
}

uint8_t spi_read_register(uint8_t spi_instr)
{
  spi_start();
  spi_putc(spi_instr | 0x80);
  spi_instr = spi_putc(0xFF);
  spi_end();

  return spi_instr;
}

void spi_read_burst(uint8_t spi_instr, uint8_t *pArr, uint8_t length)
{
  spi_start();
  spi_putc(spi_instr | 0xC0);

  for (uint8_t i = 0; i < length; i++)
  {
    pArr[i] = spi_putc(0xFF);
  }
  spi_end();
}

void spi_write_register(uint8_t spi_instr, uint8_t value)
{
  spi_start();
  spi_putc(spi_instr | 0x00);
  spi_putc(value);
  spi_end();
}

void spi_write_burst(uint8_t spi_instr, uint8_t *pArr, uint8_t length)
{
  spi_start();
  spi_putc(spi_instr | 0x40);

  for (uint8_t i = 0; i < length; i++)
  {
    spi_putc(pArr[i]);
  }
  spi_end();
}
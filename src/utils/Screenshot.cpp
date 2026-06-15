#include "Screenshot.h"
#include <SD.h>
#include <LVGL_CYD.h>

// Helper to write a 32-bit integer to the file (little-endian)
static void write32(File &f, uint32_t val) {
  f.write(val & 0xFF);
  f.write((val >> 8) & 0xFF);
  f.write((val >> 16) & 0xFF);
  f.write((val >> 24) & 0xFF);
}

// Helper to write a 16-bit integer to the file (little-endian)
static void write16(File &f, uint16_t val) {
  f.write(val & 0xFF);
  f.write((val >> 8) & 0xFF);
}

void saveScreenshot(const char* filename) {
  if (SD.cardType() == CARD_NONE) {
    Serial.println("Screenshot failed: No SD card attached.");
    return;
  }

  File f = SD.open(filename, FILE_WRITE);
  if (!f) {
    Serial.println("Screenshot failed: Could not open file for writing.");
    return;
  }

  // Get current screen dimensions
  uint16_t w = LVGL_CYD::tft->width();
  uint16_t h = LVGL_CYD::tft->height();

  // Each row in BMP must be a multiple of 4 bytes
  uint32_t rowSize = (w * 3 + 3) & ~3;
  uint32_t imageSize = rowSize * h;
  uint32_t fileSize = 54 + imageSize;

  // BMP Header (14 bytes)
  f.write('B');
  f.write('M');
  write32(f, fileSize);
  write32(f, 0); // Reserved
  write32(f, 54); // Data offset

  // DIB Header (40 bytes)
  write32(f, 40); // DIB header size
  write32(f, w); // Width
  write32(f, h); // Height (positive means bottom-up)
  write16(f, 1); // Color planes
  write16(f, 24); // Bits per pixel (RGB888)
  write32(f, 0); // Compression (0 = none)
  write32(f, imageSize); // Image size
  write32(f, 2835); // X pixels per meter (approx 72 DPI)
  write32(f, 2835); // Y pixels per meter
  write32(f, 0); // Colors in color table
  write32(f, 0); // Important color count

  // BMP files are written bottom-up
  uint8_t rowBuffer[rowSize];
  for (int y = h - 1; y >= 0; y--) {
    int bufIdx = 0;
    for (int x = 0; x < w; x++) {
      uint16_t color565 = LVGL_CYD::tft->readPixel(x, y);
      
      // Extract RGB from RGB565
      uint8_t r = (color565 >> 11) & 0x1F;
      uint8_t g = (color565 >> 5) & 0x3F;
      uint8_t b = color565 & 0x1F;
      
      // Scale up to 8-bit
      r = (r << 3) | (r >> 2);
      g = (g << 2) | (g >> 4);
      b = (b << 3) | (b >> 2);
      
      // BMP uses BGR order
      rowBuffer[bufIdx++] = b;
      rowBuffer[bufIdx++] = g;
      rowBuffer[bufIdx++] = r;
    }
    
    // Pad row to multiple of 4 bytes
    while (bufIdx < rowSize) {
      rowBuffer[bufIdx++] = 0;
    }
    
    f.write(rowBuffer, rowSize);
  }

  f.close();
  Serial.println("Screenshot saved successfully!");
}

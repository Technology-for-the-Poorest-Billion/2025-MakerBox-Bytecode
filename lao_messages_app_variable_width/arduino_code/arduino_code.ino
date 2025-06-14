#include <Wire.h>               // for I2C communication
#include <Adafruit_GFX.h>       // graphics library
#include <Adafruit_SSD1306.h>   // specific driver for OLED
#include <avr/pgmspace.h>       // for storing bitmaps in flash memory
#include "glyph_bitmaps.h"      // the bitmap file
#include "phrases_to_display.h" // the phrases file

// screen dimensions
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1 // not using a reset pin

// creating an instance of the display driver
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Function to scroll and display a phrase
void scrollPhrase(const uint8_t *lao_phrase, uint8_t len_phrase)
{
  // Calculate the total unpadded width of the phrase for determining scroll end
  int total_unpadded_scroll_width = 0;
  for (int i = 0; i < len_phrase; i++)
  {
    int glyph_index = pgm_read_byte(lao_phrase + i); // Use pgm_read_byte to fetch from PROGMEM
    total_unpadded_scroll_width += pgm_read_word(&unpadded_widths[glyph_index]);
  }

  int scroll_offset = -SCREEN_WIDTH; // Start first character off-screen to the right

  // Loop while the entire phrase hasn't scrolled off the left side
  while (scroll_offset <= total_unpadded_scroll_width)
  {
    display.clearDisplay(); // Clear the display for the next frame

    int x = -scroll_offset;                     // Current X position on screen, adjusted by scroll_offset
    int y = (SCREEN_HEIGHT - GLYPH_HEIGHT) / 2; // Y position (vertically centered)

    // Loop through each glyph in the phrase
    for (int i = 0; i < len_phrase; i++)
    {
      int glyph_index = pgm_read_byte(lao_phrase + i);                          // Get the index of the current glyph
      int bitmap_start = pgm_read_word(&bitmap_starts[glyph_index]);           // Fetch start byte offset from PROGMEM
      int glyph_width_byte_aligned = pgm_read_word(&glyph_widths[glyph_index]); // Fetch byte-alighned width from PROGMEM
      int glyph_width_unpadded = pgm_read_word(&unpadded_widths[glyph_index]);  // Fetch unpadded width (for visual spacing) from PROGMEM
      
      // Calculate bytes per row and total bytes for this specific bitmap
      int BYTES_PER_ROW = (glyph_width_byte_aligned + 7) / 8;
      int BYTES_PER_BITMAP = BYTES_PER_ROW * GLYPH_HEIGHT;

      // Create a buffer in RAM to hold the bitmap data (needs to be large enough for the widest glyph)
      uint8_t glyph_buffer[BYTES_PER_BITMAP];
      // Copy the bitmap data from PROGMEM (flash) to RAM (glyph_buffer)
      memcpy_P(glyph_buffer, glyph_bitmaps + bitmap_start, BYTES_PER_BITMAP);

      // Draw the bitmap on the display
      // Use glyph_width_byte_aligned for drawBitmap as it's the actual pixel width of the stored bitmap.
      display.drawBitmap(x, y, glyph_buffer, glyph_width_byte_aligned, GLYPH_HEIGHT, SSD1306_WHITE);
      //Advance by unpadded width to make the characters appear tightly adjacent
      x += glyph_width_unpadded;
    }

    display.display();   // Update the OLED with the new frame
    delay(5);            // Pause for a short duration to control scroll speed
    scroll_offset++;     // Move the phrase 1 pixel to the left for the next frame
  }
}

// Function to display a static phrase
void staticPhrase(const uint8_t *lao_phrase, uint8_t len_phrase)
{
  display.clearDisplay(); // Clear the display for the phrase
  int x = 0;
  int y = (SCREEN_HEIGHT - GLYPH_HEIGHT) / 2;

  for (int i = 0; i < len_phrase; i++)
  {
    int glyph_index = pgm_read_byte(lao_phrase + i);
    int bitmap_start = pgm_read_word(&bitmap_starts[glyph_index]);
    int glyph_width_byte_aligned = pgm_read_word(&glyph_widths[glyph_index]);
    int glyph_width_unpadded = pgm_read_word(&unpadded_widths[glyph_index]);

    // Calculate bytes per row and total bytes for this specific bitmap
    int BYTES_PER_ROW = (glyph_width_byte_aligned + 7) / 8;
    int BYTES_PER_BITMAP = BYTES_PER_ROW * GLYPH_HEIGHT;

     // Create a buffer in RAM to hold the bitmap data (needs to be large enough for the widest glyph)
    uint8_t glyph_buffer[BYTES_PER_BITMAP];

    // Copy the bitmap data from PROGMEM (flash) to RAM (glyph_buffer)
    memcpy_P(glyph_buffer, glyph_bitmaps + bitmap_start, BYTES_PER_BITMAP);

    // Draw the bitmap on the display
    // Use glyph_width_byte_aligned for drawBitmap as it's the actual pixel width of the stored bitmap.
    display.drawBitmap(x, y, glyph_buffer, glyph_width_byte_aligned, GLYPH_HEIGHT, SSD1306_WHITE);

    //Advance by unpadded width to make the characters appear tightly adjacent
    x += glyph_width_unpadded;
  }

  display.display(); // Update the OLED with the new frame
}

// Function to retrieve the chosen phrase from memory and display it
void displayPhraseByIndex(int input)
{
  if (input >= 1 && input <= num_phrases)
  {
    uint8_t start = pgm_read_byte(&phrase_starts[input - 1]);           // Get the phrase start in array
    uint8_t len_phrase = pgm_read_byte(&phrase_lengths[input - 1]);     // Get the length of the chosen phrase
    const uint8_t *lao_phrase = all_phrases + start;                    // Get the chosen phrase data
    scrollPhrase(lao_phrase, len_phrase);                               // Static phrase function also available (uncomment out to test)
    // staticPhrase(lao_phrase, len_phrase);
  }
}

// USE A DIFFERENT FUNCTION DEPENDING ON PROJECT SPECIFICS
// Call displayPhraseByIndex(input) in your function
void checkSerialInput()
{
  if (Serial.available())
  {
    int input = Serial.parseInt(); // Reads the first int from Serial buffer
    while (Serial.available()) Serial.read(); // Clear out any leftover characters

    if (input >= 1 && input <= num_phrases)
    {
      Serial.print("Displaying phrase ");
      Serial.println(input);
      displayPhraseByIndex(input);
    }
    else
    {
      Serial.println("Invalid index");
    }
  }
}

// Arduino setup function (runs once)
void setup()
{
  Serial.begin(9600);                                               // Start serial communication at 9600 baud
  Wire.begin();                                                     // Initialize I2C communication
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);                        // Initialize OLED display (0X3C is common I2C address)
  display.clearDisplay();                                           // Clear display buffer
  display.display();                                                // Push buffer to display (shows a blank screen)
  delay(2000);                                                      // Wait for 2 seconds
  Serial.println("Enter the number corresponding to the phrase you'd like to display");
}

// Arduino loop function (runs repeatedly)
void loop()
{
//checks whether there is a keyboard input and displays the associated phrase
//replace with linking code relevant to project specifications
  checkSerialInput();
}

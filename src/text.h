/* An ascii bitmap library.
 text_bits holds bitmap information for each character, packed into a byte array.
 This means for each pixel in a 13x7 picture of a character, 1 bit is stored in text_bits saying whether that pixel should be on or off.

 text.c is actually an xbm file (see http://en.wikipedia.org/wiki/X_BitMap)
*/
#define char_width 7
#define char_height 13
// 96 visible characters * 7 bits wide each
#define text_bits_width 672

#define offset(c, i, j) ((i) * text_bits_width + ((c) - 32) * char_width + (j))
#define byteNum(c, i, j) (offset(c, i, j) / 8)
#define bitNum(c, i, j) (offset(c, i, j) % 8)

// returns true if a 13x7 picture of character c has a pixel set at i, j
#define charHasPixelSet(c, i, j) (text_bits[byteNum(c, i, j)] & (1 << bitNum(c - 32, i, j)))

extern const uint8_t text_bits[];

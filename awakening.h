#pragma once

// Fonts are an array of unsigned char.
// Every glyph is a pair of an UTF-8 string and the glyph bitmap.
// They are encoded by a one byte size, the string, another one byte size and the bitmap data.
// The most significant bit of the bitmap size encodes if the glyph is 16 pixels high. If this
// bit is zero, the glyph is 8 pixels high.
//
// The bitmap data is one byte per column for 8 pixel high glyphs and two bytes per column
// for 16 pixel high glyphs. The least significant bit represents the topmost row.
//
// A font looks like this:
// +-------+-------+-----+---+
// | Glyph | Glyph | ... | 0 |
// +-------+-------+-----+---+
//
// A glyph looks like this:
// +-----------+------+-------------+--------+
// | Text Size | Text | Bitmap Size | Bitmap |
// +-----------+------+-------------+--------+
//    1 Byte    m Bytes   1 Byte      n Bytes
//
// In addition to that a font can have an index. The index is an array of 64 signed integers
// which represent the offset into the font data where one should begin searching for a glyph.
// To calculate the index in the array, the first byte of the string is shifted to the right by two.

class Awakening {
    const unsigned char *glyphs;
    const short *index;
    static int TEXT_LENGTH(const unsigned char *glyph) {
      return glyph[0];
    }
    static const unsigned char *TEXT(const unsigned char *glyph) {
      return glyph + 1;
    }
    static int GLYPH_LENGTH(const unsigned char *glyph) {
      return glyph[TEXT_LENGTH(glyph) + 1] & 0x7f;
    }
    static bool GLYPH_FULLSIZE(const unsigned char *glyph) {
      return glyph[TEXT_LENGTH(glyph) + 1] & 0x80;
    }
    static const unsigned char *GLYPH(const unsigned char *glyph) {
      return glyph + TEXT_LENGTH(glyph) + 2;
    }
    static unsigned GLYPH_COL(const unsigned char *glyph, int i) {
      return GLYPH_FULLSIZE(glyph) ?
        (((unsigned) GLYPH(glyph)[2 * i + 1] << 8) | GLYPH(glyph)[2 * i]) :
        ((unsigned) GLYPH(glyph)[i] << 4);
    }
    static int GLYPH_COLS(const unsigned char *glyph) {
      return GLYPH_LENGTH(glyph) >> GLYPH_FULLSIZE(glyph);
    }
    static int LENGTH(const unsigned char *glyph) {
      return TEXT_LENGTH(glyph) + GLYPH_LENGTH(glyph) + 2;
    }
    // Lookup a glyph by text.
    const unsigned char *lookup_glyph(const unsigned char *text) {
      int i, l;
      const unsigned char *glyph;
      int o;
      if (!text || !*text) return nullptr;
      o = index[*text >> 2];
      if (o < 0) {
        return 0;
      } else {
        glyph = glyphs + o;
      }
      while ((l = *glyph)) {
        if (TEXT(glyph)[0] > text[0]) return nullptr;
        for (i = 0; i < l; ++i) {
          if (TEXT(glyph)[i] != text[i]) goto next;
        }
        return glyph;
    next:
        glyph += LENGTH(glyph);
      }
      return nullptr;
    }
    int _min(int a, int b) {
      return a > b ? b : a;
    }
    int _max(int a, int b) {
      return a < b ? b : a;
    }
    static constexpr int SPACE_WIDTH = 3;
    static constexpr int TNUM_WIDTH = 5;
  public:
    // Left aligned text. This is the default.
    static constexpr int LEFT = 0;
    // Right aligned text.
    static constexpr int RIGHT = 1;
    // Centered text.
    static constexpr int CENTER = 2;
    // Fixed width numbers.
    static constexpr int TNUM = 4;
    // Do not actually render the text, just return its width.
    static constexpr int TEXT_WIDTH = 8;
    // Typeset a line of text at position `x`, `y` with a _maximum width of `w`.
    // Return the actual width used.
    template<class Display>
    int text(Display &d, const char *text, int x, int y, int w) {
      return text_with_options(d, text, x, y, w, LEFT);
    }
    // Typeset a line of text centered between x and x+w.
    template<class Display>
    int text_centered(Display &d, const char *text, int x, int y, int w) {
      return text_with_options(d, text, x, y, w, CENTER);
    }
    // Typeset a line of text with the given set of options.
    template<class Display>
    int text_with_options(Display &d, const char *text, int x, int y, int w, int options) {
      const unsigned char *glyph, *g;
      unsigned char esc[] = {27, 0, 0};
      int i, n = 0, r, s, q = 0, is_num, was_num = 0;
      unsigned b = 0, t = 0;
      if (options & TEXT_WIDTH) {
        q = 1;
        options &= ~(RIGHT + CENTER);
      }

      // Handle things recursively in the alignment cases.
      if (options & CENTER) {
        i = text_with_options(d, text, x, y, w, options | TEXT_WIDTH);
        options &= ~(RIGHT + CENTER);
        return text_with_options(d, text, x + (w - i) / 2, y, i, options);
      }
      if (options & RIGHT) {
        i = text_with_options(d, text, x, y, w, options | TEXT_WIDTH);
        options &= ~(RIGHT + CENTER);
        return text_with_options(d, text, x + w - i, y, i, options);
      }

      while (*text) {
        if (*text == ' ') {
          ++text;
          if (w > 0 && n + SPACE_WIDTH >= w) return n;
          n += SPACE_WIDTH;
          t = 0;
        } else if (*text == '\b') {
          ++text;
          t = 0;
        } else if ((glyph = lookup_glyph((const unsigned char *) text))) {
          is_num = ('0' <= *text && *text <= '9');
          if ((options & TNUM) && is_num) {
            esc[1] = *text;
          }
          text += TEXT_LENGTH(glyph);
          if ((options & TNUM) && is_num) {
            g = lookup_glyph(esc);
            if (g) glyph = g;
          }
          if (n) {
            if ((options & TNUM) && (is_num || was_num)) {
              if (w > 0 && n + (is_num ? TNUM_WIDTH : GLYPH_COLS(glyph)) + 1 > w) return n;
              n += 1;
            } else {
              // Kerning
              b = GLYPH_COL(glyph, 0);
              if ((b | (b << 1)) & (t | (t << 1))) {
                if (w > 0 && n + GLYPH_COLS(glyph) + 1 > w) return n;
                n += 1;
              } else if (w > 0 && n + GLYPH_COLS(glyph) > w) {
                return n;
              }
            }
          }
          if ((options & TNUM) && is_num) {
            n += (TNUM_WIDTH - GLYPH_COLS(glyph)) / 2;
          }
          for (i = 0; i < GLYPH_COLS(glyph); ++i) {
            b = GLYPH_COL(glyph, i);
            if (!q && x + n >= 0 && x + n < d.width()) {
              unsigned bit = 1;
              for (r = y - 4; r <= y + 12; ++r) {
                if (b & bit) d.pixel(x + n, r);
                bit <<= 1;
              }
            }
            n += 1;
          }
          t = b;
          if ((options & TNUM) && is_num) {
            n += (TNUM_WIDTH + 1 - GLYPH_COLS(glyph)) / 2;
          }
          was_num = is_num;
        } else {
          ++text;
        }
      }
      return n;
    }
    Awakening() {
      static const unsigned char awakening[] = {
        /* "\t" */ 1, 9, 8, 0, 0, 0, 0, 0, 0, 0, 0,
        /* "\e1" */ 2, 27, 49, 3, 34, 63, 32,
        /* "\ed" */ 2, 27, 100, 5, 16, 48, 112, 48, 16,
        /* "\el" */ 2, 27, 108, 3, 8, 28, 62,
        /* "\er" */ 2, 27, 114, 3, 62, 28, 8,
        /* "\eud" */ 3, 27, 117, 100, 5, 20, 54, 119, 54, 20,
        /* "\eu" */ 2, 27, 117, 5, 4, 6, 7, 6, 4,
        /* "!" */ 1, 33, 1, 46,
        /* "\"" */ 1, 34, 3, 3, 0, 3,
        /* "#" */ 1, 35, 5, 20, 62, 20, 62, 20,
        /* "$" */ 1, 36, 5, 36, 42, 107, 42, 18,
        /* "%" */ 1, 37, 5, 34, 16, 8, 4, 34,
        /* "&" */ 1, 38, 5, 20, 42, 42, 20, 40,
        /* "'" */ 1, 39, 1, 3,
        /* "(" */ 1, 40, 2, 62, 65,
        /* ")" */ 1, 41, 2, 65, 62,
        /* "+" */ 1, 43, 5, 8, 8, 62, 8, 8,
        /* "," */ 1, 44, 2, 64, 32,
        /* "-" */ 1, 45, 4, 8, 8, 8, 8,
        /* "." */ 1, 46, 1, 32,
        /* "/" */ 1, 47, 5, 32, 16, 8, 4, 2,
        /* "0" */ 1, 48, 5, 30, 33, 33, 33, 30,
        /* "1" */ 1, 49, 3, 34, 63, 32,
        /* "2" */ 1, 50, 5, 34, 49, 41, 41, 38,
        /* "3" */ 1, 51, 5, 18, 33, 33, 37, 26,
        /* "4" */ 1, 52, 5, 24, 20, 18, 63, 16,
        /* "5" */ 1, 53, 5, 23, 37, 37, 37, 25,
        /* "6" */ 1, 54, 5, 28, 38, 37, 37, 24,
        /* "7" */ 1, 55, 5, 1, 1, 57, 5, 3,
        /* "8" */ 1, 56, 5, 26, 37, 37, 37, 26,
        /* "9" */ 1, 57, 5, 6, 73, 41, 25, 14,
        /* ":" */ 1, 58, 1, 20,
        /* ";" */ 1, 59, 2, 64, 40,
        /* "<" */ 1, 60, 3, 8, 20, 34,
        /* "=" */ 1, 61, 3, 20, 20, 20,
        /* ">" */ 1, 62, 3, 34, 20, 8,
        /* "?" */ 1, 63, 5, 4, 2, 42, 10, 4,
        /* "@" */ 1, 64, 7, 62, 65, 93, 85, 93, 81, 14,
        /* "A" */ 1, 65, 5, 62, 9, 9, 9, 62,
        /* "B" */ 1, 66, 5, 63, 37, 37, 37, 26,
        /* "C" */ 1, 67, 5, 30, 33, 33, 33, 34,
        /* "D" */ 1, 68, 5, 63, 33, 33, 34, 28,
        /* "E" */ 1, 69, 5, 63, 37, 37, 37, 33,
        /* "F" */ 1, 70, 5, 63, 5, 5, 5, 1,
        /* "G" */ 1, 71, 5, 28, 34, 33, 41, 58,
        /* "H" */ 1, 72, 5, 63, 4, 4, 4, 63,
        /* "I" */ 1, 73, 3, 33, 63, 33,
        /* "J" */ 1, 74, 5, 16, 32, 32, 33, 31,
        /* "K" */ 1, 75, 5, 63, 0, 12, 18, 33,
        /* "L" */ 1, 76, 5, 63, 32, 32, 32, 32,
        /* "M" */ 1, 77, 5, 63, 2, 4, 2, 63,
        /* "N" */ 1, 78, 5, 63, 6, 12, 24, 63,
        /* "O" */ 1, 79, 5, 30, 33, 33, 33, 30,
        /* "P" */ 1, 80, 5, 63, 9, 9, 9, 6,
        /* "Q" */ 1, 81, 5, 30, 33, 41, 17, 46,
        /* "R" */ 1, 82, 5, 63, 5, 13, 21, 34,
        /* "S" */ 1, 83, 5, 18, 37, 37, 37, 24,
        /* "T" */ 1, 84, 5, 1, 1, 63, 1, 1,
        /* "U" */ 1, 85, 5, 31, 32, 32, 32, 31,
        /* "V" */ 1, 86, 5, 15, 16, 32, 16, 15,
        /* "W" */ 1, 87, 5, 63, 16, 8, 16, 63,
        /* "X" */ 1, 88, 5, 33, 18, 12, 18, 33,
        /* "Y" */ 1, 89, 5, 3, 4, 56, 4, 3,
        /* "Z" */ 1, 90, 5, 33, 49, 45, 35, 33,
        /* "[" */ 1, 91, 2, 127, 65,
        /* "\\" */ 1, 92, 5, 2, 4, 8, 16, 32,
        /* "]" */ 1, 93, 2, 65, 127,
        /* "_" */ 1, 95, 4, 32, 32, 32, 32,
        /* "a" */ 1, 97, 5, 16, 42, 42, 42, 60,
        /* "b" */ 1, 98, 5, 63, 36, 34, 34, 28,
        /* "c" */ 1, 99, 5, 28, 34, 34, 34, 36,
        /* "d" */ 1, 100, 5, 28, 34, 34, 36, 63,
        /* "e" */ 1, 101, 5, 28, 42, 42, 42, 12,
        /* "ff" */ 2, 102, 102, 7, 8, 126, 9, 9, 126, 9, 1,
        /* "f" */ 1, 102, 4, 8, 126, 9, 1,
        /* "g" */ 1, 103, 5, 24, 164, 162, 146, 126,
        /* "h" */ 1, 104, 5, 63, 4, 2, 2, 60,
        /* "i" */ 1, 105, 2, 4, 61,
        /* "j" */ 1, 106, 3, 128, 132, 125,
        /* "k" */ 1, 107, 4, 63, 8, 20, 34,
        /* "l" */ 1, 108, 2, 1, 63,
        /* "m" */ 1, 109, 5, 62, 4, 8, 4, 62,
        /* "n" */ 1, 110, 5, 62, 2, 2, 4, 56,
        /* "o" */ 1, 111, 5, 28, 34, 34, 34, 28,
        /* "p" */ 1, 112, 5, 254, 34, 34, 36, 24,
        /* "q" */ 1, 113, 5, 24, 36, 34, 34, 254,
        /* "r" */ 1, 114, 4, 62, 4, 2, 2,
        /* "s" */ 1, 115, 5, 36, 42, 42, 42, 16,
        /* "t" */ 1, 116, 4, 2, 31, 34, 32,
        /* "u" */ 1, 117, 5, 14, 16, 32, 32, 62,
        /* "v" */ 1, 118, 5, 14, 16, 32, 16, 14,
        /* "w" */ 1, 119, 5, 62, 16, 8, 16, 62,
        /* "x" */ 1, 120, 5, 34, 20, 8, 20, 34,
        /* "y" */ 1, 121, 5, 142, 144, 144, 72, 62,
        /* "z" */ 1, 122, 5, 34, 50, 42, 38, 34,
        /* "{" */ 1, 123, 3, 8, 54, 65,
        /* "|" */ 1, 124, 1, 127,
        /* "}" */ 1, 125, 3, 65, 54, 8,
        /* "¡" */ 2, 194, 161, 1, 116,
        /* "©" */ 2, 194, 169, 8, 30, 33, 12, 18, 18, 20, 33, 30,
        /* "«" */ 2, 194, 171, 4, 8, 20, 8, 20,
        /* "°" */ 2, 194, 176, 3, 7, 5, 7,
        /* "µ" */ 2, 194, 181, 5, 254, 16, 32, 32, 62,
        /* "»" */ 2, 194, 187, 4, 20, 8, 20, 8,
        /* "¿" */ 2, 194, 191, 5, 32, 80, 84, 64, 32,
        /* "À" */ 2, 195, 128, 138, 224, 3, 146, 0, 148, 0, 144, 0, 224, 3,
        /* "Á" */ 2, 195, 129, 138, 224, 3, 144, 0, 148, 0, 146, 0, 224, 3,
        /* "Â" */ 2, 195, 130, 138, 224, 3, 148, 0, 146, 0, 148, 0, 224, 3,
        /* "Ä" */ 2, 195, 132, 138, 224, 3, 148, 0, 144, 0, 148, 0, 224, 3,
        /* "Æ" */ 2, 195, 134, 9, 62, 9, 9, 9, 63, 37, 37, 37, 33,
        /* "Ç" */ 2, 195, 135, 5, 30, 161, 225, 33, 18,
        /* "È" */ 2, 195, 136, 138, 240, 3, 82, 2, 84, 2, 80, 2, 16, 2,
        /* "É" */ 2, 195, 137, 138, 240, 3, 80, 2, 84, 2, 82, 2, 16, 2,
        /* "Ê" */ 2, 195, 138, 138, 240, 3, 84, 2, 82, 2, 84, 2, 16, 2,
        /* "Ë" */ 2, 195, 139, 138, 240, 3, 84, 2, 80, 2, 84, 2, 16, 2,
        /* "Í" */ 2, 195, 141, 134, 16, 2, 244, 3, 18, 2,
        /* "Î" */ 2, 195, 142, 134, 20, 2, 242, 3, 20, 2,
        /* "Ï" */ 2, 195, 143, 134, 20, 2, 240, 3, 20, 2,
        /* "Ð" */ 2, 195, 144, 6, 8, 63, 41, 33, 34, 28,
        /* "Ñ" */ 2, 195, 145, 138, 240, 3, 100, 0, 194, 0, 132, 1, 242, 3,
        /* "Ó" */ 2, 195, 147, 138, 224, 1, 16, 2, 20, 2, 18, 2, 224, 1,
        /* "Ô" */ 2, 195, 148, 138, 224, 1, 20, 2, 18, 2, 20, 2, 224, 1,
        /* "Ö" */ 2, 195, 150, 138, 224, 1, 20, 2, 16, 2, 20, 2, 224, 1,
        /* "Ù" */ 2, 195, 153, 138, 240, 1, 4, 2, 8, 2, 0, 2, 240, 1,
        /* "Ú" */ 2, 195, 154, 138, 240, 1, 0, 2, 8, 2, 4, 2, 240, 1,
        /* "Û" */ 2, 195, 155, 138, 240, 1, 4, 2, 2, 2, 4, 2, 240, 1,
        /* "Ü" */ 2, 195, 156, 138, 240, 1, 4, 2, 0, 2, 4, 2, 240, 1,
        /* "Ý" */ 2, 195, 157, 138, 48, 0, 64, 0, 136, 3, 68, 0, 48, 0,
        /* "Þ" */ 2, 195, 158, 4, 63, 18, 18, 12,
        /* "ß" */ 2, 195, 159, 4, 62, 1, 41, 22,
        /* "à" */ 2, 195, 160, 138, 0, 1, 164, 2, 168, 2, 160, 2, 192, 3,
        /* "á" */ 2, 195, 161, 138, 0, 1, 160, 2, 168, 2, 164, 2, 192, 3,
        /* "â" */ 2, 195, 162, 138, 0, 1, 168, 2, 164, 2, 168, 2, 192, 3,
        /* "ä" */ 2, 195, 164, 138, 0, 1, 168, 2, 160, 2, 168, 2, 192, 3,
        /* "æ" */ 2, 195, 166, 9, 16, 42, 42, 42, 60, 42, 42, 42, 12,
        /* "ç" */ 2, 195, 167, 5, 28, 162, 226, 34, 36,
        /* "è" */ 2, 195, 168, 138, 192, 1, 164, 2, 168, 2, 160, 2, 192, 0,
        /* "é" */ 2, 195, 169, 138, 192, 1, 160, 2, 168, 2, 164, 2, 192, 0,
        /* "ê" */ 2, 195, 170, 138, 192, 1, 168, 2, 164, 2, 168, 2, 192, 0,
        /* "ë" */ 2, 195, 171, 138, 192, 1, 168, 2, 160, 2, 168, 2, 192, 0,
        /* "í" */ 2, 195, 173, 132, 80, 0, 200, 3,
        /* "î" */ 2, 195, 174, 134, 80, 0, 200, 3, 16, 0,
        /* "ï" */ 2, 195, 175, 3, 5, 60, 1,
        /* "ð" */ 2, 195, 176, 138, 128, 1, 72, 2, 104, 2, 80, 2, 232, 1,
        /* "ñ" */ 2, 195, 177, 138, 224, 3, 40, 0, 36, 0, 72, 0, 132, 3,
        /* "ó" */ 2, 195, 179, 138, 192, 1, 32, 2, 40, 2, 36, 2, 192, 1,
        /* "ô" */ 2, 195, 180, 138, 192, 1, 40, 2, 36, 2, 40, 2, 192, 1,
        /* "ö" */ 2, 195, 182, 138, 192, 1, 40, 2, 32, 2, 40, 2, 192, 1,
        /* "ù" */ 2, 195, 185, 138, 224, 0, 8, 1, 16, 2, 0, 2, 224, 3,
        /* "ú" */ 2, 195, 186, 138, 224, 0, 0, 1, 16, 2, 8, 2, 224, 3,
        /* "û" */ 2, 195, 187, 138, 224, 0, 8, 1, 4, 2, 8, 2, 224, 3,
        /* "ü" */ 2, 195, 188, 138, 224, 0, 8, 1, 0, 2, 8, 2, 224, 3,
        /* "ý" */ 2, 195, 189, 138, 224, 8, 0, 9, 16, 9, 136, 4, 224, 3,
        /* "þ" */ 2, 195, 190, 5, 255, 36, 34, 34, 28,
        /* "Œ" */ 2, 197, 146, 9, 30, 33, 33, 33, 63, 37, 37, 37, 33,
        /* "œ" */ 2, 197, 147, 9, 28, 34, 34, 34, 28, 42, 42, 42, 12,
        /* "Ω" */ 2, 206, 169, 5, 46, 49, 1, 49, 46,
        /* "‘" */ 3, 226, 128, 152, 2, 6, 5,
        /* "’" */ 3, 226, 128, 153, 2, 5, 3,
        /* "‚" */ 3, 226, 128, 154, 2, 80, 48,
        /* "“" */ 3, 226, 128, 156, 5, 6, 5, 0, 6, 5,
        /* "”" */ 3, 226, 128, 157, 5, 5, 3, 0, 5, 3,
        /* "„" */ 3, 226, 128, 158, 5, 80, 48, 0, 80, 48,
        /* "…" */ 3, 226, 128, 166, 5, 32, 0, 32, 0, 32,
        0
      };
      static const short awakening_index[] = {
        -1, -1, 0, -1, -1, -1, 11, -1,
        60, 78, 106, 124, 148, 178, 210, 235,
        261, 295, 327, 357, 389, 421, 453, 482,
        502, 526, 568, 594, 623, 654, 685, 715,
        -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1,
        725, 1407, -1, 1433, -1, -1, -1, -1,
        1442, -1, -1, -1, -1, -1, -1, -1,
      };
      glyphs = awakening;
      index = awakening_index;
    }
};
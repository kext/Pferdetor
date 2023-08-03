#pragma once

template<unsigned WIDTH, unsigned HEIGHT>
class Display {
    unsigned char buffer[WIDTH * HEIGHT / 8];
  public:
    unsigned width() const {
      return WIDTH;
    }
    unsigned height() const {
      return HEIGHT;
    }
    const unsigned char *data() const {
      return buffer;
    }
    // Invert everything in the area between x1, y1 and x2, y2.
    void invert(int x1, int y1, int x2, int y2) {
      int t, r, c;
      unsigned char m;
      if (x2 < x1) {
        t = x2; x2 = x1; x1 = t;
      }
      if (y2 < y1) {
        t = y2; y2 = y1; y1 = t;
      }
      x1 = _min(_max(x1, 0), WIDTH);
      y1 = _min(_max(y1, 0), HEIGHT);
      x2 = _min(_max(x2, 0), WIDTH);
      y2 = _min(_max(y2, 0), HEIGHT);
      for (r = y1 >> 3; r <= y2 >> 3; ++r) {
        m = 0xff;
        if (r == y1 >> 3) m &= 0xff << (y1 & 7);
        if (r == y2 >> 3) m &= 0xff >> (8 - y2 & 7);
        if (!m) return 0;
        for (c = x1; c < x2; ++c) {
          buffer[r * WIDTH + c] ^= m;
        }
      }
    }
    // Paint everything in the area between x1, y1 and x2, y2.
    // Paint black if v == 0, white otherwise.
    void fill_rect(int x1, int y1, int x2, int y2, int v) {
      int t, r, c;
      unsigned char m;
      if (x2 < x1) {
        t = x2; x2 = x1; x1 = t;
      }
      if (y2 < y1) {
        t = y2; y2 = y1; y1 = t;
      }
      x1 = _min(_max(x1, 0), WIDTH);
      y1 = _min(_max(y1, 0), HEIGHT);
      x2 = _min(_max(x2, 0), WIDTH);
      y2 = _min(_max(y2, 0), HEIGHT);
      for (r = y1 >> 3; r <= y2 >> 3; ++r) {
        m = 0xff;
        if (r == y1 >> 3) m &= 0xff << (y1 & 7);
        if (r == y2 >> 3) m &= 0xff >> (8 - y2 & 7);
        if (!m) return 0;
        for (c = x1; c < x2; ++c) {
          if (v) {
            buffer[r * WIDTH + c] |= m;
          } else {
            buffer[r * WIDTH + c] &= ~m;
          }
        }
      }
    }
    // Set the pixel at position x, y to value v (0 or 1).
    void pixel(int x, int y, bool v = true) {
      if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) return;
      if (v) {
        buffer[x + (y >> 3) * WIDTH] |= 1 << (y & 7);
      } else {
        buffer[x + (y >> 3) * WIDTH] &= ~(1 << (y & 7));
      }
    }
    // Draw a straight line from x1, y1 to x2, y2.
    void line(int x1, int y1, int x2, int y2) {
      int dx = _abs(x2 - x1), sx = x1 < x2 ? 1 : -1;
      int dy = _abs(y2 - y1), sy = y1 < y2 ? 1 : -1;
      int err = (dx > dy ? dx : -dy) / 2, e2;
      while (true) {
        pixel(x1, y1, 1);
        if (x1 == x2 && y1 == y2) break;
        e2 = err;
        if (e2 >-dx) { err -= dy; x1 += sx; }
        if (e2 < dy) { err += dx; y1 += sy; }
      }
    }
    // Makes all pixels black if v == 0, white otherwise.
    void clear(bool v = false) {
      memset(buffer, v ? 0xff : 0x00, sizeof(buffer));
    }
  private:
    int _max(int a, int b) {
      return a < b ? b : a;
    }
    int _min(int a, int b) {
      return a > b ? b : a;
    }
    int _abs(int a) {
      return a < 0 ? -a : a;
    }
};
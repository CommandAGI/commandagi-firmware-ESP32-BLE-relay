#include "hidmap.h"

namespace {
constexpr uint8_t MOD_CTRL = 0x01, MOD_SHIFT = 0x02, MOD_ALT = 0x04, MOD_GUI = 0x08;

// Base HID keycodes.
constexpr uint8_t K_A = 0x04, K_1 = 0x1E, K_0 = 0x27, K_ENTER = 0x28, K_ESC = 0x29, K_BSP = 0x2A,
                  K_TAB = 0x2B, K_SPACE = 0x2C, K_MINUS = 0x2D, K_EQUAL = 0x2E, K_LBRACKET = 0x2F,
                  K_RBRACKET = 0x30, K_BACKSLASH = 0x31, K_SEMI = 0x33, K_QUOTE = 0x34, K_GRAVE = 0x35,
                  K_COMMA = 0x36, K_DOT = 0x37, K_SLASH = 0x38, K_F1 = 0x3A, K_HOME = 0x4A, K_PGUP = 0x4B,
                  K_DEL = 0x4C, K_END = 0x4D, K_PGDN = 0x4E, K_RIGHT = 0x4F, K_LEFT = 0x50, K_DOWN = 0x51,
                  K_UP = 0x52, K_INS = 0x49;

// A shifted-symbol table: input char → the UNSHIFTED base key (SHIFT is applied by the caller).
bool shiftedSymbol(uint8_t c, uint8_t& key) {
  switch (c) {
    case '!': key = K_1; return true;
    case '@': key = K_1 + 1; return true;
    case '#': key = K_1 + 2; return true;
    case '$': key = K_1 + 3; return true;
    case '%': key = K_1 + 4; return true;
    case '^': key = K_1 + 5; return true;
    case '&': key = K_1 + 6; return true;
    case '*': key = K_1 + 7; return true;
    case '(': key = K_1 + 8; return true;
    case ')': key = K_0; return true;
    case '_': key = K_MINUS; return true;
    case '+': key = K_EQUAL; return true;
    case '{': key = K_LBRACKET; return true;
    case '}': key = K_RBRACKET; return true;
    case '|': key = K_BACKSLASH; return true;
    case ':': key = K_SEMI; return true;
    case '"': key = K_QUOTE; return true;
    case '~': key = K_GRAVE; return true;
    case '<': key = K_COMMA; return true;
    case '>': key = K_DOT; return true;
    case '?': key = K_SLASH; return true;
    default: return false;
  }
}

bool unshiftedSymbol(uint8_t c, uint8_t& key) {
  switch (c) {
    case '-': key = K_MINUS; return true;
    case '=': key = K_EQUAL; return true;
    case '[': key = K_LBRACKET; return true;
    case ']': key = K_RBRACKET; return true;
    case '\\': key = K_BACKSLASH; return true;
    case ';': key = K_SEMI; return true;
    case '\'': key = K_QUOTE; return true;
    case '`': key = K_GRAVE; return true;
    case ',': key = K_COMMA; return true;
    case '.': key = K_DOT; return true;
    case '/': key = K_SLASH; return true;
    default: return false;
  }
}

// A named non-printable key → keycode (no modifier). Returns 0 for unknown.
uint8_t namedKey(const String& n) {
  String k = n;
  k.toLowerCase();
  if (k == "return" || k == "enter") return K_ENTER;
  if (k == "escape" || k == "esc") return K_ESC;
  if (k == "backspace" || k == "bksp") return K_BSP;
  if (k == "tab") return K_TAB;
  if (k == "space") return K_SPACE;
  if (k == "delete" || k == "del") return K_DEL;
  if (k == "insert" || k == "ins") return K_INS;
  if (k == "home") return K_HOME;
  if (k == "end") return K_END;
  if (k == "pageup" || k == "prior") return K_PGUP;
  if (k == "pagedown" || k == "next") return K_PGDN;
  if (k == "right") return K_RIGHT;
  if (k == "left") return K_LEFT;
  if (k == "up") return K_UP;
  if (k == "down") return K_DOWN;
  if (k.length() >= 2 && k[0] == 'f') {
    int n = k.substring(1).toInt();
    if (n >= 1 && n <= 12) return K_F1 + (n - 1);
  }
  return 0;
}

// A modifier token → its bit, or 0 if the token isn't a modifier.
uint8_t modifierBit(const String& t) {
  String k = t;
  k.toLowerCase();
  if (k == "ctrl" || k == "control") return MOD_CTRL;
  if (k == "shift") return MOD_SHIFT;
  if (k == "alt" || k == "option" || k == "opt") return MOD_ALT;
  if (k == "cmd" || k == "command" || k == "super" || k == "meta" || k == "win" || k == "gui") return MOD_GUI;
  return 0;
}
}  // namespace

namespace HidMap {

bool charToKey(uint8_t c, uint8_t& mod, uint8_t& key) {
  mod = 0;
  key = 0;
  if (c >= 'a' && c <= 'z') { key = K_A + (c - 'a'); return true; }
  if (c >= 'A' && c <= 'Z') { key = K_A + (c - 'A'); mod = MOD_SHIFT; return true; }
  if (c >= '1' && c <= '9') { key = K_1 + (c - '1'); return true; }
  if (c == '0') { key = K_0; return true; }
  if (c == ' ') { key = K_SPACE; return true; }
  if (c == '\n' || c == '\r') { key = K_ENTER; return true; }
  if (c == '\t') { key = K_TAB; return true; }
  if (unshiftedSymbol(c, key)) return true;
  if (shiftedSymbol(c, key)) { mod = MOD_SHIFT; return true; }
  return false;
}

bool nameToKey(const String& name, uint8_t& mod, uint8_t& key) {
  mod = 0;
  key = 0;
  // Split on '+' — all but the last token are modifiers; the last is the key.
  int start = 0;
  String last = name;
  while (true) {
    int plus = name.indexOf('+', start);
    if (plus < 0) { last = name.substring(start); break; }
    String tok = name.substring(start, plus);
    uint8_t bit = modifierBit(tok);
    if (bit) mod |= bit; else last = tok;  // a non-modifier before a '+' is unusual; treat as the key
    start = plus + 1;
  }
  last.trim();
  if (last.length() == 0) return mod != 0;  // pure modifier chord (rare)
  uint8_t nk = namedKey(last);
  if (nk) { key = nk; return true; }
  if (last.length() == 1) {
    uint8_t cmod = 0;
    if (charToKey((uint8_t)last[0], cmod, key)) { mod |= cmod; return true; }
  }
  return false;
}

}  // namespace HidMap

/*! \file KeyCode.hpp
    \author Kyn21kx
    \date 2024-02-29
    \brief Enum to identify the key codes (taken from SDL's SDL_ScanCode enum)
*/

#pragma once
/// <summary>
/// Alias for the SDL type coming into Hush
/// </summary>
using KeyCode = int;

enum class EKeyCode : KeyCode
{
    UNKNOWN = 0,

    A = 4,
    B = 5,
    C = 6,
    D = 7,
    E = 8,
    F = 9,
    G = 10,
    H = 11,
    I = 12,
    J = 13,
    K = 14,
    L = 15,
    M = 16,
    N = 17,
    O = 18,
    P = 19,
    Q = 20,
    R = 21,
    S = 22,
    T = 23,
    U = 24,
    V = 25,
    W = 26,
    X = 27,
    Y = 28,
    Z = 29,

    Num1 = 30,
    Num2 = 31,
    Num3 = 32,
    Num4 = 33,
    Num5 = 34,
    Num6 = 35,
    Num7 = 36,
    Num8 = 37,
    Num9 = 38,
    Num0 = 39,

    RETURN = 40,
    ESCAPE = 41,
    BACKSPACE = 42,
    TAB = 43,
    SPACE = 44,

    MINUS = 45,
    EQUALS = 46,
    LEFTBRACKET = 47,
    RIGHTBRACKET = 48,
    BACKSLASH = 49, /**< Located at the lower left of the return
                     *   key on ISO keyboards and at the right end
                     *   of the QWERTY row on ANSI keyboards.
                     *   Produces REVERSE SOLIDUS (backslash) and
                     *   VERTICAL LINE in a US layout, REVERSE
                     *   SOLIDUS and VERTICAL LINE in a UK Mac
                     *   layout, NUMBER SIGN and TILDE in a UK
                     *   Windows layout, DOLLAR SIGN and POUND SIGN
                     *   in a Swiss German layout, NUMBER SIGN and
                     *   APOSTROPHE in a German layout, GRAVE
                     *   ACCENT and POUND SIGN in a French Mac
                     *   layout, and ASTERISK and MICRO SIGN in a
                     *   French Windows layout.
                     */
    NONUSHASH = 50, /**< ISO USB keyboards actually use this code
                     *   instead of 49 for the same key, but all
                     *   OSes I've seen treat the two codes
                     *   identically. So, as an implementor, unless
                     *   your keyboard generates both of those
                     *   codes and your OS treats them differently,
                     *   you should generate BACKSLASH
                     *   instead of this code. As a user, you
                     *   should not rely on this code because SDL
                     *   will never generate it with most (all?)
                     *   keyboards.
                     */
    SEMICOLON = 51,
    APOSTROPHE = 52,
    GRAVE = 53, /**< Located in the top left corner (on both ANSI
                 *   and ISO keyboards). Produces GRAVE ACCENT and
                 *   TILDE in a US Windows layout and in US and UK
                 *   Mac layouts on ANSI keyboards, GRAVE ACCENT
                 *   and NOT SIGN in a UK Windows layout, SECTION
                 *   SIGN and PLUS-MINUS SIGN in US and UK Mac
                 *   layouts on ISO keyboards, SECTION SIGN and
                 *   DEGREE SIGN in a Swiss German layout (Mac:
                 *   only on ISO keyboards), CIRCUMFLEX ACCENT and
                 *   DEGREE SIGN in a German layout (Mac: only on
                 *   ISO keyboards), SUPERSCRIPT TWO and TILDE in a
                 *   French Windows layout, COMMERCIAL AT and
                 *   NUMBER SIGN in a French Mac layout on ISO
                 *   keyboards, and LESS-THAN SIGN and GREATER-THAN
                 *   SIGN in a Swiss German, German, or French Mac
                 *   layout on ANSI keyboards.
                 */
    COMMA = 54,
    PERIOD = 55,
    SLASH = 56,

    CAPSLOCK = 57,

    F1 = 58,
    F2 = 59,
    F3 = 60,
    F4 = 61,
    F5 = 62,
    F6 = 63,
    F7 = 64,
    F8 = 65,
    F9 = 66,
    F10 = 67,
    F11 = 68,
    F12 = 69,

    PRINTSCREEN = 70,
    SCROLLLOCK = 71,
    PAUSE = 72,
    INSERT = 73, /**< insert on PC, help on some Mac keyboards (but
                                   does send code 73, not 117) */
    HOME = 74,
    PAGEUP = 75,
    DEL = 76,
    END = 77,
    PAGEDOWN = 78,
    RIGHT = 79,
    LEFT = 80,
    DOWN = 81,
    UP = 82,

    NUMLOCKCLEAR = 83, /**< num lock on PC, clear on Mac keyboards
                        */
    KpDivide = 84,
    KpMultiply = 85,
    KpMinus = 86,
    KpPlus = 87,
    KpEnter = 88,
    Kp1 = 89,
    Kp2 = 90,
    Kp3 = 91,
    Kp4 = 92,
    Kp5 = 93,
    Kp6 = 94,
    Kp7 = 95,
    Kp8 = 96,
    Kp9 = 97,
    Kp0 = 98,
    KpPeriod = 99,

    NONUSBACKSLASH = 100, /**< This is the additional key that ISO
                           *   keyboards have over ANSI ones,
                           *   located between left shift and Y.
                           *   Produces GRAVE ACCENT and TILDE in a
                           *   US or UK Mac layout, REVERSE SOLIDUS
                           *   (backslash) and VERTICAL LINE in a
                           *   US or UK Windows layout, and
                           *   LESS-THAN SIGN and GREATER-THAN SIGN
                           *   in a Swiss German, German, or French
                           *   layout. */
    APPLICATION = 101,    /**< windows contextual menu, compose */
    POWER = 102,          /**< The USB document says this is a status flag,
                           *   not a physical key - but some Mac keyboards
                           *   do have a power key. */
    KpEquals = 103,
    F13 = 104,
    F14 = 105,
    F15 = 106,
    F16 = 107,
    F17 = 108,
    F18 = 109,
    F19 = 110,
    F20 = 111,
    F21 = 112,
    F22 = 113,
    F23 = 114,
    F24 = 115,
    EXECUTE = 116,
    HELP = 117, /**< AL Integrated Help Center */
    MENU = 118, /**< Menu (show menu) */
    SELECT = 119,
    STOP = 120,  /**< AC Stop */
    AGAIN = 121, /**< AC Redo/Repeat */
    UNDO = 122,  /**< AC Undo */
    CUT = 123,   /**< AC Cut */
    COPY = 124,  /**< AC Copy */
    PASTE = 125, /**< AC Paste */
    FIND = 126,  /**< AC Find */
    MUTE = 127,
    VOLUMEUP = 128,
    VOLUMEDOWN = 129,
    /* not sure whether there's a reason to enable these */
    /*     LOCKINGCAPSLOCK = 130,  */
    /*     LOCKINGNUMLOCK = 131, */
    /*     LOCKINGSCROLLLOCK = 132, */
    KpComma = 133,
    KpEqualsaS400 = 134,

    INTERNATIONAL1 = 135, /**< used on Asian keyboards, see
                                            footnotes in USB doc */
    INTERNATIONAL2 = 136,
    INTERNATIONAL3 = 137, /**< Yen */
    INTERNATIONAL4 = 138,
    INTERNATIONAL5 = 139,
    INTERNATIONAL6 = 140,
    INTERNATIONAL7 = 141,
    INTERNATIONAL8 = 142,
    INTERNATIONAL9 = 143,
    LANG1 = 144, /**< Hangul/English toggle */
    LANG2 = 145, /**< Hanja conversion */
    LANG3 = 146, /**< Katakana */
    LANG4 = 147, /**< Hiragana */
    LANG5 = 148, /**< Zenkaku/Hankaku */
    LANG6 = 149, /**< reserved */
    LANG7 = 150, /**< reserved */
    LANG8 = 151, /**< reserved */
    LANG9 = 152, /**< reserved */

    ALTERASE = 153, /**< Erase-Eaze */
    SYSREQ = 154,
    CANCEL = 155, /**< AC Cancel */
    CLEAR = 156,
    PRIOR = 157,
    RETURN2 = 158,
    SEPARATOR = 159,
    OUTKEY = 160,
    OPER = 161,
    CLEARAGAIN = 162,
    CRSEL = 163,
    EXSEL = 164,

    Kp00 = 176,
    Kp000 = 177,
    THOUSANDSSEPARATOR = 178,
    DECIMALSEPARATOR = 179,
    CURRENCYUNIT = 180,
    CURRENCYSUBUNIT = 181,
    KpLeftparen = 182,
    KpRightparen = 183,
    KpLeftbrace = 184,
    KpRightbrace = 185,
    KpTab = 186,
    KpBackspace = 187,
    KpA = 188,
    KpB = 189,
    KpC = 190,
    KpD = 191,
    KpE = 192,
    KpF = 193,
    KpXor = 194,
    KpPower = 195,
    KpPercent = 196,
    KpLess = 197,
    KpGreater = 198,
    KpAmpersand = 199,
    KpDblampersand = 200,
    KpVerticalbar = 201,
    KpDblverticalbar = 202,
    KpColon = 203,
    KpHash = 204,
    KpSpace = 205,
    KpAt = 206,
    KpExclam = 207,
    KpMemstore = 208,
    KpMemrecall = 209,
    KpMemclear = 210,
    KpMemadd = 211,
    KpMemsubtract = 212,
    KpMemmultiply = 213,
    KpMemdivide = 214,
    KpPlusminus = 215,
    KpClear = 216,
    KpClearentry = 217,
    KpBinary = 218,
    KpOctal = 219,
    KpDecimal = 220,
    KpHexadecimal = 221,

    LCtrl = 224,
    LShift = 225,
    LAlt = 226, /**< alt, option */
    LGui = 227, /**< windows, command (apple), meta */
    RCtrl = 228,
    RShift = 229,
    RAlt = 230, /**< alt gr, option */
    RGui = 231, /**< windows, command (apple), meta */

    MODE = 257, /**< I'm not sure if this is really not covered
                 *   by any of the above, but since there's a
                 *   special KMOD_MODE for it I'm adding it here
                 */

    /* @} */ /* Usage page 0x07 */

    /**
     *  \name Usage page 0x0C
     *
     *  These values are mapped from usage page 0x0C (USB consumer page).
     *  See https://usb.org/sites/default/files/hut1_2.pdf
     *
     *  There are way more keys in the spec than we can represent in the
     *  current scancode range, so pick the ones that commonly come up in
     *  real world usage.
     */
    /* @{ */

    AUDIONEXT = 258,
    AUDIOPREV = 259,
    AUDIOSTOP = 260,
    AUDIOPLAY = 261,
    AUDIOMUTE = 262,
    MEDIASELECT = 263,
    WWW = 264, /**< AL Internet Browser */
    MAIL = 265,
    CALCULATOR = 266, /**< AL Calculator */
    COMPUTER = 267,
    AcSearch = 268,    /**< AC Search */
    AcHome = 269,      /**< AC Home */
    AcBack = 270,      /**< AC Back */
    AcForward = 271,   /**< AC Forward */
    AcStop = 272,      /**< AC Stop */
    AcRefresh = 273,   /**< AC Refresh */
    AcBookmarks = 274, /**< AC Bookmarks */

    /* @} */ /* Usage page 0x0C */

    /**
     *  \name Walther keys
     *
     *  These are values that Christian Walther added (for mac keyboard?).
     */
    /* @{ */

    BRIGHTNESSDOWN = 275,
    BRIGHTNESSUP = 276,
    DISPLAYSWITCH = 277, /**< display mirroring/dual display
                                           switch, video mode switch */
    KBDILLUMTOGGLE = 278,
    KBDILLUMDOWN = 279,
    KBDILLUMUP = 280,
    EJECT = 281,
    SLEEP = 282, /**< SC System Sleep */

    APP1 = 283,
    APP2 = 284,

    /* @} */ /* Walther keys */

    /**
     *  \name Usage page 0x0C (additional media keys)
     *
     *  These values are mapped from usage page 0x0C (USB consumer page).
     */
    /* @{ */

    AUDIOREWIND = 285,
    AUDIOFASTFORWARD = 286,

    /* @} */ /* Usage page 0x0C (additional media keys) */

    /**
     *  \name Mobile keys
     *
     *  These are values that are often used on mobile phones.
     */
    /* @{ */

    SOFTLEFT = 287,  /**< Usually situated below the display on phones and
                                       used as a multi-function feature key for selecting
                                       a software defined function shown on the bottom left
                                       of the display. */
    SOFTRIGHT = 288, /**< Usually situated below the display on phones and
                                       used as a multi-function feature key for selecting
                                       a software defined function shown on the bottom right
                                       of the display. */
    CALL = 289,      /**< Used for accepting phone calls. */
    ENDCALL = 290,   /**< Used for rejecting phone calls. */

    /* @} */ /* Mobile keys */

    /* Add any other keys here. */

    SdlNumScancodes = 512
};

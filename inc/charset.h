/*
 * charset.h
 *
 * The character set of every name that the firmware holds, a byte for each
 * character: ASCII; code page 437's 0x80-0xaf, its accented Latin letters
 * and signs, at their places there; and the Russian alphabet in KOI8-R's
 * order, the small letters at 0xc0-0xdf and the capitals at 0xe0-0xff, and
 * capital Yo at 0xb3, where KOI8-R has it, its small letter at 0xb2. The
 * rest of 0xb0-0xbf holds nothing. A character of a long name that the set
 * lacks reads as '?', which no FAT name holds, and one outside the Basic
 * Multilingual Plane as two.
 *
 * Why this layout: 437's block stays whole and in its place, so that a
 * short name written in 437 reads as itself, byte for byte, with no
 * table. The Russian letters take KOI8-R's order rather than 1251's
 * alphabetical one because a KOI8-R letter with bit 7 cleared is the
 * Latin letter that sounds like it: a display without Cyrillic glyphs,
 * such as FF OSD or an LCD with a Latin ROM, shows a name readably by a
 * subtraction, and sorting pays for it with a table of the alphabet's
 * order. Small yo is not at KOI8-R's 0xa3, which is 437's u acute here,
 * so it sits beside its capital. The set is the firmware's own and
 * ffemu's, never written to a drive, so a byte may move as long as every
 * table that maps it moves too: charset.c's, scripts/mk_font.py's and
 * ffemu's.
 *
 * Written & released by Mike Shevchenko <mike.shevchenko@gmail.com>
 *
 * This is free and unencumbered software released into the public domain.
 * See the file COPYING for more details, or visit <http://unlicense.org>.
 */

/* Short name @name, the bytes of an 8.3 entry, into the set in place: as
 * UTF-8 if its bytes are that, else in the code page of short names. */
void charset_sfn_decode(char *name);

/* Whether 8.3 entry @dir holds a byte over 0x7f. */
bool_t charset_sfn_extended(const uint8_t *dir);

/* Whether 8.3 entry @dir reads as @name, in UTF-16, letter case aside. */
bool_t charset_sfn_match(const uint8_t *dir, const uint16_t *name);

/* The key that sorts character @c, letter case aside: ASCII first, then
 * 437's characters, then the Russian letters in their alphabet's order. */
unsigned int charset_key(uint8_t c);

/*
 * Local variables:
 * mode: C
 * c-file-style: "Linux"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */

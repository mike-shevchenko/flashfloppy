/*
 * charset.c
 *
 * Names in the character set of charset.h: FatFS's code page, the names of
 * FAT entries read into the set, the order of names, and their characters
 * in the LCD's character ROM and in FF OSD's font.
 *
 * Written & released by Mike Shevchenko <mike.shevchenko@gmail.com>
 *
 * This is free and unencumbered software released into the public domain.
 * See the file COPYING for more details, or visit <http://unlicense.org>.
 */

/* The code points of code page 437's 0x80-0xaf. */
static const uint16_t uni_437[48] = {
    0x00c7, 0x00fc, 0x00e9, 0x00e2, 0x00e4, 0x00e0, 0x00e5, 0x00e7,
    0x00ea, 0x00eb, 0x00e8, 0x00ef, 0x00ee, 0x00ec, 0x00c4, 0x00c5,
    0x00c9, 0x00e6, 0x00c6, 0x00f4, 0x00f6, 0x00f2, 0x00fb, 0x00f9,
    0x00ff, 0x00d6, 0x00dc, 0x00a2, 0x00a3, 0x00a5, 0x20a7, 0x0192,
    0x00e1, 0x00ed, 0x00f3, 0x00fa, 0x00f1, 0x00d1, 0x00aa, 0x00ba,
    0x00bf, 0x2310, 0x00ac, 0x00bd, 0x00bc, 0x00a1, 0x00ab, 0x00bb
};

/* 437's capitals in that range, each with its small letter. */
static const uint8_t small_437[][2] = {
    { 0x80, 0x87 }, { 0x8e, 0x84 }, { 0x8f, 0x86 }, { 0x90, 0x82 },
    { 0x92, 0x91 }, { 0x99, 0x94 }, { 0x9a, 0x81 }, { 0xa5, 0xa4 }
};

/* The place in KOI8-R's order of each letter of the alphabet, A to Ya, and
 * the letter at each place. */
static const uint8_t koi_of_alpha[32] = {
    1, 2, 23, 7, 4, 5, 22, 26, 9, 10, 11, 12, 13, 14, 15, 16,
    18, 19, 20, 21, 6, 8, 3, 30, 27, 29, 31, 25, 24, 28, 0, 17
};
static const uint8_t alpha_of_koi[32] = {
    30, 0, 1, 22, 4, 5, 20, 3, 21, 8, 9, 10, 11, 12, 13, 14,
    15, 31, 16, 17, 18, 19, 6, 2, 28, 27, 7, 24, 29, 25, 23, 26
};

/* The code page of short names, and whether one may read as UTF-8, ff_cfg's
 * but in the bootloader, which has no configuration. */
#if TARGET == TARGET_bootloader
#define sfn_codepage CODEPAGE_866
#define sfn_detect_utf8 TRUE
#else
#define sfn_codepage ff_cfg.short_name_codepage
#define sfn_detect_utf8 ff_cfg.detect_utf8_in_short_names
#endif

/* Letter @i of the alphabet, its small letters from 32 on, in the set. */
static uint8_t letter(unsigned int i)
{
    return ((i < 32) ? 0xe0 : 0xc0) + koi_of_alpha[i & 31];
}

WCHAR ff_uni2oem(DWORD uni, WORD cp)
{
    unsigned int i;

    if (uni < 0x80)
        return uni;
    /* A character outside the Basic Multilingual Plane, a surrogate pair,
     * reads as two '?', which FatFS's put_utf() stores as a double-byte
     * character, so that the long name is kept. */
    if (uni >= 0x10000)
        return 0x3f3f;
    if (uni == 0x401)
        return 0xb3;
    if (uni == 0x451)
        return 0xb2;
    if ((uni >= 0x410) && (uni < 0x450))
        return letter(uni - 0x410);
    for (i = 0; i < ARRAY_SIZE(uni_437); i++)
        if (uni_437[i] == uni)
            return 0x80 + i;
    return '?';
}

WCHAR ff_oem2uni(WCHAR oem, WORD cp)
{
    if (oem < 0x80)
        return oem;
    if (oem < 0xb0)
        return uni_437[oem - 0x80];
    if ((oem == 0xb2) || (oem == 0xb3))
        return (oem == 0xb3) ? 0x401 : 0x451;
    if ((oem < 0xc0) || (oem > 0xff))
        return 0;
    return ((oem & 0x20) ? 0x410 : 0x430) + alpha_of_koi[oem & 31];
}

DWORD ff_wtoupper(DWORD uni)
{
    /* ASCII, Latin-1, which holds 437's letters, and Cyrillic, Yo too. */
    if (((uni >= 'a') && (uni <= 'z'))
        || ((uni >= 0xe0) && (uni <= 0xfe) && (uni != 0xf7))
        || ((uni >= 0x430) && (uni < 0x450)))
        return uni - 0x20;
    if (uni == 0x451)
        return 0x401;
    return uni;
}

/* The UTF-8 sequence at @p, of a character of the Basic Multilingual Plane:
 * its length, and its code point into @uni; 0 if none is there, as for a
 * sequence cut short, an overlong one or a surrogate. */
static unsigned int utf8_seq(const uint8_t *p, DWORD *uni)
{
    uint8_t lo = 0x80, hi = 0xbf;
    unsigned int n, i;
    DWORD u;

    if ((p[0] < 0xc2) || (p[0] > 0xef))
        return 0;
    n = (p[0] < 0xe0) ? 2 : 3;
    u = p[0] & ((n == 2) ? 0x1f : 0x0f);
    if (p[0] == 0xe0)
        lo = 0xa0;
    else if (p[0] == 0xed)
        hi = 0x9f;
    for (i = 1; i < n; i++) {
        if ((p[i] < lo) || (p[i] > hi))
            return 0;
        u = (u << 6) | (p[i] & 0x3f);
        lo = 0x80;
        hi = 0xbf;
    }
    *uni = u;
    return n;
}

/* Byte @c, over 0x7f, of a short name in the code page of short names. */
static uint8_t sfn_char(uint8_t c)
{
    unsigned int i;

    switch (sfn_codepage) {
    case CODEPAGE_437:
        return (c < 0xb0) ? c : '?';
    case CODEPAGE_mik:
        /* The alphabet at 0x80-0xbf. */
        if (c >= 0xc0)
            return '?';
        i = c - 0x80;
        break;
    default:
        /* 866: A to small p at 0x80-0xaf, small r to ya at 0xe0-0xef, then
         * capital and small Yo. */
        if (c < 0xb0)
            i = c - 0x80;
        else if ((c >= 0xe0) && (c < 0xf0))
            i = c - 0xe0 + 48;
        else if ((c == 0xf0) || (c == 0xf1))
            return 0xb3 - (c - 0xf0);
        else
            return '?';
        break;
    }
    return letter(i);
}

void charset_sfn_decode(char *name)
{
    bool_t utf8 = sfn_detect_utf8, extended = FALSE;
    unsigned int n;
    uint8_t *p, *q;
    DWORD uni;

    /* UTF-8 if every byte over 0x7f is of a sequence, unless turned off.
     * No short name in 866 or MIK, in capitals as MS-DOS and Windows write
     * it, has one. One in 437 has where a box-drawing character comes
     * before an accented capital, or ss, mu or a Greek letter before two:
     * rare, and such a sequence then reads as one '?'. */
    for (p = (uint8_t *)name; *p != '\0'; p += n) {
        n = 1;
        if (*p < 0x80)
            continue;
        extended = TRUE;
        if (!utf8 || ((n = utf8_seq(p, &uni)) == 0)) {
            utf8 = FALSE;
            break;
        }
    }
    if (!extended)
        return;

    for (p = q = (uint8_t *)name; *p != '\0'; p += n) {
        n = 1;
        if (*p < 0x80) {
            *q++ = *p;
        } else if (utf8) {
            n = utf8_seq(p, &uni);
            *q++ = ff_uni2oem(uni, 0);
        } else {
            *q++ = sfn_char(*p);
        }
    }
    *q = '\0';
}

bool_t charset_sfn_extended(const uint8_t *dir)
{
    unsigned int i;

    /* 0x05 stands for 0xe5, which marks a deleted entry. */
    for (i = 0; i < 11; i++)
        if ((dir[i] >= 0x80) || (dir[i] == 0x05))
            return TRUE;
    return FALSE;
}

bool_t charset_sfn_match(const uint8_t *dir, const uint16_t *name)
{
    char sfn[13];
    unsigned int i, n = 0;

    /* The short name as FatFS gives it, then as it reads. */
    for (i = 0; i < 11; i++) {
        if (dir[i] == ' ')
            continue;
        if (i == 8)
            sfn[n++] = '.';
        sfn[n++] = (dir[i] == 0x05) ? 0xe5 : dir[i];
    }
    sfn[n] = '\0';
    charset_sfn_decode(sfn);

    for (i = 0; sfn[i] != '\0'; i++)
        if (ff_wtoupper(ff_oem2uni((uint8_t)sfn[i], 0))
            != ff_wtoupper(name[i]))
            return FALSE;
    return name[i] == 0;
}

unsigned int charset_key(uint8_t c)
{
    unsigned int i;

    if ((c >= 'A') && (c <= 'Z'))
        return c + 'a' - 'A';
    /* The Russian letters by twice their place in the alphabet, so that Yo
     * sorts right after Ye, letter 5. */
    if ((c == 0xb2) || (c == 0xb3))
        return 0xc0 + 2*5 + 1;
    if (c >= 0xc0)
        return 0xc0 + 2*alpha_of_koi[c & 31];
    /* 437's characters keep the order of their bytes, the capitals taken
     * as their small letters. */
    for (i = 0; i < ARRAY_SIZE(small_437); i++)
        if (small_437[i][0] == c)
            return small_437[i][1];
    return c;
}

/* The bootloader shows only its own messages, in ASCII. */
#if TARGET != TARGET_bootloader

/* 437's 0x80-0xaf in each ROM of LCDCS_*, then in FF OSD's font: its own
 * glyph, else a letter's base letter, else the ROM's full block or '?'.
 * The Cyrillic ROMs' codes are those of both kinds, MELT's and Sitronix's,
 * which has nothing below 0x20 and blanks at 0x80-0x9f; A02's block is
 * CGRAM character 0, by its mirror 0x08. The reversed not sign, 0xa9, is
 * in no ROM. */
static const uint8_t rom_437[4][48] = {
    {
        0x43, 0xf5, 0x65, 0x61, 0xe1, 0x61, 0x61, 0x63, 0x65, 0x65, 0x65, 0x69,
        0x69, 0x69, 0x41, 0x41, 0x45, 0x61, 0x41, 0x6f, 0xef, 0x6f, 0x75, 0x75,
        0x79, 0x4f, 0x55, 0xec, 0xed, 0x5c, 0xff, 0x66, 0x61, 0x69, 0x6f, 0x75,
        0xee, 0x4e, 0x61, 0x6f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
    },
    {
        0xc7, 0xfc, 0xe9, 0xe2, 0xe4, 0xe0, 0xe5, 0xe7, 0xea, 0xeb, 0xe8, 0xef,
        0xee, 0xec, 0xc4, 0xc5, 0xc9, 0xe6, 0xc6, 0xf4, 0xf6, 0xf2, 0xfb, 0xf9,
        0xff, 0xd6, 0xdc, 0xa2, 0xa3, 0xa5, 0xb4, 0xa8, 0xe1, 0xed, 0xf3, 0xfa,
        0xf1, 0xd1, 0xaa, 0xba, 0xbf, 0x08, 0x08, 0xbd, 0xbc, 0xa1, 0xab, 0xbb
    },
    {
        0x43, 0x75, 0xea, 0x61, 0x61, 0x61, 0x61, 0xeb, 0x65, 0x65, 0x65, 0x69,
        0x69, 0x69, 0x41, 0x41, 0x45, 0x61, 0x41, 0x6f, 0x6f, 0x6f, 0x75, 0x75,
        0x79, 0x4f, 0x55, 0x5c, 0xcf, 0xff, 0xff, 0xce, 0x61, 0x69, 0x6f, 0x75,
        0x6e, 0x4e, 0x61, 0x6f, 0xcd, 0xff, 0xff, 0xf2, 0xf0, 0xff, 0xc8, 0xc9
    },
    {
        0x43, 0x75, 0x65, 0x61, 0x61, 0x61, 0x61, 0x63, 0x65, 0x65, 0x65, 0x69,
        0x69, 0x69, 0x41, 0x41, 0x45, 0x61, 0x41, 0x6f, 0x6f, 0x6f, 0x75, 0x75,
        0x79, 0x4f, 0x55, 0x3f, 0x3f, 0x3f, 0x3f, 0x66, 0x61, 0x69, 0x6f, 0x75,
        0x6e, 0x4e, 0x61, 0x6f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f
    }
};

/* KOI8-R's letters, small then capital, in the Cyrillic ROMs: a Latin letter
 * for one that looks alike. The capital soft sign is Latin b: only MELT's
 * ROM has the letter, at 0x08, where Sitronix's has a mirror of CGRAM, so
 * the shared codes have no capital; b, a stem with its bowl at the bottom,
 * is what the eye reads as the capital on either ROM, where the small soft
 * sign, the other stand-in both have, is half its height. */
static const uint8_t cyr_of_koi[64] = {
    0xc6, 0x61, 0xb2, 0xe5, 0xe3, 0x65, 0xe4, 0xb4, 0x78, 0xb8, 0xb9, 0xba,
    0xbb, 0xbc, 0xbd, 0x6f, 0xbe, 0xc7, 0x70, 0x63, 0xbf, 0x79, 0xb6, 0xb3,
    0xc4, 0xc3, 0xb7, 0xc1, 0xc5, 0xe6, 0xc0, 0xc2, 0xb0, 0x41, 0xa0, 0xe1,
    0xe0, 0x45, 0xaa, 0xa1, 0x58, 0xa5, 0xa6, 0x4b, 0xa7, 0x4d, 0x48, 0x4f,
    0xa8, 0xb1, 0x50, 0x43, 0x54, 0xa9, 0xa3, 0x42, 0x62, 0xae, 0xa4, 0xac,
    0xaf, 0xe2, 0xab, 0xad
};

uint8_t charset_to_rom(uint8_t c, unsigned int rom)
{
    if (c < 0x80)
        return c;
    /* A value that flash written by another firmware's layout may hold. */
    if (rom > CHARSET_osd)
        rom = LCDCS_a00;
    if (c < 0xb0)
        return rom_437[rom][c - 0x80];
    if ((c == 0xb2) || (c == 0xb3)) {
        /* Yo: both kinds of Cyrillic ROMs have it, the others show Ye. */
        if (rom == LCDCS_cyr)
            return (c == 0xb3) ? 0xa2 : 0xb5;
        c = (c == 0xb3) ? 0xe5 : 0xc5;
    }
    if (c < 0xc0)
        return rom_437[rom][0xa9 - 0x80];
    if (rom == LCDCS_cyr)
        return cyr_of_koi[c - 0xc0];
    /* KOI-7, the letter case kept: capitals to 0x40-0x5f, small letters to
     * 0x60-0x7f. */
    return c - ((c & 0x20) ? 0xa0 : 0x60);
}

#endif

/*
 * Local variables:
 * mode: C
 * c-file-style: "Linux"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */

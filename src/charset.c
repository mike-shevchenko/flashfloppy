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

/* Latin capitals at 0x80-0x9d and their small letters 32 cells below, ordered
 * by base letter, then by accent: acute, circumflex, diaeresis, grave, ring,
 * stroke and tilde. AE follows A's accents. Unpaired letters and the pound
 * sign occupy the rightmost cells. Dotless I is represented by its low byte. */
static const uint8_t uni_latin[64] = {
    0xc1, 0xc2, 0xc4, 0xc0, 0xc5, 0xc3, 0xc6, 0xc7,
    0xd0, 0xc9, 0xca, 0xcb, 0xc8, 0xcd, 0xce, 0xcf,
    0xcc, 0xd1, 0xd3, 0xd4, 0xd6, 0xd2, 0xd8, 0xd5,
    0xde, 0xda, 0xdb, 0xdc, 0xd9, 0xdd, 0x31, 0xa3,
    0xe1, 0xe2, 0xe4, 0xe0, 0xe5, 0xe3, 0xe6, 0xe7,
    0xf0, 0xe9, 0xea, 0xeb, 0xe8, 0xed, 0xee, 0xef,
    0xec, 0xf1, 0xf3, 0xf4, 0xf6, 0xf2, 0xf8, 0xf5,
    0xfe, 0xfa, 0xfb, 0xfc, 0xf9, 0xfd, 0xff, 0xdf
};

static WCHAR latin_uni(uint8_t c)
{
    return (c == 0x9e) ? 0x131 : uni_latin[c - 0x80];
}

/* Each Latin letter's base, small and capital alike: AE as A, O stroke as O,
 * Eth as D, Thorn as P, sharp S as S, and dotless I as I. Pound has no base. */
static const uint8_t base_latin[64] = {
    0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x63,
    0x64, 0x65, 0x65, 0x65, 0x65, 0x69, 0x69, 0x69,
    0x69, 0x6e, 0x6f, 0x6f, 0x6f, 0x6f, 0x6f, 0x6f,
    0x70, 0x75, 0x75, 0x75, 0x75, 0x79, 0x69, 0x00,
    0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x63,
    0x64, 0x65, 0x65, 0x65, 0x65, 0x69, 0x69, 0x69,
    0x69, 0x6e, 0x6f, 0x6f, 0x6f, 0x6f, 0x6f, 0x6f,
    0x70, 0x75, 0x75, 0x75, 0x75, 0x79, 0x79, 0x73
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
    /* Russian commonly writes Yo as Ye; share the latter's cells. */
    if (uni == 0x401)
        uni = 0x415;
    else if (uni == 0x451)
        uni = 0x435;
    if ((uni >= 0x410) && (uni < 0x450))
        return letter(uni - 0x410);
    if (uni == 0x131)
        return 0x9e;
    for (i = 0; i < ARRAY_SIZE(uni_latin); i++)
        if (uni_latin[i] == uni)
            return 0x80 + i;
    return '?';
}

WCHAR ff_oem2uni(WCHAR oem, WORD cp)
{
    if (oem < 0x80)
        return oem;
    if (oem < 0xc0)
        return latin_uni(oem);
    if (oem > 0xff)
        return 0;
    return ((oem & 0x20) ? 0x410 : 0x430) + alpha_of_koi[oem & 31];
}

DWORD ff_wtoupper(DWORD uni)
{
    /* ASCII, Latin-1, which holds 437's and 850's letters, and Cyrillic. */
    if (((uni >= 'a') && (uni <= 'z'))
        || ((uni >= 0xe0) && (uni <= 0xfe) && (uni != 0xf7))
        || ((uni >= 0x430) && (uni < 0x450)))
        return uni - 0x20;
    if (uni == 0x131)
        return 'I';
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

/* The short-name byte of each Latin cell, zero where the code page lacks the
 * character. The tables include the pound sign and 437's sharp S. */
static const uint8_t sfn_437[64] = {
    0x00, 0x00, 0x8e, 0x00, 0x8f, 0x00, 0x92, 0x80,
    0x00, 0x90, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0xa5, 0x00, 0x00, 0x99, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x9a, 0x00, 0x00, 0x00, 0x9c,
    0xa0, 0x83, 0x84, 0x85, 0x86, 0x00, 0x91, 0x87,
    0x00, 0x82, 0x88, 0x89, 0x8a, 0xa1, 0x8c, 0x8b,
    0x8d, 0xa4, 0xa2, 0x93, 0x94, 0x95, 0x00, 0x00,
    0x00, 0xa3, 0x96, 0x81, 0x97, 0x00, 0x98, 0xe1
};
static const uint8_t sfn_850[64] = {
    0xb5, 0xb6, 0x8e, 0xb7, 0x8f, 0xc7, 0x92, 0x80,
    0xd1, 0x90, 0xd2, 0xd3, 0xd4, 0xd6, 0xd7, 0xd8,
    0xde, 0xa5, 0xe0, 0xe2, 0x99, 0xe3, 0x9d, 0xe5,
    0xe8, 0xe9, 0xea, 0x9a, 0xeb, 0xed, 0xd5, 0x9c,
    0xa0, 0x83, 0x84, 0x85, 0x86, 0xc6, 0x91, 0x87,
    0xd0, 0x82, 0x88, 0x89, 0x8a, 0xa1, 0x8c, 0x8b,
    0x8d, 0xa4, 0xa2, 0x93, 0x94, 0x95, 0x9b, 0xe4,
    0xe7, 0xa3, 0x96, 0x81, 0x97, 0xec, 0x98, 0xe1
};

static uint8_t sfn_latin_char(uint8_t c, const uint8_t *page)
{
    unsigned int i;

    for (i = 0; i < 64; i++)
        if (c == page[i])
            return 0x80 + i;
    return '?';
}

/* Byte @c, over 0x7f, of a short name in the code page of short names. */
static uint8_t sfn_char(uint8_t c)
{
    unsigned int i;

    switch (sfn_codepage) {
    case CODEPAGE_850:
        return sfn_latin_char(c, sfn_850);
    case CODEPAGE_437:
        return sfn_latin_char(c, sfn_437);
    case CODEPAGE_mik:
        /* The alphabet at 0x80-0xbf. */
        if (c >= 0xc0)
            return '?';
        i = c - 0x80;
        break;
    default:
        /* 866: A to small p at 0x80-0xaf, small r to ya at 0xe0-0xef, then
         * capital and small Yo, which share Ye's cells. */
        if (c < 0xb0)
            i = c - 0x80;
        else if ((c >= 0xe0) && (c < 0xf0))
            i = c - 0xe0 + 48;
        else if ((c == 0xf0) || (c == 0xf1))
            return letter((c == 0xf0) ? 5 : 32 + 5);
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
     * it, has one. One in 437 or 850 can have one by chance, and such a
     * sequence then reads as one '?'. */
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

/* Yo and Ye share a cell, so either Unicode spelling matches. */
static DWORD match_upper(DWORD uni)
{
    if (uni == 0x401)
        uni = 0x415;
    else if (uni == 0x451)
        uni = 0x435;
    return ff_wtoupper(uni);
}

bool_t charset_uni_match(DWORD a, DWORD b)
{
    return match_upper(a) == match_upper(b);
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
        if (!charset_uni_match(ff_oem2uni((uint8_t)sfn[i], 0), name[i]))
            return FALSE;
    return name[i] == 0;
}

/* Latin letters deliberately have no secondary accent key: names that differ
 * only by accents can compare equal. */
unsigned int charset_key(uint8_t c)
{
    if ((c >= 'A') && (c <= 'Z'))
        return c + 'a' - 'A';
    if (c == 0x9f)
        return c;
    if ((c >= 0x80) && (c <= 0xbf))
        return base_latin[c - 0x80];
    if (c >= 0xc0)
        return 0xc0 + 2*alpha_of_koi[c & 31];
    return c;
}

/* The bootloader shows only its own messages, in ASCII. */
#if TARGET != TARGET_bootloader

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

/* ROM-specific glyphs that a Latin letter's base cannot stand in for. */
static const uint8_t rom_special[][3] = {
    { LCDCS_cyr, 0xa9, 0xea }, { LCDCS_cyr, 0xa7, 0xeb },
    { LCDCS_cyr, 0xab, 0xb5 }, { LCDCS_cyr, 0x8b, 0xa2 },
    { LCDCS_jap, 0xbb, 0xf5 }, { LCDCS_jap, 0xa2, 0xe1 },
    { LCDCS_jap, 0xb4, 0xef }, { LCDCS_jap, 0xb1, 0xee }
};

uint8_t charset_to_rom(uint8_t c, unsigned int rom)
{
    unsigned int u, base, i;

    if (c < 0x80)
        return c;
    /* A value that flash written by another firmware's layout may hold. */
    if (rom > CHARSET_osd)
        rom = LCDCS_jap;
    if (c >= 0xc0) {
        if (rom == LCDCS_cyr)
            return cyr_of_koi[c - 0xc0];
        /* KOI-7, the letter case kept: capitals to 0x40-0x5f, small letters to
         * 0x60-0x7f. */
        return c - ((c & 0x20) ? 0xa0 : 0x60);
    }
    u = latin_uni(c);
    /* A02 has Latin-1's letters but capital O stroke and dotless I. */
    if (rom == LCDCS_eur) {
        if (u == 0xd8)
            return 'O';
        if (u == 0x131)
            return 'i';
        return u;
    }
    if (c == 0x9f) {
        if (rom == LCDCS_cyr)
            return 0xcf;
        return (rom == CHARSET_osd) ? '?' : 0xff;
    }
    for (i = 0; i < ARRAY_SIZE(rom_special); i++)
        if ((c == rom_special[i][1]) && (rom == rom_special[i][0]))
            return rom_special[i][2];
    base = base_latin[c - 0x80];
    return ((u >= 0xc0) && (u <= 0xde)) ? base - 0x20 : base;
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

/*
 * test_names.c
 *
 * Names of files and folders as FatFS gives them to the firmware, in the
 * character set of inc/charset.h, opened again by those names, as the
 * firmware opens an image or enters a folder; and the set's conversions and
 * order.
 *
 * Written & released by Mike Shevchenko <mike.shevchenko@gmail.com>
 *
 * This is free and unencumbered software released into the public domain.
 * See the file COPYING for more details, or visit <http://unlicense.org>.
 */

#include "harness.h"

/* AGAT in Cyrillic capitals in the set, and as the short name that a
 * Russian MS-DOS or Windows writes, in code page 866, and that a Windows
 * writes whose code page is UTF-8. 866's capitals are MIK's. */
#define AGAT "\xe1\xe7\xe1\xf4"
#define AGAT_866 "\x80\x83\x80\x92"
#define AGAT_UTF8 "\xd0\x90\xd0\x93\xd0\x90\xd0\xa2"

/* Lists the root folder, which must hold names @want of @nr in that order,
 * then opens or enters each by its name. */
static void check_listed(const char * const *want, unsigned int nr)
{
    DIR dp;
    FILINFO fno;
    FIL f;
    unsigned int i;

    drive_mount();
    REQUIRE(f_opendir(&dp, "") == FR_OK);
    for (i = 0; i < nr; i++) {
        REQUIRE((f_readdir(&dp, &fno) == FR_OK) && (fno.fname[0] != '\0'));
        CHECK_STR(fno.fname, want[i]);
        if (fno.fattrib & AM_DIR) {
            CHECK_EQ(f_chdir(want[i]), FR_OK);
            CHECK_EQ(f_chdir("/"), FR_OK);
        } else {
            CHECK_EQ(f_open(&f, want[i], FA_READ), FR_OK);
            f_close(&f);
        }
    }
    CHECK((f_readdir(&dp, &fno) == FR_OK) && (fno.fname[0] == '\0'));
    f_closedir(&dp);
}

TEST(names_short)
{
    static const struct {
        uint8_t codepage;
        bool_t folder;
        const char *raw, *want;
    } t[] = {
        { CODEPAGE_866, FALSE, AGAT_866 ".DSK", AGAT ".DSK" },
        { CODEPAGE_866, TRUE, AGAT_866, AGAT },
        { CODEPAGE_mik, FALSE, AGAT_866 ".DSK", AGAT ".DSK" },
        /* Capital Yo: YOLKA. */
        { CODEPAGE_866, FALSE, "\xf0\x8b\x8a\x80", "\xb3\xec\xeb\xe1" },
        /* A capital, then MIK's small r, which 866 lacks there. */
        { CODEPAGE_866, FALSE, "\x80\xb0", "\xe1?" },
        { CODEPAGE_mik, FALSE, "\x80\xb0", "\xe1\xd2" },
        /* KASE with A umlaut, as a German MS-DOS writes it. */
        { CODEPAGE_437, FALSE, "K\x8eSE.IMG", "K\x8eSE.IMG" },
        { CODEPAGE_866, FALSE, "K\x8eSE.IMG", "K\xefSE.IMG" },
        /* UTF-8, whatever the code page. */
        { CODEPAGE_866, FALSE, AGAT_UTF8 ".DSK", AGAT ".DSK" },
        { CODEPAGE_437, TRUE, AGAT_UTF8, AGAT },
        { CODEPAGE_mik, FALSE, "CAF\xc3\x89", "CAF\x90" },
        /* Not well-formed UTF-8, overlong and a surrogate: 866's. */
        { CODEPAGE_866, FALSE, "\xe0\x80\x80", "\xd2\xe1\xe1" },
        { CODEPAGE_866, FALSE, "\xed\xa0\x80", "\xdc\xc1\xe1" },
        /* Capital Yo and three capitals in 866, YORAA, which as UTF-8
         * would be a character outside the Basic Multilingual Plane. */
        { CODEPAGE_866, FALSE, "\xf0\x90\x80\x80", "\xb3\xf2\xe1\xe1" },
        /* 437's STRASSE with A and O umlauts after the sharp s: it passes
         * for UTF-8, a character outside the set. */
        { CODEPAGE_437, FALSE, "STRA\xe1\x8e\x99", "STRA?" },
    }, off[] = {
        /* With detection off, UTF-8 in 866: box drawing and letters. */
        { CODEPAGE_866, FALSE, AGAT_UTF8 ".DSK", "?\xf2?\xf5?\xf2?\xd7.DSK" },
        { CODEPAGE_437, FALSE, "STRA\xe1\x8e\x99", "STRA?\x8e\x99" },
    };
    FIL f;
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(t); i++) {
        drive_reset();
        ff_cfg.short_name_codepage = t[i].codepage;
        if (t[i].folder)
            drive_add_folder(t[i].raw, NULL);
        else
            drive_add(t[i].raw, "x", 1);
        check_listed(&t[i].want, 1);
    }
    for (i = 0; i < ARRAY_SIZE(off); i++) {
        drive_reset();
        ff_cfg.short_name_codepage = off[i].codepage;
        ff_cfg.detect_utf8_in_short_names = FALSE;
        drive_add(off[i].raw, "x", 1);
        check_listed(&off[i].want, 1);
    }

    /* A short name, as it reads, in the other letter case. */
    drive_reset();
    drive_add(AGAT_866 ".DSK", "x", 1);
    drive_mount();
    CHECK_EQ(f_open(&f, "\xc1\xc7\xc1\xd4.dsk", FA_READ), FR_OK);
    f_close(&f);

    drive_reset();
}

TEST(names_long)
{
    static const char * const want[] = {
        "\xe9\xc7\xd2\xc1.dsk", "\xe1\xc7\xc1\xd4", "K\x84se.img",
        "?\xa2" "d?.dsk", "\xb3\xcc\xcb\xc1.dsk", "??.dsk"
    };
    FIL f;
    char c;
    UINT nr;

    drive_reset();
    /* Igra, Agat, Kase with a umlaut, Lodz with L stroke, o acute and z
     * acute, Yolka, and an emoji outside the Basic Multilingual
     * Plane, which reads as two '?' and opens by them. */
    drive_add_long("IGRA~1.DSK", "\xd0\x98\xd0\xb3\xd1\x80\xd0\xb0.dsk",
                   "x", 1);
    drive_add_folder("AGAT~1", "\xd0\x90\xd0\xb3\xd0\xb0\xd1\x82");
    drive_add_long("KASE~1.IMG", "K\xc3\xa4se.img", "x", 1);
    drive_add_long("LODZ~1.DSK", "\xc5\x81\xc3\xb3" "d\xc5\xba.dsk", "x", 1);
    drive_add_long("ELKA~1.DSK", "\xd0\x81\xd0\xbb\xd0\xba\xd0\xb0.dsk",
                   "x", 1);
    drive_add_long("SMILE~1.DSK", "\xf0\x9f\x98\x80.dsk", "x", 1);
    check_listed(want, ARRAY_SIZE(want));

    /* In the other letter case: IGRA.DSK, KASE.IMG, YOLKA.DSK. */
    CHECK_EQ(f_open(&f, "\xe9\xe7\xf2\xe1.DSK", FA_READ), FR_OK);
    f_close(&f);
    CHECK_EQ(f_open(&f, "K\x8eSE.IMG", FA_READ), FR_OK);
    f_close(&f);
    CHECK_EQ(f_open(&f, "\xb3\xec\xeb\xe1.DSK", FA_READ), FR_OK);
    f_close(&f);
    /* Yolka with Ye is another name. */
    CHECK_EQ(f_open(&f, "\xe5\xcc\xcb\xc1.dsk", FA_READ), FR_NO_FILE);
    /* '?' is a name's character, matching none that is not there. */
    CHECK_EQ(f_open(&f, "NO?.DSK", FA_READ), FR_NO_FILE);

    /* La and Sa, with L stroke and S acute, both read as "?a.dsk": the
     * first opens. */
    drive_reset();
    drive_add_long("LA~1.DSK", "\xc5\x81" "a.dsk", "1", 1);
    drive_add_long("SA~1.DSK", "\xc5\x9a" "a.dsk", "2", 1);
    drive_mount();
    REQUIRE(f_open(&f, "?a.dsk", FA_READ) == FR_OK);
    CHECK((f_read(&f, &c, 1, &nr) == FR_OK) && (nr == 1) && (c == '1'));
    f_close(&f);

    drive_reset();
}

TEST(names_conversions)
{
    unsigned int c, i;

    /* Every character of the set to Unicode and back; the rest of
     * 0xb0-0xbf, beside Yo's two, none. */
    for (c = 0x20; c < 0x100; c++) {
        if ((c >= 0xb0) && (c < 0xc0) && (c != 0xb2) && (c != 0xb3))
            CHECK_EQ(ff_oem2uni(c, 0), 0);
        else
            CHECK_EQ(ff_uni2oem(ff_oem2uni(c, 0), 0), c);
    }
    /* The Russian alphabet and back, and its letter case; then Yo. */
    for (i = 0x410; i < 0x450; i++) {
        CHECK_EQ(ff_oem2uni(ff_uni2oem(i, 0), 0), i);
        CHECK_EQ(ff_wtoupper(i), (i < 0x430) ? i : i - 0x20);
    }
    CHECK_EQ(ff_oem2uni(ff_uni2oem(0x401, 0), 0), 0x401);
    CHECK_EQ(ff_oem2uni(ff_uni2oem(0x451, 0), 0), 0x451);
    CHECK_EQ(ff_wtoupper(0x451), 0x401);
    CHECK_EQ(ff_wtoupper(0xe4), 0xc4);
    /* L stroke, which the set lacks, and a surrogate pair, two '?'. */
    CHECK_EQ(ff_uni2oem(0x141, 0), '?');
    CHECK_EQ(ff_uni2oem(0xd83dde00, 0), 0x3f3f);
}

TEST(names_key)
{
    unsigned int i, cap;

    CHECK_EQ(charset_key('A'), charset_key('a'));
    /* ASCII, then 437's characters, then the Russian letters. */
    CHECK(charset_key('z') < charset_key(0x84));
    CHECK_EQ(charset_key(0x8e), charset_key(0x84));
    CHECK(charset_key(0xaf) < charset_key(ff_uni2oem(0x410, 0)));
    for (i = 0; i < 32; i++) {
        cap = ff_uni2oem(0x410 + i, 0);
        CHECK_EQ(charset_key(cap), charset_key(ff_uni2oem(0x430 + i, 0)));
        if (i > 0)
            CHECK(charset_key(ff_uni2oem(0x410 + i - 1, 0))
                  < charset_key(cap));
    }
    /* Yo right after Ye, in either case. */
    CHECK_EQ(charset_key(0xb2), charset_key(0xb3));
    CHECK(charset_key(ff_uni2oem(0x435, 0)) < charset_key(0xb2));
    CHECK(charset_key(0xb3) < charset_key(ff_uni2oem(0x416, 0)));
}

/*
 * Local variables:
 * mode: C
 * c-file-style: "Linux"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */

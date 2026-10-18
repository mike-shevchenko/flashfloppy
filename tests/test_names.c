/*
 * test_names.c
 *
 * Names of files and folders as the firmware's FatFS gives them, opened
 * again by those names, as the firmware opens an image or enters a folder.
 *
 * Written & released by Mike Shevchenko <mike.shevchenko.kz@gmail.com>
 *
 * This is free and unencumbered software released into the public domain.
 * See the file COPYING for more details, or visit <http://unlicense.org>.
 */

#include "harness.h"

/* AGAT in Cyrillic capitals: in code page 866, as a Russian MS-DOS or
 * Windows writes a short name, and in UTF-8, as a Windows does whose code
 * page is UTF-8. Code page 437 upper-cases some of these bytes otherwise:
 * 0x83, 0x93 and 0xa2 are lower-case letters there. */
#define AGAT_866 "\x80\x83\x80\x92"
#define AGAT_UTF8 "\xd0\x90\xd0\x93\xd0\x90\xd0\xa2"

TEST(names_short_non_ascii)
{
    static const char * const names[] = {
        AGAT_866 ".DSK", AGAT_866, AGAT_UTF8 ".DSK", AGAT_UTF8, "PLAIN.DSK"
    };
    DIR dp;
    FILINFO fno;
    FIL f;
    unsigned int i = 0;

    drive_reset();
    drive_add(names[0], "x", 1);
    drive_add_folder(names[1]);
    drive_add(names[2], "x", 1);
    drive_add_folder(names[3]);
    drive_add(names[4], "x", 1);
    drive_mount();

    REQUIRE(f_opendir(&dp, "") == FR_OK);
    while ((f_readdir(&dp, &fno) == FR_OK) && (fno.fname[0] != '\0')) {
        REQUIRE(i < ARRAY_SIZE(names));
        /* With no long name, the short name's bytes as they are. */
        CHECK_STR(fno.fname, names[i]);
        if (fno.fattrib & AM_DIR) {
            CHECK_EQ(f_chdir(fno.fname), FR_OK);
            CHECK_EQ(f_chdir("/"), FR_OK);
        } else {
            CHECK_EQ(f_open(&f, fno.fname, FA_READ), FR_OK);
            f_close(&f);
        }
        i++;
    }
    f_closedir(&dp);
    CHECK_EQ(i, ARRAY_SIZE(names));

    drive_reset();
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

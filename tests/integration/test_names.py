"""Names of files and folders that are not ASCII, which the firmware holds in
the character set of inc/charset.h: a folder entered and an image mounted by
their long names in Cyrillic, and names drawn on the OLED in each font; and
the drive of mk_names_image.py, whose names come in every encoding."""

import os
import re
import tempfile
import unittest

import ffemu_run as fr
import mk_names_image

# IGRY/IGRA.IMG in Cyrillic, a folder of the root and a PC image in it, which
# the selector enters and mounts by itself.
IGRA = {"\u0418\u0433\u0440\u044b/\u0438\u0433\u0440\u0430.img":
        bytes(737280)}

# The OLED's fonts: the 6x13, the 8x16, and the 8x16 made from the 6x13, as
# the STM32F105's logfile and debug images draw it.
FONTS = {
    "6x13": {},
    "8x16": {"flash": ["oled-font = 8x16"]},
    "synth": {"flash": ["oled-font = 8x16"],
              "settings": ["oled-8x16 = synth"]},
}
MOUNT = ["wait 15000 log Name:", "sleep 300", "dump"]


def font_glyphs(font):
    """The glyphs of the build's OLED font @font, 6x13 or 8x16, by their code
    points: each its columns, a bit a pixel row from the top."""
    return fr.font_glyphs(font)


def glyph(font, code):
    """The columns of character @code in @font of FONTS, as the OLED shows
    it: the 8x16 made from the 6x13 is that glyph with a second copy a
    column to the right."""
    if font != "synth":
        return font_glyphs(font)[code]
    g = font_glyphs("6x13")[code] + [0, 0]
    return [g[x] | (g[x - 1] if x else 0) for x in range(8)]


def pixels(r):
    """The OLED's pixel rows in run @r's dump."""
    lines = r.stdout.splitlines()
    top = next(i for i, l in enumerate(lines) if l.startswith("display: "))
    return lines[top + 1:]


def name_cells(r, font, nr):
    """The first @nr cells of the OLED's top text row in run @r's dump."""
    rows = pixels(r)[:16]
    x0, w = (1, 6) if font == "6x13" else (0, 8)
    return [[sum((rows[y][x0 + w*i + x] != ".") << y for y in range(16))
             for x in range(w)] for i in range(nr)]


class Names(fr.Case, unittest.TestCase):

    def test_cyrillic_long_names(self):
        """The image mounted by its long name, which the console prints in
        the set's bytes, KOI8-R's, each a replacement character to the
        harness, which reads UTF-8."""
        r = fr.run("ffemu", ["wait 15000 log Name:", "log"], files=IGRA)
        self.assertWaits(r, 1)
        self.assertIn("Name: '%s' Type: img" % ("\ufffd" * 4), r.stdout,
                      r.report())

    def test_oled_russian(self):
        """Russian letters by the font's own glyphs, Yo's two among them,
        in each font: Yolka, yozh and Igra."""
        name = ("\u0401\u043b\u043a\u0430 \u0451\u0436 "
                "\u0418\u0433\u0440\u0430")
        for font, opts in FONTS.items():
            with self.subTest(font=font):
                r = fr.run("ffemu", MOUNT,
                           files={name + ".img": bytes(737280)}, **opts)
                self.assertWaits(r, 1)
                self.assertEqual(name_cells(r, font, len(name)),
                                 [glyph(font, ord(c)) for c in name],
                                 r.report())

    def test_oled_437(self):
        """Kaese with a umlaut, the peseta sign and L stroke: 437's glyphs,
        the peseta as the hollow square in the 6x13 font, which lacks it,
        and L stroke, which the set lacks, as '?'."""
        want = [ord("K"), 0xe4, ord("s"), ord("e"), 0x20a7, ord("?")]
        for font, opts in FONTS.items():
            with self.subTest(font=font):
                r = fr.run("ffemu", MOUNT, files={
                    "K\u00e4se\u20a7\u0141.img": bytes(737280)}, **opts)
                self.assertWaits(r, 1)
                self.assertEqual(name_cells(r, font, len(want)),
                                 [glyph(font, c) for c in want], r.report())
        self.assertEqual(glyph("6x13", 0x20a7), glyph("6x13", 0x25a1))
        self.assertNotEqual(glyph("8x16", 0x20a7), glyph("8x16", 0x25a1))


class NamesImage(fr.Case, unittest.TestCase):
    """mk_names_image.py's drive, with the short-name-codepage that each
    folder's short names need: the folder entered from the root, then its own
    folder entered and that folder's image mounted; and its first image
    mounted, its name read on the OLED by its glyphs."""

    @classmethod
    def setUpClass(cls):
        cls.tmp = tempfile.TemporaryDirectory(prefix="ffemu-test-")
        cls.image = os.path.join(cls.tmp.name, "names.img")
        mk_names_image.build(cls.image)

    @classmethod
    def tearDownClass(cls):
        cls.tmp.cleanup()

    def mount(self, i, keys):
        """Folder @i of the root entered, @keys pressed in it, until an image
        is mounted: the run."""
        _, page, _, _ = mk_names_image.FOLDERS[i]
        r = fr.run("ffemu", ["sleep 1000"] + ["key right", "sleep 200"] * i
                   + ["sleep 2500"] + keys
                   + ["wait 15000 log Name:", "sleep 300", "log", "dump"],
                   flash=["short-name-codepage = " + page],
                   drive_image=self.image)
        self.assertWaits(r, 1)
        return r

    def test_folders(self):
        """Each folder's own folder entered by its name."""
        for i, (name, _, _, _) in enumerate(mk_names_image.FOLDERS):
            with self.subTest(folder=name):
                r = self.mount(i, [])
                self.assertIn("Name: 'A' Type: img", r.stdout, r.report())

    def test_names(self):
        """Each folder's image listed first."""
        for i, (name, _, shown, _) in enumerate(mk_names_image.FOLDERS):
            with self.subTest(folder=name, image=shown):
                r = self.mount(i, ["key right"])
                self.assertEqual(name_cells(r, "6x13", len(shown)),
                                 [glyph("6x13", ord(c)) for c in shown],
                                 r.report())


# A name of Russian letters, Yo, both hard signs and the capital soft sign
# among them, of a 437 letter that the A00 ROM has and one that it lacks,
# of a sign that only A02 and the Cyrillic ROMs have, and of the not sign,
# which no ROM has.
ROM_NAME = ("\u0401\u043b\u043a\u0430 \u042a\u044a \u042c\u00e4 "
            "\u00e9\u00bd\u00ac")


class RomCharset(fr.Case, unittest.TestCase):
    """ROM_NAME on a 16x2 LCD of each ROM, by lcd-charset, and on FF OSD."""

    def rows(self, rom, charset=None, ff_cfg=None):
        """The LCD's rows, then FF OSD's, once the image is in, lcd-charset
        set in flash as @charset or by FF.CFG @ff_cfg, which resets the
        firmware when it changes the option."""
        files = {ROM_NAME + ".img": bytes(737280)}
        if ff_cfg:
            files["FF.CFG"] = ff_cfg
        r = fr.run(
            "ffemu", ["sleep 6000"] * bool(ff_cfg)
            + ["wait 10000 log Name:", "sleep 1000", "dump", "osd", "flash"],
            settings=["display = hd44780-16x2", "lcd-rom = " + rom],
            flash=["lcd-charset = " + charset] if charset else [],
            files=files)
        self.assertWaits(r, 1)
        rows = [line[1:-1].rstrip() for line in r.lines
                if line.startswith("|") and line.endswith("|")]
        self.assertEqual(len(rows), 5, r.report())
        return rows, r

    def test_a00(self):
        """Russian by KOI-7, the case kept, Yo as Ye and the small hard sign
        on 0x7f, the ROM's arrow there; A00's own a umlaut, the base letter
        of e acute, the full block for one half and the not sign. FF OSD
        alike, but for the a umlaut, which its font lacks too, and '?' for
        the signs."""
        rows, r = self.rows("a00", "a00")
        self.assertEqual(rows[0], "Elka _\u2190 X\u00e4 e\u2588\u2588",
                         r.report())
        self.assertEqual(rows[3], "Elka _\u2592 Xa e??", r.report())

    def test_a02(self):
        """Russian by KOI-7 as on a00, the ROM's house on 0x7f; A02's own a
        umlaut, e acute and one half; the not sign as the full block that
        the firmware makes in CGRAM 0 and shows by its mirror 0x08."""
        rows, r = self.rows("a02", "a02")
        self.assertEqual(
            rows[0], "Elka _\u2302 X\u00e4 \u00e9\u00bd\u2588", r.report())
        self.assertEqual(rows[3], "Elka _\u2592 Xa e??", r.report())

    def test_cyr(self):
        """The ROM's Russian letters, Latin ones where they look alike, Latin
        b for the capital soft sign; its e acute and one half, the base letter
        of a umlaut, the ROM's full block for the not sign."""
        rows, r = self.rows("cyr", "cyr")
        self.assertEqual(
            rows[0],
            "\u0401\u043b\u043aa \u042a\u044a ba \u00e9\u00bd\u2588",
            r.report())
        self.assertEqual(rows[3], "Elka _\u2592 Xa e??", r.report())

    def test_ff_cfg(self):
        """lcd-charset read from FF.CFG, which puts it in flash."""
        rows, r = self.rows("cyr", ff_cfg="lcd-charset = cyr\n")
        self.assertEqual(r.flash().get("lcd-charset"), "cyr", r.report())
        self.assertEqual(
            rows[0],
            "\u0401\u043b\u043aa \u042a\u044a ba \u00e9\u00bd\u2588",
            r.report())

    def test_ascii(self):
        """An ASCII name as it is, whatever the ROM."""
        r = fr.run(
            "ffemu", ["wait 10000 log Name:", "sleep 1000", "dump"],
            settings=["display = hd44780-16x2", "lcd-rom = cyr"],
            flash=["lcd-charset = cyr"],
            files={"Game-1 (v2).img": bytes(737280)})
        self.assertWaits(r, 1)
        self.assertIn("|Game-1 (v2)     |", r.lines, r.report())


if __name__ == "__main__":
    unittest.main()

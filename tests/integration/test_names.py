"""Names of files and folders that are not ASCII, which the firmware holds in
the character set of inc/charset.h: a folder entered and an image mounted by
their long names in Cyrillic, and names drawn on the OLED in each font."""

import os
import re
import unittest

import ffemu_run as fr

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


if __name__ == "__main__":
    unittest.main()

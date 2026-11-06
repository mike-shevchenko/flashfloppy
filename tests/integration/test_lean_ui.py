"""The lean UI variant: reduced display rendering with normal name handling."""

import unittest

import ffemu_run as fr
import test_display as display
import test_names as names


PROGRAM = "ffemu-lean-ui"


def pixels(r):
    """The OLED's pixel rows in run @r's dumps."""
    return [line for line in r.lines if line and set(line) <= set(".#")]


class LeanUI(display.Case, unittest.TestCase):

    def test_wide_is_double_height(self):
        """A wide row has the same rendering as a double-height row."""
        runs = []
        for order in ("0w,1", "0d,1"):
            r = fr.run(
                PROGRAM, ["wait 10000 log Name:", "sleep 300", "dump"],
                settings=["display = ssd1306-128x64"],
                flash=["display-type = oled-128x64",
                       "display-order = " + order],
                files=display.DSK)
            self.assertWaits(r, 1)
            runs.append(r)
        self.assertEqual(pixels(runs[0]), pixels(runs[1]),
                         runs[0].report() + runs[1].report())

    def test_hex_phase_on_oled(self):
        """The Apple2 phase digit is hexadecimal, with no spinner glyph."""
        rows, r = self.rows(
            PROGRAM, display.half_tracks(2), "ssd1306-128x32", files=display.DSK)
        self.assertEqual(rows[2], "000/001   T:01.4     ", r.report())
        self.assertEqual(
            display.oled_text(pixels(r), 16, "6x13", False, False), rows[2],
            r.report())

    def test_ascii_fonts(self):
        """Both fonts show the fallback glyph, while the name still opens."""
        for font in ("6x13", "8x16"):
            with self.subTest(font=font):
                r = fr.run(
                    PROGRAM, names.MOUNT + ["log"],
                    settings=["display = ssd1306-128x32"],
                    flash=["display-type = oled-128x32",
                           "oled-font = " + font],
                    files={"\u0418\u0433\u0440\u0430.dsk": bytes(35 * 4096)})
                self.assertWaits(r, 1)
                self.assertIn("Name: '\u0418\u0433\u0440\u0430'", r.stdout,
                              r.report())
                fallback = fr.font_glyphs(font)[0x25a1]
                self.assertEqual(names.name_cells(r, font, 4), [fallback] * 4,
                                 r.report())

    def test_no_scrolling(self):
        """A long name stays at its start after the usual scrolling pause."""
        r = fr.run(
            PROGRAM, ["wait 10000 log Name:", "sleep 300", "dump",
                      "sleep 5000", "dump"],
            settings=["display = ssd1306-128x32"],
            flash=["display-type = oled-128x32"],
            files={"ABCDEFGHIJKLMNOPQRSTUVW.DSK": bytes(35 * 4096)})
        self.assertWaits(r, 1)
        frames = pixels(r)
        self.assertEqual(len(frames), 64, r.report())
        self.assertEqual(frames[:32], frames[32:], r.report())

    def test_no_font_test(self):
        """The Main Menu stays available without the Font Test item."""
        r = fr.run(
            PROGRAM, ["key remove", "sleep 500", "key select", "sleep 500",
                      "dump", "osd"],
            settings=["display = ssd1306-128x64"])
        self.assertWaits(r, 0)
        self.assertIn("Main Menu", r.stdout, r.report())
        self.assertNotIn("Font Test", r.stdout, r.report())

    def test_error_code_only(self):
        """An error screen shows its code without the meaning from fs_hint."""
        rows, r = self.rows(
            PROGRAM, ["sleep 1000"], "hd44780-16x2", files=display.Errors.BAD)
        self.assertEqual(rows, ["BAD".ljust(16), "E31".ljust(16)], r.report())


if __name__ == "__main__":
    unittest.main()

"""ffemu-multi: the host that the multi firmware detects at boot, shown by its
7-segment banner, and in Apple2 mode the stepper phases and the image's track
on the displays as the host steps the head, and the write mark."""

import unittest

import ffemu_run as fr

DSK = {"TEST.DSK": bytes(35 * 4096)}


class Detection(fr.Case, unittest.TestCase):

    def check_banner(self, fdd_type, log, banner):
        r = fr.run(
            "ffemu-multi",
            ["wait 10000 log " + log, "wait 10000 log Intf IBM PC",
             "key remove", "wait 5000 segments " + banner],
            settings=["display = tm1651-3digit", "fdd-type = " + fdd_type])
        self.assertWaits(r, 3)

    def test_apple2_host(self):
        """A-2 in place of F-F."""
        self.check_banner("apple2", "Apple2 on", "77 40 5b")

    def test_step_dir_host(self):
        self.check_banner("step-dir", "Apple2 off", "71 40 71")


class Apple2Mode(fr.Case, unittest.TestCase):
    """As with ffemu-apple2, each "phase-in" moves to the next state of a
    seek: phases 0 and 1, phase 1 (a half track in), phases 1 and 2, phase 2
    (a track in); the multi firmware shows the image's track, not the half
    tracks."""

    def check(self, display, steps):
        kind = "segments" if display.startswith("tm1651") else "display"
        cmds = ["wait 10000 log Name: 'TEST'"]
        for keys, shown in steps:
            cmds += keys
            cmds.append("wait 3000 %s %s" % (kind, shown))
        r = fr.run(
            "ffemu-multi", cmds,
            settings=["display = " + display, "fdd-type = apple2"], files=DSK)
        self.assertWaits(r, 1 + len(steps))

    def pixels(self, order, font):
        r = fr.run(
            "ffemu-multi", ["wait 10000 log Name:", "key phase-in",
                            "sleep 500", "dump"],
            settings=["display = ssd1306-128x64", "fdd-type = apple2"],
            flash=["display-type = oled-128x64", "display-order = " + order,
                   "oled-font = " + font],
            files=DSK)
        self.assertWaits(r, 1)
        return [line for line in r.lines if line and set(line) <= set(".#")]

    def test_wide_spinner(self):
        """The default wide spinner is enlarged in both dimensions."""
        for font, x, w, wide_x in (
                ("6x13", 85, 12, 98), ("8x16", 104, 16, 96)):
            with self.subTest(font=font):
                normal = self.pixels("0,1", font)
                wide = self.pixels("Sw,Cw", font)
                spinner = [row[x:x+w] for row in normal[16:32]]
                scaled = ["".join(c * 2 for c in row)
                          for row in spinner for _ in range(2)]
                self.assertEqual(
                    [row[wide_x:wide_x+2*w] for row in wide[0:32]], scaled)

    def test_led(self):
        """The phases on the first digit's lower segments, 1 the left one
        and 2 the middle one, then the track; the top segment while
        writing."""
        self.check("tm1651-3digit", [
            ([], "00 3f 3f"), (["key phase-in"] * 2, "10 3f 3f"),
            (["key phase-in"] * 2, "40 3f 06"), (["key write"], "41 3f 06")])

    def test_lcd(self):
        """The track, then the four phase bits as one uppercase hex digit;
        `W` before the track while writing."""
        self.check("hd44780-16x2", [
            ([], "T:00.0"), (["key phase-in"] * 2, "T:00.2"),
            (["key phase-in"] * 2, "T:01.4"), (["key write"], "W T:01.4")])


if __name__ == "__main__":
    unittest.main()

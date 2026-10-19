"""ffemu-multi: the host that the multi firmware detects at boot, shown by its
7-segment banner, and in Apple2 mode the image's track on the displays as the
host steps the head, and the write mark."""

import unittest

import ffemu_run as fr

DSK = {"TEST.DSK": bytes(35 * 4096)}


class Detection(fr.Case, unittest.TestCase):

    def check_banner(self, fdd_type, log, banner):
        r = fr.run(
            "ffemu-multi",
            ["wait 10000 log " + log, "key remove",
             "wait 5000 segments " + banner],
            settings=["display = tm1651-3digit", "fdd-type = " + fdd_type])
        self.assertWaits(r, 2)

    def test_apple2_host(self):
        """A-2 in place of F-F."""
        self.check_banner("apple2", "Apple2 on", "77 40 5b")

    def test_step_dir_host(self):
        self.check_banner("step-dir", "Apple2 off", "71 40 71")


class Apple2Mode(fr.Case, unittest.TestCase):
    """As with ffemu-apple2, each "phase-in" moves to the next state of a
    seek: phases 0 and 1, phase 1 (a half track in), phases 1 and 2, phase 2
    (a track in); the multi firmware shows the image's track, not the half
    tracks. A track takes two half tracks, with time between them for the
    firmware to see each."""

    TRACK = ["key phase-in"] * 2 + ["sleep 100"] + ["key phase-in"] * 2

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

    def test_led(self):
        """The track and the side's letter, k for side 0, v while
        writing."""
        self.check("tm1651-3digit", [
            ([], "00 3f 08"), (self.TRACK, "00 06 08"),
            (self.TRACK, "00 5b 08"), (["key write"], "00 5b 09")])

    def test_lcd(self):
        """The track after `T`, `W` before it while writing."""
        self.check("hd44780-16x2", [
            ([], "T:00.0"), (self.TRACK, "T:01.0"),
            (self.TRACK, "T:02.0"), (["key write"], "W T:02.0")])


if __name__ == "__main__":
    unittest.main()

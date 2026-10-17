"""An Apple2 host: what the multi firmware detects at boot, and the stepper phases on the
displays of the multi and the apple2 firmware as the host steps the head. The write mark of the
phase digit is not tested: ffemu's host does not write."""

import unittest

import ffemu_run as fr

DSK = {"TEST.DSK": bytes(35 * 4096)}


class Detection(fr.Case, unittest.TestCase):

    def check_banner(self, fdd_type, log, banner):
        r = fr.run("ffemu-multi", ["wait 10000 log " + log,
                                   "key remove",
                                   "wait 5000 segments " + banner],
                   settings=["display = tm1651-3digit", "fdd-type = " + fdd_type])
        self.assertWaits(r, 2)

    def test_apple2_host(self):
        self.check_banner("apple2", "Apple2 on", "77 40 5b")

    def test_shugart_host(self):
        self.check_banner("step-dir", "Apple2 off", "71 40 71")


class Phases(fr.Case, unittest.TestCase):
    """Phase 0 is on at rest; each "phase-in" moves to the next state of a seek: phases 0
    and 1, phase 1, phases 1 and 2, phase 2 (a half track in)."""

    def check_steps(self, name, display, steps):
        settings = ["display = " + display]
        if name == "ffemu-multi":
            settings.append("fdd-type = apple2")
        cmds = ["wait 10000 log Name: 'TEST'"]
        kind = "segments" if display.startswith("tm1651") else "display"
        for nr, shown in steps:
            cmds += ["key phase-in"] * nr
            cmds.append("wait 3000 %s %s" % (kind, shown))
        r = fr.run(name, cmds, settings=settings, files=DSK)
        self.assertWaits(r, 1 + len(steps))

    def test_multi_led(self):
        """The phase digit, then the image's track."""
        self.check_steps("ffemu-multi", "tm1651-3digit",
                         [(1, "18 3f 3f"), (1, "10 3f 3f"), (2, "40 3f 06")])

    def test_apple2_led(self):
        """The phase digit, then the cylinder in half tracks."""
        self.check_steps("ffemu-apple2", "tm1651-3digit",
                         [(1, "18 3f 3f"), (1, "10 3f 06"), (2, "40 3f 5b")])

    def test_multi_lcd(self):
        self.check_steps("ffemu-multi", "hd44780-16x2",
                         [(1, "01.. T00"), (1, ".1.. T00"), (2, "..2. T01")])

    def test_apple2_lcd(self):
        self.check_steps("ffemu-apple2", "hd44780-16x2",
                         [(1, "01.. T00"), (1, ".1.. T01"), (2, "..2. T02")])


if __name__ == "__main__":
    unittest.main()

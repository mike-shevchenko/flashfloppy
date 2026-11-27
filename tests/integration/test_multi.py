"""ffemu-multi: the host that the multi firmware detects at boot, shown by its
7-segment banner, and in Apple2 mode the image's track on the displays as the
host steps the head, and the write mark."""

import unittest

import ffemu_run as fr

DSK = {"TEST.DSK": bytes(35 * 4096)}


class Detection(fr.Case, unittest.TestCase):

    def check_banner(self, fdd_type, log, banner, interface="IBM PC", flash=()):
        commands = []
        if log:
            commands.append("wait 10000 log " + log)
        commands += [
            "wait 10000 log Intf " + interface,
            "key remove", "wait 5000 segments " + banner]
        r = fr.run(
            "ffemu-multi", commands,
            settings=["display = tm1651-3digit", "fdd-type = " + fdd_type],
            flash=flash)
        self.assertWaits(r, 3 if log else 2)
        return r

    def test_default_is_auto(self):
        r = fr.run(
            "ffemu-multi",
            ["wait 10000 log CFG: Factory Defaults",
             "wait 10000 log Apple2 on", "wait 10000 log Intf IBM PC"])
        self.assertWaits(r, 3)

    def test_apple2_host(self):
        """A-2 in place of F-F."""
        self.check_banner("apple2", "Apple2 on", "77 40 5b")

    def test_shugart_host(self):
        self.check_banner("shugart", "Apple2 off", "71 40 71")

    def test_jc_also_detects_apple2(self):
        self.check_banner(
            "apple2", "Apple2 on", "77 40 5b", interface="Shugart",
            flash=["interface = jc"])

    def test_forced_apple2_without_phases(self):
        r = self.check_banner(
            "shugart", None, "77 40 5b", interface="Shugart",
            flash=["interface = apple2"])
        self.assertNotIn("Apple2 on (", r.stdout)
        self.assertNotIn("Apple2 off (", r.stdout)

    def test_fixed_ibmpc_ignores_apple2_phases(self):
        r = fr.run(
            "ffemu-multi", ["wait 10000 log Intf IBM PC"],
            settings=["fdd-type = apple2"], flash=["interface = ibmpc"])
        self.assertWaits(r, 1)
        self.assertNotIn("Apple2 on (", r.stdout)
        self.assertNotIn("Apple2 off (", r.stdout)

    def test_unknown_interface_is_auto(self):
        self.check_banner(
            "apple2", "Apple2 on", "77 40 5b",
            flash=["interface = 0x06"])

    def test_interface_change_resets(self):
        r = fr.run(
            "ffemu-multi",
            ["sleep 6000", "wait 10000 log Intf Shugart", "key remove",
             "wait 5000 segments 77 40 5b", "flash"],
            settings=["display = tm1651-3digit", "fdd-type = shugart"],
            files={"FF.CFG": "interface = apple2\n"})
        self.assertWaits(r, 2)
        self.assertNotIn("Apple2 on (", r.stdout)
        self.assertNotIn("Apple2 off (", r.stdout)
        self.assertEqual(
            r.flash().get("interface"), "apple2", r.report())

    def test_explicit_auto_is_parsed(self):
        r = fr.run(
            "ffemu-multi",
            ["sleep 6000", "wait 10000 log Apple2 on", "key remove",
             "wait 5000 segments 77 40 5b", "flash"],
            settings=["display = tm1651-3digit", "fdd-type = apple2"],
            flash=["interface = ibmpc"],
            files={"FF.CFG": "interface = auto\n"})
        self.assertWaits(r, 2)
        self.assertEqual(r.flash().get("interface"), "auto", r.report())

    def test_explicit_jc_is_parsed(self):
        r = fr.run(
            "ffemu-multi",
            ["sleep 6000", "wait 10000 log Apple2 off",
             "wait 10000 log Intf Shugart", "key remove",
             "wait 5000 segments 71 40 71", "flash"],
            settings=["display = tm1651-3digit", "fdd-type = shugart"],
            flash=["interface = apple2"],
            files={"FF.CFG": "interface = jc\n"})
        self.assertWaits(r, 3)
        self.assertEqual(r.flash().get("interface"), "jc", r.report())


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

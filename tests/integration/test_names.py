"""Names of files and folders that are not ASCII, which the firmware holds in
the character set of inc/charset.h: a folder entered and an image mounted by
their long names in Cyrillic."""

import unittest

import ffemu_run as fr

# IGRY/IGRA.IMG in Cyrillic, a folder of the root and a PC image in it, which
# the selector enters and mounts by itself.
IGRA = {"\u0418\u0433\u0440\u044b/\u0438\u0433\u0440\u0430.img":
        bytes(737280)}


class Names(fr.Case, unittest.TestCase):

    def test_cyrillic_long_names(self):
        """The image mounted by its long name, which the console prints in
        the set's bytes, KOI8-R's, each a replacement character to the
        harness, which reads UTF-8."""
        r = fr.run("ffemu", ["wait 15000 log Name:", "log"], files=IGRA)
        self.assertWaits(r, 1)
        self.assertIn("Name: '%s' Type: img" % ("\ufffd" * 4), r.stdout,
                      r.report())


if __name__ == "__main__":
    unittest.main()

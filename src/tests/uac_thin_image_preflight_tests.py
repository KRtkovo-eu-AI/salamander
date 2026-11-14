# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-12 | Exercise test broker manifest preflight. | Reject non-laboratory execution levels without launching a process. | #731 | GPT-6 |
import unittest
from uac_thin_image_preflight import contains_marker, manifest_level


class PreflightTests(unittest.TestCase):
    def test_only_as_invoker_without_ui_access(self):
        def document(level, access):
            return ('<assembly xmlns="urn:schemas-microsoft-com:asm.v1"><trustInfo>'
                    '<requestedExecutionLevel level="%s" uiAccess="%s" />'
                    '</trustInfo></assembly>' % (level, access)).encode()
        self.assertEqual(manifest_level(document('asInvoker', 'false')), 'asInvoker')
        for level, access in [('requireAdministrator', 'false'), ('asInvoker', 'true'), ('highestAvailable', 'false')]:
            with self.assertRaises(ValueError):
                manifest_level(document(level, access))

    def test_marker_encodings(self):
        self.assertTrue(contains_marker(b'prefix-SAM_TEST-suffix', 'SAM_TEST'))
        self.assertTrue(contains_marker('SAM_TEST'.encode('utf-16le'), 'SAM_TEST'))
        self.assertFalse(contains_marker(b'other', 'SAM_TEST'))


if __name__ == '__main__':
    unittest.main()

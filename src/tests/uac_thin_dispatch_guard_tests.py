# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-12 | Check one-shot UI-thread F5 test dispatch. | Keep repeat and reentrant requests fail-closed. | #731 | GPT-6 |
from pathlib import Path
import unittest


class TestOneShotDispatchSource(unittest.TestCase):
    def test_one_shot_guard_precedes_validation_and_synchronous_dispatch(self):
        text = (Path(__file__).resolve().parents[1] / 'mainwnd3.cpp').read_text(encoding='utf-8')
        start = text.index('#ifdef SAM_UAC_THIN_COPY_TEST\n    case WM_APP + 512:')
        end = text.index('#endif', start)
        handler = text[start:end]
        guard = handler.index('static bool requested = false;')
        branch = handler.index('if (requested)', guard)
        rejected = handler.index('signal(L"SAM_UAC_TEST_COPY_REJECT_EVENT");', branch)
        returned = handler.index('return 0;', rejected)
        consumed = handler.index('requested = true;', returned)
        validation = handler.index('bool allowed = false;', consumed)
        dispatch = handler.index('SendMessage(HWindow, WM_COMMAND, CM_COPYFILES, 0);', validation)
        self.assertLess(guard, branch)
        self.assertLess(branch, rejected)
        self.assertLess(rejected, returned)
        self.assertLess(returned, consumed)
        self.assertLess(consumed, validation)
        self.assertLess(validation, dispatch)
        self.assertEqual(handler.count('SendMessage(HWindow, WM_COMMAND, CM_COPYFILES, 0);'), 1)


if __name__ == '__main__':
    unittest.main()

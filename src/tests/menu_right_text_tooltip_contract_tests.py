# SPDX-FileCopyrightText: 2026 Open Salamander Authors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Guard menu tooltip geometry, delayed callbacks and owner-data lifetime."""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(path):
    return (ROOT / path).read_text(encoding="utf-8-sig")


def body(text, name):
    start = text.index(name)
    start = text.index("{", start)
    depth = 1
    end = start + 1
    while depth:
        depth += (text[end] == "{") - (text[end] == "}")
        end += 1
    return text[start + 1:end - 1]


class MenuRightTextToolTipContracts(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.menu = read("src/menu2.cpp")
        cls.header = read("src/menu.h")

    def test_callback_is_opt_in_concrete_api_and_survives_window_creation(self):
        popup = self.header[self.header.index("class CMenuPopup :"):self.header.index("class CMenuBar :")]
        self.assertIn("void SetRightTextToolTip(", popup)
        self.assertNotRegex(popup, r"virtual[^;]*SetRightTextToolTip")
        self.assertIn("void* RightTextToolTipContext;", popup)
        self.assertIn("RightTextToolTipCallback = NULL", body(self.menu, "CMenuPopup::CMenuPopup"))
        self.assertNotIn("RightTextToolTipCallback =", body(self.menu, "CMenuPopup::Cleanup"))

    def test_hit_area_matches_rendered_right_column_and_visible_rows(self):
        hit = body(self.menu, "CMenuPopup::HitRightTextToolTip")
        draw = read("src/menu3.cpp")
        placement = "SharedRes->TextItemHeight + 1 + item->ColumnRX"
        self.assertIn(placement, hit)
        self.assertIn(placement, draw)
        self.assertIn("point.x < left + item->ColumnRWidth", hit)
        self.assertIn("HitTest(&point, &index) != mphItem", hit)
        for guard in ("MENU_STATE_GRAYED", "MENU_TYPE_OWNERDRAW", "ColumnRLen <= 0",
                      "ColumnRWidth <= 0", "ModifyMode", "Closing", "WindowFromPoint"):
            self.assertIn(guard, hit)

    def test_delayed_callback_revalidates_current_hit_and_item_id(self):
        proc = body(self.menu, "CMenuPopup::WindowProc")
        callback = proc[proc.index("case WM_USER_TTGETTEXT:"):proc.index("case WM_ERASEBKGND:")]
        self.assertIn("buffer[0] = 0", callback)
        self.assertIn("GetCursorPos(&cursor)", callback)
        self.assertIn("HitRightTextToolTip(cursor)", callback)
        self.assertIn("index == RightTextToolTipIndex", callback)
        self.assertIn("Items[index]->ID == (DWORD)wParam", callback)

    def test_owner_data_is_detached_before_uninit_and_after_track(self):
        proc = body(self.menu, "CMenuPopup::WindowProc")
        destroy = proc[proc.index("case WM_DESTROY:"):proc.index("case WM_MOUSELEAVE:")]
        notify = destroy.index("WM_USER_UNINITMENUPOPUP")
        for reset in ("ClearRightTextToolTip()", "RightTextToolTipCallback = NULL",
                      "RightTextToolTipContext = NULL"):
            self.assertLess(destroy.index(reset), notify)
        track = body(self.menu, "CMenuPopup::Track(")
        self.assertGreater(track.index("SetRightTextToolTip(NULL, NULL)"), track.index("TrackInternal("))

    def test_rebuild_rearms_stationary_hover_through_existing_modeless_timer(self):
        self.assertIn("ClearRightTextToolTips()", body(self.menu, "CMenuPopup::BeginModifyMode"))
        end = body(self.menu, "CMenuPopup::EndModifyMode")
        self.assertGreater(end.index("UpdateRightTextToolTip(cursor, TRUE)"), end.index("ModifyMode = FALSE"))
        rearm = body(read("src/tooltip.cpp"), "CToolTip::RearmCurrentToolTip")
        self.assertIn("if (IsModal)", rearm)
        self.assertIn("SetCurrentToolTip(NULL, 0, 0)", rearm)
        self.assertIn("LastCursorPos.x ^= 1", rearm)
        self.assertIn("SetCurrentToolTip(hNotifyWindow, id, showDelay)", rearm)
        self.assertNotIn("Show(", rearm)

    def test_input_dismissal_and_thread_timer_dispatch(self):
        self.assertIn("ClearRightTextToolTips()", body(self.menu, "CMenuPopup::HideAll"))
        wheel = body(self.menu, "CMenuPopup::OnMouseWheel")
        self.assertIn("ClearRightTextToolTips()", wheel)
        dispatch = body(self.menu, "CMenuPopup::DoDispatchMessage")
        for case in ("case WM_SYSCHAR:", "case WM_KEYDOWN:", "case WM_NCMBUTTONUP:"):
            part = dispatch[dispatch.index(case):]
            self.assertLess(part.index("ClearRightTextToolTips()"), part.index("return;"))
        self.assertIn("msg->hwnd == HWindow && msg->wParam == UPDOWN_TIMER_ID", dispatch)
        self.assertIn("DispatchMessage(msg)", dispatch)


if __name__ == "__main__":
    unittest.main()

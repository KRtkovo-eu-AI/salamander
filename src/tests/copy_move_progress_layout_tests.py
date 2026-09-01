# SPDX-FileCopyrightText: 2026 Open Salamander Authors
# SPDX-License-Identifier: GPL-2.0-or-later

from dataclasses import dataclass

TRANSFER_ROW_DLU = 50
CONFLICT_EXTENSION_DLU = 102
DIALOG_Y_SCALE = 2


def map_dialog_y(dlu: int) -> int:
    return dlu * DIALOG_Y_SCALE


CONFLICT_EXTENSION_PX = map_dialog_y(CONFLICT_EXTENSION_DLU)


@dataclass(frozen=True)
class Template:
    total: int
    status: int
    link: int | None
    table: int | None
    actions: int | None
    footer: int
    height: int


LEGACY = Template(56, 69, None, None, None, 87, 110)
SCAN_AHEAD = Template(56, 69, 84, 96, 182, 202, 225)


def px(value: int | None) -> int | None:
    return None if value is None else map_dialog_y(value)


def layout(template: Template, streams: int, expanded: bool = False) -> Template:
    # Template controls and dialog bounds are captured in pixels. Only the
    # transfer increment starts in DLU and needs MapDialogRect here.
    transfer_px = map_dialog_y((streams - 1) * TRANSFER_ROW_DLU)
    conflict_px = -CONFLICT_EXTENSION_PX if template is SCAN_AHEAD and not expanded else 0
    return Template(px(template.total) + transfer_px, px(template.status) + transfer_px,
                    None if template.link is None else px(template.link) + transfer_px,
                    None if template.table is None else px(template.table) + transfer_px,
                    None if template.actions is None else px(template.actions) + transfer_px,
                    px(template.footer) + transfer_px + conflict_px,
                    px(template.height) + transfer_px + conflict_px)


def test_two_physical_template_baselines() -> None:
    assert LEGACY.footer == 87 and LEGACY.height == 110
    assert SCAN_AHEAD.footer == 202 and SCAN_AHEAD.height == 225
    assert LEGACY.link is None and SCAN_AHEAD.link == 84


def test_conflict_extension_is_mapped_exactly_once() -> None:
    captured_extension_px = map_dialog_y(CONFLICT_EXTENSION_DLU)
    expanded = layout(SCAN_AHEAD, 4, True)
    collapsed = layout(SCAN_AHEAD, 4, False)
    assert expanded.footer - collapsed.footer == captured_extension_px
    assert expanded.height - collapsed.height == captured_extension_px
    assert captured_extension_px != map_dialog_y(captured_extension_px)


def test_pixel_dlu_ordering_for_all_stream_counts() -> None:
    for streams in (1, 2, 4, 8):
        stream_bar_bottom = map_dialog_y(52 + (streams - 1) * TRANSFER_ROW_DLU)
        legacy = layout(LEGACY, streams)
        assert stream_bar_bottom < legacy.total < legacy.status < legacy.footer
        assert legacy.footer + map_dialog_y(14) < legacy.height
        for expanded in (False, True):
            scan = layout(SCAN_AHEAD, streams, expanded)
            assert stream_bar_bottom < scan.total < scan.status < scan.link
            if expanded:
                assert scan.link < scan.table < scan.actions < scan.footer
            else:
                assert scan.link < scan.footer
            assert scan.footer + map_dialog_y(14) < scan.height


def test_four_stream_collapsed_footer_follows_total_and_fourth_bar() -> None:
    scan = layout(SCAN_AHEAD, 4, False)
    fourth_bar_bottom = map_dialog_y(52 + 3 * TRANSFER_ROW_DLU)
    assert fourth_bar_bottom < scan.total
    assert scan.total < scan.status < scan.link < scan.footer
    assert scan.footer + map_dialog_y(14) < scan.height


def test_only_conflict_extension_changes_on_toggle() -> None:
    for streams in (1, 2, 4, 8):
        collapsed = layout(SCAN_AHEAD, streams, False)
        expanded = layout(SCAN_AHEAD, streams, True)
        assert collapsed.total == expanded.total
        assert collapsed.status == expanded.status
        assert collapsed.link == expanded.link
        assert expanded.footer - collapsed.footer == CONFLICT_EXTENSION_PX
        assert expanded.height - collapsed.height == CONFLICT_EXTENSION_PX


if __name__ == '__main__':
    test_two_physical_template_baselines()
    test_conflict_extension_is_mapped_exactly_once()
    test_pixel_dlu_ordering_for_all_stream_counts()
    test_four_stream_collapsed_footer_follows_total_and_fourth_bar()
    test_only_conflict_extension_changes_on_toggle()
    print('copy_move_progress_layout_tests: ok')

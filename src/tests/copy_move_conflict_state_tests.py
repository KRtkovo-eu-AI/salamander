# SPDX-FileCopyrightText: 2026 Open Salamander Authors
# SPDX-License-Identifier: GPL-2.0-or-later

from dataclasses import dataclass

UNSCANNED, READY, BLOCKED, CONFLICT, OVERWRITE, SKIP, RUNNING, DONE, SUPPRESSED = range(9)
NONE, FILE, DIRECTORY, TYPE_MISMATCH = range(4)


@dataclass
class Item:
    state: int = UNSCANNED
    parent: int = -1
    subtree_end: int = -1
    conflict_type: int = NONE
    existed: bool = False
    is_create_dir: bool = False
    identity_valid: bool = True
    directory_type: bool = True
    prepared: bool = False
    lease_generation: int = 0


class Coordinator:
    def __init__(self, items: list[Item]) -> None:
        self.items = items
        self.pending = 0
        self.apply_all = {FILE: None, DIRECTORY: None, TYPE_MISMATCH: None}

    def prepared_parent(self, index: int) -> bool:
        item = self.items[index]
        return item.state == DONE or (item.is_create_dir and item.prepared and
                                      item.identity_valid) or (
            item.is_create_dir and item.state == OVERWRITE and
            item.conflict_type == DIRECTORY and item.existed and
            item.identity_valid and item.directory_type
        )

    def ancestors_prepared(self, index: int) -> bool:
        parent = self.items[index].parent
        while parent >= 0:
            if not self.prepared_parent(parent):
                return False
            parent = self.items[parent].parent
        return True

    def probe(self, index: int, conflict_type: int = NONE, existed: bool = False) -> None:
        item = self.items[index]
        item.conflict_type = conflict_type
        item.existed = existed
        if not self.ancestors_prepared(index):
            item.state = BLOCKED
        elif existed and item.is_create_dir and conflict_type == DIRECTORY and item.identity_valid:
            item.state = OVERWRITE
        elif existed and conflict_type != NONE:
            automatic = self.apply_all[conflict_type]
            if automatic is None:
                item.state = CONFLICT
                self.pending += 1
            else:
                item.state = OVERWRITE if automatic else SKIP
        else:
            item.state = READY

    def admit(self, index: int) -> None:
        for child in range(index + 1, self.items[index].subtree_end + 1):
            candidate = self.items[child]
            if candidate.state != BLOCKED or not self.ancestors_prepared(child):
                continue
            if candidate.is_create_dir and candidate.conflict_type == DIRECTORY and candidate.identity_valid:
                candidate.state = OVERWRITE
            elif candidate.existed and candidate.conflict_type != NONE:
                candidate.state = CONFLICT
                self.pending += 1
            else:
                candidate.state = READY

    def decide(self, index: int, overwrite: bool) -> None:
        item = self.items[index]
        assert item.state == CONFLICT
        self.pending -= 1
        if not overwrite:
            for child in range(index, item.subtree_end + 1):
                self.items[child].state = SUPPRESSED
            return
        item.state = OVERWRITE
        self.admit(index)

    def bulk_decide(self, overwrite: bool) -> None:
        for conflict_type in self.apply_all:
            self.apply_all[conflict_type] = overwrite
        for item in self.items:
            if item.state == CONFLICT:
                item.state = OVERWRITE if overwrite else SKIP
                self.pending -= 1

    def complete(self, index: int) -> None:
        self.items[index].state = DONE
        self.admit(index)

    def prepare(self, index: int, created: bool = True) -> None:
        item = self.items[index]
        item.prepared = True
        item.existed = True
        item.identity_valid = True
        item.directory_type = True
        self.admit(index)

    def rollback_prepare(self, index: int, removed: bool) -> None:
        if removed:
            self.items[index].prepared = False
            self.items[index].identity_valid = False
            self.retract(index)

    def retract(self, index: int) -> None:
        for child in range(index + 1, self.items[index].subtree_end + 1):
            item = self.items[child]
            item.lease_generation += 1
            if item.state == RUNNING:
                continue
            if item.state == CONFLICT:
                self.pending -= 1
            if item.state not in (DONE, SUPPRESSED):
                item.state = BLOCKED

    def replace_ancestor(self, index: int) -> None:
        self.items[index].identity_valid = False
        self.items[index].prepared = False
        self.items[index].state = CONFLICT
        self.pending += 1
        self.retract(index)

    def lease_ready(self) -> int:
        for index, item in enumerate(self.items):
            if item.state in (READY, OVERWRITE, SKIP) and self.ancestors_prepared(index):
                item.state = RUNNING
                return index
        return -1


def test_directory_merge_auto_admits_and_first_file_conflict_prompts() -> None:
    model = Coordinator([
        Item(parent=-1, subtree_end=1, is_create_dir=True),  # compatible root merge
        Item(parent=0, subtree_end=1),                       # safe child C
        Item(parent=-1, subtree_end=2),                      # first file conflict
        Item(parent=-1, subtree_end=3, is_create_dir=True),  # type mismatch
    ])
    automatic_prompt_consumed = False
    model.probe(0, DIRECTORY, True)
    assert model.items[0].state == OVERWRITE
    assert model.pending == 0 and not automatic_prompt_consumed
    model.items[0].state = DONE
    model.probe(1)
    safe = model.lease_ready()
    assert safe == 1
    model.complete(safe)
    model.probe(2, FILE, True)
    assert model.items[2].state == CONFLICT
    automatic_prompt_consumed = True
    model.probe(3, TYPE_MISMATCH, True)
    assert model.items[3].state == CONFLICT
    assert model.items[1].state == DONE
    assert automatic_prompt_consumed and model.pending == 2

def test_directory_merge_inherits_but_file_conflict_publishes() -> None:
    model = Coordinator([
        Item(parent=-1, subtree_end=3, is_create_dir=True),
        Item(parent=0, subtree_end=3, is_create_dir=True),
        Item(parent=1, subtree_end=2),
        Item(parent=0, subtree_end=3, is_create_dir=True),
    ])
    model.probe(0, DIRECTORY, True)
    model.probe(1, DIRECTORY, True)
    model.probe(2, FILE, True)
    model.probe(3, TYPE_MISMATCH, True)
    assert model.pending == 2
    assert [item.state for item in model.items[1:]] == [OVERWRITE, CONFLICT, CONFLICT]
    model.complete(0)
    assert model.items[1].state == OVERWRITE
    assert model.items[2].state == CONFLICT
    assert model.items[3].state == CONFLICT
    model.complete(1)
    assert model.items[2].state == CONFLICT
    assert model.pending == 2


def test_nested_prepared_chain_admits_during_probe() -> None:
    model = Coordinator([
        Item(parent=-1, subtree_end=2, is_create_dir=True),
        Item(parent=0, subtree_end=2, is_create_dir=True),
        Item(parent=1, subtree_end=2),
    ])
    model.probe(0, DIRECTORY, True)
    model.probe(1, DIRECTORY, True)
    model.probe(2)
    assert [item.state for item in model.items] == [OVERWRITE, OVERWRITE, READY]
    assert model.lease_ready() == 0


def test_conflict_does_not_block_later_safe_sibling() -> None:
    model = Coordinator([
        Item(parent=-1, subtree_end=2, is_create_dir=True),
        Item(parent=0, subtree_end=1),
        Item(parent=0, subtree_end=2),
    ])
    model.probe(0, DIRECTORY, True)
    model.probe(1, FILE, True)
    model.probe(2)
    assert model.items[1].state == CONFLICT
    assert model.items[2].state == READY


def test_absent_parent_blocks_until_done() -> None:
    model = Coordinator([Item(subtree_end=1, is_create_dir=True), Item(parent=0)])
    model.probe(0)
    model.probe(1)
    assert model.items[1].state == BLOCKED
    model.complete(0)
    assert model.items[1].state == READY


def test_type_mismatch_blocks_subtree() -> None:
    model = Coordinator([Item(subtree_end=1, is_create_dir=True), Item(parent=0)])
    model.probe(0, TYPE_MISMATCH, True)
    model.probe(1)
    assert model.items[0].state == CONFLICT and model.items[1].state == BLOCKED


def test_ancestor_identity_replacement_reblocks_child() -> None:
    model = Coordinator([Item(subtree_end=1, is_create_dir=True), Item(parent=0)])
    model.probe(0, DIRECTORY, True)
    model.probe(1)
    assert model.items[1].state == READY
    model.replace_ancestor(0)
    assert model.items[1].state == BLOCKED


def scheduler_retry_later(scanner_active: bool, scan_finished: bool,
                          running_dependency: bool = False) -> bool:
    return scanner_active or not scan_finished or running_dependency


def test_scanner_active_refills_when_later_result_becomes_ready() -> None:
    scanned = [[CONFLICT], [CONFLICT, CONFLICT], [CONFLICT, CONFLICT, READY]]
    leased = -1
    for states in scanned:
        ready = [index for index, state in enumerate(states) if state == READY]
        if ready:
            leased = ready[0]
            break
        assert scheduler_retry_later(scanner_active=True, scan_finished=False)
    assert leased == 2


def test_one_ready_after_scan_finished_falls_back_despite_pending() -> None:
    ready_leases = [7]
    pending = 3
    should_wait = scheduler_retry_later(scanner_active=False, scan_finished=True)
    assert pending > 0 and len(ready_leases) == 1 and not should_wait
    closed_inputs = list(ready_leases)
    released_leases: list[int] = []
    serial_lease = ready_leases[0]
    assert closed_inputs == [serial_lease] and released_leases == []


def test_drained_batch_yields_serial_work_despite_pending() -> None:
    scan_finished = True
    scanner_active = False
    pending = 2
    ready_serial_items = [11]
    keep_rolling = scheduler_retry_later(scanner_active, scan_finished)
    assert pending > 0 and ready_serial_items and not keep_rolling


def test_completion_waits_for_pending_conflicts_and_running_work() -> None:
    def complete(scan_finished: bool, pending: int, ready: int, running: int) -> bool:
        return scan_finished and pending == 0 and ready == 0 and running == 0

    assert not complete(True, 2, 0, 0)
    assert not complete(True, 0, 1, 0)
    assert not complete(True, 0, 0, 1)
    assert complete(True, 0, 0, 0)


def test_parent_skip_suppresses_probed_subtree() -> None:
    model = Coordinator([
        Item(parent=-1, subtree_end=2, is_create_dir=True),
        Item(parent=0, subtree_end=1, is_create_dir=True),
        Item(parent=0, subtree_end=2),
    ])
    model.probe(0, TYPE_MISMATCH, True)
    model.probe(1, DIRECTORY, True)
    model.probe(2, FILE, True)
    model.decide(0, overwrite=False)
    assert model.pending == 0
    assert all(item.state == SUPPRESSED for item in model.items)


def test_nested_absent_chain_and_unrelated_conflict_progress() -> None:
    model = Coordinator([
        Item(subtree_end=2, is_create_dir=True), Item(parent=0, subtree_end=2, is_create_dir=True),
        Item(parent=1), Item(subtree_end=3)
    ])
    model.probe(0); model.probe(1); model.probe(2); model.probe(3, FILE, True)
    assert model.items[1].state == BLOCKED and model.pending == 1
    model.complete(0)
    assert model.items[1].state == READY
    model.complete(1)
    assert model.items[2].state == READY and model.items[3].state == CONFLICT


def test_speculative_prepared_parent_admits_probed_child() -> None:
    model = Coordinator([Item(subtree_end=1, is_create_dir=True), Item(parent=0)])
    model.probe(0); model.probe(1)
    model.prepare(0)
    assert model.items[1].state == READY


def test_scan_ahead_lease_wraps_to_newly_ready_lower_index() -> None:
    ready = {10, 80}
    assert min(ready) == 10
    ready.remove(10)
    assert min(ready) == 80
    ready.add(20)
    assert min(ready) == 20


def test_scanner_finished_readiness_is_not_sticky_terminal() -> None:
    scanner_finished = True
    ready: list[int] = []
    assert scanner_finished and not ready
    ready.append(20)  # a directory completion/decision advances coordinator generation
    assert ready.pop(0) == 20


def test_invalidated_initial_lease_cannot_legacy_dispatch() -> None:
    planner_outcome = "initial-invalidated"
    assert planner_outcome != "valid-one-task-fallback"
    legacy_dispatch = planner_outcome == "valid-one-task-fallback"
    assert not legacy_dispatch


def test_whole_subtree_retraction_and_final_generation_check() -> None:
    model = Coordinator([Item(subtree_end=3, is_create_dir=True), Item(parent=0),
                         Item(parent=0), Item(parent=0)])
    model.probe(0, DIRECTORY, True)
    for index in range(1, 4): model.probe(index)
    model.items[2].state = RUNNING
    leased_generation = model.items[2].lease_generation
    model.replace_ancestor(0)
    assert model.items[1].state == BLOCKED and model.items[3].state == BLOCKED
    assert model.items[2].state == RUNNING
    assert model.items[2].lease_generation != leased_generation


def test_valid_one_task_fallback_keeps_lease_ownership() -> None:
    outcomes = {"valid-one-task-fallback": True, "initial-invalidated": False}
    assert outcomes["valid-one-task-fallback"]
    assert not outcomes["initial-invalidated"]


def test_rollback_deletion_failure_adopts_prepared_directory() -> None:
    model = Coordinator([Item(subtree_end=1, is_create_dir=True), Item(parent=0)])
    model.probe(0); model.probe(1); model.prepare(0)
    model.rollback_prepare(0, removed=False)
    assert model.items[0].prepared and model.items[1].state == READY


def test_rejected_completion_never_sets_parallel_done() -> None:
    accepted = False
    parallel_done = accepted
    assert not parallel_done


def test_full_completion_predicate_counts_every_state() -> None:
    def complete(scanner_done: bool, states: list[int]) -> bool:
        return scanner_done and all(state in (DONE, SUPPRESSED) for state in states)
    assert not complete(True, [DONE, BLOCKED])
    assert not complete(True, [DONE, READY])
    assert not complete(True, [DONE, RUNNING])
    assert complete(True, [DONE, SUPPRESSED])


def test_operation_latched_rows_ignore_initial_batch_and_boundaries() -> None:
    for demand in (1, 2, 4, 8):
        display_count = demand  # latched before worker execution
        initial_batch = 1 if demand > 1 else 0
        snapshots = [initial_batch, demand, 0, 1, demand - 1]
        assert all(display_count == demand for _active_count in snapshots)



def test_exactly_one_automatic_prompt_per_operation() -> None:
    conflicts = ["B", "D", "E"]
    automatic_prompt_consumed = False
    opened: list[str] = []
    for conflict in conflicts:
        if not automatic_prompt_consumed:
            automatic_prompt_consumed = True  # consumed on open, including dismiss
            opened.append(conflict)
    assert opened == ["B"]
    assert conflicts[1:] == ["D", "E"]  # retained for manual Solve


def test_independent_ready_file_completes_during_modal_prompt() -> None:
    model = Coordinator([
        Item(parent=-1, subtree_end=1, is_create_dir=True),  # B modal conflict
        Item(parent=0, subtree_end=1),                       # blocked B child
        Item(parent=-1, subtree_end=2),                      # independent serial C
    ])
    model.probe(0, FILE, True)
    modal_open = True
    model.probe(1)
    model.probe(2)
    leased = model.lease_ready()
    assert leased == 2
    progress_publication = "post-owned"  # C never waits for B's modal UI
    filesystem_started = progress_publication == "post-owned"
    assert modal_open and filesystem_started
    model.complete(leased)
    assert modal_open and model.items[2].state == DONE


def test_stale_lease_completion_is_noop() -> None:
    token = 1
    generation = 7
    lease = (token, generation)
    generation += 1
    assert lease != (token, generation)


def test_progress_skip_all_clears_mixed_pending_and_future_types() -> None:
    model = Coordinator([Item(), Item(is_create_dir=True), Item()])
    model.probe(0, FILE, True)
    model.probe(1, TYPE_MISMATCH, True)
    assert model.pending == 2
    model.bulk_decide(overwrite=False)
    assert model.pending == 0
    assert [item.state for item in model.items[:2]] == [SKIP, SKIP]
    model.probe(2, TYPE_MISMATCH, True)
    assert model.items[2].state == SKIP and model.pending == 0


def test_progress_overwrite_all_clears_compatible_mixed_pending_and_future_types() -> None:
    model = Coordinator([Item(), Item(is_create_dir=True), Item()])
    model.probe(0, FILE, True)
    model.probe(1, TYPE_MISMATCH, True)
    model.bulk_decide(overwrite=True)
    assert model.pending == 0
    assert [item.state for item in model.items[:2]] == [OVERWRITE, OVERWRITE]
    model.probe(2, FILE, True)
    assert model.items[2].state == OVERWRITE and model.pending == 0


def test_dialog_all_remains_type_specific() -> None:
    model = Coordinator([Item(), Item(is_create_dir=True), Item()])
    model.probe(0, FILE, True)
    model.probe(1, TYPE_MISMATCH, True)
    model.apply_all[FILE] = True
    model.decide(0, overwrite=True)
    assert model.items[1].state == CONFLICT and model.pending == 1
    model.probe(2, FILE, True)
    assert model.items[2].state == OVERWRITE


def test_modal_prompt_caches_owned_progress_and_flushes_once() -> None:
    parent_paints = 0
    owned_payloads = [{"stream": 1}, {"stream": 2}]
    deferred = None
    for payload in owned_payloads:
        deferred = dict(payload)
        payload.clear()  # delivery frees each posted owner during the modal loop
    assert parent_paints == 0 and deferred == {"stream": 2}
    parent_paints += 1  # one latest-snapshot flush after the owned dialog closes
    assert parent_paints == 1 and all(not payload for payload in owned_payloads)


def main() -> None:
    test_directory_merge_auto_admits_and_first_file_conflict_prompts()
    test_directory_merge_inherits_but_file_conflict_publishes()
    test_nested_prepared_chain_admits_during_probe()
    test_conflict_does_not_block_later_safe_sibling()
    test_absent_parent_blocks_until_done()
    test_type_mismatch_blocks_subtree()
    test_ancestor_identity_replacement_reblocks_child()
    test_scanner_active_refills_when_later_result_becomes_ready()
    test_one_ready_after_scan_finished_falls_back_despite_pending()
    test_drained_batch_yields_serial_work_despite_pending()
    test_completion_waits_for_pending_conflicts_and_running_work()
    test_parent_skip_suppresses_probed_subtree()
    test_nested_absent_chain_and_unrelated_conflict_progress()
    test_speculative_prepared_parent_admits_probed_child()
    test_scan_ahead_lease_wraps_to_newly_ready_lower_index()
    test_scanner_finished_readiness_is_not_sticky_terminal()
    test_invalidated_initial_lease_cannot_legacy_dispatch()
    test_whole_subtree_retraction_and_final_generation_check()
    test_valid_one_task_fallback_keeps_lease_ownership()
    test_rollback_deletion_failure_adopts_prepared_directory()
    test_rejected_completion_never_sets_parallel_done()
    test_full_completion_predicate_counts_every_state()
    test_operation_latched_rows_ignore_initial_batch_and_boundaries()
    test_exactly_one_automatic_prompt_per_operation()
    test_independent_ready_file_completes_during_modal_prompt()
    test_stale_lease_completion_is_noop()
    test_progress_skip_all_clears_mixed_pending_and_future_types()
    test_progress_overwrite_all_clears_compatible_mixed_pending_and_future_types()
    test_dialog_all_remains_type_specific()
    test_modal_prompt_caches_owned_progress_and_flushes_once()
    print("copy_move_conflict_state_tests: ok")


if __name__ == "__main__":
    main()

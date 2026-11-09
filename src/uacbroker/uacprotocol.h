// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-09 | Version F6 completion progress. | Pair the host with a broker that reports existing-copy verification. | | Codex |
// | 2026-11-09 | Version the progress total field. | Keep the host and broker on the same wire format. | | GPT-6.1 Sol (Codex) |
// | 2026-11-09 | Version the transfer progress flag. | Keep paired binaries compatible with the request layout. | | GPT-6.1 Sol (Codex) |
// | 2026-11-09 | Version the combined compressed F5 request. | Pair the host and broker that understand the new consent scope. | | GPT-6.1 Sol (Codex) |
// | 2026-11-09 | Version unrestricted active F6 transfers. | Keep old size and deadline contracts from mixing with the new host. | | GPT-6.1 Sol (Codex) |
// | 2026-11-09 | Version the Find content revision reply. | Reject mixed builds before opening a protected Find stream. | | GPT-6.1 Sol (Codex) |
// | 2026-11-09 | Define separate image roles. | Identify the broker, application and probe before launch. | | GPT-6.1 Sol (Codex) |
// | 2026-11-09 | Share the UAC protocol marker. | Keep the handshake and executable resource on one version. | | GPT-6.1 Sol (Codex) |
#pragma once

#define SAM_UAC_PROTOCOL_VERSION 11
#define SAM_UAC_PROTOCOL_RESOURCE 63001
#define SAM_UAC_IMAGE_HOST 1
#define SAM_UAC_IMAGE_BROKER 2
#define SAM_UAC_IMAGE_PROBE 3

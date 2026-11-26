# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-12 | Check laboratory image roles and broker manifest. | Fail closed before any real-host test is enabled. | #731 | GPT-6 |
"""Preflight for explicitly identified test images; this launches no app."""
import hashlib
import os
from pathlib import Path
import subprocess
import tempfile
import xml.etree.ElementTree as ET


def manifest_level(data):
    root = ET.fromstring(data)
    matches = [node for node in root.iter() if node.tag.rsplit('}', 1)[-1] == 'requestedExecutionLevel']
    if len(matches) != 1 or matches[0].get('level') != 'asInvoker' or \
            matches[0].get('uiAccess', '').lower() != 'false':
        raise ValueError('test broker manifest must request asInvoker with uiAccess=false')
    return 'asInvoker'


def contains_marker(data, marker):
    return marker.encode('utf-16le') in data or marker.encode('ascii') in data


def validate_images(host, broker, host_hash, broker_hash, mt):
    host, broker, mt = map(Path, (host, broker, mt))
    if host.name.lower() != 'salamand.exe' or broker.name.lower() != 'uacbroker.exe':
        raise ValueError('host and broker image roles or names do not match')
    if not host.is_file() or not broker.is_file() or host.is_symlink() or broker.is_symlink():
        raise ValueError('host and broker must be regular test files')
    if not host_hash or not broker_hash:
        raise ValueError('explicit verified hashes are required for both test images')
    host_data, broker_data = host.read_bytes(), broker.read_bytes()
    actual_host = hashlib.sha256(host_data).hexdigest()
    actual_broker = hashlib.sha256(broker_data).hexdigest()
    if actual_host != host_hash.lower() or actual_broker != broker_hash.lower() or actual_host == actual_broker:
        raise ValueError('image hashes do not match verified, distinct test binaries')
    if not contains_marker(host_data, 'SAM_UAC_TEST_ISOLATION_NONCE') or \
            contains_marker(broker_data, 'SAM_UAC_TEST_ISOLATION_NONCE'):
        raise ValueError('test host and broker isolation markers do not match their roles')
    if not all(contains_marker(data, 'SAM_UAC_THIN_UNELEVATED_TEST') and
               contains_marker(data, 'SAM_UAC_THIN_TEST_INSTALL') for data in (host_data, broker_data)):
        raise ValueError('both images must include the laboratory-only thin-peer seam')
    kits = Path(os.environ.get('ProgramFiles(x86)', r'C:\Program Files (x86)')) / 'Windows Kits'
    if mt.name.lower() != 'mt.exe' or not mt.is_file() or not mt.resolve().is_relative_to(kits.resolve()):
        raise ValueError('a verified Windows SDK mt.exe is required')
    with tempfile.TemporaryDirectory(prefix='uac-thin-manifest-') as directory:
        manifest = Path(directory) / 'broker.manifest'
        result = subprocess.run([str(mt), '-nologo', '-inputresource:' + str(broker) + ';#1',
                                 '-out:' + str(manifest)], capture_output=True, timeout=15)
        if result.returncode or not manifest.is_file():
            raise ValueError('failed to extract the laboratory broker manifest')
        level = manifest_level(manifest.read_bytes())
    return {'HostSha256': actual_host, 'BrokerSha256': actual_broker, 'BrokerExecutionLevel': level}

from __future__ import annotations

import importlib.util
import json
import tempfile
import unittest
from pathlib import Path


SCRIPT = Path(__file__).resolve().parents[1] / "update_stable_plugin_catalog.py"
SPEC = importlib.util.spec_from_file_location("update_stable_plugin_catalog", SCRIPT)
assert SPEC and SPEC.loader
UPDATER = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(UPDATER)


class CatalogUpdaterTests(unittest.TestCase):
    def test_nested_installer_plugin_path_uses_spl_filename_as_id(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            installer = Path(directory) / "setup.iss"
            installer.write_text(
                'Source: "{#PayloadDir}\\plugins\\extension-runtimes\\pythonruntime\\pythonruntime.spl"; '
                "DestDir: \"{app}\\plugins\\extension-runtimes\\pythonruntime\"\n",
                encoding="utf-8",
            )

            self.assertEqual(UPDATER.parse_installer_plugins(installer), ["pythonruntime"])

    def test_installer_versions_update_with_configuration_string(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            plugins_root = Path(directory)
            plugin_root = plugins_root / "zip"
            plugin_root.mkdir()
            (plugin_root / "versinfo.rh2").write_text(
                "\n".join(
                    (
                        "#define VERSINFO_MAJOR 1",
                        "#define VERSINFO_MINORA 9",
                        "#define VERSINFO_MINORB 0",
                        '#define VERSINFO_BETAVERSION_TXT_NO_PLATFORM ""',
                        '#define VERSINFO_DESCRIPTION "ZIP archiver"',
                    )
                ),
                encoding="utf-8",
            )
            installer = (
                "  AddPlugin('zip', 'ZIP', '1.8 (x64)', "
                "'base,standard,advanced,expert');\n"
            )

            updated = UPDATER.update_installer_plugin_versions(
                installer, ["zip"], plugins_root
            )

            self.assertEqual(
                updated,
                "  AddPlugin('zip', 'ZIP', '1.9 (x64)', "
                "'base,standard,advanced,expert');\n",
            )

    def test_existing_stable_membership_wins_and_new_package_defaults_to_stable(self) -> None:
        stable = Path("plugins-stable.json")
        runtimes = Path("extension-runtimes.json")
        unofficial = Path("plugins-unofficial.json")
        catalogs = {
            stable: {"plugins": [{"id": "tar"}, {"id": "sftp"}]},
            runtimes: {"plugins": [{"id": "pythonruntime"}]},
            unofficial: {"plugins": [{"id": "tar"}]},
        }

        assignments = UPDATER.assign_packages_to_catalogs(
            catalogs, stable, ["tar", "sftp", "pythonruntime", "new-plugin"]
        )

        self.assertEqual(assignments[stable], ["tar", "sftp", "new-plugin"])
        self.assertEqual(assignments[runtimes], ["pythonruntime"])
        self.assertNotIn(unofficial, assignments)

    def test_manifest_extension_moves_to_extensions_catalog(self) -> None:
        stable = Path("plugins-stable.json")
        extensions = Path("extensions-stable.json")
        runtimes = Path("extension-runtimes.json")
        catalogs = {
            stable: {"plugins": []},
            extensions: {"plugins": []},
            runtimes: {"plugins": [{"id": "salamatrixdemos"}]},
        }

        assignments = UPDATER.assign_packages_to_catalogs(
            catalogs,
            stable,
            ["salamatrixdemos"],
            extension_ids={"salamatrixdemos"},
            extension_catalog=extensions,
        )

        self.assertEqual(assignments, {extensions: ["salamatrixdemos"]})

    def test_plugin_beta_suffix_is_preserved(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            plugins_root = Path(directory)
            plugin_root = plugins_root / "sftp"
            plugin_root.mkdir()
            (plugin_root / "versinfo.rh2").write_text(
                "\n".join(
                    (
                        "#define VERSINFO_MAJOR 1",
                        "#define VERSINFO_MINORA 0",
                        "#define VERSINFO_MINORB 1",
                        '#define VERSINFO_BETAVERSION_TXT_NO_PLATFORM " beta"',
                        '#define VERSINFO_DESCRIPTION "SFTP client"',
                    )
                ),
                encoding="utf-8",
            )

            version, description = UPDATER.read_plugin_metadata("sftp", plugins_root)

            self.assertEqual(version, "1.0.1 beta (x64)")
            self.assertEqual(description, "SFTP client")

    def test_plugin_zero_patch_version_is_omitted(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            plugins_root = Path(directory)
            plugin_root = plugins_root / "zip"
            plugin_root.mkdir()
            (plugin_root / "versinfo.rh2").write_text(
                "\n".join(
                    (
                        "#define VERSINFO_MAJOR 1",
                        "#define VERSINFO_MINORA 7",
                        "#define VERSINFO_MINORB 0",
                        '#define VERSINFO_BETAVERSION_TXT_NO_PLATFORM ""',
                        '#define VERSINFO_DESCRIPTION "ZIP archiver"',
                    )
                ),
                encoding="utf-8",
            )

            version, description = UPDATER.read_plugin_metadata("zip", plugins_root)

            self.assertEqual(version, "1.7 (x64)")
            self.assertEqual(description, "ZIP archiver")

    def test_manifest_extension_is_added_with_version_and_runtime_dependency(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            extension_root = root / "extensions"
            manifest_root = extension_root / "extension-menu-builder"
            manifest_root.mkdir(parents=True)
            (manifest_root / "extension.json").write_text(
                json.dumps(
                    {
                        "name": "Extension Menu Builder",
                        "version": "1.0.0",
                        "description": "Create menu-driven extensions.",
                        "runtime": "PowerShell",
                    }
                ),
                encoding="utf-8",
            )

            updated = UPDATER.update_catalog(
                {"generatedAt": "old", "plugins": []},
                ["extension-menu-builder"],
                root / "plugins",
                "fixed",
                extension_ids={"extension-menu-builder"},
                extensions_root=extension_root,
            )

            self.assertEqual(updated["generatedAt"], "fixed")
            self.assertEqual(updated["schemaVersion"], 6)
            self.assertEqual(updated["plugins"][0]["packageType"], "extension")
            self.assertEqual(updated["plugins"][0]["latestVersion"], "1.0.0 (x64)")
            self.assertEqual(updated["plugins"][0]["name"]["english"], "Extension Menu Builder")
            self.assertEqual(updated["plugins"][0]["dependencies"], ["powershellruntime"])
            self.assertTrue(
                updated["plugins"][0]["downloadPageUrl"].endswith(
                    "/plugin_5.0_extension-menu-builder_1.0.0_x64/"
                    "plugin_5.0_extension-menu-builder_1.0.0_x64.7z"
                )
            )

    def test_demo_extensions_are_one_salamatrixdemos_bundle(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            extensions_root = root / "extensions"
            for demo_name in ("python", "powershell"):
                demo_root = extensions_root / "demos" / demo_name
                demo_root.mkdir(parents=True)
                (demo_root / "extension.json").write_text(
                    json.dumps(
                        {
                            "name": demo_name,
                            "version": "1.0.0",
                            "description": f"{demo_name} demo",
                        }
                    ),
                    encoding="utf-8",
                )
            installer = root / "setup.iss"
            installer.write_text(
                'Source: "{#PayloadDir}\\extensions\\demos\\*"; '
                'DestDir: "{app}\\extensions\\demos"; '
                "Check: IsPluginSelected('salamatrixdemos')\n",
                encoding="utf-8",
            )

            extensions = UPDATER.parse_installer_extensions(installer, extensions_root)
            metadata = UPDATER.read_extension_metadata(
                "salamatrixdemos", extensions_root
            )

            self.assertEqual(extensions, [("salamatrixdemos", "salamatrixdemos")])
            self.assertEqual(metadata[0], "1.0.0 (x64)")
            self.assertEqual(metadata[3], "salamatrix")

    def test_schema_upgrade_marks_plugins_and_extensions_without_reordering(self) -> None:
        catalog = {
            "schemaVersion": 4,
            "plugins": [
                {"id": "pythonruntime", "name": "Python Runtime"},
                {"id": "salamatrixdemos", "name": "Demos"},
            ],
        }

        updated = UPDATER.update_catalog_schema(catalog, {"salamatrixdemos"})

        self.assertEqual(updated["schemaVersion"], 6)
        self.assertEqual(
            [entry["id"] for entry in updated["plugins"]],
            ["pythonruntime", "salamatrixdemos"],
        )
        self.assertEqual(updated["plugins"][0]["packageType"], "plugin")
        self.assertEqual(updated["plugins"][1]["packageType"], "extension")

    def test_trust_fields_preserve_hash_when_url_and_version_match(self) -> None:
        previous = {
            "id": "7zip",
            "downloadPageUrl": "https://example.invalid/7zip.7z",
            "latestVersion": "1.34 (x64)",
            "packageSha256": "ABCD" * 16,
        }
        entry = dict(previous)
        entry.pop("packageSha256")
        UPDATER.apply_package_trust_fields(entry, previous)
        self.assertEqual(entry["packageSha256"], ("abcd" * 16))
        self.assertEqual(entry["security"]["networkAccess"], "no")

    def test_trust_fields_drop_hash_when_version_changes(self) -> None:
        previous = {
            "id": "7zip",
            "downloadPageUrl": "https://example.invalid/7zip.7z",
            "latestVersion": "1.34 (x64)",
            "packageSha256": "abcd" * 16,
        }
        entry = {
            "id": "7zip",
            "downloadPageUrl": "https://example.invalid/7zip.7z",
            "latestVersion": "1.35 (x64)",
        }
        UPDATER.apply_package_trust_fields(entry, previous)
        self.assertNotIn("packageSha256", entry)

    def test_trust_fields_drop_hash_when_url_changes(self) -> None:
        previous = {
            "id": "7zip",
            "downloadPageUrl": "https://example.invalid/old.7z",
            "latestVersion": "1.34 (x64)",
            "packageSha256": "abcd" * 16,
        }
        entry = {
            "id": "7zip",
            "downloadPageUrl": "https://example.invalid/new.7z",
            "latestVersion": "1.34 (x64)",
        }
        UPDATER.apply_package_trust_fields(entry, previous)
        self.assertNotIn("packageSha256", entry)

    def test_capabilities_snapshot_keeps_curated_security(self) -> None:
        catalogs = {
            Path("plugins-stable.json"): {
                "plugins": [
                    {
                        "id": "7zip",
                        "packageType": "plugin",
                        "security": {
                            "networkAccess": "no",
                            "externalProcesses": "no",
                            "scriptExecution": "no",
                            "activeWebContent": "no",
                            "elevation": "never",
                        },
                    }
                ]
            }
        }
        snapshot = UPDATER.build_capabilities_snapshot(catalogs, "2026-08-26T00:00:00Z")
        self.assertEqual(snapshot["schemaVersion"], 1)
        self.assertEqual(snapshot["packages"][0]["id"], "7zip")
        self.assertEqual(snapshot["packages"][0]["security"]["networkAccess"], "no")


FILL_SCRIPT = Path(__file__).resolve().parents[1] / "fill_package_hashes.py"
FILL_SPEC = importlib.util.spec_from_file_location("fill_package_hashes", FILL_SCRIPT)
assert FILL_SPEC and FILL_SPEC.loader
FILLER = importlib.util.module_from_spec(FILL_SPEC)
FILL_SPEC.loader.exec_module(FILLER)


class FillPackageHashesTests(unittest.TestCase):
    def test_fill_writes_sha256_for_matching_download_name(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            archive = root / "plugin_5.0_demo_1.0_x64.7z"
            archive.write_bytes(b"official-package-bytes")
            catalog = {
                "schemaVersion": 6,
                "plugins": [
                    {
                        "id": "demo",
                        "downloadPageUrl": (
                            "https://github.com/KRtkovo-eu-AI/salamander-plugins/"
                            "releases/download/plugin_5.0_demo_1.0_x64/"
                            "plugin_5.0_demo_1.0_x64.7z"
                        ),
                    }
                ],
            }
            self.assertEqual(FILLER.fill_catalog(catalog, FILLER.index_archives(root)), 1)
            self.assertEqual(catalog["plugins"][0]["packageSha256"], FILLER.sha256_file(archive))

    def test_fill_prefers_catalog_latest_version_when_old_archive_remains(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            old_archive = root / "plugin_5.0_demo_1.0_x64.7z"
            new_archive = root / "plugin_5.0_demo_2.0_x64.7z"
            old_archive.write_bytes(b"old-package-bytes")
            new_archive.write_bytes(b"updated-package-bytes")
            catalog = {
                "schemaVersion": 6,
                "plugins": [
                    {
                        "id": "demo",
                        "latestVersion": "2.0 (x64)",
                        "downloadPageUrl": "https://example.invalid/plugin_5.0_demo_1.0_x64.7z",
                    }
                ],
            }
            self.assertEqual(FILLER.fill_catalog(catalog, FILLER.index_archives(root)), 1)
            self.assertEqual(catalog["plugins"][0]["packageSha256"], FILLER.sha256_file(new_archive))

    def test_fill_uses_unique_package_id_match_when_catalog_url_is_old(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            archive = root / "plugin_5.0_demo_2.0_x64.7z"
            archive.write_bytes(b"updated-package-bytes")
            catalog = {
                "schemaVersion": 6,
                "plugins": [
                    {
                        "id": "demo",
                        "downloadPageUrl": (
                            "https://github.com/KRtkovo-eu-AI/salamander-plugins/"
                            "releases/download/plugin_5.0_demo_1.0_x64/"
                            "plugin_5.0_demo_1.0_x64.7z"
                        ),
                    }
                ],
            }
            self.assertEqual(FILLER.fill_catalog(catalog, FILLER.index_archives(root)), 1)
            self.assertEqual(catalog["plugins"][0]["packageSha256"], FILLER.sha256_file(archive))


if __name__ == "__main__":
    unittest.main()

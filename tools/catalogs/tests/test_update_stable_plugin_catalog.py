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

    def test_existing_catalog_membership_wins_and_new_package_defaults_to_stable(self) -> None:
        stable = Path("plugins-stable.json")
        runtimes = Path("extension-runtimes.json")
        unofficial = Path("plugins-unofficial.json")
        catalogs = {
            stable: {"plugins": [{"id": "tar"}]},
            runtimes: {"plugins": [{"id": "pythonruntime"}]},
            unofficial: {"plugins": [{"id": "tar"}]},
        }

        assignments = UPDATER.assign_packages_to_catalogs(
            catalogs, stable, ["tar", "pythonruntime", "new-plugin"]
        )

        self.assertEqual(assignments[stable], ["tar", "new-plugin"])
        self.assertEqual(assignments[runtimes], ["pythonruntime"])
        self.assertNotIn(unofficial, assignments)

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
            self.assertEqual(updated["plugins"][0]["latestVersion"], "1.0.0 (x64)")
            self.assertEqual(updated["plugins"][0]["name"]["english"], "Extension Menu Builder")
            self.assertEqual(updated["plugins"][0]["dependencies"], ["powershellruntime"])
            self.assertTrue(
                updated["plugins"][0]["downloadPageUrl"].endswith(
                    "/plugin_5.0_extension-menu-builder_1.0.0_x64/"
                    "plugin_5.0_extension-menu-builder_1.0.0_x64.7z"
                )
            )


if __name__ == "__main__":
    unittest.main()

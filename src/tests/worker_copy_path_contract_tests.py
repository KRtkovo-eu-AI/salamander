# SPDX-FileCopyrightText: 2026 Samandarin Contributors
# SPDX-License-Identifier: GPL-2.0-or-later
from pathlib import Path
import unittest
SRC = Path(__file__).resolve().parents[1]
def text(name):
    return (SRC / name).read_text(encoding="utf-8-sig")
def section(source, first, last):
    return source.split(first, 1)[1].split(last, 1)[0]
class WorkerCopyPathContracts(unittest.TestCase):
    def test_owned_target_registers_handle_once(self):
        # Preserve the target's earlier handle-accounting fix when porting the
        # donor copy opens. Both paths register once after the conditional.
        code = section(text("worker.cpp"), "if (op->TargetNameWValid)", "if (out == INVALID_HANDLE_VALUE && encryptionNotSupported")
        self.assertEqual(code.count("NOHANDLES(CreateFileW(op->TargetNameW.c_str()"), 2)
        self.assertNotIn("HANDLES_Q(CreateFileW(op->TargetNameW.c_str()", code)
    def test_ads_preflight_opens_wide(self):
        code = section(text("worker.cpp"), "BOOL CheckFileOrDirADS(", "BOOL DeleteAllADS(")
        self.assertIn("HANDLES_Q(CreateFileW(WorkerOperationPathW(fileName, NULL).c_str(),", code)
        self.assertNotIn("CreateFile(", code)
        self.assertNotIn("fileNameCrFileCopy", code)
    def test_copy_planning_error_probe_retains_owned_path(self):
        code = section(text("fileswn6.cpp"), "BOOL CFilesWindow::BuildScriptFile(", "case atDelete:")
        self.assertIn("HANDLES_Q(CreateFileW(WorkerOperationPathW(op.SourceName, op.SourceNameWValid ? op.SourceNameW.c_str() : NULL).c_str(),", code)
        self.assertNotIn("CreateFile(op.SourceName", code)
    def test_every_copy_retry_uses_wide_boundary(self):
        code = text("worker.cpp")
        self.assertNotIn("CreateFile(op->", code)
        self.assertNotIn("CreateFile(Op->", code)
        for item in ("op->SourceName", "op->TargetName", "Op->SourceName", "Op->TargetName"):
            self.assertIn("HANDLES_Q(CreateFileW(WorkerOperationPathW(" + item + ", " + item + "WValid ? " + item + "W.c_str() : NULL).c_str(),", code)
    def test_stream_copy_and_delete_keep_dynamic_wide_names(self):
        code = text("worker.cpp")
        copy = section(code, "BOOL DoCopyADS(", "HANDLE SalCreateFileEx(")
        self.assertIn("std::wstring sourceStream = sourceBase + streamNames[i]", copy)
        self.assertIn("std::wstring targetStream = targetBase + streamNames[i]", copy)
        self.assertNotIn("wchar_t srcName[", copy)
        self.assertNotIn("MultiByteToWideChar(CP_ACP", copy)
        delete = section(code, "BOOL DeleteAllADS(", "void MyStrCpyNW(")
        self.assertIn("std::wstring adsFullName", delete)
        self.assertNotIn("adsFullName[2 * MAX_PATH]", delete)
    def test_attribute_reopen_stays_wide(self):
        code = section(text("worker.cpp"), "void SetCompressAndEncryptedAttrs(", "void CorrectCaseOfTgtName(")
        self.assertIn("GetFileAttributesW(nameW.c_str())", code)
        self.assertIn("HANDLES_Q(CreateFileW(WorkerOperationPathW(name, nameW.c_str()).c_str(),", code)
        self.assertIn("EncryptFileW(nameW.c_str())", code)
        self.assertIn("DecryptFileW(nameW.c_str(), 0)", code)
        self.assertNotIn("CreateFile(", code)
if __name__ == "__main__":
    unittest.main()

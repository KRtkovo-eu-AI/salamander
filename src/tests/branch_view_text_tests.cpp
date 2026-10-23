// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later
#include "../branch_view_text.h"
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>

using Salamander::BranchViewText::FormatStatus;
using Salamander::BranchViewText::FormatCompressedSizeError;

static int Checks = 0;
static int Failures = 0;

static void Check(bool ok, const char* description)
{
    ++Checks;
    if (!ok)
    {
        ++Failures;
        std::printf("FAIL %s\n", description);
    }
}

struct ResourceCase
{
    const char* File;
    const char* Marker;
    bool RcEscaping;
    bool Compression;
    const char* Expected;
};

// Exact expected output is intentionally independent of the formatter. Input
// patterns are read from the shipped English resource and every SLT translation.
static const ResourceCase Resources[] = {
    {"src/lang/texts.rc2", "IDS_BRANCH_SCANNING, \"", true, false, u8"Branch View - scanning... 17 files"},
    {"src/lang/texts.rc2", "IDS_BRANCH_READY, \"", true, false, u8"Branch View - 17 files"},
    {"src/lang/texts.rc2", "IDS_BRANCH_STOPPED, \"", true, false, u8"Branch View - stopped; 17 files (incomplete)"},
    {"src/lang/texts.rc2", "IDS_BRANCH_ERRORS, \"", true, false, u8"Branch View - 17 files; 3 folders could not be read"},
    {"src/lang/texts.rc2", "IDS_BRANCH_REFRESHING, \"", true, false, u8"Branch View - refreshing... 17 files retained"},
    {"src/lang/texts.rc2", "IDS_BRANCH_REFRESH_STOPPED, \"", true, false, u8"Branch View - refresh stopped; previous results retained (17 files)"},
    {"src/lang/texts.rc2", "IDS_BRANCH_PREPARING, \"", true, false, u8"Branch View - preparing 17 files; 3 previous results retained"},
    {"src/lang/texts.rc2", "IDS_GETCOMPRFILESIZEERROR, \"", true, true, u8"Unable to get compressed file size for file: \"C:\\žluťoučký\\東京\\%s%n.txt\"\nError: Přístup %n odepřen.\nDo you want to skip all similar errors?"},
    {"translations/chinesesimplified/salamand.slt", "14506,1,\"", false, false, u8"分支视图 - 正在扫描... 17 个文件"},
    {"translations/chinesesimplified/salamand.slt", "14507,1,\"", false, false, u8"分支视图 - 17 个文件"},
    {"translations/chinesesimplified/salamand.slt", "14508,1,\"", false, false, u8"分支视图 - 已停止；17 个文件（不完整）"},
    {"translations/chinesesimplified/salamand.slt", "14509,1,\"", false, false, u8"分支视图 - 17 个文件；无法读取 3 个文件夹"},
    {"translations/chinesesimplified/salamand.slt", "14513,1,\"", false, false, u8"分支视图 - 正在刷新... 保留 17 个文件"},
    {"translations/chinesesimplified/salamand.slt", "14514,1,\"", false, false, u8"分支视图 - 刷新已停止；已保留先前结果（17 个文件）"},
    {"translations/chinesesimplified/salamand.slt", "14515,1,\"", false, false, u8"分支视图 - 正在准备 17 个文件；保留 3 个先前结果"},
    {"translations/chinesesimplified/salamand.slt", "12320,1,\"", false, true, u8"无法得到文件 \"C:\\žluťoučký\\東京\\%s%n.txt\" 压缩后的文件大小。\n错误: Přístup %n odepřen.\n你要跳过所有类似错误吗？"},
    {"translations/czech/salamand.slt", "14506,1,\"", false, false, u8"Pohled na větev - prohledávání... 17 souborů"},
    {"translations/czech/salamand.slt", "14507,1,\"", false, false, u8"Pohled na větev - 17 souborů"},
    {"translations/czech/salamand.slt", "14508,1,\"", false, false, u8"Pohled na větev - zastaveno; 17 souborů (neúplné)"},
    {"translations/czech/salamand.slt", "14509,1,\"", false, false, u8"Pohled na větev - 17 souborů; 3 složek nebylo možné přečíst"},
    {"translations/czech/salamand.slt", "14513,1,\"", false, false, u8"Pohled na větev - obnovování... ponecháno 17 souborů"},
    {"translations/czech/salamand.slt", "14514,1,\"", false, false, u8"Pohled na větev - obnova zastavena; předchozí výsledky ponechány (17 souborů)"},
    {"translations/czech/salamand.slt", "14515,1,\"", false, false, u8"Pohled na větev - příprava 17 souborů; ponecháno 3 předchozích výsledků"},
    {"translations/czech/salamand.slt", "12320,1,\"", false, true, u8"Nelze získat komprimovanou velikost souboru pro soubor: \"C:\\žluťoučký\\東京\\%s%n.txt\"\nChyba: Přístup %n odepřen.\nChcete přeskočit všechny podobné chyby?"},
    {"translations/dutch/salamand.slt", "14506,1,\"", false, false, u8"Vertakkingsweergave - scannen... 17 bestanden"},
    {"translations/dutch/salamand.slt", "14507,1,\"", false, false, u8"Vertakkingsweergave - 17 bestanden"},
    {"translations/dutch/salamand.slt", "14508,1,\"", false, false, u8"Vertakkingsweergave - gestopt; 17 bestanden (onvolledig)"},
    {"translations/dutch/salamand.slt", "14509,1,\"", false, false, u8"Vertakkingsweergave - 17 bestanden; 3 mappen konden niet worden gelezen"},
    {"translations/dutch/salamand.slt", "14513,1,\"", false, false, u8"Vertakkingsweergave - vernieuwen... 17 bestanden behouden"},
    {"translations/dutch/salamand.slt", "14514,1,\"", false, false, u8"Vertakkingsweergave - vernieuwen gestopt; vorige resultaten behouden (17 bestanden)"},
    {"translations/dutch/salamand.slt", "14515,1,\"", false, false, u8"Vertakkingsweergave - 17 bestanden voorbereiden; 3 vorige resultaten behouden"},
    {"translations/dutch/salamand.slt", "12320,1,\"", false, true, u8"Kan gecomprimeerde bestandsgrootte niet vaststellen voor bestand: \"C:\\žluťoučký\\東京\\%s%n.txt\" \nFout:Přístup %n odepřen. \nWilt u alle soortgelijke fouten overslaan?"},
    {"translations/french/salamand.slt", "14506,1,\"", false, false, u8"Vue de branche - analyse en cours... 17 fichiers"},
    {"translations/french/salamand.slt", "14507,1,\"", false, false, u8"Vue de branche - 17 fichiers"},
    {"translations/french/salamand.slt", "14508,1,\"", false, false, u8"Vue de branche - arrêtée ; 17 fichiers (incomplet)"},
    {"translations/french/salamand.slt", "14509,1,\"", false, false, u8"Vue de branche - 17 fichiers ; 3 dossiers n’ont pas pu être lus"},
    {"translations/french/salamand.slt", "14513,1,\"", false, false, u8"Vue de branche - actualisation... 17 fichiers conservés"},
    {"translations/french/salamand.slt", "14514,1,\"", false, false, u8"Vue de branche - actualisation arrêtée ; résultats précédents conservés (17 fichiers)"},
    {"translations/french/salamand.slt", "14515,1,\"", false, false, u8"Vue de branche - préparation de 17 fichiers ; 3 résultats précédents conservés"},
    {"translations/french/salamand.slt", "12320,1,\"", false, true, u8"Impossible de récupérer la taille du fichier compressé pour le fichier \"C:\\žluťoučký\\東京\\%s%n.txt\"\nErreur : Přístup %n odepřen.\nPasser toutes les erreurs analogues ?"},
    {"translations/german/salamand.slt", "14506,1,\"", false, false, u8"Zweigansicht - Suche läuft... 17 Dateien"},
    {"translations/german/salamand.slt", "14507,1,\"", false, false, u8"Zweigansicht - 17 Dateien"},
    {"translations/german/salamand.slt", "14508,1,\"", false, false, u8"Zweigansicht - gestoppt; 17 Dateien (unvollständig)"},
    {"translations/german/salamand.slt", "14509,1,\"", false, false, u8"Zweigansicht - 17 Dateien; 3 Ordner konnten nicht gelesen werden"},
    {"translations/german/salamand.slt", "14513,1,\"", false, false, u8"Zweigansicht - Aktualisierung läuft... 17 Dateien beibehalten"},
    {"translations/german/salamand.slt", "14514,1,\"", false, false, u8"Zweigansicht - Aktualisierung gestoppt; vorherige Ergebnisse beibehalten (17 Dateien)"},
    {"translations/german/salamand.slt", "14515,1,\"", false, false, u8"Zweigansicht - 17 Dateien werden vorbereitet; 3 vorherige Ergebnisse beibehalten"},
    {"translations/german/salamand.slt", "12320,1,\"", false, true, u8"Größe der komprimierten Datei kann nicht ermittelt werden: \"C:\\žluťoučký\\東京\\%s%n.txt\"\nFehler: Přístup %n odepřen.\nMöchten Sie alle identischen Fehler überspringen?"},
    {"translations/hungarian/salamand.slt", "14506,1,\"", false, false, u8"Ágnézet - beolvasás... 17 fájl"},
    {"translations/hungarian/salamand.slt", "14507,1,\"", false, false, u8"Ágnézet - 17 fájl"},
    {"translations/hungarian/salamand.slt", "14508,1,\"", false, false, u8"Ágnézet - leállítva; 17 fájl (hiányos)"},
    {"translations/hungarian/salamand.slt", "14509,1,\"", false, false, u8"Ágnézet - 17 fájl; 3 mappa nem olvasható"},
    {"translations/hungarian/salamand.slt", "14513,1,\"", false, false, u8"Ágnézet - frissítés... 17 fájl megtartva"},
    {"translations/hungarian/salamand.slt", "14514,1,\"", false, false, u8"Ágnézet - frissítés leállítva; korábbi eredmények megtartva (17 fájl)"},
    {"translations/hungarian/salamand.slt", "14515,1,\"", false, false, u8"Ágnézet - 17 fájl előkészítése; 3 korábbi eredmény megtartva"},
    {"translations/hungarian/salamand.slt", "12320,1,\"", false, true, u8"Nem észlelhető a fájl tömörítési mérete: \"C:\\žluťoučký\\東京\\%s%n.txt\"\nHiba: Přístup %n odepřen.\nKihagy minden hasonló hibát?"},
    {"translations/italian/salamand.slt", "14506,1,\"", false, false, u8"Vista ramo - scansione... 17 file"},
    {"translations/italian/salamand.slt", "14507,1,\"", false, false, u8"Vista ramo - 17 file"},
    {"translations/italian/salamand.slt", "14508,1,\"", false, false, u8"Vista ramo - interrotta; 17 file (incompleta)"},
    {"translations/italian/salamand.slt", "14509,1,\"", false, false, u8"Vista ramo - 17 file; impossibile leggere 3 cartelle"},
    {"translations/italian/salamand.slt", "14513,1,\"", false, false, u8"Vista ramo - aggiornamento... 17 file mantenuti"},
    {"translations/italian/salamand.slt", "14514,1,\"", false, false, u8"Vista ramo - aggiornamento interrotto; risultati precedenti mantenuti (17 file)"},
    {"translations/italian/salamand.slt", "14515,1,\"", false, false, u8"Vista ramo - preparazione di 17 file; 3 risultati precedenti mantenuti"},
    {"translations/italian/salamand.slt", "12320,1,\"", false, true, u8"Impossibile ottenere la dimensione del file compresso per il file: \"C:\\žluťoučký\\東京\\%s%n.txt\"\nErrore: Přístup %n odepřen.\nVuoi ignorare tutti gli errori simili?"},
    {"translations/romanian/salamand.slt", "14506,1,\"", false, false, u8"Vizualizare ramură - scanare... 17 fișiere"},
    {"translations/romanian/salamand.slt", "14507,1,\"", false, false, u8"Vizualizare ramură - 17 fișiere"},
    {"translations/romanian/salamand.slt", "14508,1,\"", false, false, u8"Vizualizare ramură - oprită; 17 fișiere (incomplet)"},
    {"translations/romanian/salamand.slt", "14509,1,\"", false, false, u8"Vizualizare ramură - 17 fișiere; 3 foldere nu au putut fi citite"},
    {"translations/romanian/salamand.slt", "14513,1,\"", false, false, u8"Vizualizare ramură - reîmprospătare... 17 fișiere păstrate"},
    {"translations/romanian/salamand.slt", "14514,1,\"", false, false, u8"Vizualizare ramură - reîmprospătare oprită; rezultatele anterioare păstrate (17 fișiere)"},
    {"translations/romanian/salamand.slt", "14515,1,\"", false, false, u8"Vizualizare ramură - pregătirea a 17 fișiere; 3 rezultate anterioare păstrate"},
    {"translations/romanian/salamand.slt", "12320,1,\"", false, true, u8"Nu pot obtine marimea fisierului comprimat pentru fisierul: \"C:\\žluťoučký\\東京\\%s%n.txt\"\nEroare: Přístup %n odepřen.\nDoriti sa sariti peste toate erorile similare?"},
    {"translations/russian/salamand.slt", "14506,1,\"", false, false, u8"Просмотр ветви - сканирование... 17 файлов"},
    {"translations/russian/salamand.slt", "14507,1,\"", false, false, u8"Просмотр ветви - 17 файлов"},
    {"translations/russian/salamand.slt", "14508,1,\"", false, false, u8"Просмотр ветви - остановлено; 17 файлов (неполный список)"},
    {"translations/russian/salamand.slt", "14509,1,\"", false, false, u8"Просмотр ветви - 17 файлов; не удалось прочитать 3 папок"},
    {"translations/russian/salamand.slt", "14513,1,\"", false, false, u8"Просмотр ветви - обновление... сохранено 17 файлов"},
    {"translations/russian/salamand.slt", "14514,1,\"", false, false, u8"Просмотр ветви - обновление остановлено; предыдущие результаты сохранены (17 файлов)"},
    {"translations/russian/salamand.slt", "14515,1,\"", false, false, u8"Просмотр ветви - подготовка 17 файлов; сохранено 3 предыдущих результатов"},
    {"translations/russian/salamand.slt", "12320,1,\"", false, true, u8"Не удается определить сжатый размер файла: \"C:\\žluťoučký\\東京\\%s%n.txt\"\nОшибка: Přístup %n odepřen.\nПропустить все подобные ошибки?"},
    {"translations/slovak/salamand.slt", "14506,1,\"", false, false, u8"Zobrazenie vetvy - prehľadávanie... 17 súborov"},
    {"translations/slovak/salamand.slt", "14507,1,\"", false, false, u8"Zobrazenie vetvy - 17 súborov"},
    {"translations/slovak/salamand.slt", "14508,1,\"", false, false, u8"Zobrazenie vetvy - zastavené; 17 súborov (neúplné)"},
    {"translations/slovak/salamand.slt", "14509,1,\"", false, false, u8"Zobrazenie vetvy - 17 súborov; 3 priečinkov sa nepodarilo prečítať"},
    {"translations/slovak/salamand.slt", "14513,1,\"", false, false, u8"Zobrazenie vetvy - obnovovanie... ponechaných 17 súborov"},
    {"translations/slovak/salamand.slt", "14514,1,\"", false, false, u8"Zobrazenie vetvy - obnova zastavená; predchádzajúce výsledky ponechané (17 súborov)"},
    {"translations/slovak/salamand.slt", "14515,1,\"", false, false, u8"Zobrazenie vetvy - príprava 17 súborov; ponechaných 3 predchádzajúcich výsledkov"},
    {"translations/slovak/salamand.slt", "12320,1,\"", false, true, u8"Nie je možné získať komprimovanú veľkosť súboru pre súbor: \"C:\\žluťoučký\\東京\\%s%n.txt\"\nChyba: Přístup %n odepřen.\nChcete preskočiť všetky podobné chyby?"},
    {"translations/spanish/salamand.slt", "14506,1,\"", false, false, u8"Vista de rama - explorando... 17 archivos"},
    {"translations/spanish/salamand.slt", "14507,1,\"", false, false, u8"Vista de rama - 17 archivos"},
    {"translations/spanish/salamand.slt", "14508,1,\"", false, false, u8"Vista de rama - detenida; 17 archivos (incompleto)"},
    {"translations/spanish/salamand.slt", "14509,1,\"", false, false, u8"Vista de rama - 17 archivos; no se pudieron leer 3 carpetas"},
    {"translations/spanish/salamand.slt", "14513,1,\"", false, false, u8"Vista de rama - actualizando... 17 archivos conservados"},
    {"translations/spanish/salamand.slt", "14514,1,\"", false, false, u8"Vista de rama - actualización detenida; resultados anteriores conservados (17 archivos)"},
    {"translations/spanish/salamand.slt", "14515,1,\"", false, false, u8"Vista de rama - preparando 17 archivos; 3 resultados anteriores conservados"},
    {"translations/spanish/salamand.slt", "12320,1,\"", false, true, u8"Imposible saber el tamaño del fichero comprimido: \"C:\\žluťoučký\\東京\\%s%n.txt\"\nError: Přístup %n odepřen.\n¿Desea saltar el resto de errores similares?"},
};

static std::string DecodeResource(const std::string& raw, bool rcEscaping)
{
    std::string text;
    for (std::size_t i = 0; i < raw.size(); ++i)
    {
        if (rcEscaping && raw[i] == '"' && i + 1 < raw.size() && raw[i + 1] == '"')
        {
            text += '"';
            ++i;
        }
        else if (raw[i] == '\\' && i + 1 < raw.size() &&
                 (raw[i + 1] == 'n' || raw[i + 1] == 'r' || raw[i + 1] == 't'))
        {
            text += raw[i + 1] == 'n' ? '\n' : raw[i + 1] == 'r' ? '\r' : '\t';
            ++i;
        }
        else
            text += raw[i];
    }
    return text;
}

static void ResourceTests(const std::filesystem::path& root)
{
    for (const auto& test : Resources)
    {
        std::ifstream input(root / test.File, std::ios::binary);
        std::string line, pattern;
        int matches = 0;
        while (std::getline(input, line))
        {
            const std::size_t start = line.find(test.Marker);
            if (start == std::string::npos)
                continue;
            const std::size_t begin = start + std::char_traits<char>::length(test.Marker);
            const std::size_t end = line.rfind('"');
            if (end != std::string::npos && end >= begin)
            {
                pattern = DecodeResource(line.substr(begin, end - begin), test.RcEscaping);
                ++matches;
            }
        }
        Check(matches == 1, test.File);
        const std::string actual = test.Compression ?
            FormatCompressedSizeError(pattern.c_str(), u8"C:\\žluťoučký\\東京\\%s%n.txt",
                                      u8"Přístup %n odepřen.") :
            FormatStatus(pattern.c_str(), 17, 3);
        if (actual != test.Expected)
            std::printf("Resource mismatch: %s, %s\n", test.File, test.Marker);
        Check(actual == test.Expected, test.Marker);
    }
}

static void StatusTests()
{
    Check(FormatStatus("Branch View - %I64u files", 17, 3) == "Branch View - 17 files",
          "ordinary count");
    Check(FormatStatus("%I64u files; %I64u errors", 17, 3) == "17 files; 3 errors",
          "two ordered counts");
    Check(FormatStatus("%I64u%I64u", 17, 3) == "173", "adjacent counts");
    Check(FormatStatus("%I64u / %I64u", 0, 0) == "0 / 0", "zero counts");
    Check(FormatStatus("%I64u / %I64u", std::numeric_limits<std::uint64_t>::max(),
                       std::uint64_t(1) << 63) ==
              "18446744073709551615 / 9223372036854775808",
          "full unsigned 64-bit range without signed wraparound");
    Check(FormatStatus("%I64u", 4294967296ULL, 0) == "4294967296", "count beyond 32 bits");
    Check(FormatStatus("%% %I64u %% %I64u %%", 17, 3) == "% 17 % 3 %",
          "escaped percent around counts");
    Check(FormatStatus("%%I64u | %I64u | %I64u", 17, 3) == "%I64u | 17 | 3",
          "escaped count does not consume an argument");
    Check(FormatStatus("%%%I64u", 17, 3) == "%17", "escape before count");
    Check(FormatStatus("%n %s %d %u %llu %I64d %10I64u %.2I64u %*I64u %1$I64u %I64u", 17, 3) ==
              "%n %s %d %u %llu %I64d %10I64u %.2I64u %*I64u %1$I64u 17",
          "unsupported numeric and dangerous directives stay literal");
    Check(FormatStatus("%I64u/%I64u/%I64u/%n/%%", 17, 3) == "17/3/%I64u/%n/%",
          "excess counts stay literal and escapes still work");
    Check(FormatStatus("%", 17, 3) == "%", "trailing percent");
    Check(FormatStatus("%I", 17, 3) == "%I", "short malformed count");
    Check(FormatStatus("%I64", 17, 3) == "%I64", "unterminated count");
    Check(FormatStatus("100% hotovo", 17, 3) == "100% hotovo", "literal percent");
    Check(FormatStatus(nullptr, 17, 3).empty(), "null status pattern");
    Check(FormatStatus("", 17, 3).empty(), "empty status pattern");
    Check(FormatStatus(u8"東京: %I64u souborů, %I64u chyb 🗂", 17, 3) ==
              u8"東京: 17 souborů, 3 chyb 🗂", "status UTF-8 preserved");
    const std::string longPrefix(12000, 'x');
    Check(FormatStatus((longPrefix + "%I64u").c_str(), 17, 3) == longPrefix + "17",
          "status has no fixed display buffer");
}

static void CompressionTests()
{
    Check(FormatCompressedSizeError("File: %s\nError: %s", "C:\\test.txt", "Access denied") ==
              "File: C:\\test.txt\nError: Access denied", "ordinary error message");
    Check(FormatCompressedSizeError("%s%s", "A", "B") == "AB", "adjacent text values");
    Check(FormatCompressedSizeError("%% %s %% %s %%", "A", "B") == "% A % B %",
          "escaped text percent");
    Check(FormatCompressedSizeError("%%s | %s | %s", "A", "B") == "%s | A | B",
          "escaped text token consumes no argument");
    Check(FormatCompressedSizeError("%%%s", "A", "B") == "%A", "escape before text");
    Check(FormatCompressedSizeError("%n %d %ls %S %1$s %2$s %10s %.2s %*s %I64u %s %s", "A", "B") ==
              "%n %d %ls %S %1$s %2$s %10s %.2s %*s %I64u A B",
          "unsupported text and dangerous directives stay literal");
    Check(FormatCompressedSizeError("%s / %s / %s / %n", "A", "B") ==
              "A / B / %s / %n", "excess text tokens stay literal");
    Check(FormatCompressedSizeError("[%s][%s]", "%s%n%%", u8"東京%s%1$s") ==
              u8"[%s%n%%][東京%s%1$s]", "values cannot inject further format directives");
    Check(FormatCompressedSizeError(nullptr, "A", "B").empty(), "null error pattern");
    Check(FormatCompressedSizeError("", nullptr, nullptr).empty(), "empty error pattern");
    Check(FormatCompressedSizeError("[%s][%s]", nullptr, nullptr) == "[][]", "null values");
    Check(FormatCompressedSizeError("%s only", "A", "B") == "A only", "single text token");
    Check(FormatCompressedSizeError("No values %", "A", "B") == "No values %",
          "missing text tokens and trailing percent");
    Check(FormatCompressedSizeError("[%s] %s", u8"C:\\ASCII\\žluťoučký.txt", "error") ==
              u8"[C:\\ASCII\\žluťoučký.txt] error", "Unicode leaf in ASCII directory");
    Check(FormatCompressedSizeError("[%s] %s", u8"C:\\žluťoučký\\東京\\file.txt", "error") ==
              u8"[C:\\žluťoučký\\東京\\file.txt] error", "multiple Unicode parents");
    std::string shortUtf16LongUtf8 = "C:\\";
    for (int i = 0; i < 100; ++i)
        shortUtf16LongUtf8 += u8"東京";
    shortUtf16LongUtf8 += "\\file.txt";
    Check(FormatCompressedSizeError("%s|%s", shortUtf16LongUtf8.c_str(), "error") ==
              shortUtf16LongUtf8 + "|error", "UTF-8 over 260 bytes, UTF-16 under 260 units");
    std::string longPath = u8"\\\\?\\UNC\\server\\share\\";
    for (int i = 0; i < 3000; ++i)
        longPath += u8"žluťoučký東京\\";
    longPath += u8"%n%s-📄.txt";
    const std::string longError = std::string(21000, 'E') + u8"終%n";
    Check(FormatCompressedSizeError("File=%s\nError=%s", longPath.c_str(), longError.c_str()) ==
              "File=" + longPath + "\nError=" + longError,
          "long Unicode UNC path and error preserved completely without reformatting");
    Check(FormatCompressedSizeError(u8"Soubor 🗂 %s: %s", longPath.c_str(), "") ==
              u8"Soubor 🗂 " + longPath + ": ", "Unicode template and empty error preserved");
}

int wmain(int argc, wchar_t** argv)
{
    StatusTests();
    CompressionTests();
    ResourceTests(argc == 2 ? std::filesystem::path(argv[1]) : std::filesystem::current_path());
    std::printf("Branch View text: %d checks, %d failures (96 real localized patterns).\n",
                Checks, Failures);
    return Failures == 0 ? 0 : 1;
}

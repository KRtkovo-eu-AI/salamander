// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later
#include "../drivefreespace_text.h"
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
using Salamander::DriveFreeSpaceText::FormatDescription;
static int Checks=0, Failures=0;
static void Check(bool condition,const char* message) {
    ++Checks;
    if(!condition) { ++Failures; std::printf("FAIL %s\n",message); }
}
struct ResourceCase { const char* File; const char* Marker; const char* Expected; };
// Explicit output expectations retain all three status labels in English and
// every shipped translation; the patterns themselves are read from real files.
static const ResourceCase Resources[] = {
    {"src/lang/texts.rc2", "IDS_DRIVE_SPACE_FREE, \"", u8"14,5 GiB 東京 free (updated 26. září 2026, 12:34)"},
    {"src/lang/texts.rc2", "IDS_DRIVE_SPACE_CACHED, \"", u8"14,5 GiB 東京 free (cached, updated 26. září 2026, 12:34)"},
    {"src/lang/texts.rc2", "IDS_DRIVE_SPACE_STALE, \"", u8"14,5 GiB 東京 free (stale, updated 26. září 2026, 12:34)"},
    {"translations/chinesesimplified/salamand.slt", "15003,1,\"", u8"14,5 GiB 東京 可用（更新于 26. září 2026, 12:34）"},
    {"translations/chinesesimplified/salamand.slt", "15004,1,\"", u8"14,5 GiB 東京 可用（缓存，更新于 26. září 2026, 12:34）"},
    {"translations/chinesesimplified/salamand.slt", "15005,1,\"", u8"14,5 GiB 東京 可用（已过期，更新于 26. září 2026, 12:34）"},
    {"translations/czech/salamand.slt", "15003,1,\"", u8"14,5 GiB 東京 volného místa (aktualizováno 26. září 2026, 12:34)"},
    {"translations/czech/salamand.slt", "15004,1,\"", u8"14,5 GiB 東京 volného místa (v mezipaměti, aktualizováno 26. září 2026, 12:34)"},
    {"translations/czech/salamand.slt", "15005,1,\"", u8"14,5 GiB 東京 volného místa (zastaralé, aktualizováno 26. září 2026, 12:34)"},
    {"translations/dutch/salamand.slt", "15003,1,\"", u8"14,5 GiB 東京 vrij (bijgewerkt 26. září 2026, 12:34)"},
    {"translations/dutch/salamand.slt", "15004,1,\"", u8"14,5 GiB 東京 vrij (in cache, bijgewerkt 26. září 2026, 12:34)"},
    {"translations/dutch/salamand.slt", "15005,1,\"", u8"14,5 GiB 東京 vrij (verouderd, bijgewerkt 26. září 2026, 12:34)"},
    {"translations/french/salamand.slt", "15003,1,\"", u8"14,5 GiB 東京 libres (mis à jour 26. září 2026, 12:34)"},
    {"translations/french/salamand.slt", "15004,1,\"", u8"14,5 GiB 東京 libres (en cache, mis à jour 26. září 2026, 12:34)"},
    {"translations/french/salamand.slt", "15005,1,\"", u8"14,5 GiB 東京 libres (périmés, mis à jour 26. září 2026, 12:34)"},
    {"translations/german/salamand.slt", "15003,1,\"", u8"14,5 GiB 東京 frei (aktualisiert 26. září 2026, 12:34)"},
    {"translations/german/salamand.slt", "15004,1,\"", u8"14,5 GiB 東京 frei (zwischengespeichert, aktualisiert 26. září 2026, 12:34)"},
    {"translations/german/salamand.slt", "15005,1,\"", u8"14,5 GiB 東京 frei (veraltet, aktualisiert 26. září 2026, 12:34)"},
    {"translations/hungarian/salamand.slt", "15003,1,\"", u8"14,5 GiB 東京 szabad (frissítve: 26. září 2026, 12:34)"},
    {"translations/hungarian/salamand.slt", "15004,1,\"", u8"14,5 GiB 東京 szabad (gyorsítótárból, frissítve: 26. září 2026, 12:34)"},
    {"translations/hungarian/salamand.slt", "15005,1,\"", u8"14,5 GiB 東京 szabad (elavult, frissítve: 26. září 2026, 12:34)"},
    {"translations/italian/salamand.slt", "15003,1,\"", u8"14,5 GiB 東京 liberi (aggiornato 26. září 2026, 12:34)"},
    {"translations/italian/salamand.slt", "15004,1,\"", u8"14,5 GiB 東京 liberi (in cache, aggiornato 26. září 2026, 12:34)"},
    {"translations/italian/salamand.slt", "15005,1,\"", u8"14,5 GiB 東京 liberi (non aggiornato, aggiornato 26. září 2026, 12:34)"},
    {"translations/romanian/salamand.slt", "15003,1,\"", u8"14,5 GiB 東京 liber (actualizat 26. září 2026, 12:34)"},
    {"translations/romanian/salamand.slt", "15004,1,\"", u8"14,5 GiB 東京 liber (în cache, actualizat 26. září 2026, 12:34)"},
    {"translations/romanian/salamand.slt", "15005,1,\"", u8"14,5 GiB 東京 liber (neactualizat, actualizat 26. září 2026, 12:34)"},
    {"translations/russian/salamand.slt", "15003,1,\"", u8"14,5 GiB 東京 свободно (обновлено 26. září 2026, 12:34)"},
    {"translations/russian/salamand.slt", "15004,1,\"", u8"14,5 GiB 東京 свободно (из кэша, обновлено 26. září 2026, 12:34)"},
    {"translations/russian/salamand.slt", "15005,1,\"", u8"14,5 GiB 東京 свободно (устарело, обновлено 26. září 2026, 12:34)"},
    {"translations/slovak/salamand.slt", "15003,1,\"", u8"14,5 GiB 東京 voľného miesta (aktualizované 26. září 2026, 12:34)"},
    {"translations/slovak/salamand.slt", "15004,1,\"", u8"14,5 GiB 東京 voľného miesta (v medzipamäti, aktualizované 26. září 2026, 12:34)"},
    {"translations/slovak/salamand.slt", "15005,1,\"", u8"14,5 GiB 東京 voľného miesta (zastarané, aktualizované 26. září 2026, 12:34)"},
    {"translations/spanish/salamand.slt", "15003,1,\"", u8"14,5 GiB 東京 libres (actualizado 26. září 2026, 12:34)"},
    {"translations/spanish/salamand.slt", "15004,1,\"", u8"14,5 GiB 東京 libres (en caché, actualizado 26. září 2026, 12:34)"},
    {"translations/spanish/salamand.slt", "15005,1,\"", u8"14,5 GiB 東京 libres (obsoleto, actualizado 26. září 2026, 12:34)"},
};
static void ResourceTests(const std::filesystem::path& root) {
    for(const auto& test:Resources) {
        std::ifstream input(root/test.File,std::ios::binary);
        std::string line,pattern;int matches=0;
        while(std::getline(input,line)) {
            size_t start=line.find(test.Marker);
            if(start==std::string::npos) continue;
            start+=std::char_traits<char>::length(test.Marker);
            size_t end=line.rfind('"');
            if(end>=start) { pattern=line.substr(start,end-start);++matches; }
        }
        Check(matches==1,test.File);
        Check(FormatDescription(pattern.c_str(),u8"14,5 GiB 東京",u8"26. září 2026, 12:34")==test.Expected,test.Marker);
    }
}
static void LiteralTests() {
    Check(FormatDescription("%s free (updated %s)","14 GB","now")=="14 GB free (updated now)","normal description");
    Check(FormatDescription("%s%s","A","B")=="AB","adjacent substitutions");
    Check(FormatDescription("100% free: %s / %s","A","B")=="100% free: A / B","literal percent");
    Check(FormatDescription("%% %s %% %s %%","A","B")=="% A % B %","escaped percents");
    Check(FormatDescription("%%%s | %s","A","B")=="%A | B","escape immediately before substitution");
    Check(FormatDescription("%%s | %s | %s","A","B")=="%s | A | B","escaped placeholder consumes no argument");
    Check(FormatDescription("%n %d %ls %S %1$s %2$s %10s %.2s %*s %s %s","A","B")=="%n %d %ls %S %1$s %2$s %10s %.2s %*s A B","all other printf directives remain literal");
    Check(FormatDescription("%s / %s / %s / %n","A","B")=="A / B / %s / %n","excess placeholders remain literal");
    Check(FormatDescription("%","A","B")=="%","trailing percent");
    Check(FormatDescription("literal","A","B")=="literal","missing placeholders");
    Check(FormatDescription("%s only","A","B")=="A only","single placeholder");
    Check(FormatDescription("[%s][%s]","%s%n%%",u8"東京%s%1$s")==u8"[%s%n%%][東京%s%1$s]","inserted values are never reparsed");
    Check(FormatDescription(nullptr,"A","B").empty(),"null pattern");
    Check(FormatDescription("",nullptr,nullptr).empty(),"empty pattern and null values");
    Check(FormatDescription("[%s][%s]",nullptr,nullptr)=="[][]","null values are empty strings");
    const std::string longSize(20000,'A'),longStamp(21000,'B');
    Check(FormatDescription("%s|%s",longSize.c_str(),longStamp.c_str())==longSize+"|"+longStamp,"formatting preserves complete large values without truncation");
}
int wmain(int argc,wchar_t** argv) {
    LiteralTests();
    ResourceTests(argc==2 ? std::filesystem::path(argv[1]) : std::filesystem::current_path());
    std::printf("Drive free-space text: %d checks, %d failures (36 real localized patterns).\n",Checks,Failures);
    return Failures ? 1 : 0;
}

# S02b-preflight — Debug startup/shutdown acceptance

Aktualizace: [S02b-preflightb](S02b-preflightb.md) opravuje nativní storage,
stale diagnostic pointer i pořadí init readiness. Lokální runtime brána ověřena;
nový kandidát čeká na exact-head fork CI. Níže historie preflighta.

Stav: **PARTIAL/BLOCKED**, 2026-10-13. S02b ani S03 nejsou přijaté/pokročilé.

První samostatný kandidát S02b-preflighta opravuje tři lokálně vlastněné ikony
Service Exploreru (fs1 EnsureServiceImageList, plugin CreateServiceBitmap a
SetServiceExplorerIconList) párováním HANDLES(LoadImage) a HANDLES(DestroyIcon).
GetFSIcon/GetPluginIcon dál předávají nezaregistrované ikony hostiteli;
nevkládají se do pluginového trackeru. SDK/Branch View beze změny.

Buildy dotčeného pluginu a hostitele Debug/Release x64 PASS. Běžný Debug
první start bez původních icon modalů, F3/F5/hash PASS. Release shutdown PASS.
Plný Debug shutdown dál heap FAIL. Diagnostický TRACE_TO_FILE běh a A/B
control izolovaly Samandarin Update Notifier shutdown/storage; control bez
notifieru není validace plné sestavy ani oprava. Vlastní SPL byl obnoven.

Zjištěné source parity a skutečné důkazy jsou v VALIDATION.md a v externí
S02b-preflight evidenci. Přesný parent runtime nebyl znovu sestaven. Kandidát
může být zveřejněn PENDING k CI, ale accepted head zůstává S02a; žádný reset,
vyřazení testů, změna pinů nebo označení celé S02b DONE.

Nejmenší další krok [S02b-preflightb](S02b-preflightb.md) musí uvolnit nativní
resources notifieru bezpečně i při rychlém ukončení během CLR inicializace,
bez odstranění heap/tracker kontrol a bez návratu zablokovaných callbacks.
Po něm zopakovat plnou Debug/Release startup/view/copy/shutdown acceptance
s odpovídajícím kompletním runtime. Portables language peers zachovat.

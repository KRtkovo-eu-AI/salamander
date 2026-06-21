; *** Inno Setup version 6.5.0+ Romanian messages ***
;
; To download user-contributed translations of this file, go to:
;   https://jrsoftware.org/files/istrans/
;
; Note: When translating this text, do not add periods (.) to the end of
; messages that didn't have them already, because on those messages Inno
; Setup adds the periods automatically (appending a period would result in
; two periods being displayed).

[LangOptions]
; The following three entries are very important. Be sure to read and
; understand the '[LangOptions] section' topic in the help file.
LanguageName=Rom<00E2>n<0103>
LanguageID=$0418
; LanguageCodePage should always be set if possible, even if this file is Unicode
; For English it's set to zero anyway because English only uses ASCII characters
LanguageCodePage=1250
; If the language you are translating to requires special font faces or
; sizes, uncomment any of the following entries and change them accordingly.
;DialogFontName=
;DialogFontSize=9
;DialogFontBaseScaleWidth=7
;DialogFontBaseScaleHeight=15
;WelcomeFontName=Segoe UI
;WelcomeFontSize=14

[Messages]

; *** Application titles
SetupAppTitle=Instalare
SetupWindowTitle=Instalare - %1
UninstallAppTitle=Dezinstalare
UninstallAppFullTitle=Dezinstalare %1

; *** Misc. common
InformationTitle=Informaţii
ConfirmTitle=Confirmare
ErrorTitle=Eroare

; *** SetupLdr messages
SetupLdrStartupMessage=Se va instala %1. Doriţi să continuaţi?
LdrCannotCreateTemp=Nu se poate crea un fişier temporar. Instalarea a fost abandonată
LdrCannotExecTemp=Nu se poate executa fişierul din directorul temporar. Instalarea a fost abandonată
HelpTextNote=

; *** Startup error messages
LastErrorMessage=%1.%n%nEroare %2: %3
SetupFileMissing=Fişierul %1 lipseşte din directorul de instalare. Corectaţi problema sau obţineţi o copie nouă a programului.
SetupFileCorrupt=Fişierele de instalare sunt corupte. Obţineţi o copie nouă a programului.
SetupFileCorruptOrWrongVer=Fişierele de instalare sunt corupte sau sunt incompatibile cu această versiune a programului de instalare. Corectaţi problema sau obţineţi o copie nouă a programului.
InvalidParameter=A fost transmis un parametru nevalid în linia de comandă:%n%n%1
SetupAlreadyRunning=Programul de instalare rulează deja.
WindowsVersionNotSupported=Acest program nu acceptă versiunea de Windows care rulează pe computerul dumneavoastră.
WindowsServicePackRequired=Acest program necesită %1 Service Pack %2 sau o versiune ulterioară.
NotOnThisPlatform=Acest program nu va rula pe %1.
OnlyOnThisPlatform=Acest program trebuie rulat pe %1.
OnlyOnTheseArchitectures=Acest program poate fi instalat numai pe versiuni de Windows proiectate pentru următoarele arhitecturi de procesor:%n%n%1
WinVersionTooLowError=Acest program necesită %1 versiunea %2 sau o versiune ulterioară.
WinVersionTooHighError=Acest program nu poate fi instalat pe %1 versiunea %2 sau o versiune ulterioară.
AdminPrivilegesRequired=Trebuie să fiţi autentificat ca administrator pentru a instala acest program.
PowerUserPrivilegesRequired=Trebuie să fiţi autentificat ca administrator sau ca membru al grupului Power Users pentru a instala acest program.
SetupAppRunningError=Programul de instalare a detectat că %1 rulează în acest moment.%n%nÎnchideţi toate instanţele acestuia, apoi faceţi clic pe OK pentru a continua sau pe Anulare pentru a ieşi.
UninstallAppRunningError=Programul de dezinstalare a detectat că %1 rulează în acest moment.%n%nÎnchideţi toate instanţele acestuia, apoi faceţi clic pe OK pentru a continua sau pe Anulare pentru a ieşi.

; *** Startup questions
PrivilegesRequiredOverrideTitle=Selectaţi modul de instalare
PrivilegesRequiredOverrideInstruction=Selectaţi modul de instalare
PrivilegesRequiredOverrideText1=%1 poate fi instalat pentru toţi utilizatorii (necesită privilegii administrative) sau numai pentru dumneavoastră.
PrivilegesRequiredOverrideText2=%1 poate fi instalat numai pentru dumneavoastră sau pentru toţi utilizatorii (necesită privilegii administrative).
PrivilegesRequiredOverrideAllUsers=Instalare pentru toţi utiliz&atorii
PrivilegesRequiredOverrideAllUsersRecommended=Instalare pentru toţi utiliz&atorii (recomandat)
PrivilegesRequiredOverrideCurrentUser=Instalare numai pentru &mine
PrivilegesRequiredOverrideCurrentUserRecommended=Instalare numai pentru &mine (recomandat)

; *** Misc. errors
ErrorCreatingDir=Programul de instalare nu a putut crea directorul "%1"
ErrorTooManyFilesInDir=Nu se poate crea un fişier în directorul "%1", deoarece acesta conţine prea multe fişiere

; *** Setup common messages
ExitSetupTitle=Ieşire din instalare
ExitSetupMessage=Instalarea nu este completă. Dacă ieşiţi acum, programul nu va fi instalat.%n%nPuteţi rula din nou programul de instalare altă dată pentru a finaliza instalarea.%n%nIeşiţi din instalare?
AboutSetupMenuItem=&Despre programul de instalare...
AboutSetupTitle=Despre programul de instalare
AboutSetupMessage=%1 versiunea %2%n%3%n%nPagina principală %1:%n%4
AboutSetupNote=
TranslatorNote=Traducere în limba română pregătită pe baza fişierului Default.isl şi a traducerii româneşti Inno Setup.

; *** Buttons
ButtonBack=< Îna&poi
ButtonNext=Î&nainte >
ButtonInstall=&Instalare
ButtonOK=OK
ButtonCancel=Anulare
ButtonYes=&Da
ButtonYesToAll=Da pentru &toate
ButtonNo=&Nu
ButtonNoToAll=Nu pentru t&oate
ButtonFinish=&Finalizare
ButtonBrowse=&Răsfoire...
ButtonWizardBrowse=Ră&sfoire...
ButtonNewFolder=&Creare director nou

; *** "Select Language" dialog messages
SelectLanguageTitle=Selectaţi limba instalării
SelectLanguageLabel=Selectaţi limba care va fi folosită în timpul instalării.

; *** Common wizard text
ClickNext=Faceţi clic pe Înainte pentru a continua sau pe Anulare pentru a ieşi din instalare.
BeveledLabel=
BrowseDialogTitle=Răsfoire după director
BrowseDialogLabel=Selectaţi un director din lista de mai jos, apoi faceţi clic pe OK.
NewFolderName=Director nou

; *** "Welcome" wizard page
WelcomeLabel1=Bun venit în expertul de instalare [name]
WelcomeLabel2=Acesta va instala [name/ver] pe computerul dumneavoastră.%n%nSe recomandă să închideţi toate celelalte aplicaţii înainte de a continua.

; *** "Password" wizard page
WizardPassword=Parolă
PasswordLabel1=Această instalare este protejată prin parolă.
PasswordLabel3=Introduceţi parola, apoi faceţi clic pe Înainte pentru a continua. Parolele diferenţiază literele mari de cele mici.
PasswordEditLabel=&Parolă:
IncorrectPassword=Parola introdusă nu este corectă. Încercaţi din nou.

; *** "License Agreement" wizard page
WizardLicense=Acord de licenţă
LicenseLabel=Citiţi următoarele informaţii importante înainte de a continua.
LicenseLabel3=Citiţi următorul acord de licenţă. Trebuie să acceptaţi termenii acestui acord înainte de a continua instalarea.
LicenseAccepted=&Accept acordul
LicenseNotAccepted=&Nu accept acordul

; *** "Information" wizard pages
WizardInfoBefore=Informaţii
InfoBeforeLabel=Citiţi următoarele informaţii importante înainte de a continua.
InfoBeforeClickLabel=Când sunteţi gata să continuaţi instalarea, faceţi clic pe Înainte.
WizardInfoAfter=Informaţii
InfoAfterLabel=Citiţi următoarele informaţii importante înainte de a continua.
InfoAfterClickLabel=Când sunteţi gata să continuaţi instalarea, faceţi clic pe Înainte.

; *** "User Information" wizard page
WizardUserInfo=Informaţii utilizator
UserInfoDesc=Introduceţi informaţiile dumneavoastră.
UserInfoName=&Nume utilizator:
UserInfoOrg=&Organizaţie:
UserInfoSerial=Număr de &serie:
UserInfoNameRequired=Trebuie să introduceţi un nume.

; *** "Select Destination Location" wizard page
WizardSelectDir=Selectaţi locaţia de destinaţie
SelectDirDesc=Unde trebuie instalat [name]?
SelectDirLabel3=Programul de instalare va instala [name] în următorul director.
SelectDirBrowseLabel=Pentru a continua, faceţi clic pe Înainte. Dacă doriţi să selectaţi un alt director, faceţi clic pe Răsfoire.
DiskSpaceGBLabel=Este necesar cel puţin [gb] GB de spaţiu liber pe disc.
DiskSpaceMBLabel=Este necesar cel puţin [mb] MB de spaţiu liber pe disc.
CannotInstallToNetworkDrive=Programul de instalare nu poate instala pe o unitate de reţea.
CannotInstallToUNCPath=Programul de instalare nu poate instala într-o cale UNC.
InvalidPath=Trebuie să introduceţi o cale completă cu litera unităţii; de exemplu:%n%nC:\APP%n%nsau o cale UNC de forma:%n%n\\server\share
InvalidDrive=Unitatea sau partajarea UNC selectată nu există sau nu este accesibilă. Selectaţi alta.
DiskSpaceWarningTitle=Spaţiu insuficient pe disc
DiskSpaceWarning=Programul de instalare necesită cel puţin %1 KB de spaţiu liber pentru instalare, dar unitatea selectată are disponibil numai %2 KB.%n%nDoriţi să continuaţi oricum?
DirNameTooLong=Numele directorului sau calea este prea lungă.
InvalidDirName=Numele directorului nu este valid.
BadDirName32=Numele directoarelor nu pot include niciunul dintre următoarele caractere:%n%n%1
DirExistsTitle=Directorul există
DirExists=Directorul:%n%n%1%n%nexistă deja. Doriţi să instalaţi totuşi în acel director?
DirDoesntExistTitle=Directorul nu există
DirDoesntExist=Directorul:%n%n%1%n%nnu există. Doriţi să fie creat?

; *** "Select Components" wizard page
WizardSelectComponents=Selectaţi componentele
SelectComponentsDesc=Ce componente trebuie instalate?
SelectComponentsLabel2=Selectaţi componentele pe care doriţi să le instalaţi; deselectaţi componentele pe care nu doriţi să le instalaţi. Faceţi clic pe Înainte când sunteţi gata să continuaţi.
FullInstallation=Instalare completă
; if possible don't translate 'Compact' as 'Minimal' (I mean 'Minimal' in your language)
CompactInstallation=Instalare compactă
CustomInstallation=Instalare personalizată
NoUninstallWarningTitle=Componente existente
NoUninstallWarning=Programul de instalare a detectat că următoarele componente sunt deja instalate pe computerul dumneavoastră:%n%n%1%n%nDeselectarea acestor componente nu le va dezinstala.%n%nDoriţi să continuaţi oricum?
ComponentSize1=%1 KB
ComponentSize2=%1 MB
ComponentsDiskSpaceGBLabel=Selecţia curentă necesită cel puţin [gb] GB de spaţiu pe disc.
ComponentsDiskSpaceMBLabel=Selecţia curentă necesită cel puţin [mb] MB de spaţiu pe disc.

; *** "Select Additional Tasks" wizard page
WizardSelectTasks=Selectaţi sarcini suplimentare
SelectTasksDesc=Ce sarcini suplimentare trebuie efectuate?
SelectTasksLabel2=Selectaţi sarcinile suplimentare pe care doriţi să le efectueze programul de instalare în timpul instalării [name], apoi faceţi clic pe Înainte.

; *** "Select Start Menu Folder" wizard page
WizardSelectProgramGroup=Selectaţi directorul din meniul Start
SelectStartMenuFolderDesc=Unde trebuie să plaseze programul de instalare comenzile rapide ale programului?
SelectStartMenuFolderLabel3=Programul de instalare va crea comenzile rapide ale programului în următorul director din meniul Start.
SelectStartMenuFolderBrowseLabel=Pentru a continua, faceţi clic pe Înainte. Dacă doriţi să selectaţi un alt director, faceţi clic pe Răsfoire.
MustEnterGroupName=Trebuie să introduceţi un nume de director.
GroupNameTooLong=Numele directorului sau calea este prea lungă.
InvalidGroupName=Numele directorului nu este valid.
BadGroupName=Numele directorului nu poate include niciunul dintre următoarele caractere:%n%n%1
NoProgramGroupCheck2=&Nu crea director în meniul Start

; *** "Ready to Install" wizard page
WizardReady=Gata de instalare
ReadyLabel1=Programul de instalare este gata să înceapă instalarea [name] pe computerul dumneavoastră.
ReadyLabel2a=Faceţi clic pe Instalare pentru a continua instalarea sau pe Înapoi dacă doriţi să revizuiţi sau să modificaţi setări.
ReadyLabel2b=Faceţi clic pe Instalare pentru a continua instalarea.
ReadyMemoUserInfo=Informaţii utilizator:
ReadyMemoDir=Locaţie destinaţie:
ReadyMemoType=Tip instalare:
ReadyMemoComponents=Componente selectate:
ReadyMemoGroup=Director meniul Start:
ReadyMemoTasks=Sarcini suplimentare:

; *** TDownloadWizardPage wizard page and DownloadTemporaryFile
DownloadingLabel2=Se descarcă fişierele...
ButtonStopDownload=&Oprire descărcare
StopDownload=Sigur doriţi să opriţi descărcarea?
ErrorDownloadAborted=Descărcare abandonată
ErrorDownloadFailed=Descărcare eşuată: %1 %2
ErrorDownloadSizeFailed=Obţinerea dimensiunii a eşuat: %1 %2
ErrorProgress=Progres nevalid: %1 din %2
ErrorFileSize=Dimensiune fişier nevalidă: se aştepta %1, s-a găsit %2

; *** TExtractionWizardPage wizard page and ExtractArchive
ExtractingLabel=Se extrag fişierele...
ButtonStopExtraction=&Oprire extragere
StopExtraction=Sigur doriţi să opriţi extragerea?
ErrorExtractionAborted=Extragere abandonată
ErrorExtractionFailed=Extragere eşuată: %1

; *** Archive extraction failure details
ArchiveIncorrectPassword=Parola este incorectă
ArchiveIsCorrupted=Arhiva este coruptă
ArchiveUnsupportedFormat=Formatul arhivei nu este acceptat

; *** "Preparing to Install" wizard page
WizardPreparing=Pregătire pentru instalare
PreparingDesc=Programul de instalare pregăteşte instalarea [name] pe computerul dumneavoastră.
PreviousInstallNotCompleted=Instalarea/eliminarea unui program anterior nu a fost finalizată. Va trebui să reporniţi computerul pentru a finaliza acea instalare.%n%nDupă repornirea computerului, rulaţi din nou programul de instalare pentru a finaliza instalarea [name].
CannotContinue=Programul de instalare nu poate continua. Faceţi clic pe Anulare pentru a ieşi.
ApplicationsFound=Următoarele aplicaţii folosesc fişiere care trebuie actualizate de programul de instalare. Se recomandă să permiteţi programului de instalare să închidă automat aceste aplicaţii.
ApplicationsFound2=Următoarele aplicaţii folosesc fişiere care trebuie actualizate de programul de instalare. Se recomandă să permiteţi programului de instalare să închidă automat aceste aplicaţii. După finalizarea instalării, programul de instalare va încerca să repornească aplicaţiile.
CloseApplications=Închide &automat aplicaţiile
DontCloseApplications=&Nu închide aplicaţiile
ErrorCloseApplications=Programul de instalare nu a putut închide automat toate aplicaţiile. Se recomandă să închideţi toate aplicaţiile care folosesc fişiere ce trebuie actualizate de programul de instalare înainte de a continua.
PrepareToInstallNeedsRestart=Programul de instalare trebuie să repornească computerul. După repornirea computerului, rulaţi din nou programul de instalare pentru a finaliza instalarea [name].%n%nDoriţi să reporniţi acum?

; *** "Installing" wizard page
WizardInstalling=Instalare
InstallingLabel=Aşteptaţi cât timp programul de instalare instalează [name] pe computerul dumneavoastră.

; *** "Setup Completed" wizard page
FinishedHeadingLabel=Finalizarea expertului de instalare [name]
FinishedLabelNoIcons=Programul de instalare a terminat instalarea [name] pe computerul dumneavoastră.
FinishedLabel=Programul de instalare a terminat instalarea [name] pe computerul dumneavoastră. Aplicaţia poate fi lansată selectând comenzile rapide instalate.
ClickFinish=Faceţi clic pe Finalizare pentru a ieşi din instalare.
FinishedRestartLabel=Pentru a finaliza instalarea [name], programul de instalare trebuie să repornească computerul. Doriţi să reporniţi acum?
FinishedRestartMessage=Pentru a finaliza instalarea [name], programul de instalare trebuie să repornească computerul.%n%nDoriţi să reporniţi acum?
ShowReadmeCheck=Da, doresc să vizualizez fişierul README
YesRadio=&Da, reporneşte computerul acum
NoRadio=&Nu, voi reporni computerul mai târziu
; used for example as 'Run MyProg.exe'
RunEntryExec=Rulează %1
; used for example as 'View Readme.txt'
RunEntryShellExec=Vizualizează %1

; *** "Setup Needs the Next Disk" stuff
ChangeDiskTitle=Programul de instalare necesită discul următor
SelectDiskLabel2=Introduceţi discul %1 şi faceţi clic pe OK.%n%nDacă fişierele de pe acest disc pot fi găsite într-un alt director decât cel afişat mai jos, introduceţi calea corectă sau faceţi clic pe Răsfoire.
PathLabel=&Cale:
FileNotInDir2=Fişierul "%1" nu a putut fi găsit în "%2". Introduceţi discul corect sau selectaţi un alt director.
SelectDirectoryLabel=Specificaţi locaţia discului următor.

; *** Installation phase messages
SetupAborted=Instalarea nu a fost finalizată.%n%nCorectaţi problema şi rulaţi din nou programul de instalare.
AbortRetryIgnoreSelectAction=Selectaţi acţiunea
AbortRetryIgnoreRetry=Încercaţi din &nou
AbortRetryIgnoreIgnore=&Ignoraţi eroarea şi continuaţi
AbortRetryIgnoreCancel=Anulare instalare
RetryCancelSelectAction=Selectaţi acţiunea
RetryCancelRetry=Încercaţi din &nou
RetryCancelCancel=Anulare

; *** Installation status messages
StatusClosingApplications=Se închid aplicaţiile...
StatusCreateDirs=Se creează directoarele...
StatusExtractFiles=Se extrag fişierele...
StatusDownloadFiles=Se descarcă fişierele...
StatusCreateIcons=Se creează comenzile rapide...
StatusCreateIniEntries=Se creează intrările INI...
StatusCreateRegistryEntries=Se creează intrările în registry...
StatusRegisterFiles=Se înregistrează fişierele...
StatusSavingUninstall=Se salvează informaţiile de dezinstalare...
StatusRunProgram=Se finalizează instalarea...
StatusRestartingApplications=Se repornesc aplicaţiile...
StatusRollback=Se anulează modificările...

; *** Misc. errors
ErrorInternal2=Eroare internă: %1
ErrorFunctionFailedNoCode=%1 a eşuat
ErrorFunctionFailed=%1 a eşuat; cod %2
ErrorFunctionFailedWithMessage=%1 a eşuat; cod %2.%n%3
ErrorExecutingProgram=Nu se poate executa fişierul:%n%1

; *** Registry errors
ErrorRegOpenKey=Eroare la deschiderea cheii registry:%n%1\%2
ErrorRegCreateKey=Eroare la crearea cheii registry:%n%1\%2
ErrorRegWriteKey=Eroare la scrierea în cheia registry:%n%1\%2

; *** INI errors
ErrorIniEntry=Eroare la crearea intrării INI în fişierul "%1".

; *** File copying errors
FileAbortRetryIgnoreSkipNotRecommended=&Omite acest fişier (nerecomandat)
FileAbortRetryIgnoreIgnoreNotRecommended=&Ignoră eroarea şi continuă (nerecomandat)
SourceIsCorrupted=Fişierul sursă este corupt
SourceDoesntExist=Fişierul sursă "%1" nu există
SourceVerificationFailed=Verificarea fişierului sursă a eşuat: %1
VerificationSignatureDoesntExist=Fişierul de semnătură "%1" nu există
VerificationSignatureInvalid=Fişierul de semnătură "%1" nu este valid
VerificationKeyNotFound=Fişierul de semnătură "%1" foloseşte o cheie necunoscută
VerificationFileNameIncorrect=Numele fişierului este incorect
VerificationFileTagIncorrect=Eticheta fişierului este incorectă
VerificationFileSizeIncorrect=Dimensiunea fişierului este incorectă
VerificationFileHashIncorrect=Hash-ul fişierului este incorect
ExistingFileReadOnly2=Fişierul existent nu a putut fi înlocuit deoarece este marcat doar pentru citire.
ExistingFileReadOnlyRetry=&Elimină atributul doar pentru citire şi încearcă din nou
ExistingFileReadOnlyKeepExisting=&Păstrează fişierul existent
ErrorReadingExistingDest=A apărut o eroare la încercarea de citire a fişierului existent:
FileExistsSelectAction=Selectaţi acţiunea
FileExists2=Fişierul există deja.
FileExistsOverwriteExisting=&Suprascrie fişierul existent
FileExistsKeepExisting=&Păstrează fişierul existent
FileExistsOverwriteOrKeepAll=&Aplică pentru următoarele conflicte
ExistingFileNewerSelectAction=Selectaţi acţiunea
ExistingFileNewer2=Fişierul existent este mai nou decât cel pe care programul de instalare încearcă să îl instaleze.
ExistingFileNewerOverwriteExisting=&Suprascrie fişierul existent
ExistingFileNewerKeepExisting=&Păstrează fişierul existent (recomandat)
ExistingFileNewerOverwriteOrKeepAll=&Aplică pentru următoarele conflicte
ErrorChangingAttr=A apărut o eroare la încercarea de schimbare a atributelor fişierului existent:
ErrorCreatingTemp=A apărut o eroare la încercarea de creare a unui fişier în directorul de destinaţie:
ErrorReadingSource=A apărut o eroare la încercarea de citire a fişierului sursă:
ErrorCopying=A apărut o eroare la încercarea de copiere a unui fişier:
ErrorDownloading=A apărut o eroare la încercarea de descărcare a unui fişier:
ErrorExtracting=A apărut o eroare la încercarea de extragere a unei arhive:
ErrorReplacingExistingFile=A apărut o eroare la încercarea de înlocuire a fişierului existent:
ErrorRestartReplace=RestartReplace a eşuat:
ErrorRenamingTemp=A apărut o eroare la încercarea de redenumire a unui fişier în directorul de destinaţie:
ErrorRegisterServer=Nu se poate înregistra DLL/OCX: %1
ErrorRegSvr32Failed=RegSvr32 a eşuat cu codul de ieşire %1
ErrorRegisterTypeLib=Nu se poate înregistra biblioteca de tipuri: %1

; *** Uninstall display name markings
; used for example as 'My Program (32-bit)'
UninstallDisplayNameMark=%1 (%2)
; used for example as 'My Program (32-bit, All users)'
UninstallDisplayNameMarks=%1 (%2, %3)
UninstallDisplayNameMark32Bit=32 de biţi
UninstallDisplayNameMark64Bit=64 de biţi
UninstallDisplayNameMarkAllUsers=Toţi utilizatorii
UninstallDisplayNameMarkCurrentUser=Utilizator curent

; *** Post-installation errors
ErrorOpeningReadme=A apărut o eroare la încercarea de deschidere a fişierului README.
ErrorRestartingComputer=Programul de instalare nu a putut reporni computerul. Faceţi acest lucru manual.

; *** Uninstaller messages
UninstallNotFound=Fişierul "%1" nu există. Nu se poate dezinstala.
UninstallOpenError=Fişierul "%1" nu a putut fi deschis. Nu se poate dezinstala
UninstallUnsupportedVer=Fişierul jurnal de dezinstalare "%1" este într-un format nerecunoscut de această versiune a programului de dezinstalare. Nu se poate dezinstala
UninstallUnknownEntry=A fost întâlnită o intrare necunoscută (%1) în jurnalul de dezinstalare
ConfirmUninstall=Sigur doriţi să eliminaţi complet %1 şi toate componentele sale?
UninstallOnlyOnWin64=Această instalare poate fi dezinstalată numai pe Windows pe 64 de biţi.
OnlyAdminCanUninstall=Această instalare poate fi dezinstalată numai de un utilizator cu privilegii administrative.
UninstallStatusLabel=Aşteptaţi cât timp %1 este eliminat de pe computerul dumneavoastră.
UninstalledAll=%1 a fost eliminat cu succes de pe computerul dumneavoastră.
UninstalledMost=Dezinstalarea %1 este completă.%n%nUnele elemente nu au putut fi eliminate. Acestea pot fi eliminate manual.
UninstalledAndNeedsRestart=Pentru a finaliza dezinstalarea %1, computerul trebuie repornit.%n%nDoriţi să reporniţi acum?
UninstallDataCorrupted=Fişierul "%1" este corupt. Nu se poate dezinstala

; *** Uninstallation phase messages
ConfirmDeleteSharedFileTitle=Eliminaţi fişierul partajat?
ConfirmDeleteSharedFile2=Sistemul indică faptul că următorul fişier partajat nu mai este folosit de niciun program. Doriţi ca programul de dezinstalare să elimine acest fişier partajat?%n%nDacă vreun program încă foloseşte acest fişier şi acesta este eliminat, programul respectiv poate să nu funcţioneze corect. Dacă nu sunteţi sigur, alegeţi Nu. Păstrarea fişierului pe sistem nu va cauza probleme.
SharedFileNameLabel=Nume fişier:
SharedFileLocationLabel=Locaţie:
WizardUninstalling=Stare dezinstalare
StatusUninstalling=Se dezinstalează %1...

; *** Shutdown block reasons
ShutdownBlockReasonInstallingApp=Se instalează %1.
ShutdownBlockReasonUninstallingApp=Se dezinstalează %1.

; The custom messages below aren't used by Setup itself, but if you make
; use of them in your scripts, you'll want to translate them.

[CustomMessages]

NameAndVersion=%1 versiunea %2
AdditionalIcons=Comenzi rapide suplimentare:
CreateDesktopIcon=Creează o comandă rapidă pe &desktop
CreateQuickLaunchIcon=Creează o comandă rapidă în &Quick Launch
ProgramOnTheWeb=%1 pe web
UninstallProgram=Dezinstalează %1
LaunchProgram=Lansează %1
AssocFileExtension=&Asociază %1 cu extensia de fişier %2
AssocingFileExtension=Se asociază %1 cu extensia de fişier %2...
AutoStartProgramGroupDescription=Pornire:
AutoStartProgram=Porneşte automat %1
AddonHostProgramNotFound=%1 nu a putut fi găsit în directorul selectat.%n%nDoriţi să continuaţi oricum?

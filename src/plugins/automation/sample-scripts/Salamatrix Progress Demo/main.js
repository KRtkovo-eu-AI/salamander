// Demonstrates the Salamatrix-backed Automation API.
// Requires the Salamatrix Framework plugin to be installed and loaded.

var progress = Salamander.UI.progress("Salamatrix progress demo");
progress.Maximum = 5;
progress.Show();

try {
    for (var i = 1; i <= 5; ++i) {
        progress.AddText("Step " + i + " of 5");
        progress.Position = i;
        Salamander.Sleep(150);
        if (progress.IsCancelled) {
            break;
        }
    }
}
finally {
    progress.CanCancel = false;
    progress.Hide();
}

var commandResult = Salamander.Commands.execute("QuickRename");
var sourceSide = Salamander.Sides.Source;
var activeTab = sourceSide.ActiveTab;
var previousSourcePath = "(Storage unavailable in legacy Automation)";
var storageNamespace = "(legacy Automation)";
try {
    previousSourcePath =
        Salamander.Storage.get("lastSourcePath", "(first run)");
    Salamander.Storage.set("lastSourcePath", activeTab.Path);
    storageNamespace = Salamander.Storage.Namespace;
}
catch (storageError) {
    // The same script can be launched from Automation's legacy script list,
    // where an older installation may not have the adjacent extension.json.
    // The progress/API demonstration remains useful without package storage.
}
var sideSummary =
    "\n\nSource side: " + sourceSide.Name +
    "\nTabs on side: " + sourceSide.TabCount +
    "\nActive tab index: " + activeTab.Index +
    "\nActive tab path: " + activeTab.Path +
    "\nPath saved by previous run: " + previousSourcePath +
    "\nStorage namespace: " + storageNamespace;
if (commandResult == "not_available") {
    Salamander.MsgBox("Quick Rename is not available in the current panel context. Select or focus a file first and run the demo again." + sideSummary, 0, "Salamatrix Demo");
}
else {
    Salamander.MsgBox("Salamatrix command result: " + commandResult + sideSummary, 0, "Salamatrix Demo");
}

// Build the complete gallery here through Salamatrix.UI, just like every
// external runtime demo does through its worker facade.
var dialog = Salamander.UI.dialog("Salamatrix UI capabilities", 463, 236);
function add(kind,id,text,x,y,w,h,style,result) {
    dialog.add(kind,id,text,x,y,w,h,style,result);
}
var uptime = "System was started " + Salamander.UI.uptime + " ms ago.";
add("groupbox","static-group","CGUIStaticTextAbstract",6,4,254,108);
add("label","not-attached-label","Not attached static text",14,17,80,8);
add("label","uptime-plain",uptime,102,17,152,8);
var rows = [
    ["static-none","0 (no flags)",uptime,27,0],
    ["static-cache","STF_CACHED_PAINT",uptime,37,1],
    ["static-bold","STF_BOLD","Bold &text",47,0x10082],
    ["static-underline","STF_UNDERLINE","Underlined text",56,0x20004],
    ["static-end","STF_END_ELLIPSIS","Long long long long long long long long long string.",66,0x20],
    ["static-path","STF_PATH_ELLIPSIS","C:\\Program Files\\Some Application With Long Path\\example.exe",76,0x40],
    ["static-path-url","STF_PATH_ELLIPSIS","ftp://ftp.altap.cz/pub/salamander/example.exe",87,0x40]];
for (var row=0; row<rows.length; ++row) {
    add("label",rows[row][0]+"-label",rows[row][1],14,rows[row][3],75,8);
    add("statictext",rows[row][0],rows[row][2],102,rows[row][3],152,8,rows[row][4]);
}
dialog.set("static-path-url","pathSeparator","/");
add("label","drag-hint","Drag texts to change their size.",151,97,103,8);
add("groupbox","progress-group","CGUIProgressBarAbstract",6,118,254,66);
add("label","progress-label","Progress label",15,129,60,8);
add("progressbar","progress","",15,138,235,12); dialog.set("progress","progress",120);
add("label","unknown-label","Unknown progress",15,154,67,8);
add("progressbar","unknown-progress","",15,163,235,12); dialog.set("unknown-progress","indeterminate",-1,100); dialog.set("unknown-progress","progress",-1);
add("groupbox","buttons-group","Button, CGUITextArrowButtonAbstract, CGUIColorArrowButtonAbstract",6,188,254,40);
add("button","more","...",15,204,15,14); add("arrowbutton","arrow","",37,204,15,14);
add("textarrowbutton","choose","&Choose",60,204,50,14,8); add("textarrowbutton","drop","&Drop",117,204,50,14,2);
add("colorarrowbutton","color","",174,204,33,14); dialog.set("color","color",0xff8000,0xff8000);
add("colorarrowbutton","color-text","ABC",215,204,33,14); dialog.set("color-text","color",0,0xffff);
add("groupbox","hyperlink-group","CGUIHyperLinkAbstract",269,4,185,48);
add("label","open-label","SetActionOpen",277,17,75,8); add("hyperlink","open-link","www.altap.cz",365,17,47,8,0x14); dialog.set("open-link","actionOpen","https://www.altap.cz");
add("label","command-label","SetActionPostCommand",277,27,81,8); add("hyperlink","command-link","Say something!",365,27,55,8,0x14); dialog.set("command-link","actionCommand",0x7f01);
add("label","hint-label","SetActionShowHint",277,37,81,8); add("hyperlink","hint-link","mask hints",365,37,40,8,8); dialog.set("hint-link","actionHint","text 1 text 1 text 1 text 1\ntext 2 text 2 text 2");
add("groupbox","tooltip-group","SetCurrentToolTip",269,59,185,31);
add("statictext","tooltip","Pause the mouse pointer over this text.",278,73,130,8,0x40000); dialog.set("tooltip","toolTip","ToolTip");
add("listview","header-list","",269,113,185,50,0x01e00000);
add("toolbarheader","toolbar-header","CGUIToolbarHeaderAbstract",269,102,96,8); dialog.set("toolbar-header","toolbarHeader","header-list",0x31);
add("groupbox","origin-group","Created by",269,169,185,38);
add("label","runtime-label","Runtime:",277,181,42,8); add("statictext","runtime-value","Automation.JScript",323,181,122,8,2);
add("label","extension-label","Extension:",277,192,42,8); add("statictext","extension-value","Salamatrix Progress Demo",323,192,122,8,2);
add("button","close","Close",403,213,50,14,0x100000,1);
try { dialog.show(); } finally { dialog.close(); }

// SPDX-FileCopyrightText: 2024 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

using System;
using System.Collections.Concurrent;
using System.Diagnostics;
using System.Globalization;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Web.Script.Serialization;
using System.Net;
using System.Net.Http;
using System.Net.Http.Headers;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Runtime.InteropServices;
using Timer = System.Threading.Timer;

namespace OpenSalamander.Samandarin;

public static class EntryPoint
{
    private static bool _visualsEnabled;

    [STAThread]
    public static int Dispatch(string? argument)
    {
        IntPtr parentHandle = IntPtr.Zero;
        try
        {
            EnsureApplicationInitialized();
            var parts = (argument ?? string.Empty).Split(new[] { ';' }, 3);
            var command = parts.Length > 0 ? parts[0] : string.Empty;
            parentHandle = ParseHandle(parts.Length > 1 ? parts[1] : string.Empty);
            var payload = parts.Length > 2 ? parts[2] : string.Empty;

            return command switch
            {
                "Initialize" => Initialize(parentHandle, payload),
                "Configure" => ShowConfiguration(parentHandle),
                "CheckNow" => CheckNow(parentHandle),
                "PluginUpdates" => ShowPluginUpdates(parentHandle),
                "Shutdown" => Shutdown(),
                "ColorsChanged" => ColorsChanged(),
                _ => 1,
            };
        }
        catch (Exception ex)
        {
            ShowError(parentHandle, NativeStrings.Get(NativeStringId.UnexpectedException), ex);
            return -1;
        }
    }

    private static int Initialize(IntPtr parent, string currentVersion)
    {
        UpdateCoordinator.Initialize(currentVersion, parent);
        return 0;
    }

    private static int ShowConfiguration(IntPtr parent)
    {
        using var dialog = new ConfigurationDialog(UpdateCoordinator.GetSnapshot());
        ThemeHelper.ApplyTheme(dialog);
        var result = ShowDialog(dialog, parent);
        if (result == DialogResult.OK)
        {
            UpdateCoordinator.ApplySettings(dialog.Settings);
        }

        return 0;
    }

    private static int CheckNow(IntPtr parent)
    {
        _ = UpdateCoordinator.CheckForUpdatesAsync(parent, userInitiated: true, showIfCurrent: true);
        return 0;
    }

    private static int ShowPluginUpdates(IntPtr parent)
    {
        using var dialog = new PluginUpdatesDialog();
        ThemeHelper.ApplyTheme(dialog);
        ShowDialog(dialog, parent);
        return 0;
    }

    private static int Shutdown()
    {
        UpdateCoordinator.Shutdown();
        return 0;
    }

    private static int ColorsChanged()
    {
        ThemeHelper.InvalidatePalette();
        return 0;
    }

    private static void EnsureApplicationInitialized()
    {
        if (_visualsEnabled)
        {
            return;
        }

        Application.EnableVisualStyles();
        Application.SetCompatibleTextRenderingDefault(false);
        _visualsEnabled = true;
    }

    private static DialogResult ShowDialog(Form dialog, IntPtr parent)
    {
        IWin32Window? owner = parent != IntPtr.Zero ? new WindowHandleWrapper(parent) : null;
        return owner is null ? dialog.ShowDialog() : dialog.ShowDialog(owner);
    }

    private static IntPtr ParseHandle(string text)
    {
        if (ulong.TryParse(text, NumberStyles.Integer, CultureInfo.InvariantCulture, out var value))
        {
            return new IntPtr(unchecked((long)value));
        }

        return IntPtr.Zero;
    }

    private static void ShowError(IntPtr parent, string caption, Exception ex)
    {
        var owner = parent != IntPtr.Zero ? new WindowHandleWrapper(parent) : null;
        var message = $"{caption}{Environment.NewLine}{ex.Message}";
        ThemeHelper.ShowMessageBox(owner, message, NativeStrings.PluginCaption, MessageBoxButtons.OK, MessageBoxIcon.Error);
    }
}

internal enum NativeStringId
{
    PluginName = 46,
    UnexpectedException = 51,
    UpdateAvailable = 55,
    CurrentVersion = 56,
    LatestVersion = 57,
    OpenDownloadPage = 58,
    OpenBrowserError = 59,
    UpToDateRelease = 60,
    UpToDateNoRelease = 61,
    CheckError = 62,
    Unknown = 63,
    ConfigTitle = 64,
    ConfigDescription = 65,
    CheckOnStartup = 66,
    PeriodicCheck = 67,
    FrequencyDisabled = 68,
    FrequencyDaily = 69,
    FrequencyWeekly = 70,
    FrequencyMonthly = 71,
    Ok = 72,
    Cancel = 73,
    CheckNow = 74,
    StatusChecking = 75,
    PerformCheckError = 76,
    StatusCompleted = 77,
    LastAutoCheck = 78,
    LastAutoCheckNever = 79,
    LastKnownRelease = 80,
    LastKnownReleaseUnknown = 81,
    MenuPluginUpdates = 82,
    PluginUpdatesTitle = 83,
    PluginUpdatesDescription = 84,
    PluginUpdatesRefresh = 85,
    PluginUpdatesOpenPage = 86,
    PluginUpdatesClose = 87,
    PluginUpdatesShowOnly = 88,
    PluginUpdatesSources = 89,
    PluginUpdatesSaveSources = 90,
    PluginColumnName = 91,
    PluginColumnInstalled = 92,
    PluginColumnLatest = 93,
    PluginColumnStatus = 94,
    PluginColumnSource = 95,
    PluginStatusCurrent = 96,
    PluginStatusUpdate = 97,
    PluginStatusUnknownVersion = 98,
    PluginStatusNotInCatalog = 99,
    PluginStatusCatalogError = 100,
    PluginStatusDifferent = 101,
    PluginUpdatesLoading = 102,
    PluginUpdatesReady = 103,
    PluginUpdatesNoUrl = 104,
    PluginSourcesSaved = 105,
    PluginColumnAuthor = 106,
    PluginColumnHomepage = 107,
}

internal static class NativeStrings
{
    private const int BufferLength = 1024;

    public static string PluginCaption => Get(NativeStringId.PluginName);

    public static string Get(NativeStringId id)
    {
        var buffer = new StringBuilder(BufferLength);
        int length = Samandarin_LoadString((int)id, buffer, buffer.Capacity);
        return length > 0 ? buffer.ToString() : id.ToString();
    }

    public static string Format(NativeStringId id, params object[] args)
    {
        return string.Format(CultureInfo.CurrentCulture, Get(id), args);
    }

    [DllImport("Samandarin.Spl", EntryPoint = "Samandarin_LoadString", ExactSpelling = true, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
    private static extern int Samandarin_LoadString(int resourceId, StringBuilder buffer, int bufferLength);
}

internal static class UpdateCoordinator
{
    private static readonly object SyncRoot = new();
    private static readonly Uri ReleasesUri = new("https://github.com/KRtkovo-eu-AI/salamander/releases/latest");
    private static readonly SemaphoreSlim CheckSemaphore = new(1, 1);
    private static readonly TimeSpan MinimumDelay = TimeSpan.FromSeconds(10);
    private static readonly HttpClient HttpClient;
    private static readonly object UiThreadLock = new();

    private static UpdateSettings Settings;
    private static string CurrentVersion = string.Empty;
    private static Timer? UpdateTimer;
    private static BlockingCollection<Action>? UiQueue;
    private static Thread? UiThread;

    static UpdateCoordinator()
    {
        Settings = UpdateSettings.Load();
        EnableModernTlsProtocols();

        var handler = new HttpClientHandler
        {
            AllowAutoRedirect = true,
            AutomaticDecompression = DecompressionMethods.GZip | DecompressionMethods.Deflate,
            UseProxy = true,
            Proxy = WebRequest.DefaultWebProxy,
        };

        HttpClient = new HttpClient(handler, disposeHandler: true)
        {
            Timeout = TimeSpan.FromSeconds(15),
        };
        HttpClient.DefaultRequestHeaders.UserAgent.ParseAdd("SamandarinUpdateNotifier/1.0");
    }

    public static void Initialize(string currentVersion, IntPtr parent)
    {
        lock (SyncRoot)
        {
            CurrentVersion = (currentVersion ?? string.Empty).Trim();
            ScheduleTimer_NoLock();
        }

        if (Settings.CheckOnStartup)
        {
            _ = CheckForUpdatesAsync(parent, userInitiated: false, showIfCurrent: false);
        }
    }

    public static void ApplySettings(UpdateSettings newSettings)
    {
        lock (SyncRoot)
        {
            Settings = newSettings.Clone();
            SaveSettings_NoLock();
            ScheduleTimer_NoLock();
        }
    }

    public static UpdateSnapshot GetSnapshot()
    {
        lock (SyncRoot)
        {
            return new UpdateSnapshot(Settings.Clone(), CurrentVersion);
        }
    }

    public static async Task CheckForUpdatesAsync(IntPtr parent, bool userInitiated, bool showIfCurrent)
    {
        await CheckSemaphore.WaitAsync().ConfigureAwait(false);
        try
        {
            string? latestVersion = null;
            bool noPublishedReleases = false;
            string? errorMessage = null;

            try
            {
                (latestVersion, noPublishedReleases) = await FetchLatestVersionAsync().ConfigureAwait(false);
            }
            catch (Exception ex)
            {
                errorMessage = BuildErrorMessage(ex);
            }

            bool notify = false;
            bool showCurrentMessage = false;
            string? latestVersionForMessage = null;

            lock (SyncRoot)
            {
                Settings.LastCheckUtc = DateTimeOffset.UtcNow;

                if (!string.IsNullOrEmpty(latestVersion))
                {
                    Settings.LastKnownRemoteVersion = latestVersion;
                    int comparison = VersionComparer.Compare(latestVersion, CurrentVersion);
                    if (comparison > 0 && !string.Equals(Settings.LastPromptedVersion, latestVersion, StringComparison.OrdinalIgnoreCase))
                    {
                        Settings.LastPromptedVersion = latestVersion;
                        notify = true;
                    }
                    else if (comparison <= 0 && showIfCurrent)
                    {
                        showCurrentMessage = true;
                        latestVersionForMessage = latestVersion;
                    }
                }
                else if (showIfCurrent && string.IsNullOrEmpty(errorMessage))
                {
                    var storedVersion = Settings.LastKnownRemoteVersion;
                    if (noPublishedReleases)
                    {
                        showCurrentMessage = true;
                        latestVersionForMessage = storedVersion;
                    }
                    else if (!string.IsNullOrWhiteSpace(storedVersion) && VersionComparer.Compare(storedVersion, CurrentVersion) <= 0)
                    {
                        showCurrentMessage = true;
                        latestVersionForMessage = storedVersion;
                    }
                }

                SaveSettings_NoLock();
                ScheduleTimer_NoLock();
            }

            if (latestVersion is string latestVersionValue && latestVersionValue.Length > 0)
            {
                if (notify)
                {
                    await ShowUpdateAvailableAsync(parent, latestVersionValue).ConfigureAwait(false);
                }
                else if (showCurrentMessage)
                {
                    var versionToShow = latestVersionForMessage ?? latestVersionValue;
                    await ShowUpToDateAsync(parent, versionToShow).ConfigureAwait(false);
                }
            }
            else if (errorMessage is not null && userInitiated)
            {
                await ShowErrorAsync(parent, errorMessage).ConfigureAwait(false);
            }
            else if (showCurrentMessage && userInitiated)
            {
                await ShowUpToDateAsync(parent, latestVersionForMessage).ConfigureAwait(false);
            }
        }
        finally
        {
            CheckSemaphore.Release();
        }
    }

    public static void Shutdown()
    {
        lock (SyncRoot)
        {
            UpdateTimer?.Dispose();
            UpdateTimer = null;
        }

        CheckSemaphore.Wait();
        CheckSemaphore.Release();

        StopUiThread();
    }

    private static void EnableModernTlsProtocols()
    {
        try
        {
            ServicePointManager.SecurityProtocol |= SecurityProtocolType.Tls12;

            const SecurityProtocolType tls13 = (SecurityProtocolType)0x00003000;
            if (Enum.IsDefined(typeof(SecurityProtocolType), tls13))
            {
                ServicePointManager.SecurityProtocol |= tls13;
            }
        }
        catch (NotSupportedException)
        {
            // Older platforms may not allow overriding protocol settings; ignore.
        }
    }

    private static async Task<(string? Version, bool NoPublishedReleases)> FetchLatestVersionAsync()
    {
        using var request = new HttpRequestMessage(HttpMethod.Get, ReleasesUri);
        using var response = await HttpClient.SendAsync(request, HttpCompletionOption.ResponseHeadersRead).ConfigureAwait(false);
        response.EnsureSuccessStatusCode();

        var finalUri = response.RequestMessage?.RequestUri ?? ReleasesUri;
        string finalPath = finalUri.AbsolutePath.TrimEnd('/');
        bool noPublishedReleases = finalPath.EndsWith("/releases", StringComparison.OrdinalIgnoreCase);
        return (ExtractVersionFromUri(finalUri), noPublishedReleases);
    }

    private static string BuildErrorMessage(Exception exception)
    {
        var builder = new StringBuilder();
        var current = exception;
        while (current is not null)
        {
            if (builder.Length > 0)
            {
                builder.Append(" → ");
            }

            builder.Append(current.Message);
            current = current.InnerException;
        }

        return builder.Length > 0 ? builder.ToString() : exception.Message;
    }

    private static string? ExtractVersionFromUri(Uri? uri)
    {
        if (uri is null)
        {
            return null;
        }

        var segments = uri.AbsolutePath.Split(new[] { '/' }, StringSplitOptions.RemoveEmptyEntries);
        if (segments.Length == 0)
        {
            return null;
        }

        var candidate = segments[segments.Length - 1];
        if (candidate.Equals("latest", StringComparison.OrdinalIgnoreCase) && segments.Length > 1)
        {
            candidate = segments[segments.Length - 2];
        }

        candidate = Uri.UnescapeDataString(candidate).Trim();
        return IsRecognizedReleaseTag(candidate) ? candidate : null;
    }

    private static bool IsRecognizedReleaseTag(string value)
    {
        if (string.IsNullOrEmpty(value))
        {
            return false;
        }

        var segments = value.Split(new[] { '-' }, StringSplitOptions.RemoveEmptyEntries);
        if (segments.Length < 3)
        {
            return false;
        }

        int markerIndex = -1;
        for (int i = 0; i < segments.Length; i++)
        {
            if (segments[i].Equals("samandarin", StringComparison.OrdinalIgnoreCase))
            {
                markerIndex = i;
                break;
            }
        }

        if (markerIndex <= 0 || markerIndex == segments.Length - 1)
        {
            return false;
        }

        bool hasNumericPrefix = false;
        for (int i = 0; i < markerIndex; i++)
        {
            if (!IsNumericVersionComponent(segments[i]))
            {
                return false;
            }

            hasNumericPrefix = true;
        }

        if (!hasNumericPrefix)
        {
            return false;
        }

        string versionSegment = segments[markerIndex + 1];
        for (int i = markerIndex + 2; i < segments.Length; i++)
        {
            versionSegment += "-" + segments[i];
        }

        return IsNumericVersion(versionSegment);
    }

    private static bool IsNumericVersion(string value)
    {
        if (string.IsNullOrEmpty(value))
        {
            return false;
        }

        var parts = value.Split(new[] { '.' }, StringSplitOptions.RemoveEmptyEntries);
        if (parts.Length == 0)
        {
            return false;
        }

        foreach (var part in parts)
        {
            if (part.Length == 0)
            {
                return false;
            }

            for (int i = 0; i < part.Length; i++)
            {
                if (!char.IsDigit(part[i]))
                {
                    return false;
                }
            }
        }

        return true;
    }

    private static bool IsNumericVersionComponent(string value)
    {
        if (string.IsNullOrEmpty(value))
        {
            return false;
        }

        bool hasDigit = false;
        for (int i = 0; i < value.Length; i++)
        {
            char c = value[i];
            if (char.IsDigit(c))
            {
                hasDigit = true;
                continue;
            }

            if (c != '.')
            {
                return false;
            }
        }

        return hasDigit;
    }

    private static void ScheduleTimer_NoLock()
    {
        UpdateTimer?.Dispose();
        UpdateTimer = null;

        if (Settings.Frequency == UpdateFrequency.Disabled)
        {
            return;
        }

        TimeSpan interval = Settings.Frequency switch
        {
            UpdateFrequency.Daily => TimeSpan.FromDays(1),
            UpdateFrequency.Monthly => TimeSpan.FromDays(30),
            _ => TimeSpan.FromDays(7),
        };

        var next = Settings.LastCheckUtc.HasValue
            ? Settings.LastCheckUtc.Value + interval
            : DateTimeOffset.UtcNow + MinimumDelay;
        var now = DateTimeOffset.UtcNow;
        if (next <= now)
        {
            next = now + MinimumDelay;
        }

        var due = next - now;
        if (due < MinimumDelay)
        {
            due = MinimumDelay;
        }

        UpdateTimer = new Timer(_ => TimerCallback(), null, due, Timeout.InfiniteTimeSpan);
    }

    private static void TimerCallback()
    {
        _ = CheckForUpdatesAsync(IntPtr.Zero, userInitiated: false, showIfCurrent: false);
    }

    private static async Task ShowUpdateAvailableAsync(IntPtr parent, string latestVersion)
    {
        string current = GetCurrentVersion();
        string message = NativeStrings.Get(NativeStringId.UpdateAvailable) + Environment.NewLine + Environment.NewLine +
            NativeStrings.Format(NativeStringId.CurrentVersion, current) + Environment.NewLine +
            NativeStrings.Format(NativeStringId.LatestVersion, latestVersion) + Environment.NewLine + Environment.NewLine +
            NativeStrings.Get(NativeStringId.OpenDownloadPage);

        var result = await ShowMessageAsync(parent, owner => ThemeHelper.ShowMessageBox(owner, message, NativeStrings.PluginCaption, MessageBoxButtons.OKCancel, MessageBoxIcon.Information)).ConfigureAwait(false);
        if (result == DialogResult.OK)
        {
            await RunOnUiThreadAsync(parent, owner =>
            {
                try
                {
                    var info = new ProcessStartInfo("https://github.com/KRtkovo-eu-AI/salamander/releases/latest")
                    {
                        UseShellExecute = true,
                    };
                    Process.Start(info);
                }
                catch (Exception ex)
                {
                    var errorMessage = $"{NativeStrings.Get(NativeStringId.OpenBrowserError)}{Environment.NewLine}{ex.Message}";
                    ThemeHelper.ShowMessageBox(owner, errorMessage, NativeStrings.PluginCaption, MessageBoxButtons.OK, MessageBoxIcon.Error);
                }
            }).ConfigureAwait(false);
        }
    }

    private static Task ShowUpToDateAsync(IntPtr parent, string? latestVersion)
    {
        string current = GetCurrentVersion();
        string message = latestVersion is { Length: > 0 }
            ? NativeStrings.Format(NativeStringId.UpToDateRelease, current, latestVersion)
            : NativeStrings.Format(NativeStringId.UpToDateNoRelease, current);
        return ShowMessageAsync(parent, owner => ThemeHelper.ShowMessageBox(owner, message, NativeStrings.PluginCaption, MessageBoxButtons.OK, MessageBoxIcon.Information));
    }

    private static Task ShowErrorAsync(IntPtr parent, string error)
    {
        string message = $"{NativeStrings.Get(NativeStringId.CheckError)}{Environment.NewLine}{error}";
        return ShowMessageAsync(parent, owner => ThemeHelper.ShowMessageBox(owner, message, NativeStrings.PluginCaption, MessageBoxButtons.OK, MessageBoxIcon.Error));
    }

    private static Task<DialogResult> ShowMessageAsync(IntPtr parent, Func<IWin32Window?, DialogResult> presenter)
    {
        var completion = new TaskCompletionSource<DialogResult>();
        RunOnUiThread(() =>
        {
            try
            {
                var owner = parent != IntPtr.Zero ? new WindowHandleWrapper(parent) : null;
                completion.SetResult(presenter(owner));
            }
            catch (Exception ex)
            {
                completion.SetException(ex);
            }
        });
        return completion.Task;
    }

    private static Task RunOnUiThreadAsync(IntPtr parent, Action<IWin32Window?> action)
    {
        var completion = new TaskCompletionSource<bool>();
        RunOnUiThread(() =>
        {
            try
            {
                var owner = parent != IntPtr.Zero ? new WindowHandleWrapper(parent) : null;
                action(owner);
                completion.SetResult(true);
            }
            catch (Exception ex)
            {
                completion.SetException(ex);
            }
        });
        return completion.Task;
    }

    private static void RunOnUiThread(Action action)
    {
        EnsureUiThread();

        var queue = UiQueue;
        if (queue is null)
        {
            action();
            return;
        }

        if (Thread.CurrentThread == UiThread)
        {
            action();
            return;
        }

        try
        {
            queue.Add(action);
        }
        catch (InvalidOperationException)
        {
            // The queue has been marked complete during shutdown; run inline as a best effort.
            action();
        }
    }

    private static string GetCurrentVersion()
    {
        lock (SyncRoot)
        {
            return string.IsNullOrWhiteSpace(CurrentVersion) ? NativeStrings.Get(NativeStringId.Unknown) : CurrentVersion;
        }
    }

    private static void SaveSettings_NoLock()
    {
        Settings.Save();
    }

    private static void EnsureUiThread()
    {
        if (UiQueue is not null)
        {
            return;
        }

        lock (UiThreadLock)
        {
            if (UiQueue is not null)
            {
                return;
            }

            var queue = new BlockingCollection<Action>();
            var thread = new Thread(() => UiThreadLoop(queue))
            {
                IsBackground = true,
                Name = "Samandarin UI Thread",
            };
            thread.SetApartmentState(ApartmentState.STA);

            UiQueue = queue;
            UiThread = thread;
            thread.Start();
        }
    }

    private static void UiThreadLoop(BlockingCollection<Action> queue)
    {
        foreach (var action in queue.GetConsumingEnumerable())
        {
            try
            {
                action();
            }
            catch (Exception ex)
            {
                Debug.WriteLine($"Samandarin UI thread action failed: {ex}");
            }
        }
    }

    private static void StopUiThread()
    {
        BlockingCollection<Action>? queue;
        Thread? thread;

        lock (UiThreadLock)
        {
            queue = UiQueue;
            thread = UiThread;
            UiQueue = null;
            UiThread = null;
            queue?.CompleteAdding();
        }

        if (thread is not null)
        {
            try
            {
                thread.Join(TimeSpan.FromSeconds(2));
            }
            catch (ThreadStateException)
            {
                // The thread was never started or has already terminated.
            }
        }

        queue?.Dispose();
    }
}

internal sealed class ConfigurationDialog : Form
{
    private readonly CheckBox _checkOnStartup;
    private readonly ComboBox _frequency;
    private readonly Label _currentVersionLabel;
    private readonly Label _lastCheckLabel;
    private readonly Label _latestVersionLabel;
    private readonly Label _statusLabel;
    private readonly Button _checkNowButton;

    private UpdateSettings _settings;

    public ConfigurationDialog(UpdateSnapshot snapshot)
    {
        _settings = snapshot.Settings.Clone();

        Text = NativeStrings.Get(NativeStringId.ConfigTitle);
        StartPosition = FormStartPosition.CenterParent;
        FormBorderStyle = FormBorderStyle.FixedDialog;
        MaximizeBox = false;
        MinimizeBox = false;
        ShowInTaskbar = false;
        AutoSize = true;
        AutoSizeMode = AutoSizeMode.GrowAndShrink;

        var layout = new TableLayoutPanel
        {
            Dock = DockStyle.Fill,
            AutoSize = true,
            AutoSizeMode = AutoSizeMode.GrowAndShrink,
            ColumnCount = 2,
            RowCount = 8,
            Padding = new Padding(12),
        };
        layout.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));
        layout.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100f));

        var description = new Label
        {
            Text = NativeStrings.Get(NativeStringId.ConfigDescription),
            AutoSize = true,
            MaximumSize = new System.Drawing.Size(460, 0),
        };
        layout.SetColumnSpan(description, 2);
        layout.Controls.Add(description, 0, 0);

        _checkOnStartup = new CheckBox
        {
            Text = NativeStrings.Get(NativeStringId.CheckOnStartup),
            AutoSize = true,
        };
        layout.SetColumnSpan(_checkOnStartup, 2);
        layout.Controls.Add(_checkOnStartup, 0, 1);

        var frequencyLabel = new Label
        {
            Text = NativeStrings.Get(NativeStringId.PeriodicCheck),
            AutoSize = true,
            Anchor = AnchorStyles.Left,
        };
        layout.Controls.Add(frequencyLabel, 0, 2);

        _frequency = new ComboBox
        {
            DropDownStyle = ComboBoxStyle.DropDownList,
            Width = 200,
        };
        _frequency.Items.AddRange(new object[]
        {
            new FrequencyOption(UpdateFrequency.Disabled, NativeStrings.Get(NativeStringId.FrequencyDisabled)),
            new FrequencyOption(UpdateFrequency.Daily, NativeStrings.Get(NativeStringId.FrequencyDaily)),
            new FrequencyOption(UpdateFrequency.Weekly, NativeStrings.Get(NativeStringId.FrequencyWeekly)),
            new FrequencyOption(UpdateFrequency.Monthly, NativeStrings.Get(NativeStringId.FrequencyMonthly)),
        });
        layout.Controls.Add(_frequency, 1, 2);

        _currentVersionLabel = new Label { AutoSize = true };
        layout.SetColumnSpan(_currentVersionLabel, 2);
        layout.Controls.Add(_currentVersionLabel, 0, 3);

        _lastCheckLabel = new Label { AutoSize = true };
        layout.SetColumnSpan(_lastCheckLabel, 2);
        layout.Controls.Add(_lastCheckLabel, 0, 4);

        _latestVersionLabel = new Label { AutoSize = true };
        layout.SetColumnSpan(_latestVersionLabel, 2);
        layout.Controls.Add(_latestVersionLabel, 0, 5);

        _statusLabel = new Label { AutoSize = true };
        layout.SetColumnSpan(_statusLabel, 2);
        layout.Controls.Add(_statusLabel, 0, 6);

        var buttons = new FlowLayoutPanel
        {
            FlowDirection = FlowDirection.RightToLeft,
            AutoSize = true,
            AutoSizeMode = AutoSizeMode.GrowAndShrink,
            Dock = DockStyle.Fill,
            Padding = new Padding(0, 12, 0, 0),
        };

        var okButton = new Button { Text = NativeStrings.Get(NativeStringId.Ok), DialogResult = DialogResult.OK, AutoSize = true };
        var cancelButton = new Button { Text = NativeStrings.Get(NativeStringId.Cancel), DialogResult = DialogResult.Cancel, AutoSize = true };
        _checkNowButton = new Button { Text = NativeStrings.Get(NativeStringId.CheckNow), AutoSize = true };
        _checkNowButton.Click += CheckNowButtonOnClick;

        buttons.Controls.Add(okButton);
        buttons.Controls.Add(cancelButton);
        buttons.Controls.Add(_checkNowButton);

        layout.SetColumnSpan(buttons, 2);
        layout.Controls.Add(buttons, 0, 7);

        Controls.Add(layout);

        AcceptButton = okButton;
        CancelButton = cancelButton;

        _checkOnStartup.Checked = _settings.CheckOnStartup;
        SelectFrequency(_settings.Frequency);
        _currentVersionLabel.Text = string.IsNullOrWhiteSpace(snapshot.CurrentVersion)
            ? NativeStrings.Format(NativeStringId.CurrentVersion, NativeStrings.Get(NativeStringId.Unknown))
            : NativeStrings.Format(NativeStringId.CurrentVersion, snapshot.CurrentVersion);
        UpdateStatusLabels();
        _statusLabel.Text = string.Empty;
    }

    public UpdateSettings Settings => new UpdateSettings
    {
        CheckOnStartup = _checkOnStartup.Checked,
        Frequency = ((_frequency.SelectedItem as FrequencyOption) ?? new FrequencyOption(UpdateFrequency.Weekly, string.Empty)).Frequency,
        LastCheckUtc = _settings.LastCheckUtc,
        LastPromptedVersion = _settings.LastPromptedVersion,
        LastKnownRemoteVersion = _settings.LastKnownRemoteVersion,
    };

    private async void CheckNowButtonOnClick(object? sender, EventArgs e)
    {
        _checkNowButton.Enabled = false;
        _statusLabel.Text = NativeStrings.Get(NativeStringId.StatusChecking);
        try
        {
            await UpdateCoordinator.CheckForUpdatesAsync(Handle, userInitiated: true, showIfCurrent: true).ConfigureAwait(true);
        }
        catch (Exception ex)
        {
            ThemeHelper.ShowMessageBox(this, $"{NativeStrings.Get(NativeStringId.PerformCheckError)}{Environment.NewLine}{ex.Message}", NativeStrings.PluginCaption, MessageBoxButtons.OK, MessageBoxIcon.Error);
        }
        finally
        {
            _checkNowButton.Enabled = true;
            UpdateStatusLabels();
            _statusLabel.Text = NativeStrings.Get(NativeStringId.StatusCompleted);
        }
    }

    private void UpdateStatusLabels()
    {
        var snapshot = UpdateCoordinator.GetSnapshot();
        _settings = snapshot.Settings.Clone();
        _lastCheckLabel.Text = _settings.LastCheckUtc.HasValue
            ? NativeStrings.Format(NativeStringId.LastAutoCheck, _settings.LastCheckUtc.Value.ToLocalTime().ToString("g", CultureInfo.CurrentCulture))
            : NativeStrings.Get(NativeStringId.LastAutoCheckNever);
        _latestVersionLabel.Text = string.IsNullOrWhiteSpace(_settings.LastKnownRemoteVersion)
            ? NativeStrings.Get(NativeStringId.LastKnownReleaseUnknown)
            : NativeStrings.Format(NativeStringId.LastKnownRelease, _settings.LastKnownRemoteVersion!);
    }

    private void SelectFrequency(UpdateFrequency frequency)
    {
        for (int i = 0; i < _frequency.Items.Count; i++)
        {
            if (_frequency.Items[i] is FrequencyOption option && option.Frequency == frequency)
            {
                _frequency.SelectedIndex = i;
                return;
            }
        }

        _frequency.SelectedIndex = 2; // default to weekly
    }

    private sealed class FrequencyOption
    {
        public FrequencyOption(UpdateFrequency frequency, string text)
        {
            Frequency = frequency;
            Text = text;
        }

        public UpdateFrequency Frequency { get; }
        public string Text { get; }

        public override string ToString() => Text;
    }
}


internal sealed class PluginUpdatesDialog : Form
{
    private readonly ListView _listView;
    private readonly TextBox _sourcesTextBox;
    private readonly CheckBox _showOnlyUpdates;
    private readonly Label _statusLabel;
    private readonly Button _openButton;
    private readonly List<PluginUpdateRow> _rows = new();
    private int _sortColumn;
    private SortOrder _sortOrder = SortOrder.Ascending;

    public PluginUpdatesDialog()
    {
        Text = NativeStrings.Get(NativeStringId.PluginUpdatesTitle);
        StartPosition = FormStartPosition.CenterParent;
        MinimizeBox = false;
        ShowInTaskbar = false;
        Width = 980;
        Height = 640;
        MinimumSize = new System.Drawing.Size(720, 520);
        Icon = PluginIconLoader.Load();

        var layout = new TableLayoutPanel
        {
            Dock = DockStyle.Fill,
            ColumnCount = 1,
            RowCount = 7,
            Padding = new Padding(12),
        };
        layout.RowStyles.Add(new RowStyle(SizeType.AutoSize));
        layout.RowStyles.Add(new RowStyle(SizeType.Percent, 100f));
        layout.RowStyles.Add(new RowStyle(SizeType.AutoSize));
        layout.RowStyles.Add(new RowStyle(SizeType.AutoSize));
        layout.RowStyles.Add(new RowStyle(SizeType.Absolute, 84));
        layout.RowStyles.Add(new RowStyle(SizeType.AutoSize));
        layout.RowStyles.Add(new RowStyle(SizeType.AutoSize));

        layout.Controls.Add(new Label { Text = NativeStrings.Get(NativeStringId.PluginUpdatesDescription), AutoSize = true, MaximumSize = new System.Drawing.Size(800, 0) }, 0, 0);

        _listView = new ListView
        {
            Dock = DockStyle.Fill,
            FullRowSelect = true,
            GridLines = false,
            HideSelection = false,
            MultiSelect = false,
            View = View.Details,
        };
        _listView.Columns.Add(NativeStrings.Get(NativeStringId.PluginColumnName), 220);
        _listView.Columns.Add(NativeStrings.Get(NativeStringId.PluginColumnInstalled), 120);
        _listView.Columns.Add(NativeStrings.Get(NativeStringId.PluginColumnLatest), 120);
        _listView.Columns.Add(NativeStrings.Get(NativeStringId.PluginColumnStatus), 150);
        _listView.Columns.Add(NativeStrings.Get(NativeStringId.PluginColumnAuthor), 140);
        _listView.Columns.Add(NativeStrings.Get(NativeStringId.PluginColumnHomepage), 220);
        _listView.Columns.Add(NativeStrings.Get(NativeStringId.PluginColumnSource), 260);
        _listView.ColumnClick += ListViewOnColumnClick;
        _listView.MouseDoubleClick += ListViewOnMouseDoubleClick;
        layout.Controls.Add(_listView, 0, 1);

        var aboveSourcesPanel = new TableLayoutPanel
        {
            Dock = DockStyle.Fill,
            AutoSize = true,
            ColumnCount = 2,
            Padding = new Padding(0, 6, 0, 6),
        };
        aboveSourcesPanel.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100f));
        aboveSourcesPanel.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));

        _showOnlyUpdates = new CheckBox { Text = NativeStrings.Get(NativeStringId.PluginUpdatesShowOnly), AutoSize = true, Anchor = AnchorStyles.Left };
        _showOnlyUpdates.CheckedChanged += (_, _) => BindRows();
        aboveSourcesPanel.Controls.Add(_showOnlyUpdates, 0, 0);

        _openButton = new Button { Text = NativeStrings.Get(NativeStringId.PluginUpdatesOpenPage), AutoSize = true, Anchor = AnchorStyles.Right };
        _openButton.Click += (_, _) => OpenSelectedPage();
        aboveSourcesPanel.Controls.Add(_openButton, 1, 0);
        layout.Controls.Add(aboveSourcesPanel, 0, 2);

        layout.Controls.Add(new Label { Text = NativeStrings.Get(NativeStringId.PluginUpdatesSources), AutoSize = true }, 0, 3);

        _sourcesTextBox = new TextBox { Multiline = true, ScrollBars = ScrollBars.Vertical, Dock = DockStyle.Fill, Text = string.Join(Environment.NewLine, PluginCatalogSources.Load()) };
        layout.Controls.Add(_sourcesTextBox, 0, 4);

        _statusLabel = new Label { AutoSize = true, Padding = new Padding(0, 6, 0, 0) };
        layout.Controls.Add(_statusLabel, 0, 5);

        var buttons = new FlowLayoutPanel
        {
            Dock = DockStyle.Fill,
            AutoSize = true,
            WrapContents = false,
            FlowDirection = FlowDirection.RightToLeft,
            Padding = new Padding(0, 10, 0, 0),
            Margin = new Padding(0),
        };
        var closeButton = new Button { Text = NativeStrings.Get(NativeStringId.PluginUpdatesClose), DialogResult = DialogResult.Cancel, AutoSize = true };
        var refreshButton = new Button { Text = NativeStrings.Get(NativeStringId.PluginUpdatesRefresh), AutoSize = true };
        var saveButton = new Button { Text = NativeStrings.Get(NativeStringId.PluginUpdatesSaveSources), AutoSize = true };
        refreshButton.Click += async (_, _) => await RefreshAsync().ConfigureAwait(true);
        saveButton.Click += (_, _) => SaveSources(showMessage: true);
        buttons.Controls.Add(closeButton);
        buttons.Controls.Add(refreshButton);
        buttons.Controls.Add(saveButton);
        layout.Controls.Add(buttons, 0, 6);

        Controls.Add(layout);
        CancelButton = closeButton;
        Shown += async (_, _) => await RefreshAsync().ConfigureAwait(true);
    }

    private async Task RefreshAsync()
    {
        SaveSources(showMessage: false);
        _statusLabel.Text = NativeStrings.Get(NativeStringId.PluginUpdatesLoading);
        _openButton.Enabled = false;
        try
        {
            var result = await PluginCatalogService.CheckAsync().ConfigureAwait(true);
            _rows.Clear();
            _rows.AddRange(result);
            BindRows();
            _statusLabel.Text = PluginCatalogService.LastErrors.Count == 0
                ? NativeStrings.Get(NativeStringId.PluginUpdatesReady)
                : $"{NativeStrings.Get(NativeStringId.PluginUpdatesReady)} {string.Join(" | ", PluginCatalogService.LastErrors)}";
        }
        catch (Exception ex)
        {
            _statusLabel.Text = ex.Message;
        }
        finally
        {
            _openButton.Enabled = true;
        }
    }

    private void SaveSources(bool showMessage)
    {
        PluginCatalogSources.Save(_sourcesTextBox.Lines.Select(line => line.Trim()).Where(line => line.Length > 0));
        if (showMessage)
        {
            ThemeHelper.ShowMessageBox(this, NativeStrings.Get(NativeStringId.PluginSourcesSaved), NativeStrings.PluginCaption, MessageBoxButtons.OK, MessageBoxIcon.Information);
        }
    }

    private void BindRows()
    {
        var selectedName = _listView.SelectedItems.Count > 0 ? _listView.SelectedItems[0].Text : null;
        var rows = _showOnlyUpdates.Checked ? _rows.Where(row => row.Status == PluginUpdateStatus.UpdateAvailable).ToList() : _rows.ToList();
        rows.Sort(new PluginUpdateRowComparer(_sortColumn, _sortOrder));

        _listView.BeginUpdate();
        try
        {
            _listView.Items.Clear();
            foreach (var row in rows)
            {
                var item = new ListViewItem(row.Name) { Tag = row };
                item.SubItems.Add(row.InstalledVersion);
                item.SubItems.Add(row.LatestVersion);
                item.SubItems.Add(row.StatusText);
                item.SubItems.Add(row.Author);
                item.SubItems.Add(row.Homepage);
                item.SubItems.Add(row.Source);
                _listView.Items.Add(item);
                if (selectedName is not null && string.Equals(selectedName, row.Name, StringComparison.CurrentCultureIgnoreCase))
                {
                    item.Selected = true;
                }
            }
        }
        finally
        {
            _listView.EndUpdate();
        }

        NativeListView.SetSortArrow(_listView, _sortColumn, _sortOrder);
        ThemeHelper.ApplyNativeDarkMode(_listView);
    }

    private void ListViewOnColumnClick(object? sender, ColumnClickEventArgs e)
    {
        if (_sortColumn == e.Column)
        {
            _sortOrder = _sortOrder == SortOrder.Ascending ? SortOrder.Descending : SortOrder.Ascending;
        }
        else
        {
            _sortColumn = e.Column;
            _sortOrder = SortOrder.Ascending;
        }

        BindRows();
    }

    private void ListViewOnMouseDoubleClick(object? sender, MouseEventArgs e)
    {
        if (_listView.GetItemAt(e.X, e.Y) is null)
        {
            return;
        }

        OpenSelectedPage();
    }

    private void OpenSelectedPage()
    {
        if (_listView.SelectedItems.Count == 0 || _listView.SelectedItems[0].Tag is not PluginUpdateRow row || string.IsNullOrWhiteSpace(row.WebUrl))
        {
            ThemeHelper.ShowMessageBox(this, NativeStrings.Get(NativeStringId.PluginUpdatesNoUrl), NativeStrings.PluginCaption, MessageBoxButtons.OK, MessageBoxIcon.Information);
            return;
        }

        try
        {
            Process.Start(new ProcessStartInfo(row.WebUrl!) { UseShellExecute = true });
        }
        catch (Exception ex)
        {
            ThemeHelper.ShowMessageBox(this, $"{NativeStrings.Get(NativeStringId.OpenBrowserError)}{Environment.NewLine}{ex.Message}", NativeStrings.PluginCaption, MessageBoxButtons.OK, MessageBoxIcon.Error);
        }
    }

    private sealed class PluginUpdateRowComparer : IComparer<PluginUpdateRow>
    {
        private readonly int _column;
        private readonly SortOrder _order;

        public PluginUpdateRowComparer(int column, SortOrder order)
        {
            _column = column;
            _order = order;
        }

        public int Compare(PluginUpdateRow? x, PluginUpdateRow? y)
        {
            if (ReferenceEquals(x, y))
            {
                return 0;
            }

            if (x is null)
            {
                return _order == SortOrder.Descending ? 1 : -1;
            }

            if (y is null)
            {
                return _order == SortOrder.Descending ? -1 : 1;
            }

            string left = GetValue(x);
            string right = GetValue(y);
            int result = string.Compare(left, right, StringComparison.CurrentCultureIgnoreCase);
            return _order == SortOrder.Descending ? -result : result;
        }

        private string GetValue(PluginUpdateRow row) => _column switch
        {
            1 => row.InstalledVersion,
            2 => row.LatestVersion,
            3 => row.StatusText,
            4 => row.Author,
            5 => row.Homepage,
            6 => row.Source,
            _ => row.Name,
        };
    }
}


internal static class NativeListView
{
    private const int HDI_FORMAT = 0x0004;
    private const int HDF_SORTDOWN = 0x0200;
    private const int HDF_SORTUP = 0x0400;
    private const int HDM_FIRST = 0x1200;
    private const int HDM_GETITEMW = HDM_FIRST + 11;
    private const int HDM_SETITEMW = HDM_FIRST + 12;
    private const int LVM_FIRST = 0x1000;
    private const int LVM_GETHEADER = LVM_FIRST + 31;

    public static void SetSortArrow(ListView listView, int column, SortOrder order)
    {
        if (!listView.IsHandleCreated)
        {
            return;
        }

        var header = SendMessage(listView.Handle, LVM_GETHEADER, IntPtr.Zero, IntPtr.Zero);
        if (header == IntPtr.Zero)
        {
            return;
        }

        for (int i = 0; i < listView.Columns.Count; i++)
        {
            var item = new HDITEM { mask = HDI_FORMAT };
            if (SendMessage(header, HDM_GETITEMW, new IntPtr(i), ref item) == IntPtr.Zero)
            {
                continue;
            }

            item.fmt &= ~(HDF_SORTUP | HDF_SORTDOWN);
            if (i == column)
            {
                item.fmt |= order == SortOrder.Descending ? HDF_SORTDOWN : HDF_SORTUP;
            }

            SendMessage(header, HDM_SETITEMW, new IntPtr(i), ref item);
        }
    }

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    private struct HDITEM
    {
        public int mask;
        public int cxy;
        public IntPtr pszText;
        public IntPtr hbm;
        public int cchTextMax;
        public int fmt;
        public IntPtr lParam;
        public int iImage;
        public int iOrder;
        public uint type;
        public IntPtr pvFilter;
        public uint state;
    }

    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    private static extern IntPtr SendMessage(IntPtr hWnd, int msg, IntPtr wParam, IntPtr lParam);

    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    private static extern IntPtr SendMessage(IntPtr hWnd, int msg, IntPtr wParam, ref HDITEM lParam);
}

internal static class PluginIconLoader
{
    public static System.Drawing.Icon? Load()
    {
        try
        {
            var pluginDirectory = Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location);
            if (pluginDirectory is null)
            {
                return null;
            }

            var splPath = Path.Combine(pluginDirectory, "samandarin.spl");
            return File.Exists(splPath) ? System.Drawing.Icon.ExtractAssociatedIcon(splPath) : null;
        }
        catch
        {
            return null;
        }
    }
}

internal static class PluginCatalogService
{
    private static readonly JavaScriptSerializer Serializer = new();

    public static IReadOnlyList<string> LastErrors { get; private set; } = Array.Empty<string>();

    public static async Task<IReadOnlyList<PluginUpdateRow>> CheckAsync()
    {
        var installed = InstalledPluginScanner.Scan().ToList();
        var catalog = new Dictionary<string, PluginCatalogEntry>(StringComparer.OrdinalIgnoreCase);
        var sourceErrors = new List<string>();

        foreach (var source in PluginCatalogSources.Load())
        {
            try
            {
                var json = await FetchStringAsync(source).ConfigureAwait(false);
                var manifest = Serializer.Deserialize<PluginCatalogManifest>(json);
                if (manifest?.plugins is null)
                {
                    continue;
                }

                foreach (var entry in manifest.plugins.Where(entry => !string.IsNullOrWhiteSpace(entry.id)))
                {
                    entry.source = source;
                    catalog[entry.id!] = entry;
                }
            }
            catch (Exception ex)
            {
                sourceErrors.Add($"{source}: {ex.Message}");
            }
        }

        var rows = installed.Select(plugin => BuildRow(plugin, catalog.TryGetValue(plugin.Id, out var entry) ? entry : null)).ToList();
        LastErrors = sourceErrors;
        return rows.OrderBy(row => row.Name, StringComparer.CurrentCultureIgnoreCase).ToList();
    }

    private static async Task<string> FetchStringAsync(string source)
    {
        if (Uri.TryCreate(source, UriKind.Absolute, out var uri) && (uri.Scheme == Uri.UriSchemeHttp || uri.Scheme == Uri.UriSchemeHttps))
        {
            using var request = new HttpRequestMessage(HttpMethod.Get, AddNoCacheQuery(uri));
            request.Headers.CacheControl = new CacheControlHeaderValue { NoCache = true, NoStore = true, MaxAge = TimeSpan.Zero };
            request.Headers.Pragma.ParseAdd("no-cache");
            using var response = await SharedHttpClient.Instance.SendAsync(request).ConfigureAwait(false);
            response.EnsureSuccessStatusCode();
            return await response.Content.ReadAsStringAsync().ConfigureAwait(false);
        }

        return File.ReadAllText(source, Encoding.UTF8);
    }

    private static Uri AddNoCacheQuery(Uri uri)
    {
        var separator = string.IsNullOrEmpty(uri.Query) ? "?" : "&";
        return new Uri(uri, uri.PathAndQuery + separator + "samandarinRefresh=" + DateTimeOffset.UtcNow.ToUnixTimeMilliseconds().ToString(CultureInfo.InvariantCulture));
    }

    private static PluginUpdateRow BuildRow(InstalledPlugin plugin, PluginCatalogEntry? entry)
    {
        if (entry is null)
        {
            return new PluginUpdateRow(plugin.DisplayName, plugin.VersionText, string.Empty, NativeStrings.Get(NativeStringId.PluginStatusNotInCatalog), string.Empty, string.Empty, PluginUpdateStatus.NotInCatalog, string.Empty, null);
        }

        var comparison = PluginVersionComparer.Compare(plugin.VersionText, entry.latestVersion, entry.versionScheme);
        var statusId = comparison switch
        {
            PluginVersionComparison.Current => NativeStringId.PluginStatusCurrent,
            PluginVersionComparison.UpdateAvailable => NativeStringId.PluginStatusUpdate,
            PluginVersionComparison.Different => NativeStringId.PluginStatusDifferent,
            _ => NativeStringId.PluginStatusUnknownVersion,
        };

        var homepage = entry.homepageUrl ?? string.Empty;
        return new PluginUpdateRow(LocalizedText.Resolve(entry.name) ?? plugin.DisplayName, plugin.VersionText, entry.latestVersion ?? string.Empty, NativeStrings.Get(statusId), entry.author ?? string.Empty, homepage, ToStatus(comparison), entry.source ?? string.Empty, entry.downloadPageUrl ?? entry.homepageUrl);
    }

    private static PluginUpdateStatus ToStatus(PluginVersionComparison comparison) => comparison == PluginVersionComparison.UpdateAvailable ? PluginUpdateStatus.UpdateAvailable : PluginUpdateStatus.Other;
}

internal static class SharedHttpClient
{
    public static readonly HttpClient Instance = new(new HttpClientHandler { AutomaticDecompression = DecompressionMethods.GZip | DecompressionMethods.Deflate, UseProxy = true, Proxy = WebRequest.DefaultWebProxy }) { Timeout = TimeSpan.FromSeconds(20) };

    static SharedHttpClient()
    {
        Instance.DefaultRequestHeaders.UserAgent.ParseAdd("SamandarinPluginCatalog/0.3");
    }
}

internal static class PluginCatalogSources
{
    private const string DefaultSource = "https://raw.githubusercontent.com/KRtkovo-eu-AI/salamander/main/doc/plugin-catalog.json";

    public static IReadOnlyList<string> Load()
    {
        var file = GetPath();
        if (!File.Exists(file))
        {
            return new[] { DefaultSource };
        }

        var lines = File.ReadAllLines(file, Encoding.UTF8).Select(line => line.Trim()).Where(line => line.Length > 0).Distinct(StringComparer.OrdinalIgnoreCase).ToList();
        return lines.Count == 0 ? new[] { DefaultSource } : lines;
    }

    public static void Save(IEnumerable<string> sources)
    {
        var file = GetPath();
        Directory.CreateDirectory(Path.GetDirectoryName(file)!);
        File.WriteAllLines(file, sources.Where(source => !string.IsNullOrWhiteSpace(source)).Distinct(StringComparer.OrdinalIgnoreCase), Encoding.UTF8);
    }

    private static string GetPath() => Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData), "Open Salamander", "Samandarin", "plugin-catalog-sources.txt");
}

internal static class InstalledPluginScanner
{
    public static IEnumerable<InstalledPlugin> Scan()
    {
        var assemblyPath = Assembly.GetExecutingAssembly().Location;
        var pluginDirectory = Directory.GetParent(assemblyPath)?.FullName;
        var pluginsRoot = pluginDirectory is null ? null : Directory.GetParent(pluginDirectory)?.FullName;
        if (pluginsRoot is null || !Directory.Exists(pluginsRoot))
        {
            yield break;
        }

        foreach (var file in Directory.EnumerateFiles(pluginsRoot, "*.spl", SearchOption.AllDirectories))
        {
            FileVersionInfo info;
            try
            {
                info = FileVersionInfo.GetVersionInfo(file);
            }
            catch
            {
                info = FileVersionInfo.GetVersionInfo(Assembly.GetExecutingAssembly().Location);
            }

            var relative = file.Substring(pluginsRoot.Length).TrimStart(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar);
            var id = relative.Split(new[] { Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar }).FirstOrDefault() ?? Path.GetFileNameWithoutExtension(file);
            var display = BuildDisplayName(id, file, info);
            var version = !string.IsNullOrWhiteSpace(info.FileVersion) ? info.FileVersion! : info.ProductVersion ?? string.Empty;
            yield return new InstalledPlugin(id, display, version);
        }
    }
    private static string BuildDisplayName(string id, string file, FileVersionInfo info)
    {
        if (!string.IsNullOrWhiteSpace(info.FileDescription) &&
            !info.FileDescription!.Equals("Open Salamander", StringComparison.OrdinalIgnoreCase))
        {
            return TrimOpenSalamanderSuffix(info.FileDescription!);
        }

        var fileName = Path.GetFileNameWithoutExtension(file);
        return string.IsNullOrWhiteSpace(fileName) ? id : fileName;
    }

    private static string TrimOpenSalamanderSuffix(string name)
    {
        var result = name.Trim();
        foreach (var suffix in new[] { " plugin for Open Salamander", " for Open Salamander" })
        {
            if (result.EndsWith(suffix, StringComparison.OrdinalIgnoreCase))
            {
                result = result.Substring(0, result.Length - suffix.Length).TrimEnd();
                break;
            }
        }

        return result;
    }
}

internal static class PluginVersionComparer
{
    public static PluginVersionComparison Compare(string? installed, string? latest, string? scheme)
    {
        if (string.IsNullOrWhiteSpace(installed) || string.IsNullOrWhiteSpace(latest))
        {
            return PluginVersionComparison.Unknown;
        }

        if (string.Equals(installed.Trim(), latest.Trim(), StringComparison.OrdinalIgnoreCase))
        {
            return PluginVersionComparison.Current;
        }

        var normalized = (scheme ?? "fileversion").Trim().ToLowerInvariant();
        if (normalized == "opaque")
        {
            return PluginVersionComparison.Different;
        }

        int comparison = VersionComparer.Compare(latest, installed);
        return comparison > 0 ? PluginVersionComparison.UpdateAvailable : comparison == 0 ? PluginVersionComparison.Current : PluginVersionComparison.Different;
    }
}

internal sealed class PluginCatalogManifest { public int schemaVersion { get; set; } public PluginCatalogEntry[]? plugins { get; set; } }
internal sealed class PluginCatalogEntry
{
    public string? id { get; set; }
    public object? name { get; set; }
    public string? author { get; set; }
    public string? latestVersion { get; set; }
    public string? versionScheme { get; set; }
    public string? homepageUrl { get; set; }
    public string? downloadPageUrl { get; set; }
    public string? source { get; set; }
}

internal static class LocalizedText
{
    public static string? Resolve(object? value)
    {
        if (value is null) return null;
        if (value is string text) return text;
        if (value is Dictionary<string, object> map)
        {
            var language = CultureInfo.CurrentUICulture.TwoLetterISOLanguageName;
            if (map.TryGetValue(language, out var localized)) return Convert.ToString(localized, CultureInfo.CurrentCulture);
            if (map.TryGetValue("en", out var english)) return Convert.ToString(english, CultureInfo.CurrentCulture);
            return map.Values.Select(v => Convert.ToString(v, CultureInfo.CurrentCulture)).FirstOrDefault(v => !string.IsNullOrWhiteSpace(v));
        }
        return Convert.ToString(value, CultureInfo.CurrentCulture);
    }
}

internal enum PluginVersionComparison { Unknown, Current, UpdateAvailable, Different }
internal enum PluginUpdateStatus { Other, UpdateAvailable, NotInCatalog, CatalogError }

internal sealed class InstalledPlugin
{
    public InstalledPlugin(string id, string displayName, string versionText)
    {
        Id = id;
        DisplayName = displayName;
        VersionText = versionText;
    }

    public string Id { get; }
    public string DisplayName { get; }
    public string VersionText { get; }
}

internal sealed class PluginUpdateRow
{
    public PluginUpdateRow(string name, string installedVersion, string latestVersion, string statusText, string author, string homepage, PluginUpdateStatus status, string source, string? webUrl)
    {
        Name = name;
        InstalledVersion = installedVersion;
        LatestVersion = latestVersion;
        StatusText = statusText;
        Author = author;
        Homepage = homepage;
        Status = status;
        Source = source;
        WebUrl = webUrl;
    }

    public string Name { get; }
    public string InstalledVersion { get; }
    public string LatestVersion { get; }
    public string StatusText { get; }
    public string Author { get; }
    public string Homepage { get; }
    public PluginUpdateStatus Status { get; }
    public string Source { get; }
    public string? WebUrl { get; }

    public static PluginUpdateRow SourceError(string source, string error) => new PluginUpdateRow(source, string.Empty, string.Empty, $"{NativeStrings.Get(NativeStringId.PluginStatusCatalogError)}: {error}", string.Empty, string.Empty, PluginUpdateStatus.CatalogError, source, null);
}

internal static class VersionComparer
{
    public static int Compare(string? left, string? right)
    {
        if (string.IsNullOrWhiteSpace(left) && string.IsNullOrWhiteSpace(right))
        {
            return 0;
        }

        if (string.IsNullOrWhiteSpace(left))
        {
            return -1;
        }

        if (string.IsNullOrWhiteSpace(right))
        {
            return 1;
        }

        var leftTokens = Tokenize(left!);
        var rightTokens = Tokenize(right!);
        int count = Math.Max(leftTokens.Length, rightTokens.Length);
        for (int i = 0; i < count; i++)
        {
            var leftToken = i < leftTokens.Length ? leftTokens[i] : VersionToken.Zero;
            var rightToken = i < rightTokens.Length ? rightTokens[i] : VersionToken.Zero;
            int result = leftToken.CompareTo(rightToken);
            if (result != 0)
            {
                return result;
            }
        }

        return 0;
    }

    private static VersionToken[] Tokenize(string value)
    {
        var raw = value.Split(new[] { '.', '-', '_' }, StringSplitOptions.RemoveEmptyEntries);
        var tokens = new VersionToken[raw.Length];
        for (int i = 0; i < raw.Length; i++)
        {
            if (long.TryParse(raw[i], NumberStyles.Integer, CultureInfo.InvariantCulture, out var number))
            {
                tokens[i] = VersionToken.FromNumber(number);
            }
            else
            {
                tokens[i] = VersionToken.FromText(raw[i]);
            }
        }

        return tokens;
    }

    private readonly struct VersionToken : IComparable<VersionToken>
    {
        public static VersionToken Zero => new(true, 0, string.Empty);

        private VersionToken(bool isNumeric, long numericValue, string text)
        {
            IsNumeric = isNumeric;
            NumericValue = numericValue;
            Text = text;
        }

        public bool IsNumeric { get; }
        public long NumericValue { get; }
        public string Text { get; }

        public static VersionToken FromNumber(long value) => new(true, value, string.Empty);

        public static VersionToken FromText(string text) => new(false, 0, text);

        public int CompareTo(VersionToken other)
        {
            if (IsNumeric && other.IsNumeric)
            {
                return NumericValue.CompareTo(other.NumericValue);
            }

            if (IsNumeric != other.IsNumeric)
            {
                string left = IsNumeric ? NumericValue.ToString(CultureInfo.InvariantCulture) : Text;
                string right = other.IsNumeric ? other.NumericValue.ToString(CultureInfo.InvariantCulture) : other.Text;
                return string.Compare(left, right, StringComparison.OrdinalIgnoreCase);
            }

            return string.Compare(Text, other.Text, StringComparison.OrdinalIgnoreCase);
        }
    }
}

internal enum UpdateFrequency
{
    Disabled,
    Daily,
    Weekly,
    Monthly,
}

internal sealed class UpdateSettings
{
    public bool CheckOnStartup { get; set; } = true;
    public UpdateFrequency Frequency { get; set; } = UpdateFrequency.Weekly;
    public DateTimeOffset? LastCheckUtc { get; set; }
    public string? LastPromptedVersion { get; set; }
    public string? LastKnownRemoteVersion { get; set; }

    public UpdateSettings Clone()
    {
        return new UpdateSettings
        {
            CheckOnStartup = CheckOnStartup,
            Frequency = Frequency,
            LastCheckUtc = LastCheckUtc,
            LastPromptedVersion = LastPromptedVersion,
            LastKnownRemoteVersion = LastKnownRemoteVersion,
        };
    }

    public static UpdateSettings Load()
    {
        return NativeConfiguration.LoadOrDefault();
    }

    public void Save()
    {
        NativeConfiguration.Save(this);
    }
}

internal sealed class UpdateSnapshot
{
    public UpdateSnapshot(UpdateSettings settings, string currentVersion)
    {
        Settings = settings;
        CurrentVersion = currentVersion;
    }

    public UpdateSettings Settings { get; }
    public string CurrentVersion { get; }
}

internal sealed class WindowHandleWrapper : IWin32Window
{
    public WindowHandleWrapper(IntPtr handle)
    {
        Handle = handle;
    }

    public IntPtr Handle { get; }
}

internal static class NativeConfiguration
{
    private const int MaxVersionLength = 128;

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
    private struct NativeUpdateSettings
    {
        public int CheckOnStartup;
        public UpdateFrequency Frequency;
        public int HasLastCheckUtc;
        public long LastCheckUtcTicks;

        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = MaxVersionLength)]
        public string LastPromptedVersion;

        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = MaxVersionLength)]
        public string LastKnownRemoteVersion;
    }

    [DllImport("Samandarin.Spl", CallingConvention = CallingConvention.StdCall)]
    private static extern int Samandarin_LoadSettings(out NativeUpdateSettings settings);

    [DllImport("Samandarin.Spl", CallingConvention = CallingConvention.StdCall)]
    private static extern int Samandarin_SaveSettings(ref NativeUpdateSettings settings);

    public static UpdateSettings LoadOrDefault()
    {
        if (TryLoad(out var native))
        {
            return ToManaged(native);
        }

        return new UpdateSettings();
    }

    public static void Save(UpdateSettings settings)
    {
        var native = ToNative(settings);
        TrySave(ref native);
    }

    private static bool TryLoad(out NativeUpdateSettings settings)
    {
        try
        {
            return Samandarin_LoadSettings(out settings) != 0;
        }
        catch (DllNotFoundException)
        {
            settings = default;
            return false;
        }
        catch (EntryPointNotFoundException)
        {
            settings = default;
            return false;
        }
    }

    private static bool TrySave(ref NativeUpdateSettings settings)
    {
        try
        {
            return Samandarin_SaveSettings(ref settings) != 0;
        }
        catch (DllNotFoundException)
        {
            return false;
        }
        catch (EntryPointNotFoundException)
        {
            return false;
        }
    }

    private static UpdateSettings ToManaged(NativeUpdateSettings native)
    {
        var result = new UpdateSettings
        {
            CheckOnStartup = native.CheckOnStartup != 0,
            Frequency = Enum.IsDefined(typeof(UpdateFrequency), (int)native.Frequency)
                ? native.Frequency
                : UpdateFrequency.Weekly,
        };

        if (native.HasLastCheckUtc != 0)
        {
            try
            {
                result.LastCheckUtc = new DateTimeOffset(native.LastCheckUtcTicks, TimeSpan.Zero);
            }
            catch (ArgumentOutOfRangeException)
            {
                result.LastCheckUtc = null;
            }
        }

        if (!string.IsNullOrWhiteSpace(native.LastPromptedVersion))
        {
            result.LastPromptedVersion = native.LastPromptedVersion;
        }

        if (!string.IsNullOrWhiteSpace(native.LastKnownRemoteVersion))
        {
            result.LastKnownRemoteVersion = native.LastKnownRemoteVersion;
        }

        return result;
    }

    private static NativeUpdateSettings ToNative(UpdateSettings settings)
    {
        var native = new NativeUpdateSettings
        {
            CheckOnStartup = settings.CheckOnStartup ? 1 : 0,
            Frequency = settings.Frequency,
            HasLastCheckUtc = settings.LastCheckUtc.HasValue ? 1 : 0,
            LastCheckUtcTicks = settings.LastCheckUtc?.ToUniversalTime().UtcTicks ?? 0,
            LastPromptedVersion = Sanitize(settings.LastPromptedVersion),
            LastKnownRemoteVersion = Sanitize(settings.LastKnownRemoteVersion),
        };

        return native;
    }

    private static string Sanitize(string? value)
    {
        if (string.IsNullOrEmpty(value))
        {
            return string.Empty;
        }

        var sanitized = value!;
        return sanitized.Length >= MaxVersionLength ? sanitized.Substring(0, MaxVersionLength - 1) : sanitized;
    }
}

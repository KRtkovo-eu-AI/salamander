// SPDX-FileCopyrightText: 2024 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

using System;
using System.Collections.Concurrent;
using System.Diagnostics;
using System.Drawing;
using System.Globalization;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text.RegularExpressions;
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
        ThemeHelper.RefreshOpenForms();
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
        ThemeHelper.CenterDialogOverOwner(dialog, owner);
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
    PluginCopyValue = 108,
    PluginCopyRowWithHeaders = 109,
    PluginStatusNotInstalled = 110,
    PluginUpdatesConfigureSources = 111,
    PluginUpdatesSourcesTitle = 112,
    PluginDetails = 113,
    PluginDescriptionLabel = 114,
    PluginDownloadPage = 115,
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

    public static string? GetLanguageModulePath()
    {
        var buffer = new StringBuilder(BufferLength);
        int length = Samandarin_GetLanguageModulePath(buffer, buffer.Capacity);
        return length > 0 ? buffer.ToString() : null;
    }

    [DllImport("Samandarin.Spl", EntryPoint = "Samandarin_LoadString", ExactSpelling = true, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
    private static extern int Samandarin_LoadString(int resourceId, StringBuilder buffer, int bufferLength);

    [DllImport("Samandarin.Spl", EntryPoint = "Samandarin_GetLanguageModulePath", ExactSpelling = true, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
    private static extern int Samandarin_GetLanguageModulePath(StringBuilder buffer, int bufferLength);
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
        HttpClient.DefaultRequestHeaders.UserAgent.ParseAdd($"SamandarinUpdateNotifier/{SamandarinVersion.PluginVersion}");
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

internal class DpiAwareForm : Form
{
    private const int WmDpiChanged = 0x02E0;

    protected DpiAwareForm()
    {
        AutoScaleMode = AutoScaleMode.Dpi;
        Font = SystemFonts.MessageBoxFont;
    }

    protected override void WndProc(ref Message m)
    {
        if (m.Msg == WmDpiChanged)
        {
            Font = SystemFonts.MessageBoxFont;
            PerformLayout();
            Invalidate(true);
        }

        base.WndProc(ref m);
    }
}

internal sealed class ConfigurationDialog : DpiAwareForm
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
        PluginCatalogSourcesText = _settings.PluginCatalogSourcesText,
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


internal sealed class ThemedBorderPanel : Panel
{
    private static readonly Color DarkBorder = Color.FromArgb(56, 56, 56);

    protected override void OnPaint(PaintEventArgs e)
    {
        base.OnPaint(e);
        using var pen = new Pen(IsDarkBackColor ? DarkBorder : SystemColors.ControlDark);
        e.Graphics.DrawRectangle(pen, 0, 0, Width - 1, Height - 1);
    }

    protected override void OnBackColorChanged(EventArgs e)
    {
        base.OnBackColorChanged(e);
        Invalidate();
    }

    private bool IsDarkBackColor => BackColor.GetBrightness() < 0.5f;
}

internal sealed class ThemedGroupBox : GroupBox
{
    private static readonly Color DarkBorder = Color.FromArgb(56, 56, 56);

    protected override void OnPaint(PaintEventArgs e)
    {
        e.Graphics.Clear(BackColor);

        TextRenderer.DrawText(
            e.Graphics,
            Text,
            Font,
            new Point(8, 0),
            ForeColor,
            TextFormatFlags.Left | TextFormatFlags.NoPrefix);

        var textSize = TextRenderer.MeasureText(Text, Font);
        var borderTop = Math.Max(Font.Height / 2, 1);
        using var pen = new Pen(BackColor.GetBrightness() < 0.5f ? DarkBorder : SystemColors.ControlLight);

        e.Graphics.DrawLine(pen, 0, borderTop, 6, borderTop);
        e.Graphics.DrawLine(pen, 10 + textSize.Width, borderTop, Width - 1, borderTop);
        e.Graphics.DrawLine(pen, 0, borderTop, 0, Height - 1);
        e.Graphics.DrawLine(pen, Width - 1, borderTop, Width - 1, Height - 1);
        e.Graphics.DrawLine(pen, 0, Height - 1, Width - 1, Height - 1);
    }
}

internal sealed class PluginUpdatesDialog : DpiAwareForm
{
    private readonly ListView _listView;
    private readonly CheckBox _showOnlyUpdates;
    private readonly Label _statusLabel;
    private readonly Button _refreshButton;
    private readonly Button _sourcesButton;
    private readonly ContextMenuStrip _listContextMenu;
    private readonly ImageList _pluginImages;
    private readonly Dictionary<string, string> _catalogImageKeys = new(StringComparer.OrdinalIgnoreCase);
    private readonly Label _detailNameValue;
    private readonly Label _detailAuthorValue;
    private readonly Label _detailInstalledValue;
    private readonly Label _detailLatestValue;
    private readonly Label _detailStatusValue;
    private readonly Label _detailSourceValue;
    private readonly LinkLabel _detailHomepageValue;
    private readonly LinkLabel _detailDownloadValue;
    private readonly TextBox _detailDescriptionValue;
    private ListViewItem? _contextMenuItem;
    private static readonly int[] ListColumnMinimumWidths = { 46, 120, 180, 90, 90, 120, 110 };
    private static readonly float[] ListColumnWidthWeights = { 0.00f, 0.15f, 0.30f, 0.12f, 0.12f, 0.18f, 0.13f };
    private int _contextMenuSubItemIndex;
    private readonly List<PluginUpdateRow> _rows = new();
    private int _sortColumn = 2;
    private SortOrder _sortOrder = SortOrder.Ascending;

    public PluginUpdatesDialog()
    {
        Text = NativeStrings.Get(NativeStringId.PluginUpdatesTitle);
        StartPosition = FormStartPosition.CenterParent;
        MinimizeBox = false;
        ShowInTaskbar = false;
        Width = 980;
        Height = 640;
        MinimumSize = new System.Drawing.Size(870, 650);
        AutoScroll = true;
        Icon = PluginIconLoader.Load();

        var layout = new TableLayoutPanel
        {
            Dock = DockStyle.Fill,
            ColumnCount = 1,
            RowCount = 5,
            Padding = new Padding(12),
        };
        layout.RowStyles.Add(new RowStyle(SizeType.AutoSize));
        layout.RowStyles.Add(new RowStyle(SizeType.Percent, 58f));
        layout.RowStyles.Add(new RowStyle(SizeType.AutoSize));
        layout.RowStyles.Add(new RowStyle(SizeType.Percent, 42f));
        layout.RowStyles.Add(new RowStyle(SizeType.AutoSize));

        layout.Controls.Add(new Label { Text = NativeStrings.Get(NativeStringId.PluginUpdatesDescription), AutoSize = true, Anchor = AnchorStyles.Left | AnchorStyles.Right | AnchorStyles.Top, Padding = new Padding(0, 6, 0, 6) }, 0, 0);

        _listView = new ListView
        {
            Dock = DockStyle.Fill,
            FullRowSelect = true,
            GridLines = true,
            HideSelection = false,
            MultiSelect = false,
            Scrollable = true,
            View = View.Details,
            OwnerDraw = true,
            BorderStyle = BorderStyle.None,
        };
        EnableSmoothListViewPainting(_listView);
        _pluginImages = new ImageList { ColorDepth = ColorDepth.Depth32Bit, ImageSize = new System.Drawing.Size(16, 16) };
        _listView.SmallImageList = _pluginImages;
        _listView.Columns.Add(string.Empty, 46);
        _listView.Columns.Add(NativeStrings.Get(NativeStringId.PluginColumnSource), 140);
        _listView.Columns.Add(NativeStrings.Get(NativeStringId.PluginColumnName), 240);
        _listView.Columns.Add(NativeStrings.Get(NativeStringId.PluginColumnInstalled), 120);
        _listView.Columns.Add(NativeStrings.Get(NativeStringId.PluginColumnLatest), 120);
        _listView.Columns.Add(NativeStrings.Get(NativeStringId.PluginColumnStatus), 150);
        _listView.Columns.Add(NativeStrings.Get(NativeStringId.PluginColumnAuthor), 140);
        _listView.ColumnClick += ListViewOnColumnClick;
        _listView.DrawColumnHeader += ListViewOnDrawColumnHeader;
        _listView.DrawSubItem += ListViewOnDrawSubItem;
        _listView.MouseDoubleClick += ListViewOnMouseDoubleClick;
        _listView.MouseDown += ListViewOnMouseDown;
        _listView.Resize += (_, _) => AdjustListViewColumns();
        _listView.SelectedIndexChanged += (_, _) => UpdateDetails();
        _listContextMenu = new ContextMenuStrip();
        _listContextMenu.Opening += ListContextMenuOnOpening;
        _listContextMenu.Items.Add(NativeStrings.Get(NativeStringId.PluginCopyValue), null, (_, _) => CopyContextCellValue());
        _listContextMenu.Items.Add(NativeStrings.Get(NativeStringId.PluginCopyRowWithHeaders), null, (_, _) => CopyContextRowWithHeaders());
        _listView.ContextMenuStrip = _listContextMenu;
        var listFrame = new ThemedBorderPanel { Dock = DockStyle.Fill, Padding = new Padding(1) };
        listFrame.Controls.Add(_listView);
        layout.Controls.Add(listFrame, 0, 1);

        _showOnlyUpdates = new CheckBox { Text = NativeStrings.Get(NativeStringId.PluginUpdatesShowOnly), AutoSize = true, Anchor = AnchorStyles.Left, Padding = new Padding(0, 6, 0, 6) };
        _showOnlyUpdates.CheckedChanged += (_, _) => BindRows();
        layout.Controls.Add(_showOnlyUpdates, 0, 2);

        var detailGroup = new ThemedGroupBox { Text = NativeStrings.Get(NativeStringId.PluginDetails), Dock = DockStyle.Fill, Padding = new Padding(10) };
        var detailLayout = new TableLayoutPanel { Dock = DockStyle.Fill, ColumnCount = 4, RowCount = 5 };
        detailLayout.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));
        detailLayout.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 50f));
        detailLayout.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));
        detailLayout.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 50f));
        for (int i = 0; i < 4; i++) detailLayout.RowStyles.Add(new RowStyle(SizeType.AutoSize));
        detailLayout.RowStyles.Add(new RowStyle(SizeType.Percent, 100f));

        _detailNameValue = AddValue(detailLayout, NativeStringId.PluginColumnName, 0, 0);
        _detailSourceValue = AddValue(detailLayout, NativeStringId.PluginColumnSource, 2, 0);
        _detailAuthorValue = AddValue(detailLayout, NativeStringId.PluginColumnAuthor, 0, 1);
        _detailStatusValue = AddValue(detailLayout, NativeStringId.PluginColumnStatus, 2, 1);
        _detailInstalledValue = AddValue(detailLayout, NativeStringId.PluginColumnInstalled, 0, 2);
        _detailLatestValue = AddValue(detailLayout, NativeStringId.PluginColumnLatest, 2, 2);
        _detailHomepageValue = AddLinkValue(detailLayout, NativeStringId.PluginColumnHomepage, 0, 3);
        _detailDownloadValue = AddLinkValue(detailLayout, NativeStringId.PluginDownloadPage, 2, 3);

        detailLayout.Controls.Add(new Label { Text = NativeStrings.Get(NativeStringId.PluginDescriptionLabel), AutoSize = true, Anchor = AnchorStyles.Left | AnchorStyles.Top, Padding = new Padding(0, 3, 8, 0) }, 0, 4);
        _detailDescriptionValue = new TextBox { Multiline = true, ReadOnly = true, ScrollBars = ScrollBars.Vertical, Dock = DockStyle.Fill, BorderStyle = BorderStyle.FixedSingle };
        detailLayout.SetColumnSpan(_detailDescriptionValue, 3);
        detailLayout.Controls.Add(_detailDescriptionValue, 1, 4);

        detailGroup.Controls.Add(detailLayout);
        layout.Controls.Add(detailGroup, 0, 3);

        var bottomPanel = new TableLayoutPanel { Dock = DockStyle.Fill, AutoSize = true, ColumnCount = 2, Padding = new Padding(0, 10, 0, 0) };
        bottomPanel.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100f));
        bottomPanel.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));
        _statusLabel = new Label { AutoSize = false, AutoEllipsis = true, Dock = DockStyle.Fill, Height = 28, Padding = new Padding(0, 6, 8, 0) };
        bottomPanel.Controls.Add(_statusLabel, 0, 0);

        var buttons = new FlowLayoutPanel { Dock = DockStyle.Fill, AutoSize = true, WrapContents = false, FlowDirection = FlowDirection.RightToLeft, Margin = new Padding(0) };
        var closeButton = new Button { Text = NativeStrings.Get(NativeStringId.PluginUpdatesClose), DialogResult = DialogResult.Cancel, AutoSize = true };
        _refreshButton = new Button { Text = NativeStrings.Get(NativeStringId.PluginUpdatesRefresh), AutoSize = true };
        _sourcesButton = new Button { Text = NativeStrings.Get(NativeStringId.PluginUpdatesConfigureSources), AutoSize = true };
        _refreshButton.Click += async (_, _) => await RefreshAsync().ConfigureAwait(true);
        _sourcesButton.Click += (_, _) => ShowSourcesDialog();
        buttons.Controls.Add(closeButton);
        buttons.Controls.Add(_refreshButton);
        buttons.Controls.Add(_sourcesButton);
        bottomPanel.Controls.Add(buttons, 1, 0);
        layout.Controls.Add(bottomPanel, 0, 4);

        Controls.Add(layout);
        CancelButton = closeButton;
        Shown += async (_, _) =>
        {
            await RefreshAsync().ConfigureAwait(true);
            AdjustListViewColumns();
        };
        UpdateDetails();
    }

    private void ListViewOnDrawColumnHeader(object? sender, DrawListViewColumnHeaderEventArgs e)
    {
        e.DrawDefault = true;
    }

    private void ListViewOnDrawSubItem(object? sender, DrawListViewSubItemEventArgs e)
    {
        bool selected = e.Item.Selected;
        var background = selected ? SystemColors.Highlight : _listView.BackColor;
        var foreground = selected ? SystemColors.HighlightText : _listView.ForeColor;

        using (var backgroundBrush = new SolidBrush(background))
        {
            e.Graphics.FillRectangle(backgroundBrush, e.Bounds);
        }

        if (e.ColumnIndex == 0)
        {
            DrawListViewImage(e.Graphics, e.Item, e.Bounds);
        }
        else
        {
            TextRenderer.DrawText(
                e.Graphics,
                e.SubItem.Text,
                _listView.Font,
                new Rectangle(e.Bounds.Left + 4, e.Bounds.Top, Math.Max(0, e.Bounds.Width - 8), e.Bounds.Height),
                foreground,
                TextFormatFlags.Left | TextFormatFlags.VerticalCenter | TextFormatFlags.EndEllipsis | TextFormatFlags.SingleLine);
        }

        using var gridPen = new Pen(ControlPaint.Dark(_listView.BackColor));
        e.Graphics.DrawLine(gridPen, e.Bounds.Left, e.Bounds.Bottom - 1, e.Bounds.Right, e.Bounds.Bottom - 1);
        e.Graphics.DrawLine(gridPen, e.Bounds.Right - 1, e.Bounds.Top, e.Bounds.Right - 1, e.Bounds.Bottom);
    }

    private void DrawListViewImage(Graphics graphics, ListViewItem item, Rectangle bounds)
    {
        Image? image = null;
        if (!string.IsNullOrEmpty(item.ImageKey) && _pluginImages.Images.ContainsKey(item.ImageKey))
        {
            image = _pluginImages.Images[item.ImageKey];
        }
        else if (item.ImageIndex >= 0 && item.ImageIndex < _pluginImages.Images.Count)
        {
            image = _pluginImages.Images[item.ImageIndex];
        }

        if (image is null)
        {
            return;
        }

        int x = bounds.Left + Math.Max(0, (bounds.Width - image.Width) / 2);
        int y = bounds.Top + Math.Max(0, (bounds.Height - image.Height) / 2);
        graphics.DrawImage(image, x, y, image.Width, image.Height);
    }

    private void AdjustListViewColumns()
    {
        if (_listView.Columns.Count != ListColumnMinimumWidths.Length || _listView.ClientSize.Width <= 0)
        {
            return;
        }

        int availableWidth = Math.Max(0, _listView.ClientSize.Width - 4);
        int minimumWidth = ListColumnMinimumWidths.Sum();
        if (availableWidth <= minimumWidth)
        {
            for (int i = 0; i < ListColumnMinimumWidths.Length; i++)
            {
                _listView.Columns[i].Width = ListColumnMinimumWidths[i];
            }

            return;
        }

        int extraWidth = availableWidth - minimumWidth;
        int assignedWidth = 0;
        for (int i = 0; i < ListColumnMinimumWidths.Length; i++)
        {
            int width = i == ListColumnMinimumWidths.Length - 1
                ? availableWidth - assignedWidth
                : ListColumnMinimumWidths[i] + (int)Math.Round(extraWidth * ListColumnWidthWeights[i]);
            _listView.Columns[i].Width = Math.Max(ListColumnMinimumWidths[i], width);
            assignedWidth += _listView.Columns[i].Width;
        }
    }

    private static Label AddValue(TableLayoutPanel layout, NativeStringId captionId, int column, int row)
    {
        layout.Controls.Add(new Label { Text = NativeStrings.Get(captionId) + ":", AutoSize = true, Anchor = AnchorStyles.Left, Padding = new Padding(0, 3, 8, 0) }, column, row);
        var value = new Label { AutoSize = false, AutoEllipsis = true, Dock = DockStyle.Fill, Height = 22, Padding = new Padding(0, 6, 8, 0) };
        layout.Controls.Add(value, column + 1, row);
        return value;
    }

    private static LinkLabel AddLinkValue(TableLayoutPanel layout, NativeStringId captionId, int column, int row)
    {
        layout.Controls.Add(new Label { Text = NativeStrings.Get(captionId) + ":", AutoSize = true, Anchor = AnchorStyles.Left, Padding = new Padding(0, 3, 8, 0) }, column, row);
        var value = new LinkLabel { AutoSize = false, AutoEllipsis = true, Dock = DockStyle.Fill, Height = 22, Padding = new Padding(0, 6, 8, 0) };
        value.LinkClicked += (_, _) => OpenUrl(value.Text);
        layout.Controls.Add(value, column + 1, row);
        return value;
    }

    private async Task RefreshAsync()
    {
        SetLoadingState(true);
        _statusLabel.Text = NativeStrings.Get(NativeStringId.PluginUpdatesLoading);
        try
        {
            var result = await PluginCatalogService.CheckAsync().ConfigureAwait(true);
            _rows.Clear();
            _rows.AddRange(result);
            BindRows();
            _statusLabel.Text = PluginCatalogService.LastErrors.Count == 0
                ? NativeStrings.Get(NativeStringId.PluginUpdatesReady)
                : $"{NativeStrings.Get(NativeStringId.PluginUpdatesReady)} {string.Join(" | ", PluginCatalogService.LastErrors)}";
            SetLoadingState(false);
            await EnsureCatalogImagesAsync(_rows).ConfigureAwait(true);
            RefreshCatalogImages();
        }
        catch (Exception ex)
        {
            _statusLabel.Text = ex.Message;
        }
        finally
        {
            SetLoadingState(false);
            UpdateDetails();
        }
    }

    private void SetLoadingState(bool isLoading)
    {
        _showOnlyUpdates.Enabled = !isLoading;
        _refreshButton.Enabled = !isLoading;
        _sourcesButton.Enabled = !isLoading;
        UseWaitCursor = isLoading;
    }

    private void ShowSourcesDialog()
    {
        using var dialog = new PluginCatalogSourcesDialog();
        ThemeHelper.ApplyTheme(dialog);
        dialog.Shown += (_, _) => ThemeHelper.ApplyNativeDarkMode(dialog.ListView);
        if (dialog.ShowDialog(this) == DialogResult.OK)
        {
            _ = RefreshAsync();
        }
    }

    private static void EnableSmoothListViewPainting(ListView listView)
    {
        listView.GetType().GetProperty("DoubleBuffered", BindingFlags.Instance | BindingFlags.NonPublic)?.SetValue(listView, true, null);
    }

    private void BindRows()
    {
        var selectedName = _listView.SelectedItems.Count > 0 ? (_listView.SelectedItems[0].Tag as PluginUpdateRow)?.Name : null;
        var rows = _showOnlyUpdates.Checked ? _rows.Where(row => row.Status == PluginUpdateStatus.UpdateAvailable).ToList() : _rows.ToList();
        rows.Sort(new PluginUpdateRowComparer(_sortColumn, _sortOrder));

        _listView.BeginUpdate();
        try
        {
            _listView.Items.Clear();
            foreach (var row in rows)
            {
                var item = new ListViewItem(string.Empty) { Tag = row, ImageKey = EnsurePluginImage(row) };
                item.SubItems.Add(row.Source);
                item.SubItems.Add(row.Name);
                item.SubItems.Add(row.InstalledVersion);
                item.SubItems.Add(row.LatestVersion);
                item.SubItems.Add(row.StatusText);
                item.SubItems.Add(row.Author);
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

        AdjustListViewColumns();
        NativeListView.SetSortArrow(_listView, _sortColumn, _sortOrder);
        ThemeHelper.ApplyNativeDarkMode(_listView);
        EnsureListViewLightModeBackground();
        UpdateDetails();
    }

    private void EnsureListViewLightModeBackground()
    {
        if (BackColor.GetBrightness() >= 0.5f)
        {
            _listView.BackColor = Color.White;
            _listView.ForeColor = SystemColors.WindowText;
        }
    }

    private void RefreshCatalogImages()
    {
        _listView.BeginUpdate();
        try
        {
            foreach (ListViewItem item in _listView.Items)
            {
                if (item.Tag is PluginUpdateRow row)
                {
                    item.ImageKey = EnsurePluginImage(row);
                }
            }
        }
        finally
        {
            _listView.EndUpdate();
        }
    }

    private string EnsurePluginImage(PluginUpdateRow row)
    {
        var catalogIconKey = GetCachedCatalogIconImageKey(row);
        if (!string.IsNullOrEmpty(catalogIconKey))
        {
            return catalogIconKey;
        }

        if (string.IsNullOrWhiteSpace(row.IconPath))
        {
            return string.Empty;
        }

        var key = row.IconPath!;
        if (_pluginImages.Images.ContainsKey(key))
        {
            return key;
        }

        try
        {
            using var icon = Icon.ExtractAssociatedIcon(key);
            if (icon is not null)
            {
                _pluginImages.Images.Add(key, icon);
                return key;
            }
        }
        catch
        {
        }

        return string.Empty;
    }

    private async Task EnsureCatalogImagesAsync(IEnumerable<PluginUpdateRow> rows)
    {
        foreach (var row in rows)
        {
            if (!string.IsNullOrEmpty(GetCachedCatalogIconImageKey(row)))
            {
                continue;
            }

            await EnsureCatalogIconImageAsync(row).ConfigureAwait(true);
        }
    }

    private string GetCachedCatalogIconImageKey(PluginUpdateRow row)
    {
        if (!TryResolveCatalogIconReference(row, out var resolvedIcon))
        {
            return string.Empty;
        }

        return _catalogImageKeys.TryGetValue(resolvedIcon, out var key) && _pluginImages.Images.ContainsKey(key) ? key : string.Empty;
    }

    private async Task EnsureCatalogIconImageAsync(PluginUpdateRow row)
    {
        if (!TryResolveCatalogIconReference(row, out var resolvedIcon))
        {
            return;
        }

        var key = "catalog:" + resolvedIcon;
        if (_pluginImages.Images.ContainsKey(key))
        {
            _catalogImageKeys[resolvedIcon] = key;
            return;
        }

        try
        {
            if (Uri.TryCreate(resolvedIcon, UriKind.Absolute, out var uri) && !uri.IsFile)
            {
                using var request = new HttpRequestMessage(HttpMethod.Get, AddNoCacheQuery(uri));
                request.Headers.Accept.ParseAdd("image/*");
                request.Headers.CacheControl = new CacheControlHeaderValue { NoCache = true, NoStore = true, MaxAge = TimeSpan.Zero };
                request.Headers.Pragma.ParseAdd("no-cache");
                using var response = await SharedHttpClient.Instance.SendAsync(request).ConfigureAwait(true);
                response.EnsureSuccessStatusCode();
                using var stream = await response.Content.ReadAsStreamAsync().ConfigureAwait(true);
                AddCatalogImageFromStream(key, stream, resolvedIcon);
            }
            else
            {
                var path = Uri.TryCreate(resolvedIcon, UriKind.Absolute, out var fileUri) && fileUri.IsFile ? fileUri.LocalPath : resolvedIcon;
                using var stream = File.OpenRead(path);
                AddCatalogImageFromStream(key, stream, path);
            }

            if (_pluginImages.Images.ContainsKey(key))
            {
                _catalogImageKeys[resolvedIcon] = key;
            }
        }
        catch
        {
        }
    }

    private void AddCatalogImageFromStream(string key, Stream stream, string source)
    {
        if (Path.GetExtension(source).Equals(".ico", StringComparison.OrdinalIgnoreCase))
        {
            using var icon = new Icon(stream, _pluginImages.ImageSize);
            _pluginImages.Images.Add(key, icon);
            return;
        }

        using var image = Image.FromStream(stream);
        using var bitmap = CreateImageListBitmap(image, _pluginImages.ImageSize);
        _pluginImages.Images.Add(key, bitmap);
    }

    private static Bitmap CreateImageListBitmap(Image image, System.Drawing.Size size)
    {
        var bitmap = new Bitmap(size.Width, size.Height, System.Drawing.Imaging.PixelFormat.Format32bppArgb);
        using var graphics = Graphics.FromImage(bitmap);
        graphics.Clear(Color.Transparent);
        graphics.InterpolationMode = System.Drawing.Drawing2D.InterpolationMode.HighQualityBicubic;
        graphics.PixelOffsetMode = System.Drawing.Drawing2D.PixelOffsetMode.HighQuality;
        graphics.SmoothingMode = System.Drawing.Drawing2D.SmoothingMode.HighQuality;

        var scale = Math.Min((double)size.Width / image.Width, (double)size.Height / image.Height);
        var width = Math.Max(1, (int)Math.Round(image.Width * scale));
        var height = Math.Max(1, (int)Math.Round(image.Height * scale));
        var x = (size.Width - width) / 2;
        var y = (size.Height - height) / 2;
        graphics.DrawImage(image, x, y, width, height);
        return bitmap;
    }

    private static bool TryResolveCatalogIconReference(PluginUpdateRow row, out string resolvedIcon)
    {
        resolvedIcon = string.Empty;
        var iconReference = row.CatalogIconReference;
        if (string.IsNullOrWhiteSpace(iconReference) || string.Equals(iconReference, "plugin", StringComparison.OrdinalIgnoreCase))
        {
            return false;
        }

        resolvedIcon = ResolveCatalogIconReference(iconReference!, row.CatalogSourceUrl);
        return !string.IsNullOrWhiteSpace(resolvedIcon);
    }

    private static string ResolveCatalogIconReference(string iconReference, string catalogSourceUrl)
    {
        var trimmed = iconReference.Trim();
        if (Uri.TryCreate(trimmed, UriKind.Absolute, out var absoluteUri))
        {
            return absoluteUri.ToString();
        }

        if (!string.IsNullOrWhiteSpace(catalogSourceUrl) && Uri.TryCreate(catalogSourceUrl, UriKind.Absolute, out var catalogUri))
        {
            return new Uri(catalogUri, trimmed).ToString();
        }

        return trimmed;
    }

    private static Uri AddNoCacheQuery(Uri uri)
    {
        var separator = string.IsNullOrEmpty(uri.Query) ? "?" : "&";
        return new Uri(uri, uri.PathAndQuery + separator + "samandarinRefresh=" + DateTimeOffset.UtcNow.ToUnixTimeMilliseconds().ToString(CultureInfo.InvariantCulture));
    }

    private void UpdateDetails()
    {
        var row = _listView.SelectedItems.Count > 0 ? _listView.SelectedItems[0].Tag as PluginUpdateRow : null;
        _detailNameValue.Text = row?.Name ?? string.Empty;
        _detailAuthorValue.Text = row?.Author ?? string.Empty;
        _detailInstalledValue.Text = row?.InstalledVersion ?? string.Empty;
        _detailLatestValue.Text = row?.LatestVersion ?? string.Empty;
        _detailStatusValue.Text = row?.StatusText ?? string.Empty;
        _detailSourceValue.Text = row?.Source ?? string.Empty;
        _detailHomepageValue.Text = row?.Homepage ?? string.Empty;
        _detailDownloadValue.Text = row?.WebUrl ?? string.Empty;
        _detailDescriptionValue.Text = row?.Description ?? string.Empty;
    }

    private void ListViewOnMouseDown(object? sender, MouseEventArgs e)
    {
        if (e.Button != MouseButtons.Right)
        {
            return;
        }

        var hit = _listView.HitTest(e.Location);
        _contextMenuItem = hit.Item;
        _contextMenuSubItemIndex = hit.Item is null || hit.SubItem is null ? -1 : hit.Item.SubItems.IndexOf(hit.SubItem);
    }

    private void ListContextMenuOnOpening(object? sender, System.ComponentModel.CancelEventArgs e)
    {
        bool hasCell = _contextMenuItem is not null && _contextMenuSubItemIndex >= 0;
        e.Cancel = !hasCell;
    }

    private void CopyContextCellValue()
    {
        if (_contextMenuItem is null || _contextMenuSubItemIndex < 0 || _contextMenuSubItemIndex >= _contextMenuItem.SubItems.Count)
        {
            return;
        }

        Clipboard.SetText(_contextMenuItem.SubItems[_contextMenuSubItemIndex].Text ?? string.Empty);
    }

    private void CopyContextRowWithHeaders()
    {
        if (_contextMenuItem is null)
        {
            return;
        }

        var parts = new List<string>();
        for (int i = 0; i < _listView.Columns.Count && i < _contextMenuItem.SubItems.Count; i++)
        {
            parts.Add($"{_listView.Columns[i].Text}: {_contextMenuItem.SubItems[i].Text}");
        }

        if (_contextMenuItem.Tag is PluginUpdateRow row && !string.IsNullOrWhiteSpace(row.Description))
        {
            parts.Add($"{NativeStrings.Get(NativeStringId.PluginDescriptionLabel)}: {row.Description}");
        }

        Clipboard.SetText(string.Join(Environment.NewLine, parts));
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
        var row = _listView.SelectedItems.Count == 0 ? null : _listView.SelectedItems[0].Tag as PluginUpdateRow;
        if (row is null || string.IsNullOrWhiteSpace(row.WebUrl))
        {
            ThemeHelper.ShowMessageBox(this, NativeStrings.Get(NativeStringId.PluginUpdatesNoUrl), NativeStrings.PluginCaption, MessageBoxButtons.OK, MessageBoxIcon.Information);
            return;
        }

        OpenUrl(row.WebUrl!);
    }

    private static void OpenUrl(string? url)
    {
        if (string.IsNullOrWhiteSpace(url))
        {
            return;
        }

        try
        {
            Process.Start(new ProcessStartInfo(url) { UseShellExecute = true });
        }
        catch (Exception ex)
        {
            ThemeHelper.ShowMessageBox(null, $"{NativeStrings.Get(NativeStringId.OpenBrowserError)}{Environment.NewLine}{ex.Message}", NativeStrings.PluginCaption, MessageBoxButtons.OK, MessageBoxIcon.Error);
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
            if (ReferenceEquals(x, y)) return 0;
            if (x is null) return _order == SortOrder.Descending ? 1 : -1;
            if (y is null) return _order == SortOrder.Descending ? -1 : 1;
            string left = GetValue(x);
            string right = GetValue(y);
            int result = string.Compare(left, right, StringComparison.CurrentCultureIgnoreCase);
            return _order == SortOrder.Descending ? -result : result;
        }

        private string GetValue(PluginUpdateRow row) => _column switch
        {
            1 => row.Source,
            2 => row.Name,
            3 => row.InstalledVersion,
            4 => row.LatestVersion,
            5 => row.StatusText,
            6 => row.Author,
            _ => row.Name,
        };
    }
}

internal sealed class PluginCatalogSourcesDialog : DpiAwareForm
{
    private readonly ListView _listView;
    private readonly List<PluginCatalogSource> _sources;
    private ListViewItem? _lastCheckedItem;
    private bool _lastCheckedValue;

    public ListView ListView => _listView;

    public PluginCatalogSourcesDialog()
    {
        Text = NativeStrings.Get(NativeStringId.PluginUpdatesSourcesTitle);
        StartPosition = FormStartPosition.CenterParent;
        MinimizeBox = false;
        MaximizeBox = false;
        ShowInTaskbar = false;
        FormBorderStyle = FormBorderStyle.FixedDialog;
        Width = 680;
        Height = 400;
        MinimumSize = new System.Drawing.Size(500, 300);
        Icon = PluginIconLoader.Load();

        var layout = new TableLayoutPanel { Dock = DockStyle.Fill, ColumnCount = 1, RowCount = 3, Padding = new Padding(12) };
        layout.RowStyles.Add(new RowStyle(SizeType.AutoSize));
        layout.RowStyles.Add(new RowStyle(SizeType.Percent, 100f));
        layout.RowStyles.Add(new RowStyle(SizeType.AutoSize));
        layout.Controls.Add(new Label { Text = NativeStrings.Get(NativeStringId.PluginUpdatesConfigureSources) + ":", AutoSize = true, MaximumSize = new System.Drawing.Size(640, 0) }, 0, 0);

        _listView = new ListView
        {
            Dock = DockStyle.Fill,
            FullRowSelect = true,
            GridLines = true,
            HideSelection = false,
            MultiSelect = false,
            Scrollable = true,
            View = View.Details,
            CheckBoxes = true,
            LabelEdit = true,
        };
        _listView.Columns.Add(string.Empty, 630);
        _listView.ItemCheck += ListViewOnItemCheck;
        _listView.DoubleClick += ListViewOnDoubleClick;
        _listView.KeyDown += ListViewOnKeyDown;
        _listView.AfterLabelEdit += ListViewOnAfterLabelEdit;
        _listView.ItemChecked += ListViewOnItemChecked;
        layout.Controls.Add(_listView, 0, 1);

        var buttons = new FlowLayoutPanel { Dock = DockStyle.Fill, AutoSize = true, WrapContents = false, FlowDirection = FlowDirection.RightToLeft, Padding = new Padding(0, 10, 0, 0), Margin = new Padding(0) };
        var closeButton = new Button { Text = NativeStrings.Get(NativeStringId.PluginUpdatesClose), DialogResult = DialogResult.Cancel, AutoSize = true };
        var saveButton = new Button { Text = NativeStrings.Get(NativeStringId.PluginUpdatesSaveSources), DialogResult = DialogResult.OK, AutoSize = true };
        saveButton.Click += (_, _) => SaveSources();
        buttons.Controls.Add(closeButton);
        buttons.Controls.Add(saveButton);
        layout.Controls.Add(buttons, 0, 2);

        Controls.Add(layout);
        CancelButton = closeButton;

        _sources = PluginCatalogSources.Load().Select(s => new PluginCatalogSource { Url = s.Url, Enabled = s.Enabled }).ToList();
        PopulateList();
    }

    private void PopulateList()
    {
        _listView.BeginUpdate();
        try
        {
            _listView.Items.Clear();
            foreach (var source in _sources)
            {
                var item = new ListViewItem(source.Url) { Tag = source, Checked = source.Enabled };
                _listView.Items.Add(item);
            }

            AddEmptySourceRow();
        }
        finally
        {
            _listView.EndUpdate();
        }
    }

    private void AddEmptySourceRow()
    {
        var source = new PluginCatalogSource { Url = string.Empty, Enabled = true };
        _sources.Add(source);
        var addItem = new ListViewItem(string.Empty) { Tag = source, Checked = true };
        _listView.Items.Add(addItem);
    }

    private void ListViewOnItemCheck(object? sender, ItemCheckEventArgs e)
    {
        var item = _listView.Items[e.Index];
        _lastCheckedItem = item;
        _lastCheckedValue = e.CurrentValue == CheckState.Checked;
    }

    private void ListViewOnDoubleClick(object? sender, EventArgs e)
    {
        if (_listView.SelectedItems.Count == 0) return;
        var item = _listView.SelectedItems[0];

        if (_lastCheckedItem == item && _lastCheckedValue != item.Checked)
        {
            item.Checked = _lastCheckedValue;
        }

        item.BeginEdit();
    }

    private void ListViewOnKeyDown(object? sender, KeyEventArgs e)
    {
        if (e.KeyCode == Keys.F2 && _listView.SelectedItems.Count > 0)
        {
            _listView.SelectedItems[0].BeginEdit();
            e.Handled = true;
        }
        else if (e.KeyCode == Keys.Delete && _listView.SelectedItems.Count > 0)
        {
            var item = _listView.SelectedItems[0];
            var source = item.Tag as PluginCatalogSource;
            if (source is not null && source.Url.Length > 0)
            {
                _sources.Remove(source);
                _listView.Items.Remove(item);
            }

            e.Handled = true;
        }
    }

    private void ListViewOnAfterLabelEdit(object? sender, LabelEditEventArgs e)
    {
        if (e.Label is null) return;
        var item = _listView.Items[e.Item];
        var source = item.Tag as PluginCatalogSource;
        if (source is null) return;

        var newUrl = e.Label.Trim();
        if (newUrl.Length == 0)
        {
            if (source.Url.Length == 0)
            {
                e.CancelEdit = true;
            }

            return;
        }

        source.Url = newUrl;

        if (item.Index == _listView.Items.Count - 1 && source.Url.Length > 0)
        {
            source.Enabled = true;
            AddEmptySourceRow();
        }
    }

    private void ListViewOnItemChecked(object? sender, ItemCheckedEventArgs e)
    {
        if (e.Item?.Tag is PluginCatalogSource source)
        {
            source.Enabled = e.Item.Checked;
        }
    }

    private void SaveSources()
    {
        var last = _sources.Count > 0 ? _sources[_sources.Count - 1] : null;
        if (last is not null && string.IsNullOrWhiteSpace(last.Url))
        {
            _sources.RemoveAt(_sources.Count - 1);
        }

        PluginCatalogSources.Save(_sources);
        ThemeHelper.ShowMessageBox(this, NativeStrings.Get(NativeStringId.PluginSourcesSaved), NativeStrings.PluginCaption, MessageBoxButtons.OK, MessageBoxIcon.Information);
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
        var catalog = new List<PluginCatalogEntry>();
        var sourceErrors = new List<string>();

        var sourceResults = await Task.WhenAll(PluginCatalogSources.LoadEnabledUrls().Select(FetchCatalogSourceAsync)).ConfigureAwait(false);
        foreach (var sourceResult in sourceResults)
        {
            catalog.AddRange(sourceResult.Entries);
            if (!string.IsNullOrEmpty(sourceResult.Error))
            {
                sourceErrors.Add(sourceResult.Error);
            }
        }

        var rows = installed.SelectMany(plugin =>
        {
            var entries = catalog.Where(entry => IsCatalogEntryMatch(entry, plugin)).ToList();
            if (entries.Count == 0)
            {
                return new[] { BuildRow(plugin, null) };
            }

            return entries.Select(entry => BuildRow(plugin, entry));
        }).ToList();
        rows.AddRange(catalog
            .Where(entry => entry.id is not null && !installed.Any(plugin => IsCatalogEntryMatch(entry, plugin)))
            .Select(BuildCatalogOnlyRow));
        LastErrors = sourceErrors;
        return rows.OrderBy(row => row.Name, StringComparer.CurrentCultureIgnoreCase).ToList();
    }


    private static async Task<(List<PluginCatalogEntry> Entries, string Error)> FetchCatalogSourceAsync(string source)
    {
        try
        {
            var json = await FetchStringAsync(source).ConfigureAwait(false);
            var manifest = Serializer.Deserialize<PluginCatalogManifest>(json);
            var entries = new List<PluginCatalogEntry>();
            if (manifest?.plugins is not null)
            {
                foreach (var entry in manifest.plugins.Where(entry => !string.IsNullOrWhiteSpace(entry.id)))
                {
                    entry.sourceUrl = source;
                    entry.source = string.IsNullOrWhiteSpace(manifest.catalogName) ? source : manifest.catalogName;
                    entries.Add(entry);
                }
            }

            return (entries, string.Empty);
        }
        catch (Exception ex)
        {
            return (new List<PluginCatalogEntry>(), $"{source}: {ex.Message}");
        }
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

    private static bool IsCatalogEntryMatch(PluginCatalogEntry entry, InstalledPlugin plugin)
    {
        if (string.Equals(entry.id, plugin.Id, StringComparison.OrdinalIgnoreCase))
        {
            return true;
        }

        return IsTotalCommanderProxyCatalogEntry(entry) && IsTotalCommanderProxyInstance(plugin);
    }

    private static bool IsTotalCommanderProxyCatalogEntry(PluginCatalogEntry entry)
    {
        return string.Equals(entry.id, "x-tc-proxy", StringComparison.OrdinalIgnoreCase);
    }

    private static bool IsTotalCommanderProxyInstance(InstalledPlugin plugin)
    {
        return Regex.IsMatch(plugin.DisplayName, @"^Total Commander Proxy \([^()]+\.(?:wcx|wfx|wlx|wdx)64?\)$", RegexOptions.IgnoreCase | RegexOptions.CultureInvariant);
    }

    private static PluginUpdateRow BuildCatalogOnlyRow(PluginCatalogEntry entry)
    {
        var homepage = entry.homepageUrl ?? string.Empty;
        return new PluginUpdateRow(LocalizedText.Resolve(entry.name) ?? entry.id ?? NativeStrings.Get(NativeStringId.Unknown), string.Empty, entry.latestVersion ?? string.Empty, NativeStrings.Get(NativeStringId.PluginStatusNotInstalled), entry.author ?? string.Empty, homepage, PluginUpdateStatus.Other, entry.source ?? string.Empty, entry.downloadPageUrl ?? entry.homepageUrl, LocalizedText.Resolve(entry.description) ?? string.Empty, null, entry.icon ?? string.Empty, entry.sourceUrl ?? string.Empty);
    }

    private static PluginUpdateRow BuildRow(InstalledPlugin plugin, PluginCatalogEntry? entry)
    {
        if (entry is null)
        {
            return new PluginUpdateRow(plugin.DisplayName, plugin.VersionText, string.Empty, NativeStrings.Get(NativeStringId.PluginStatusNotInCatalog), string.Empty, string.Empty, PluginUpdateStatus.NotInCatalog, string.Empty, null, string.Empty, plugin.IconPath, string.Empty, string.Empty);
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
        var displayName = IsTotalCommanderProxyCatalogEntry(entry) && IsTotalCommanderProxyInstance(plugin)
            ? plugin.DisplayName
            : LocalizedText.Resolve(entry.name) ?? plugin.DisplayName;
        return new PluginUpdateRow(displayName, plugin.VersionText, entry.latestVersion ?? string.Empty, NativeStrings.Get(statusId), entry.author ?? string.Empty, homepage, ToStatus(comparison), entry.source ?? string.Empty, entry.downloadPageUrl ?? entry.homepageUrl, LocalizedText.Resolve(entry.description) ?? string.Empty, plugin.IconPath, entry.icon ?? string.Empty, entry.sourceUrl ?? string.Empty);
    }

    private static PluginUpdateStatus ToStatus(PluginVersionComparison comparison) => comparison == PluginVersionComparison.UpdateAvailable ? PluginUpdateStatus.UpdateAvailable : PluginUpdateStatus.Other;
}

internal static class SharedHttpClient
{
    public static readonly HttpClient Instance = new(new HttpClientHandler { AutomaticDecompression = DecompressionMethods.GZip | DecompressionMethods.Deflate, UseProxy = true, Proxy = WebRequest.DefaultWebProxy }) { Timeout = TimeSpan.FromSeconds(20) };

    static SharedHttpClient()
    {
        Instance.DefaultRequestHeaders.UserAgent.ParseAdd($"SamandarinPluginCatalog/{SamandarinVersion.PluginVersion}");
    }
}

internal sealed class PluginCatalogSource
{
    public string Url { get; set; } = string.Empty;
    public bool Enabled { get; set; } = true;
}

internal static class PluginCatalogSources
{
    private const string StablePluginSource = "https://samandarin.krtkovo.eu/catalogs/plugins-stable.json";
    private const string UnofficialExternalSource = "https://samandarin.krtkovo.eu/catalogs/plugins-unofficial.json";
    private const string ExtensionRuntimesSource = "https://samandarin.krtkovo.eu/catalogs/extension-runtimes.json";
    private const string LegacyStablePluginSource = "https://krtkovo-eu-ai.github.io/salamander/catalogs/plugins-stable.json";
    private const string LegacyUnofficialExternalSource = "https://krtkovo-eu-ai.github.io/salamander/catalogs/plugins-unofficial.json";

    public static IReadOnlyList<PluginCatalogSource> Load()
    {
        var settings = NativeConfiguration.LoadOrDefault();
        var text = settings.PluginCatalogSourcesText;
        if (string.IsNullOrWhiteSpace(text))
        {
            return DefaultSources();
        }

        var lines = text!.Split(new[] { '\r', '\n' }, StringSplitOptions.RemoveEmptyEntries);
        var sources = new List<PluginCatalogSource>();
        var seen = new HashSet<string>(StringComparer.OrdinalIgnoreCase);

        foreach (var raw in lines)
        {
            var line = raw.Trim();
            if (line.Length == 0) continue;

            bool enabled;
            string url;
            int pipeIndex = line.IndexOf('|');
            if (pipeIndex >= 0)
            {
                var prefix = line.Substring(0, pipeIndex).Trim();
                enabled = prefix == "1";
                url = line.Substring(pipeIndex + 1).Trim();
            }
            else
            {
                enabled = true;
                url = line;
            }

            if (url.Length == 0 || !seen.Add(url)) continue;
            sources.Add(new PluginCatalogSource { Url = url, Enabled = enabled });
        }

        if (sources.Count == 0)
        {
            return DefaultSources();
        }

        if (NormalizeDefaultSources(sources))
        {
            settings.PluginCatalogSourcesText = Serialize(sources);
            NativeConfiguration.Save(settings);
        }

        return sources;
    }

    public static IReadOnlyList<string> LoadEnabledUrls()
    {
        return Load().Where(s => s.Enabled).Select(s => s.Url).ToList();
    }

    public static void Save(IEnumerable<PluginCatalogSource> sources)
    {
        var settings = NativeConfiguration.LoadOrDefault();
        var distinct = sources
            .Where(s => !string.IsNullOrWhiteSpace(s.Url))
            .GroupBy(s => s.Url.Trim(), StringComparer.OrdinalIgnoreCase)
            .Select(g => g.First())
            .ToList();
        settings.PluginCatalogSourcesText = Serialize(distinct);
        NativeConfiguration.Save(settings);
    }

    private static IReadOnlyList<PluginCatalogSource> DefaultSources() => new[]
    {
        new PluginCatalogSource { Url = StablePluginSource, Enabled = true },
        new PluginCatalogSource { Url = UnofficialExternalSource, Enabled = true },
        new PluginCatalogSource { Url = ExtensionRuntimesSource, Enabled = true },
    };

    private static bool NormalizeDefaultSources(List<PluginCatalogSource> sources)
    {
        var changed = false;
        changed |= ReplaceSourceUrl(sources, LegacyStablePluginSource, StablePluginSource);
        changed |= ReplaceSourceUrl(sources, LegacyUnofficialExternalSource, UnofficialExternalSource);
        changed |= AddMissingDefaultSource(sources, StablePluginSource);
        changed |= AddMissingDefaultSource(sources, UnofficialExternalSource);
        changed |= AddMissingDefaultSource(sources, ExtensionRuntimesSource);
        changed |= RemoveDuplicateSources(sources);
        return changed;
    }

    private static bool ReplaceSourceUrl(IEnumerable<PluginCatalogSource> sources, string oldUrl, string newUrl)
    {
        var changed = false;
        foreach (var source in sources.Where(source => string.Equals(source.Url, oldUrl, StringComparison.OrdinalIgnoreCase)))
        {
            source.Url = newUrl;
            changed = true;
        }

        return changed;
    }

    private static bool AddMissingDefaultSource(ICollection<PluginCatalogSource> sources, string url)
    {
        if (sources.Any(source => string.Equals(source.Url, url, StringComparison.OrdinalIgnoreCase)))
        {
            return false;
        }

        sources.Add(new PluginCatalogSource { Url = url, Enabled = true });
        return true;
    }

    private static bool RemoveDuplicateSources(List<PluginCatalogSource> sources)
    {
        var seen = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        var changed = false;
        for (int i = 0; i < sources.Count; i++)
        {
            if (seen.Add(sources[i].Url.Trim()))
            {
                continue;
            }

            sources.RemoveAt(i);
            i--;
            changed = true;
        }

        return changed;
    }

    private static string Serialize(IEnumerable<PluginCatalogSource> sources) => string.Join("\n", sources.Select(s => $"{(s.Enabled ? "1" : "0")}|{s.Url.Trim()}"));
}

internal static class InstalledPluginScanner
{
    public static IEnumerable<InstalledPlugin> Scan()
    {
        return NativeInstalledPluginProvider.TryReadInstalledPlugins(out var livePlugins)
            ? livePlugins
            : ScanPluginDirectory();
    }

    private static IEnumerable<InstalledPlugin> ScanPluginDirectory()
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
            var display = BuildDisplayName(id, file);
            var version = !string.IsNullOrWhiteSpace(info.FileVersion) ? info.FileVersion! : info.ProductVersion ?? string.Empty;
            yield return new InstalledPlugin(id, display, version, file);
        }
    }

    public static string BuildDisplayName(string id, string file)
    {
        var fileName = Path.GetFileNameWithoutExtension(file);
        return string.IsNullOrWhiteSpace(fileName) ? id : fileName;
    }
}

internal static class NativeInstalledPluginProvider
{
    [DllImport("Samandarin.Spl", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Ansi)]
    private static extern int Samandarin_ExportInstalledPlugins(StringBuilder? buffer, int cchBuffer);

    public static bool TryReadInstalledPlugins(out IReadOnlyList<InstalledPlugin> plugins)
    {
        plugins = Array.Empty<InstalledPlugin>();
        try
        {
            var required = Samandarin_ExportInstalledPlugins(null, 0);
            if (required <= 1)
            {
                return false;
            }

            var buffer = new StringBuilder(required);
            var confirmed = Samandarin_ExportInstalledPlugins(buffer, buffer.Capacity);
            if (confirmed <= 1)
            {
                return false;
            }

            plugins = ParseExport(buffer.ToString()).ToList();
            return plugins.Count > 0;
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

    private static IEnumerable<InstalledPlugin> ParseExport(string exportText)
    {
        foreach (var line in exportText.Split(new[] { '\r', '\n' }, StringSplitOptions.RemoveEmptyEntries))
        {
            var parts = line.Split('\t');
            if (parts.Length < 3 || string.IsNullOrWhiteSpace(parts[0]))
            {
                continue;
            }

            var dllName = parts[0];
            var id = PluginMetadata.BuildIdFromRegisteredDll(dllName);
            var displayName = parts[1];
            var version = parts[2];
            var resolvedDllPath = PluginMetadata.ResolveRegisteredDllPath(dllName);

            if (string.IsNullOrWhiteSpace(displayName))
            {
                displayName = id;
            }

            yield return new InstalledPlugin(id, displayName!, version ?? string.Empty, resolvedDllPath);
        }
    }
}

internal static class PluginMetadata
{
    public static string? ResolveRegisteredDllPath(string dllName)
    {
        var normalized = dllName.Trim();
        if (Path.IsPathRooted(normalized))
        {
            return normalized;
        }

        var executableDirectory = GetExecutableDirectory();
        if (string.IsNullOrWhiteSpace(executableDirectory))
        {
            return null;
        }

        var pluginsRoot = Path.Combine(executableDirectory!, "plugins");
        return Path.GetFullPath(Path.Combine(pluginsRoot, normalized));
    }

    public static string BuildIdFromRegisteredDll(string dllName)
    {
        var normalized = dllName.Trim().TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar);
        var fileName = Path.GetFileNameWithoutExtension(normalized);
        if (Path.IsPathRooted(normalized))
        {
            return string.IsNullOrWhiteSpace(fileName) ? normalized : fileName!;
        }

        var firstSegment = normalized.Split(new[] { Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar }, StringSplitOptions.RemoveEmptyEntries).FirstOrDefault();
        return string.IsNullOrWhiteSpace(firstSegment)
            ? (string.IsNullOrWhiteSpace(fileName) ? normalized : fileName!)
            : firstSegment!;
    }

    private static string? GetExecutableDirectory()
    {
        var executablePath = Application.ExecutablePath;
        return string.IsNullOrWhiteSpace(executablePath) ? null : Path.GetDirectoryName(executablePath);
    }
}

internal sealed class PluginCatalogManifest { public int schemaVersion { get; set; } public string? catalogName { get; set; } public PluginCatalogEntry[]? plugins { get; set; } }
internal sealed class PluginCatalogEntry
{
    public string? id { get; set; }
    public object? name { get; set; }
    public string? author { get; set; }
    public object? description { get; set; }
    public string? latestVersion { get; set; }
    public string? icon { get; set; }
    public string? versionScheme { get; set; }
    public string? homepageUrl { get; set; }
    public string? downloadPageUrl { get; set; }
    public string? source { get; set; }
    public string? sourceUrl { get; set; }
}

internal static class LocalizedText
{
    public static string? Resolve(object? value)
    {
        if (value is null) return null;
        if (value is string text) return text;
        if (value is Dictionary<string, object> map)
        {
            var normalized = new Dictionary<string, object>(map, StringComparer.OrdinalIgnoreCase);
            foreach (var key in GetPreferredKeys())
            {
                if (normalized.TryGetValue(key, out var localized))
                {
                    var textValue = Convert.ToString(localized, CultureInfo.CurrentCulture);
                    if (!string.IsNullOrWhiteSpace(textValue)) return textValue;
                }
            }

            return normalized.Values.Select(v => Convert.ToString(v, CultureInfo.CurrentCulture)).FirstOrDefault(v => !string.IsNullOrWhiteSpace(v));
        }
        return Convert.ToString(value, CultureInfo.CurrentCulture);
    }

    private static IEnumerable<string> GetPreferredKeys()
    {
        var emitted = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        foreach (var candidate in GetSalamanderLanguageCandidates())
        {
            foreach (var key in ExpandLanguageKey(candidate))
            {
                if (emitted.Add(key)) yield return key;
            }
        }

        if (emitted.Add("english")) yield return "english";
        if (emitted.Add("en")) yield return "en";
    }

    private static IEnumerable<string> GetSalamanderLanguageCandidates()
    {
        var languageModulePath = NativeStrings.GetLanguageModulePath();
        if (!string.IsNullOrWhiteSpace(languageModulePath))
        {
            var fileName = Path.GetFileNameWithoutExtension(languageModulePath);
            if (!string.IsNullOrWhiteSpace(fileName)) yield return fileName;

            var directoryName = Path.GetFileName(Path.GetDirectoryName(languageModulePath));
            if (!string.IsNullOrWhiteSpace(directoryName)) yield return directoryName;
        }

        yield return CultureInfo.CurrentUICulture.TwoLetterISOLanguageName;
    }

    private static IEnumerable<string> ExpandLanguageKey(string language)
    {
        var normalized = language.Replace("-", string.Empty).Replace("_", string.Empty).Replace(" ", string.Empty).ToLowerInvariant();
        var salamanderKey = normalized switch
        {
            "cs" or "cz" or "czech" => "czech",
            "de" or "ger" or "deu" or "german" => "german",
            "fr" or "fre" or "fra" or "french" => "french",
            "nl" or "dut" or "nld" or "dutch" => "dutch",
            "hu" or "hun" or "hungarian" => "hungarian",
            "ro" or "rum" or "ron" or "romanian" => "romanian",
            "ru" or "rus" or "russian" => "russian",
            "sk" or "slo" or "slk" or "slovak" => "slovak",
            "es" or "spa" or "spanish" => "spanish",
            "zh" or "zho" or "chi" or "chinese" or "chinesesimplified" or "chs" => "chinesesimplified",
            "en" or "eng" or "english" => "english",
            _ => normalized,
        };

        yield return salamanderKey;
        yield return normalized;
    }
}

internal static class PluginVersionComparer
{
    public static PluginVersionComparison Compare(string? installed, string? latest, string? scheme)
    {
        var installedVersion = InstalledPluginVersionFormatter.RemovePlatformSuffix(installed);
        var latestVersion = InstalledPluginVersionFormatter.RemovePlatformSuffix(latest);
        if (string.IsNullOrWhiteSpace(installedVersion) || string.IsNullOrWhiteSpace(latestVersion))
        {
            return PluginVersionComparison.Unknown;
        }

        if (string.Equals(installedVersion, latestVersion, StringComparison.OrdinalIgnoreCase))
        {
            return PluginVersionComparison.Current;
        }

        var normalized = (scheme ?? "fileversion").Trim().ToLowerInvariant();
        if (normalized == "opaque")
        {
            return PluginVersionComparison.Different;
        }

        int comparison = VersionComparer.Compare(latestVersion, installedVersion);
        return comparison > 0 ? PluginVersionComparison.UpdateAvailable : comparison == 0 ? PluginVersionComparison.Current : PluginVersionComparison.Different;
    }
}

internal enum PluginVersionComparison { Unknown, Current, UpdateAvailable, Different }
internal enum PluginUpdateStatus { Other, UpdateAvailable, NotInCatalog, CatalogError }

internal sealed class InstalledPlugin
{
    public InstalledPlugin(string id, string displayName, string versionText, string? iconPath = null)
    {
        Id = id;
        DisplayName = displayName;
        VersionText = InstalledPluginVersionFormatter.WithCurrentPlatform(versionText);
        IconPath = iconPath ?? string.Empty;
    }

    public string Id { get; }
    public string DisplayName { get; }
    public string VersionText { get; }
    public string IconPath { get; }
}

internal static class InstalledPluginVersionFormatter
{
    private static readonly Regex PlatformSuffixRegex = new(@"\s\((x86|x64)\)$", RegexOptions.IgnoreCase | RegexOptions.CultureInvariant);

    public static string WithCurrentPlatform(string version)
    {
        if (string.IsNullOrWhiteSpace(version) || PlatformSuffixRegex.IsMatch(version))
        {
            return version;
        }

        return version.TrimEnd() + (Environment.Is64BitProcess ? " (x64)" : " (x86)");
    }

    public static string RemovePlatformSuffix(string? version)
    {
        if (string.IsNullOrWhiteSpace(version))
        {
            return string.Empty;
        }

        return PlatformSuffixRegex.Replace(version!.Trim(), string.Empty);
    }
}

internal sealed class PluginUpdateRow
{
    public PluginUpdateRow(string name, string installedVersion, string latestVersion, string statusText, string author, string homepage, PluginUpdateStatus status, string source, string? webUrl, string description, string? iconPath, string? catalogIconReference, string? catalogSourceUrl)
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
        Description = description;
        IconPath = iconPath ?? string.Empty;
        CatalogIconReference = catalogIconReference ?? string.Empty;
        CatalogSourceUrl = catalogSourceUrl ?? string.Empty;
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
    public string Description { get; }
    public string IconPath { get; }
    public string CatalogIconReference { get; }
    public string CatalogSourceUrl { get; }

    public static PluginUpdateRow SourceError(string source, string error) => new PluginUpdateRow(source, string.Empty, string.Empty, $"{NativeStrings.Get(NativeStringId.PluginStatusCatalogError)}: {error}", string.Empty, string.Empty, PluginUpdateStatus.CatalogError, source, null, string.Empty, null, string.Empty, string.Empty);
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

        if (TryCompareDecimalVersion(left!, right!, out var decimalComparison))
        {
            return decimalComparison;
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

    private static bool TryCompareDecimalVersion(string left, string right, out int comparison)
    {
        comparison = 0;
        var leftMatch = DecimalVersionRegex.Match(left.Trim());
        var rightMatch = DecimalVersionRegex.Match(right.Trim());
        if (!leftMatch.Success || !rightMatch.Success)
        {
            return false;
        }

        var leftMajor = long.Parse(leftMatch.Groups[1].Value, CultureInfo.InvariantCulture);
        var rightMajor = long.Parse(rightMatch.Groups[1].Value, CultureInfo.InvariantCulture);
        comparison = leftMajor.CompareTo(rightMajor);
        if (comparison != 0)
        {
            return true;
        }

        var leftMinor = leftMatch.Groups[2].Value;
        var rightMinor = rightMatch.Groups[2].Value;
        var width = Math.Max(leftMinor.Length, rightMinor.Length);
        var leftFraction = long.Parse(leftMinor.PadRight(width, '0'), CultureInfo.InvariantCulture);
        var rightFraction = long.Parse(rightMinor.PadRight(width, '0'), CultureInfo.InvariantCulture);
        comparison = leftFraction.CompareTo(rightFraction);
        return true;
    }

    private static readonly Regex DecimalVersionRegex = new(@"^(\d+)\.(\d+)$", RegexOptions.CultureInvariant);

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
    public string? PluginCatalogSourcesText { get; set; }

    public UpdateSettings Clone()
    {
        return new UpdateSettings
        {
            CheckOnStartup = CheckOnStartup,
            Frequency = Frequency,
            LastCheckUtc = LastCheckUtc,
            LastPromptedVersion = LastPromptedVersion,
            LastKnownRemoteVersion = LastKnownRemoteVersion,
            PluginCatalogSourcesText = PluginCatalogSourcesText,
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
    private const int MaxCatalogSourcesLength = 4096;

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

        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = MaxCatalogSourcesLength)]
        public string PluginCatalogSourcesText;
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

        if (!string.IsNullOrWhiteSpace(native.PluginCatalogSourcesText))
        {
            result.PluginCatalogSourcesText = native.PluginCatalogSourcesText;
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
            PluginCatalogSourcesText = Sanitize(settings.PluginCatalogSourcesText, MaxCatalogSourcesLength),
        };

        return native;
    }

    private static string Sanitize(string? value)
    {
        return Sanitize(value, MaxVersionLength);
    }

    private static string Sanitize(string? value, int maxLength)
    {
        if (string.IsNullOrEmpty(value))
        {
            return string.Empty;
        }

        var sanitized = value!;
        return sanitized.Length >= maxLength ? sanitized.Substring(0, maxLength - 1) : sanitized;
    }
}

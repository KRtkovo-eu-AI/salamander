// SPDX-FileCopyrightText: 2024 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

using System;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.Net.Http;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;
using Timer = System.Threading.Timer;

namespace OpenSalamander.Samandarin;

public static class EntryPoint
{
    private static bool _visualsEnabled;
    private static SynchronizationContext? _syncContext;

    [STAThread]
    public static int Dispatch(string? argument)
    {
        IntPtr parentHandle = IntPtr.Zero;
        try
        {
            EnsureApplicationInitialized();
            _syncContext ??= SynchronizationContext.Current;

            var parts = (argument ?? string.Empty).Split(new[] { ';' }, 3);
            var command = parts.Length > 0 ? parts[0] : string.Empty;
            parentHandle = ParseHandle(parts.Length > 1 ? parts[1] : string.Empty);
            var payload = parts.Length > 2 ? parts[2] : string.Empty;

            return command switch
            {
                "Initialize" => Initialize(parentHandle, payload),
                "Configure" => ShowConfiguration(parentHandle),
                "CheckNow" => CheckNow(parentHandle),
                "Shutdown" => Shutdown(),
                "ColorsChanged" => ColorsChanged(),
                _ => 1,
            };
        }
        catch (Exception ex)
        {
            ShowError(parentHandle, "Unexpected managed exception:", ex);
            return -1;
        }
    }

    private static int Initialize(IntPtr parent, string currentVersion)
    {
        UpdateCoordinator.Initialize(currentVersion, _syncContext, parent);
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
        ThemeHelper.ShowMessageBox(owner, message, "Samandarin Update Notifier", MessageBoxButtons.OK, MessageBoxIcon.Error);
    }
}

internal static class UpdateCoordinator
{
    private static readonly object SyncRoot = new();
    private static readonly Uri ReleasesUri = new("https://github.com/KRtkovo-eu-AI/salamander/releases/latest");
    private static readonly SemaphoreSlim CheckSemaphore = new(1, 1);
    private static readonly TimeSpan MinimumDelay = TimeSpan.FromSeconds(10);
    private static readonly string SettingsFilePath;
    private static readonly HttpClient HttpClient;

    private static SynchronizationContext? SyncContext;
    private static UpdateSettings Settings;
    private static string CurrentVersion = string.Empty;
    private static Timer? UpdateTimer;

    static UpdateCoordinator()
    {
        SettingsFilePath = Path.Combine(AppContext.BaseDirectory ?? Environment.CurrentDirectory, "Samandarin.UpdateNotifier.config");
        Settings = UpdateSettings.Load(SettingsFilePath);
        HttpClient = new HttpClient
        {
            Timeout = TimeSpan.FromSeconds(15),
        };
        HttpClient.DefaultRequestHeaders.UserAgent.ParseAdd("SamandarinUpdateNotifier/1.0");
    }

    public static void Initialize(string currentVersion, SynchronizationContext? context, IntPtr parent)
    {
        lock (SyncRoot)
        {
            CurrentVersion = (currentVersion ?? string.Empty).Trim();
            if (context != null)
            {
                SyncContext = context;
            }
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
            string? errorMessage = null;

            try
            {
                latestVersion = await FetchLatestVersionAsync().ConfigureAwait(false);
            }
            catch (Exception ex)
            {
                errorMessage = ex.Message;
            }

            bool notify = false;
            bool showCurrentMessage = false;

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
                    }
                }

                SaveSettings_NoLock();
                ScheduleTimer_NoLock();
            }

            if (!string.IsNullOrEmpty(latestVersion))
            {
                if (notify)
                {
                    await ShowUpdateAvailableAsync(parent, latestVersion).ConfigureAwait(false);
                }
                else if (showIfCurrent)
                {
                    await ShowUpToDateAsync(parent, latestVersion).ConfigureAwait(false);
                }
            }
            else if (errorMessage is not null && userInitiated)
            {
                await ShowErrorAsync(parent, errorMessage).ConfigureAwait(false);
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
    }

    private static async Task<string?> FetchLatestVersionAsync()
    {
        using var request = new HttpRequestMessage(HttpMethod.Get, ReleasesUri);
        using var response = await HttpClient.SendAsync(request, HttpCompletionOption.ResponseHeadersRead).ConfigureAwait(false);
        response.EnsureSuccessStatusCode();

        var finalUri = response.RequestMessage?.RequestUri ?? ReleasesUri;
        return ExtractVersionFromUri(finalUri);
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

        return Uri.UnescapeDataString(candidate);
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
        string message = $"A newer Samandarin build is available.{Environment.NewLine}{Environment.NewLine}" +
            $"Current version: {current}{Environment.NewLine}Latest version: {latestVersion}{Environment.NewLine}{Environment.NewLine}" +
            "Open the download page now?";

        var result = await ShowMessageAsync(parent, owner => ThemeHelper.ShowMessageBox(owner, message, "Samandarin Update Notifier", MessageBoxButtons.OKCancel, MessageBoxIcon.Information)).ConfigureAwait(false);
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
                    var errorMessage = $"Unable to open the browser.{Environment.NewLine}{ex.Message}";
                    ThemeHelper.ShowMessageBox(owner, errorMessage, "Samandarin Update Notifier", MessageBoxButtons.OK, MessageBoxIcon.Error);
                }
            }).ConfigureAwait(false);
        }
    }

    private static Task ShowUpToDateAsync(IntPtr parent, string latestVersion)
    {
        string current = GetCurrentVersion();
        string message = string.IsNullOrWhiteSpace(latestVersion)
            ? $"Samandarin {current} is the latest version available."
            : $"Samandarin {current} is the latest version available.{Environment.NewLine}Latest release: {latestVersion}.";
        return ShowMessageAsync(parent, owner => ThemeHelper.ShowMessageBox(owner, message, "Samandarin Update Notifier", MessageBoxButtons.OK, MessageBoxIcon.Information));
    }

    private static Task ShowErrorAsync(IntPtr parent, string error)
    {
        string message = $"Unable to check for updates.{Environment.NewLine}{error}";
        return ShowMessageAsync(parent, owner => ThemeHelper.ShowMessageBox(owner, message, "Samandarin Update Notifier", MessageBoxButtons.OK, MessageBoxIcon.Error));
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
        var context = SyncContext;
        if (context is null)
        {
            action();
        }
        else
        {
            context.Post(_ => action(), null);
        }
    }

    private static string GetCurrentVersion()
    {
        lock (SyncRoot)
        {
            return string.IsNullOrWhiteSpace(CurrentVersion) ? "Unknown" : CurrentVersion;
        }
    }

    private static void SaveSettings_NoLock()
    {
        Settings.Save(SettingsFilePath);
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

        Text = "Samandarin Update Settings";
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
            Text = "Choose when Samandarin should check GitHub for a newer build.",
            AutoSize = true,
            MaximumSize = new System.Drawing.Size(460, 0),
        };
        layout.SetColumnSpan(description, 2);
        layout.Controls.Add(description, 0, 0);

        _checkOnStartup = new CheckBox
        {
            Text = "Check when Salamander starts",
            AutoSize = true,
        };
        layout.SetColumnSpan(_checkOnStartup, 2);
        layout.Controls.Add(_checkOnStartup, 0, 1);

        var frequencyLabel = new Label
        {
            Text = "Periodic check:",
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
            new FrequencyOption(UpdateFrequency.Disabled, "Disabled"),
            new FrequencyOption(UpdateFrequency.Daily, "Daily"),
            new FrequencyOption(UpdateFrequency.Weekly, "Weekly"),
            new FrequencyOption(UpdateFrequency.Monthly, "Monthly"),
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

        var okButton = new Button { Text = "OK", DialogResult = DialogResult.OK, AutoSize = true };
        var cancelButton = new Button { Text = "Cancel", DialogResult = DialogResult.Cancel, AutoSize = true };
        _checkNowButton = new Button { Text = "Check now", AutoSize = true };
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
            ? "Current version: Unknown"
            : string.Format(CultureInfo.CurrentCulture, "Current version: {0}", snapshot.CurrentVersion);
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
        _statusLabel.Text = "Status: Checking for updates...";
        try
        {
            await UpdateCoordinator.CheckForUpdatesAsync(Handle, userInitiated: true, showIfCurrent: true).ConfigureAwait(true);
        }
        catch (Exception ex)
        {
            ThemeHelper.ShowMessageBox(this, $"Unable to perform the check.{Environment.NewLine}{ex.Message}", "Samandarin Update Notifier", MessageBoxButtons.OK, MessageBoxIcon.Error);
        }
        finally
        {
            _checkNowButton.Enabled = true;
            UpdateStatusLabels();
            _statusLabel.Text = "Status: Last check completed.";
        }
    }

    private void UpdateStatusLabels()
    {
        var snapshot = UpdateCoordinator.GetSnapshot();
        _settings = snapshot.Settings.Clone();
        _lastCheckLabel.Text = _settings.LastCheckUtc.HasValue
            ? string.Format(CultureInfo.CurrentCulture, "Last automatic check: {0:g}", _settings.LastCheckUtc.Value.ToLocalTime())
            : "Last automatic check: Never";
        _latestVersionLabel.Text = string.IsNullOrWhiteSpace(_settings.LastKnownRemoteVersion)
            ? "Last known release: Unknown"
            : string.Format(CultureInfo.CurrentCulture, "Last known release: {0}", _settings.LastKnownRemoteVersion);
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

    public static UpdateSettings Load(string path)
    {
        var settings = new UpdateSettings();
        if (!File.Exists(path))
        {
            return settings;
        }

        try
        {
            foreach (var line in File.ReadAllLines(path))
            {
                var trimmed = line.Trim();
                if (trimmed.Length == 0 || trimmed.StartsWith("#", StringComparison.Ordinal))
                {
                    continue;
                }

                int separator = trimmed.IndexOf('=');
                if (separator <= 0)
                {
                    continue;
                }

                var key = trimmed.Substring(0, separator).Trim();
                var value = trimmed.Substring(separator + 1).Trim();

                switch (key)
                {
                    case "checkOnStartup":
                        if (bool.TryParse(value, out var check))
                        {
                            settings.CheckOnStartup = check;
                        }
                        break;
                    case "frequency":
                        if (Enum.TryParse(value, true, out UpdateFrequency frequency))
                        {
                            settings.Frequency = frequency;
                        }
                        break;
                    case "lastCheckUtc":
                        if (DateTimeOffset.TryParse(value, CultureInfo.InvariantCulture, DateTimeStyles.AssumeUniversal | DateTimeStyles.AdjustToUniversal, out var timestamp))
                        {
                            settings.LastCheckUtc = timestamp;
                        }
                        break;
                    case "lastPromptedVersion":
                        settings.LastPromptedVersion = value;
                        break;
                    case "lastKnownRemoteVersion":
                        settings.LastKnownRemoteVersion = value;
                        break;
                }
            }
        }
        catch
        {
            return new UpdateSettings();
        }

        return settings;
    }

    public void Save(string path)
    {
        var builder = new StringBuilder();
        builder.AppendLine(string.Format(CultureInfo.InvariantCulture, "checkOnStartup={0}", CheckOnStartup));
        builder.AppendLine(string.Format(CultureInfo.InvariantCulture, "frequency={0}", Frequency));
        if (LastCheckUtc.HasValue)
        {
            builder.AppendLine(string.Format(CultureInfo.InvariantCulture, "lastCheckUtc={0:o}", LastCheckUtc.Value.ToUniversalTime()));
        }
        if (!string.IsNullOrWhiteSpace(LastPromptedVersion))
        {
            builder.AppendLine(string.Format(CultureInfo.InvariantCulture, "lastPromptedVersion={0}", LastPromptedVersion));
        }
        if (!string.IsNullOrWhiteSpace(LastKnownRemoteVersion))
        {
            builder.AppendLine(string.Format(CultureInfo.InvariantCulture, "lastKnownRemoteVersion={0}", LastKnownRemoteVersion));
        }

        File.WriteAllText(path, builder.ToString(), Encoding.UTF8);
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

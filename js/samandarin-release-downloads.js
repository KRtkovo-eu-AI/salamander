(() => {
  const OWNER = "KRtkovo-eu-AI";
  const REPO = "salamander";

  // Assets shown in the Download block for the latest release.
  // The global total below intentionally counts ALL release assets from ALL public releases.
  const LATEST_ASSET_RE = /_win_x64\.(exe|zip)$/i;

  const info = document.getElementById("latest-release-info");
  const list = document.getElementById("latest-release-downloads");

  if (!info || !list) {
    return;
  }

  const numberFormatter = new Intl.NumberFormat(undefined);
  const sizeFormatter = new Intl.NumberFormat(undefined, {
    maximumFractionDigits: 1
  });

  const apiUrl = `https://api.github.com/repos/${OWNER}/${REPO}/releases?per_page=100`;

  const preferredOrder = (name) => {
    const lower = name.toLowerCase();
    if (lower.endsWith(".exe")) return 0;
    if (lower.endsWith(".zip")) return 1;
    return 2;
  };

  const describeAsset = (name) => {
    const lower = name.toLowerCase();
    if (lower.endsWith(".exe")) return "Installer/Extract Portable";
    if (lower.endsWith(".zip")) return "ZIP Archive";
    return "Download";
  };

  const formatSize = (bytes) => {
    const value = Number(bytes);
    if (!Number.isFinite(value)) return "";
    return `${sizeFormatter.format(value / 1048576)} MB`;
  };

  const formatDownloads = (value) => {
    const count = Number(value || 0);
    return `${numberFormatter.format(count)} download${count === 1 ? "" : "s"}`;
  };

  const formatDate = (isoString) => {
    if (!isoString) return "";
    return new Intl.DateTimeFormat(undefined, {
      year: "numeric",
      month: "short",
      day: "numeric"
    }).format(new Date(isoString));
  };

  const getNextLink = (linkHeader) => {
    if (!linkHeader) return null;

    for (const part of linkHeader.split(",")) {
      const match = part.match(/<([^>]+)>;\s*rel="([^"]+)"/);
      if (match && match[2] === "next") {
        return match[1];
      }
    }

    return null;
  };

  async function fetchAllReleases() {
    const releases = [];
    let url = apiUrl;

    while (url) {
      const response = await fetch(url, {
        cache: "no-store",
        headers: {
          "Accept": "application/vnd.github+json",
          "X-GitHub-Api-Version": "2022-11-28"
        }
      });

      if (!response.ok) {
        throw new Error(`GitHub API returned ${response.status}`);
      }

      const page = await response.json();
      releases.push(...page);

      url = getNextLink(response.headers.get("Link"));
    }

    return releases;
  }

  async function loadLatestReleaseDownloads() {
    const releases = await fetchAllReleases();
    const publicReleases = releases.filter((release) => !release.draft);

    const latest = publicReleases
      .slice()
      .sort((a, b) =>
        new Date(b.published_at || b.created_at) -
        new Date(a.published_at || a.created_at)
      )[0];

    if (!latest) {
      throw new Error("No public GitHub release found.");
    }

    // Same principle as github-release-stats: sum download_count across assets from all releases.
    const allReleaseDownloads = publicReleases.reduce((releaseSum, release) => {
      const assetDownloads = (release.assets || []).reduce((assetSum, asset) => {
        return assetSum + Number(asset.download_count || 0);
      }, 0);

      return releaseSum + assetDownloads;
    }, 0);

    const assets = (latest.assets || [])
      .filter((asset) => LATEST_ASSET_RE.test(asset.name))
      .sort((a, b) =>
        preferredOrder(a.name) - preferredOrder(b.name) ||
        a.name.localeCompare(b.name)
      );

    if (assets.length === 0) {
      throw new Error(`Release ${latest.tag_name} has no matching x64 .exe/.zip assets.`);
    }

    const latestReleaseDownloads = assets.reduce((sum, asset) => {
      return sum + Number(asset.download_count || 0);
    }, 0);

    const releaseName = latest.tag_name || latest.name || "GitHub release";
    const published = formatDate(latest.published_at || latest.created_at);

    info.replaceChildren();

    const latestLabel = document.createElement("strong");
    latestLabel.textContent = "Latest release: ";

    const releaseLink = document.createElement("a");
    releaseLink.href = latest.html_url;
    releaseLink.textContent = releaseName;

    info.append(latestLabel, releaseLink);

    if (published) {
      info.append(`, published ${published}`);
    }

    info.append(document.createElement("br"));

    const allDownloadsLabel = document.createElement("strong");
    allDownloadsLabel.textContent = "Total downloads: ";
    info.append(allDownloadsLabel, `${formatDownloads(allReleaseDownloads)} across all releases`);

    info.append(document.createElement("br"));

    const latestDownloadsLabel = document.createElement("strong");
    latestDownloadsLabel.textContent = "Latest release downloads: ";
    info.append(latestDownloadsLabel, formatDownloads(latestReleaseDownloads));

    list.replaceChildren();

    for (const asset of assets) {
      const li = document.createElement("li");

      const link = document.createElement("a");
      link.href = asset.browser_download_url;
      link.textContent = asset.name;

      const details = ` ${describeAsset(asset.name)} ~ ${formatSize(asset.size)}`;
      const downloads = `; ${formatDownloads(asset.download_count)}`;

      li.append(link, details, downloads);
      list.append(li);
    }
  }

  loadLatestReleaseDownloads().catch((error) => {
    console.warn("Could not load latest GitHub release downloads:", error);
    info.textContent = "Could not load latest GitHub release data. Showing fallback download links.";
  });
})();

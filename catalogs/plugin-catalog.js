(function () {
  "use strict";

  var PUBLIC_BASE_URL = "https://krtkovo-eu-ai.github.io/salamander/catalogs/";

  var catalogDefinitions = [
    {
      key: "stable",
      fallbackName: "Stable",
      fileName: "plugins-stable.json"
    },
    {
      key: "unofficial",
      fallbackName: "Unofficial 3rd party",
      fileName: "plugins-unofficial.json"
    },
    {
      key: "extension-runtimes",
      fallbackName: "Extension Runtimes",
      fileName: "extension-runtimes.json"
    }
  ];

  var state = {
    catalogs: [],
    plugins: [],
    filteredPlugins: [],
    selectedKey: null,
    sortKey: "plugin",
    sortDirection: "asc"
  };

  var elements = {};

  document.addEventListener("DOMContentLoaded", function () {
    elements = {
      searchBox: document.getElementById("searchBox"),
      sourceFilter: document.getElementById("sourceFilter"),
      languageSelect: document.getElementById("languageSelect"),
      clearFiltersButton: document.getElementById("clearFiltersButton"),
      refreshButton: document.getElementById("refreshButton"),
      message: document.getElementById("message"),
      statusText: document.getElementById("statusText"),
      table: document.getElementById("pluginTable"),
      tableFrame: document.querySelector(".table-frame"),
      rows: document.getElementById("pluginRows"),
      emptyDetails: document.getElementById("emptyDetails"),
      pluginDetails: document.getElementById("pluginDetails"),
      detailPlugin: document.getElementById("detailPlugin"),
      detailSource: document.getElementById("detailSource"),
      detailAuthor: document.getElementById("detailAuthor"),
      detailLatestVersion: document.getElementById("detailLatestVersion"),
      detailVersionScheme: document.getElementById("detailVersionScheme"),
      detailHomepage: document.getElementById("detailHomepage"),
      detailDownload: document.getElementById("detailDownload"),
      detailDescription: document.getElementById("detailDescription")
    };

    elements.searchBox.addEventListener("input", applyFilters);
    elements.sourceFilter.addEventListener("change", applyFilters);
    elements.languageSelect.addEventListener("change", function () {
      applyFilters();
      if (state.selectedKey) {
        selectPlugin(state.selectedKey);
      }
    });
    elements.clearFiltersButton.addEventListener("click", clearFilters);
    elements.refreshButton.addEventListener("click", loadCatalogs);

    elements.rows.addEventListener("click", function (event) {
      var row = event.target.closest("tr[data-key]");
      if (row) {
        selectPlugin(row.getAttribute("data-key"));
      }
    });

    elements.rows.addEventListener("keydown", function (event) {
      var row = event.target.closest("tr[data-key]");
      if (!row) {
        return;
      }
      if (event.key === "Enter" || event.key === " ") {
        event.preventDefault();
        selectPlugin(row.getAttribute("data-key"));
      }
    });

    elements.table.querySelector("thead").addEventListener("click", function (event) {
      var header = event.target.closest("th[data-sort]");
      if (!header) {
        return;
      }

      var sortKey = header.getAttribute("data-sort");
      if (state.sortKey === sortKey) {
        state.sortDirection = state.sortDirection === "asc" ? "desc" : "asc";
      } else {
        state.sortKey = sortKey;
        state.sortDirection = "asc";
      }

      applyFilters();
    });

    loadCatalogs();
  });

  function buildCatalogUrls(fileName) {
    var urls = [];
    var pagePath = window.location.pathname || "/";
    var firstPathSegment = pagePath.split("/").filter(Boolean)[0] || "";
    var projectBase = firstPathSegment ? "/" + firstPathSegment + "/" : "/";

    // Works when plugin-catalog.html is in the same folder as the JSON files.
    urls.push(fileName);

    // Works when plugin-catalog.html is in the repository root and JSON files are in ./catalogs/.
    urls.push("catalogs/" + fileName);

    // Works for GitHub project pages such as /salamander/plugin-catalog.html.
    urls.push(projectBase + "catalogs/" + fileName);

    // Last-resort fallback to the already published public catalog URL.
    urls.push(PUBLIC_BASE_URL + fileName);

    return urls.filter(function (url, index, array) {
      return array.indexOf(url) === index;
    });
  }

  async function fetchJsonFromFirstWorkingUrl(fileName) {
    var urls = buildCatalogUrls(fileName);
    var attempts = [];

    for (var i = 0; i < urls.length; i += 1) {
      var url = urls[i];
      try {
        var response = await fetch(url, {
          cache: "no-cache",
          headers: {
            "Accept": "application/json"
          }
        });

        if (!response.ok) {
          attempts.push(url + " -> HTTP " + response.status);
          continue;
        }

        var json = await response.json();
        return {
          json: json,
          url: url
        };
      } catch (error) {
        attempts.push(url + " -> " + error.message);
      }
    }

    throw new Error("Could not load " + fileName + "\n" + attempts.join("\n"));
  }

  async function loadCatalogs() {
    setMessage("Loading plugin catalogs…", "");
    setStatus("Loading plugin catalogs…");
    elements.refreshButton.disabled = true;

    try {
      var loadedCatalogs = [];

      for (var i = 0; i < catalogDefinitions.length; i += 1) {
        var definition = catalogDefinitions[i];
        var result = await fetchJsonFromFirstWorkingUrl(definition.fileName);
        var catalogName = result.json.catalogName || definition.fallbackName;

        loadedCatalogs.push({
          key: definition.key,
          source: catalogName,
          url: result.url,
          generatedAt: result.json.generatedAt || "",
          plugins: Array.isArray(result.json.plugins) ? result.json.plugins : []
        });
      }

      state.catalogs = loadedCatalogs;
      state.plugins = normalizePlugins(loadedCatalogs);
      state.selectedKey = null;
      fillSourceFilter(loadedCatalogs);
      applyFilters();

      var total = state.plugins.length;
      var sourceText = loadedCatalogs.map(function (catalog) {
        return catalog.source + " (" + catalog.plugins.length + ")";
      }).join(", ");

      setMessage("Loaded " + total + " plugins from " + loadedCatalogs.length + " catalogs.", "ok");
      setStatus("Plugin catalog load completed. " + sourceText + ".");
      clearDetails();
    } catch (error) {
      setMessage(error.message, "error");
      setStatus("Plugin catalog load failed.");
      elements.rows.innerHTML = "";
      clearDetails();
      console.error(error);
    } finally {
      elements.refreshButton.disabled = false;
    }
  }

  function normalizePlugins(catalogs) {
    var result = [];

    catalogs.forEach(function (catalog) {
      catalog.plugins.forEach(function (plugin, index) {
        var key = catalog.key + ":" + (plugin.id || index);
        result.push({
          key: key,
          source: catalog.source,
          sourceKey: catalog.key,
          sourceUrl: catalog.url,
          id: plugin.id || "",
          name: plugin.name || {},
          author: plugin.author || "",
          description: plugin.description || {},
          latestVersion: plugin.latestVersion || "",
          versionScheme: plugin.versionScheme || "",
          homepageUrl: plugin.homepageUrl || "",
          downloadPageUrl: plugin.downloadPageUrl || ""
        });
      });
    });

    return result;
  }

  function fillSourceFilter(catalogs) {
    var previous = elements.sourceFilter.value || "all";
    elements.sourceFilter.innerHTML = "";

    var allOption = document.createElement("option");
    allOption.value = "all";
    allOption.textContent = "All sources";
    elements.sourceFilter.appendChild(allOption);

    catalogs.forEach(function (catalog) {
      var option = document.createElement("option");
      option.value = catalog.source;
      option.textContent = catalog.source;
      elements.sourceFilter.appendChild(option);
    });

    elements.sourceFilter.value = Array.from(elements.sourceFilter.options).some(function (option) {
      return option.value === previous;
    }) ? previous : "all";
  }

  function applyFilters() {
    var query = normalizeSearchText(elements.searchBox.value);
    var source = elements.sourceFilter.value;
    var language = elements.languageSelect.value;

    state.filteredPlugins = state.plugins.filter(function (plugin) {
      if (source !== "all" && plugin.source !== source) {
        return false;
      }

      if (!query) {
        return true;
      }

      var haystack = [
        plugin.source,
        getLocalizedText(plugin.name, language),
        plugin.author,
        plugin.latestVersion,
        plugin.versionScheme,
        getLocalizedText(plugin.description, language),
        plugin.homepageUrl,
        plugin.downloadPageUrl
      ].join(" ");

      return normalizeSearchText(haystack).indexOf(query) !== -1;
    });

    sortPlugins();
    renderTable();
    updateSortHeaders();
    setStatus("Showing " + state.filteredPlugins.length + " of " + state.plugins.length + " plugins.");
  }

  function sortPlugins() {
    var language = elements.languageSelect.value;
    var direction = state.sortDirection === "desc" ? -1 : 1;

    state.filteredPlugins.sort(function (a, b) {
      var left = getSortValue(a, state.sortKey, language);
      var right = getSortValue(b, state.sortKey, language);
      return direction * left.localeCompare(right, undefined, { numeric: true, sensitivity: "base" });
    });
  }

  function getSortValue(plugin, key, language) {
    switch (key) {
      case "source":
        return plugin.source;
      case "plugin":
        return getLocalizedText(plugin.name, language);
      case "latestVersion":
        return plugin.latestVersion;
      case "author":
        return plugin.author;
      case "description":
        return getLocalizedText(plugin.description, language);
      default:
        return "";
    }
  }

  function renderTable() {
    var language = elements.languageSelect.value;
    var fragment = document.createDocumentFragment();

    state.filteredPlugins.forEach(function (plugin) {
      var row = document.createElement("tr");
      row.setAttribute("data-key", plugin.key);
      row.tabIndex = 0;
      if (plugin.key === state.selectedKey) {
        row.className = "selected";
      }

      row.appendChild(createCell(plugin.source, "col-source"));
      row.appendChild(createCell(getLocalizedText(plugin.name, language), "col-plugin"));
      row.appendChild(createCell(plugin.latestVersion, "col-version"));
      row.appendChild(createCell(plugin.author, "col-author"));
      row.appendChild(createCell(getLocalizedText(plugin.description, language), "col-description"));


      fragment.appendChild(row);
    });

    elements.rows.replaceChildren(fragment);
  }

  function createCell(text, className) {
    var cell = document.createElement("td");
    cell.className = className;
    cell.textContent = text || "—";
    cell.title = text || "";
    return cell;
  }

  function updateSortHeaders() {
    elements.table.querySelectorAll("th[data-sort]").forEach(function (header) {
      header.classList.remove("sort-asc", "sort-desc");
      if (header.getAttribute("data-sort") === state.sortKey) {
        header.classList.add(state.sortDirection === "asc" ? "sort-asc" : "sort-desc");
      }
    });
  }

  function updateSelectedRowClasses() {
    elements.rows.querySelectorAll("tr[data-key]").forEach(function (row) {
      row.classList.toggle("selected", row.getAttribute("data-key") === state.selectedKey);
    });
  }

  function selectPlugin(key) {
    var plugin = state.plugins.find(function (item) {
      return item.key === key;
    });

    if (!plugin) {
      clearDetails();
      return;
    }

    state.selectedKey = key;
    updateSelectedRowClasses();

    var language = elements.languageSelect.value;
    var pluginName = getLocalizedText(plugin.name, language);
    var description = getLocalizedText(plugin.description, language);

    elements.emptyDetails.hidden = true;
    elements.pluginDetails.hidden = false;
    elements.detailPlugin.textContent = pluginName || "—";
    elements.detailSource.textContent = plugin.source || "—";
    elements.detailAuthor.textContent = plugin.author || "—";
    elements.detailLatestVersion.textContent = plugin.latestVersion || "—";
    elements.detailVersionScheme.textContent = plugin.versionScheme || "—";
    elements.detailDescription.textContent = description || "—";

    replaceWithLink(elements.detailHomepage, plugin.homepageUrl);
    replaceWithLink(elements.detailDownload, plugin.downloadPageUrl);

    setStatus("Selected plugin: " + pluginName + ".");
  }

  function clearDetails() {
    state.selectedKey = null;
    elements.emptyDetails.hidden = false;
    elements.pluginDetails.hidden = true;
    [
      elements.detailPlugin,
      elements.detailSource,
      elements.detailAuthor,
      elements.detailLatestVersion,
      elements.detailVersionScheme,
      elements.detailHomepage,
      elements.detailDownload,
      elements.detailDescription
    ].forEach(function (element) {
      element.textContent = "";
    });
  }

  function replaceWithLink(container, url) {
    container.textContent = "";
    if (!url) {
      container.textContent = "—";
      return;
    }

    var link = document.createElement("a");
    link.href = url;
    link.target = "_blank";
    link.rel = "noopener noreferrer";
    link.textContent = url;
    container.appendChild(link);
  }

  function getLocalizedText(value, preferredLanguage) {
    if (value == null) {
      return "";
    }

    if (typeof value === "string") {
      return value;
    }

    return value[preferredLanguage] || value.english || value.czech || firstObjectValue(value) || "";
  }

  function firstObjectValue(value) {
    var keys = Object.keys(value || {});
    return keys.length ? String(value[keys[0]] || "") : "";
  }

  function normalizeSearchText(value) {
    return String(value || "")
      .normalize("NFD")
      .replace(/[\u0300-\u036f]/g, "")
      .toLowerCase();
  }

  function clearFilters() {
    elements.searchBox.value = "";
    elements.sourceFilter.value = "all";
    applyFilters();
    elements.searchBox.focus();
  }

  function setMessage(text, mode) {
    elements.message.textContent = text;
    elements.message.className = "message" + (mode ? " " + mode : "");
  }

  function setStatus(text) {
    elements.statusText.textContent = text;
  }
})();

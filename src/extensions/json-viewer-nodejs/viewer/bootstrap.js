/* Loads JSON beside this page, then starts the pd4d10/json-viewer content script. */
(function () {
  "use strict";
  function fail(message) {
    document.body.textContent = String(message || "Failed to load JSON.");
  }
  function loadScript(src) {
    return new Promise(function (resolve, reject) {
      var script = document.createElement("script");
      script.src = src;
      script.onload = function () {
        resolve();
      };
      script.onerror = function () {
        reject(new Error("Failed to load " + src));
      };
      document.head.appendChild(script);
    });
  }
  fetch("data.json")
    .then(function (response) {
      if (!response.ok) {
        throw new Error("HTTP " + response.status);
      }
      return response.text();
    })
    .then(function (text) {
      var pre = document.createElement("pre");
      pre.textContent = text;
      document.body.appendChild(pre);
      return loadScript("chrome-shim.js").then(function () {
        return loadScript("content-script.js");
      });
    })
    .catch(function (error) {
      fail(error && error.message ? error.message : error);
    });
})();

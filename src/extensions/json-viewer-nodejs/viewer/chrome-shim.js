/* Chrome extension API stub for the pd4d10/json-viewer content script in WebView2. */
(function () {
  "use strict";
  var memory = Object.create(null);
  function storageGet(keys, callback) {
    var result = {};
    if (keys == null) {
      Object.keys(memory).forEach(function (key) {
        result[key] = memory[key];
      });
    } else if (typeof keys === "string") {
      if (Object.prototype.hasOwnProperty.call(memory, keys)) {
        result[keys] = memory[keys];
      }
    } else if (Array.isArray(keys)) {
      keys.forEach(function (key) {
        if (Object.prototype.hasOwnProperty.call(memory, key)) {
          result[key] = memory[key];
        }
      });
    } else if (typeof keys === "object") {
      Object.keys(keys).forEach(function (key) {
        result[key] = Object.prototype.hasOwnProperty.call(memory, key)
          ? memory[key]
          : keys[key];
      });
    }
    if (typeof callback === "function") {
      callback(result);
    }
  }
  function storageSet(items, callback) {
    if (items && typeof items === "object") {
      Object.keys(items).forEach(function (key) {
        memory[key] = items[key];
      });
    }
    if (typeof callback === "function") {
      callback();
    }
  }
  function storageRemove(keys, callback) {
    var list = Array.isArray(keys) ? keys : [keys];
    list.forEach(function (key) {
      delete memory[key];
    });
    if (typeof callback === "function") {
      callback();
    }
  }
  window.chrome = window.chrome || {};
  window.chrome.storage = window.chrome.storage || {};
  window.chrome.storage.sync = {
    get: storageGet,
    set: storageSet,
    remove: storageRemove,
  };
})();

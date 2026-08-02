const openTargetedLanguageExample = () => {
  const target = document.getElementById(decodeURIComponent(location.hash.slice(1)));

  if (target?.matches("details.language-example")) {
    target.open = true;
  }
};

openTargetedLanguageExample();
window.addEventListener("hashchange", openTargetedLanguageExample);

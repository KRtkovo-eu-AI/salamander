(() => {
  const backToTop = document.querySelector("[data-back-to-top]");

  if (!backToTop) {
    return;
  }

  const toggleBackToTop = () => {
    backToTop.classList.toggle("back-to-top--visible", window.scrollY > 480);
  };

  backToTop.addEventListener("click", (event) => {
    event.preventDefault();
    window.scrollTo({ top: 0, behavior: "smooth" });
  });

  toggleBackToTop();
  window.addEventListener("scroll", toggleBackToTop, { passive: true });
})();

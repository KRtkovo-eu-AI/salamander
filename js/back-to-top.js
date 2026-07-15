(() => {
  const backToTop = document.querySelector("[data-back-to-top]");

  if (!backToTop) {
    return;
  }

  const toggleBackToTop = () => {
    backToTop.classList.toggle("back-to-top--visible", window.scrollY > 480);
  };

  const prefersReducedMotion = window.matchMedia("(prefers-reduced-motion: reduce)");

  backToTop.addEventListener("click", (event) => {
    event.preventDefault();
    window.scrollTo({
      top: 0,
      behavior: prefersReducedMotion.matches ? "auto" : "smooth"
    });
  });

  toggleBackToTop();
  window.addEventListener("scroll", toggleBackToTop, { passive: true });
})();

(() => {
  const backToTop = document.querySelector("[data-back-to-top]");
  const scrolledHeaderClass = "site-header--scrolled";

  const toggleBackToTop = () => {
    if (backToTop) {
      backToTop.classList.toggle("back-to-top--visible", window.scrollY > 480);
    }

    document.body.classList.toggle(scrolledHeaderClass, window.scrollY > 0);
  };

  const prefersReducedMotion = window.matchMedia("(prefers-reduced-motion: reduce)");

  if (backToTop) {
    backToTop.addEventListener("click", (event) => {
      event.preventDefault();
      window.scrollTo({
        top: 0,
        behavior: prefersReducedMotion.matches ? "auto" : "smooth"
      });
    });
  }

  toggleBackToTop();
  window.addEventListener("scroll", toggleBackToTop, { passive: true });
})();

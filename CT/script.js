// SwinIR-Med landing — interactions + GSAP animations
(function () {
  "use strict";

  // ---------- Navbar background on scroll ----------
  var nav = document.getElementById("nav");
  var onScroll = function () {
    if (!nav) return;
    nav.classList.toggle("is-scrolled", window.scrollY > 24);
  };
  window.addEventListener("scroll", onScroll, { passive: true });
  onScroll();

  // ---------- Mobile nav toggle ----------
  var toggle = document.getElementById("navToggle");
  var links = document.getElementById("navLinks");
  if (toggle && links) {
    toggle.addEventListener("click", function () {
      var open = links.classList.toggle("is-open");
      toggle.setAttribute("aria-expanded", String(open));
      toggle.setAttribute("aria-label", open ? "关闭菜单" : "打开菜单");
    });
    links.querySelectorAll("a").forEach(function (a) {
      a.addEventListener("click", function () {
        links.classList.remove("is-open");
        toggle.setAttribute("aria-expanded", "false");
      });
    });
  }

  // ---------- Reveal animations ----------
  var prefersReduced = window.matchMedia("(prefers-reduced-motion: reduce)").matches;
  var reveals = document.querySelectorAll(".reveal");

  // Fallback (no GSAP or reduced motion): CSS-transition reveal via IntersectionObserver
  if (prefersReduced || !window.gsap) {
    if (prefersReduced || !("IntersectionObserver" in window)) {
      reveals.forEach(function (el) { el.classList.add("is-visible"); });
    } else {
      var io = new IntersectionObserver(function (entries) {
        entries.forEach(function (entry) {
          if (entry.isIntersecting) {
            entry.target.classList.add("is-visible");
            io.unobserve(entry.target);
          }
        });
      }, { threshold: 0.14, rootMargin: "0px 0px -8% 0px" });
      reveals.forEach(function (el) { io.observe(el); });
    }
    return;
  }

  // ---------- GSAP path ----------
  var gsap = window.gsap;
  var ST = window.ScrollTrigger || null;
  if (ST) gsap.registerPlugin(ST);
  document.body.classList.add("gsap-on");

  var ctx = gsap.context(function () {
    // Hero entrance timeline (title block, plays on load)
    var heroTl = gsap.timeline({ defaults: { ease: "power3.out", duration: 0.85 } });
    heroTl
      .from(".hero__eyebrow", { y: 18, autoAlpha: 0, duration: 0.7 })
      .from(".hero__title", { y: 28, autoAlpha: 0 }, "-=0.4")
      .from(".hero__authors", { y: 18, autoAlpha: 0 }, "-=0.55")
      .from(".hero__badges li", { y: 14, autoAlpha: 0, stagger: 0.07 }, "-=0.5")
      .from(".hero__links li", { y: 14, autoAlpha: 0, stagger: 0.06 }, "-=0.55")
      .from(".hero__stats li", { y: 16, autoAlpha: 0, stagger: 0.08 }, "-=0.55")
      .from(".scroll-hint", { autoAlpha: 0, duration: 0.6 }, "-=0.3");

    if (ST) {
      // Grouped, staggered reveals for grid cards / lists
      ScrollTrigger.batch(".grid .reveal", {
        start: "top 88%",
        onEnter: function (batch) {
          gsap.fromTo(batch,
            { y: 28, autoAlpha: 0 },
            { y: 0, autoAlpha: 1, duration: 0.8, stagger: 0.1, ease: "power3.out", overwrite: true });
        }
      });

      // Single-element reveals (section heads, download block, flow)
      gsap.utils.toArray(".reveal").forEach(function (el) {
        if (el.closest(".grid")) return;
        gsap.fromTo(el,
          { y: 26, autoAlpha: 0 },
          {
            y: 0, autoAlpha: 1, duration: 0.9, ease: "power3.out",
            scrollTrigger: { trigger: el, start: "top 85%", once: true }
          });
      });
    } else {
      // GSAP present but ScrollTrigger failed — reveal everything immediately
      gsap.set(".reveal", { autoAlpha: 1, y: 0 });
    }
  });

  // Recompute positions once images/fonts settle
  window.addEventListener("load", function () {
    if (ST) ST.refresh();
  });
})();

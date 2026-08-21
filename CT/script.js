// SwinIR-Med research page — restrained interactions with graceful degradation
(function () {
  "use strict";

  var root = document.documentElement;
  var reduceMotion = window.matchMedia("(prefers-reduced-motion: reduce)").matches;
  var desktopNav = window.matchMedia("(min-width: 981px)");
  var header = document.querySelector(".site-header");
  var progress = document.getElementById("scrollProgress");
  var progressChapter = document.getElementById("progressChapter");
  var toTop = document.getElementById("toTop");
  var navToggle = document.getElementById("navToggle");
  var navLinks = document.getElementById("navLinks");
  var framePending = false;

  function renderScrollState() {
    var y = window.scrollY || document.documentElement.scrollTop;
    var range = document.documentElement.scrollHeight - window.innerHeight;
    if (header) header.classList.toggle("is-scrolled", y > 12);
    if (progress) {
      progress.style.transform = "scaleX(" + (range > 0 ? Math.min(1, y / range) : 0) + ")";
    }
    if (toTop) toTop.classList.toggle("show", y > 520);
    framePending = false;
  }

  function requestScrollRender() {
    if (!framePending) {
      framePending = true;
      window.requestAnimationFrame(renderScrollState);
    }
  }

  window.addEventListener("scroll", requestScrollRender, { passive: true });
  window.addEventListener("resize", requestScrollRender, { passive: true });
  renderScrollState();

  if (toTop) {
    toTop.addEventListener("click", function () {
      window.scrollTo({ top: 0, behavior: reduceMotion ? "auto" : "smooth" });
    });
  }

  function setMenu(open, returnFocus) {
    if (!navToggle || !navLinks) return;
    navLinks.classList.toggle("open", open);
    navToggle.setAttribute("aria-expanded", open ? "true" : "false");
    navToggle.setAttribute("aria-label", open ? "收起导航菜单" : "展开导航菜单");
    if ("inert" in navLinks) navLinks.inert = !open && !desktopNav.matches;
    if (returnFocus) navToggle.focus();
  }

  if (navToggle && navLinks) {
    setMenu(false, false);
    navToggle.addEventListener("click", function () {
      setMenu(!navLinks.classList.contains("open"), false);
    });
    navLinks.addEventListener("click", function (event) {
      if (event.target.closest("a")) setMenu(false, false);
    });
    document.addEventListener("keydown", function (event) {
      if (event.key === "Escape" && navLinks.classList.contains("open")) setMenu(false, true);
    });
    document.addEventListener("click", function (event) {
      if (!navLinks.classList.contains("open")) return;
      if (!navLinks.contains(event.target) && !navToggle.contains(event.target)) setMenu(false, false);
    });
    var onNavBreakpoint = function () {
      setMenu(false, false);
      if ("inert" in navLinks) navLinks.inert = !desktopNav.matches;
    };
    if (desktopNav.addEventListener) desktopNav.addEventListener("change", onNavBreakpoint);
    else desktopNav.addListener(onNavBreakpoint);
  }

  var tocList = document.querySelector(".toc-list");
  var indicator = document.getElementById("tocIndicator");
  var allLinks = Array.prototype.slice.call(document.querySelectorAll(".nav-links a, .toc-link"));
  var currentActive = null;

  function moveIndicator(link) {
    if (!tocList || !indicator) return;
    if (!link) {
      indicator.style.opacity = "0";
      return;
    }
    var listRect = tocList.getBoundingClientRect();
    var linkRect = link.getBoundingClientRect();
    indicator.style.transform = "translateY(" + (linkRect.top - listRect.top) + "px)";
    indicator.style.height = linkRect.height + "px";
    indicator.style.opacity = "1";
  }

  function setActive(id) {
    var href = "#" + id;
    allLinks.forEach(function (link) {
      var active = link.getAttribute("href") === href;
      link.classList.toggle("is-active", active);
      if (active) link.setAttribute("aria-current", "location");
      else link.removeAttribute("aria-current");
    });
    currentActive = document.querySelector('.toc-link[href="' + href + '"]');
    moveIndicator(currentActive);
    var section = document.getElementById(id);
    var label = section && section.getAttribute("data-nav");
    if (progressChapter) {
      progressChapter.textContent = label || "";
      progressChapter.classList.toggle("show", Boolean(label));
    }
  }

  function setupNavSpy() {
    var seen = Object.create(null);
    var sections = [];
    allLinks.forEach(function (link) {
      var selector = link.getAttribute("href");
      if (!selector || selector.charAt(0) !== "#" || seen[selector]) return;
      var section = document.querySelector(selector);
      if (section) {
        seen[selector] = true;
        sections.push(section);
      }
    });
    if (!sections.length) return;
    setActive(sections[0].id);
    if (!("IntersectionObserver" in window)) return;
    var spy = new IntersectionObserver(function (entries) {
      var visible = entries.filter(function (entry) { return entry.isIntersecting; });
      if (!visible.length) return;
      visible.sort(function (a, b) {
        return Math.abs(a.boundingClientRect.top - window.innerHeight * .45) - Math.abs(b.boundingClientRect.top - window.innerHeight * .45);
      });
      setActive(visible[0].target.id);
    }, { rootMargin: "-42% 0px -53% 0px" });
    sections.forEach(function (section) { spy.observe(section); });
  }

  setupNavSpy();
  window.addEventListener("resize", function () { moveIndicator(currentActive); }, { passive: true });
  if (document.fonts && document.fonts.ready) document.fonts.ready.then(function () { moveIndicator(currentActive); });

  // The manuscript is readable without GSAP, a network connection, or motion.
  function revealAll() {
    root.classList.remove("gsap-on");
  }
  revealAll();
})();

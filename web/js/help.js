// Local help actions reuse navigation; they never send actuator commands.
document.addEventListener("DOMContentLoaded", () => {
  const help = document.getElementById("tab14");
  help.addEventListener("click", event => {
    const shortcut = event.target.closest("[data-help-open]");
    if (shortcut) {
      event.preventDefault();
      const panel = document.getElementById(shortcut.dataset.helpOpen);
      panel.open = true;
      panel.querySelector("summary").focus({ preventScroll: true });
      panel.scrollIntoView({ block: "start", behavior: "smooth" });
    }
    const button = event.target.closest("[data-help-page]");
    if (button) document.querySelector(`.nav-list a[data-tab="${button.dataset.helpPage}"]`).click();
  });
});

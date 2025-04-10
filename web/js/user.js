function initLogType() {
  var typeSelect = document.getElementById("p10_logger_type");
  changeLogType(typeSelect);
}

function changeLogType(selectElem) {
  // get log type
  var selectedValue = selectElem.value;

  // get element-id of logger-level and logger-filter
  var levelSelect = document.getElementById("cfg_logger_level");
  var filterSelect = document.getElementById("cfg_logger_filter");

  if (selectedValue === "0") {
    levelSelect.style.display = "inline-block";
    filterSelect.style.display = "none";
  } else if (selectedValue === "1") {
    levelSelect.style.display = "none";
    filterSelect.style.display = "inline-block";
  }
}

async function loadSimulatedData() {
  if (!isGitHubPages()) {
    return; // Kein Simulationsmodus, wenn nicht auf GitHub Pages
  }

  console.log("GitHub Pages erkannt – Simulationsdaten werden geladen.");

  try {
    const response = await fetch("sim.json");
    if (!response.ok)
      throw new Error("Fehler beim Abrufen der Simulationsdaten");

    const simData = await response.json();
    updateJSON(simData); // Aktualisiert die UI mit den Simulationsdaten
  } catch (error) {
    console.error("Fehler beim Laden von sim.json:", error);
  }
}

// <<< START Sidebar Script >>>
let sidebar = document.querySelector(".sidebar");
let closeBtn = document.querySelector("#btn");

  closeBtn.addEventListener("click", ()=>{
    sidebar.classList.toggle("open");
    menuBtnChange();//calling the function(optional)
  });

  // Event-Listener für Tab-Menü
  document.querySelectorAll('.nav-list a').forEach(tab => {
    tab.onclick = function(e) {
        e.preventDefault();
        document.querySelectorAll('.nav-list a').forEach(t => t.classList.remove('active'));
        document.querySelectorAll('.tab-content').forEach(content => content.classList.remove('active'));

        const activeTab = this.getAttribute('data-tab');
        this.classList.add('active');
        document.getElementById(activeTab).classList.add('active');
    };
  });

  // following are the code to change sidebar button(optional)
  function menuBtnChange() {
   if(sidebar.classList.contains("open")){
     closeBtn.classList.replace("bx-menu", "bx-menu-alt-right");//replacing the iocns class
   }else {
     closeBtn.classList.replace("bx-menu-alt-right","bx-menu");//replacing the iocns class
   }
  }

  // Funktion zum Umschalten des Menüs basierend auf der Fensterbreite
function toggleMenuBasedOnWidth() {
  const screenWidth = window.innerWidth;
  const threshold = 1024; // Schwellenwert für die Fensterbreite

  if(screenWidth > threshold) {
    // Fenster ist schmal, Menü einklappen
      sidebar.classList.add("open");
      menuBtnChange(); // Optional, ändert den Button
  } else {
    // Fenster ist breit, Menü ausklappen
      sidebar.classList.remove("open");
      menuBtnChange(); // Optional, ändert den Button
  }
}

// Event-Listener für Fenstergrößenänderung hinzufügen
window.addEventListener("resize", toggleMenuBasedOnWidth);

// Initialprüfung beim Laden der Seite
toggleMenuBasedOnWidth();
// <<< END Sidebar Script >>>
// <<< START Sidebar Script >>>
let sidebar = document.querySelector(".sidebar");
let closeBtn = document.querySelector("#btn");

closeBtn.addEventListener("click", () => {
  sidebar.classList.toggle("open");
  menuBtnChange(); //calling the function(optional)
});

// Event-Listener for Tab-Menü
document.querySelectorAll(".nav-list a").forEach((tab) => {
  tab.onclick = function (e) {
    e.preventDefault();
    document
      .querySelectorAll(".nav-list a")
      .forEach((t) => t.classList.remove("active"));
    document
      .querySelectorAll(".tab-content")
      .forEach((content) => content.classList.remove("active"));

    const activeTab = this.getAttribute("data-tab");
    this.classList.add("active");
    document.getElementById(activeTab).classList.add("active");
  };
});

// Function for switching the menu based on the window width
function toggleMenuBasedOnWidth() {
  const screenWidth = window.innerWidth;
  const threshold = 1024;

  if (screenWidth > threshold) {
    sidebar.classList.add("open");
  } else {
    sidebar.classList.remove("open");
  }
}

// Event-Listener for window resize
window.addEventListener("resize", toggleMenuBasedOnWidth);

// check window size on refresh
toggleMenuBasedOnWidth();
// <<< END Sidebar Script >>>

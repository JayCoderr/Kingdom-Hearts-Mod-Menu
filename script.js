const tabButtons = document.querySelectorAll(".tab-button");
const tabContents = document.querySelectorAll(".tab-content");

function showTab(tabName) {

```
// Hide every tab
tabContents.forEach((content) => {
    content.classList.remove("active");
});


// Remove active state from every button
tabButtons.forEach((button) => {
    button.classList.remove("active");
});


// Show requested tab
const selectedContent = document.getElementById(tabName);

if (selectedContent) {
    selectedContent.classList.add("active");
}


// Activate matching button
const selectedButton = document.querySelector(
    `.tab-button[data-tab="${tabName}"]`
);

if (selectedButton) {
    selectedButton.classList.add("active");
}


// Update URL without reloading the page
history.replaceState(
    null,
    "",
    `#${tabName}`
);
```

}

// ============================================================
// TAB CLICK EVENTS
// ============================================================

tabButtons.forEach((button) => {

```
button.addEventListener("click", () => {

    const tabName = button.dataset.tab;

    showTab(tabName);

});
```

});

// ============================================================
// LOAD TAB FROM URL
// ============================================================

const startingTab =
window.location.hash.replace("#", "");

if (startingTab && document.getElementById(startingTab)) {

```
showTab(startingTab);
```

} else {

```
showTab("home");
```

}

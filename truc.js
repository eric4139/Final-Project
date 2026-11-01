const primaryButton = document.querySelector(".primary");
const reduceMotion = window.matchMedia("(prefers-reduced-motion: reduce)");

if (primaryButton && !reduceMotion.matches) {
  primaryButton.addEventListener("pointerenter", () => {
    primaryButton.animate(
      [
        { transform: "translateY(0) scale(1)" },
        { transform: "translateY(-3px) scale(1.04)" },
        { transform: "translateY(-1px) scale(1.02)" },
      ],
      { duration: 260, easing: "ease-out" },
    );
  });

  primaryButton.addEventListener("click", () => {
    primaryButton.animate(
      [
        { transform: "scale(1)" },
        { transform: "scale(0.94)" },
        { transform: "scale(1.06)" },
        { transform: "scale(1)" },
      ],
      { duration: 320, easing: "cubic-bezier(.34, 1.56, .64, 1)" },
    );
  });
}
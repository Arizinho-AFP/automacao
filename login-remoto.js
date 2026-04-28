// ============================================================
// AFP Sistemas — login-remoto.js
// Autenticação segura via servidor
// ============================================================

document.addEventListener("DOMContentLoaded", () => {
  const dots = document.querySelectorAll(".pin-display span");
  const errorMsg = document.getElementById("error-message");
  let currentPin = "";

  // Detecta slug da URL → index.html?slug=demo
  const params = new URLSearchParams(window.location.search);
  const slug = params.get("slug") || "demo";

  // Carrega nome e slogan do cliente via API (rota info — sem autenticação)
  fetch(`api.php?rota=info&slug=${slug}`)
    .then((r) => r.json())
    .then((data) => {
      if (data.nome) {
        const h1 = document.querySelector(".login-header h1");
        const p = document.querySelector(".login-header p");
        if (h1) h1.textContent = data.nome;
        if (p) p.textContent = data.slogan || "Automação inteligente";
      }
    })
    .catch(() => {});

  // ── TECLADO VIRTUAL ────────────────────────────────────
  document.querySelectorAll(".key").forEach((btn) => {
    btn.addEventListener("click", () => {
      if (currentPin.length < 6) {
        currentPin += btn.dataset.value;
        updateDisplay();
        clearError();
      }
    });
  });

  document.getElementById("key-clear").addEventListener("click", () => {
    currentPin = currentPin.slice(0, -1);
    updateDisplay();
  });

  document.getElementById("key-submit").addEventListener("click", (e) => {
    e.preventDefault();
    currentPin.length === 6
      ? fazerLogin()
      : showError("Digite o código completo");
  });

  // ── TECLADO FÍSICO ─────────────────────────────────────
  document.addEventListener("keydown", (e) => {
    if (e.key >= "0" && e.key <= "9" && currentPin.length < 6) {
      currentPin += e.key;
      updateDisplay();
    } else if (e.key === "Backspace") {
      currentPin = currentPin.slice(0, -1);
      updateDisplay();
    } else if (e.key === "Enter" && currentPin.length === 6) {
      fazerLogin();
    }
  });

  // ── DISPLAY DOS PONTOS ─────────────────────────────────
  function updateDisplay() {
    dots.forEach((dot, i) =>
      dot.classList.toggle("filled", i < currentPin.length),
    );
  }

  // ── LOGIN ──────────────────────────────────────────────
  function fazerLogin() {
    showError("Verificando...", "var(--primary)");

    const form = new FormData();
    form.append("slug", slug);
    form.append("pin", currentPin);

    fetch("api.php?rota=login", { method: "POST", body: form })
      .then((r) => r.text().then((text) => ({ status: r.status, text })))
      .then(({ status, text }) => {
        let data;
        try {
          data = JSON.parse(text);
        } catch {
          throw new Error("Resposta inválida do servidor.");
        }

        if (status === 200 && data.token) {
          sessionStorage.setItem("afp_token", data.token);
          sessionStorage.setItem("afp_slug", data.slug);
          sessionStorage.setItem("afp_nome", data.nome);
          sessionStorage.setItem("afp_slogan", data.slogan || "");
          window.location.href = "painel.html";
        } else {
          vibrateError();
          currentPin = "";
          updateDisplay();
          showError(data.erro || "Código incorreto");
        }
      })
      .catch(() => {
        currentPin = "";
        updateDisplay();
        showError("Erro de conexão. Tente novamente.");
      });
  }

  // ── FEEDBACK VISUAL ────────────────────────────────────
  function showError(msg, color = "#ff4b4b") {
    errorMsg.textContent = msg;
    errorMsg.style.color = color;
    errorMsg.style.opacity = "1";
  }

  function clearError() {
    errorMsg.textContent = "Digite seu PIN de acesso";
    errorMsg.style.color = "";
    errorMsg.style.opacity = "1";
  }

  function vibrateError() {
    const card = document.querySelector(".login-card");
    card.style.animation = "none";
    void card.offsetHeight;
    card.style.animation = "shake 0.4s ease";
  }

  const style = document.createElement("style");
  style.textContent = `
        @keyframes shake {
            0%,100% { transform: translateX(0); }
            25%      { transform: translateX(-10px); }
            75%      { transform: translateX(10px); }
        }`;
  document.head.appendChild(style);
});

// ============================================================
// AFP Sistemas — app-remoto.js
// Lógica do painel remoto — genérico e multi-cliente
// ============================================================

// ── AUTENTICAÇÃO ───────────────────────────────────────────
const AFPToken = sessionStorage.getItem("afp_token");
const AFPSlug = sessionStorage.getItem("afp_slug");
const AFPNome = sessionStorage.getItem("afp_nome");
const AFPSlogan = sessionStorage.getItem("afp_slogan");

if (!AFPToken || !AFPSlug) window.location.href = "index.html";

// ── ESTADO GLOBAL ──────────────────────────────────────────
let schedules = [];
let newScheduleAction = 1;
let selectedDays = [];
let currentTargetId = null;
let isGroupTarget = false;

// ── NOMES DOS DISPOSITIVOS ─────────────────────────────────
// Conjunto base — cliente pode ter mais ou menos
const deviceNames = {
  // Luzes
  luz_01: "Luz 01",
  luz_02: "Luz 02",
  luz_03: "Luz 03",
  luz_04: "Luz 04",
  luz_05: "Luz 05",
  luz_06: "Luz 06",
  luz_07: "Luz 07",
  luz_08: "Luz 08",
  // Ventiladores
  vent_01: "Ventilador 01",
  vent_02: "Ventilador 02",
  vent_03: "Ventilador 03",
  vent_04: "Ventilador 04",
  vent_05: "Ventilador 05",
  vent_06: "Ventilador 06",
  vent_07: "Ventilador 07",
  vent_08: "Ventilador 08",
  vent_09: "Ventilador 09",
  // Ar condicionado
  ar_01: "Ar Condicionado 01",
  ar_02: "Ar Condicionado 02",
  ar_03: "Ar Condicionado 03",
  ar_04: "Ar Condicionado 04",
  ar_05: "Ar Condicionado 05",
  // Externos
  refletor_dir: "Refletor Direito",
  refletor_esq: "Refletor Esquerdo",
  logo: "Logotipo",
  fachada: "Fachada",
  // Tomadas
  tomada_01: "Tomada 01",
  tomada_02: "Tomada 02",
  tomada_03: "Tomada 03",
  tomada_04: "Tomada 04",
};

// ── INICIALIZAÇÃO ──────────────────────────────────────────
document.addEventListener("DOMContentLoaded", () => {
  // Preenche identidade do cliente
  const nomeEl = document.querySelector(".user-info h1 strong");
  const sloganEl = document.querySelector(".user-info p");
  if (nomeEl && AFPNome) nomeEl.textContent = AFPNome;
  if (sloganEl && AFPSlogan) sloganEl.textContent = AFPSlogan;

  // Status remoto
  const statusMsg = document.getElementById("status-message");
  if (statusMsg) {
    statusMsg.textContent = "Modo Remoto";
    statusMsg.className = "online";
  }

  // Logout
  document.querySelector(".logout-button")?.addEventListener("click", (e) => {
    e.preventDefault();
    sessionStorage.clear();
    window.location.href = "index.html";
  });

  // Habilita cards e switches
  document
    .querySelectorAll(".device-card")
    .forEach((c) => c.classList.remove("disabled"));
  document
    .querySelectorAll(".switch input")
    .forEach((t) => (t.disabled = false));

  handleNavigation();
  checkStatus();
  setInterval(checkStatus, 3000);
});

// ── COMUNICAÇÃO COM A API ──────────────────────────────────

function authHeaders() {
  return { "X-Session-Token": AFPToken };
}

function apiUrl(rota = "estados") {
  return `api.php?rota=${rota}&slug=${AFPSlug}`;
}

function checkStatus() {
  fetch(apiUrl("estados"), { headers: authHeaders() })
    .then((res) => {
      if (res.status === 401) {
        sessionStorage.clear();
        window.location.href = "index.html";
        return null;
      }
      return res.json();
    })
    .then((states) => {
      if (states) updateUI(states);
    })
    .catch((err) => console.error("Erro ao ler estados:", err));
}

function updateUI(states) {
  for (const id in states) {
    const toggle = document.getElementById("switch-" + id);
    if (toggle && toggle.checked !== (states[id] == 1))
      toggle.checked = states[id] == 1;
  }
}

function sendCommand(id, estado, timer = 0) {
  const form = new FormData();
  form.append("id", id);
  form.append("estado", estado);
  form.append("timer", timer);
  form.append("slug", AFPSlug);

  fetch(apiUrl("estados"), {
    method: "POST",
    headers: authHeaders(),
    body: form,
  })
    .then((res) => {
      if (res.status === 401) {
        sessionStorage.clear();
        window.location.href = "index.html";
      } else checkStatus();
    })
    .catch(() => {
      if (/^[a-z]+_\d+$/.test(id)) {
        const el = document.getElementById("switch-" + id);
        if (el) el.checked = !el.checked;
      }
    });
}

function toggleDevice(deviceId, isChecked) {
  sendCommand(deviceId, isChecked ? "1" : "0");
}

function toggleAll(group, state, timer = 0) {
  sendCommand(group, state === "on" ? "1" : "0", timer);
}

// ── AGENDA ────────────────────────────────────────────────

function loadSchedules() {
  fetch(apiUrl("agenda"), { headers: authHeaders() })
    .then((res) => res.json())
    .then((data) => {
      schedules = Array.isArray(data) ? data : [];
      renderSchedules();
    })
    .catch(() => {
      const list = document.getElementById("lista-agendamentos");
      if (list)
        list.innerHTML =
          "<p style='text-align:center;color:red'>Erro ao carregar agenda.</p>";
    });
}

function uploadSchedules() {
  fetch(apiUrl("agenda"), {
    method: "POST",
    headers: { ...authHeaders(), "Content-Type": "application/json" },
    body: JSON.stringify(schedules),
  })
    .then((res) => res.json())
    .then((data) => {
      if (data.status === "agenda_salva") {
        closeAgendaModal();
        loadSchedules();
      } else alert("Erro ao salvar agenda: " + (data.erro || "desconhecido"));
    })
    .catch(() => alert("Erro de conexão ao salvar agenda."));
}

function renderSchedules() {
  const list = document.getElementById("lista-agendamentos");
  if (!list) return;
  list.innerHTML = "";
  if (!schedules.length) {
    list.innerHTML =
      "<p style='text-align:center;color:#888;margin-top:30px'>Nenhum agendamento.</p>";
    return;
  }
  const days = ["Dom", "Seg", "Ter", "Qua", "Qui", "Sex", "Sáb"];
  schedules.forEach((item, index) => {
    const time = `${String(item.h).padStart(2, "0")}:${String(item.m).padStart(2, "0")}`;
    const day = item.once
      ? "Timer Temporário"
      : item.d === -1
        ? "Todos os dias"
        : days[item.d];
    const icon = item.once
      ? '<i class="fa-solid fa-hourglass-half"></i>'
      : '<i class="fa-regular fa-calendar"></i>';
    const color = item.s === 1 ? "#28a745" : "#dc3545";
    const name = deviceNames[item.id] || item.id;
    const style = item.once ? "border-left:4px solid var(--accent);" : "";
    list.innerHTML += `
            <div class="schedule-card" style="${style}">
                <div class="schedule-info">
                    <h4>${name} <span style="font-size:.8em;color:${color}">(${item.s === 1 ? "LIGAR" : "DESLIGAR"})</span></h4>
                    <p>${icon} ${day}</p>
                </div>
                <div class="schedule-time">${time}</div>
                <button class="btn-delete" onclick="deleteSchedule(${index})">
                    <i class="fa-solid fa-trash"></i>
                </button>
            </div>`;
  });
}

function deleteSchedule(index) {
  if (confirm("Apagar este agendamento?")) {
    schedules.splice(index, 1);
    uploadSchedules();
  }
}

// ── MODAIS DE AGENDA ───────────────────────────────────────

function openAgendaModal() {
  document.getElementById("agenda-modal").style.display = "flex";
  selectedDays = [];
  newScheduleAction = 1;
  updateModalUI();
  initTimePicker();
  ["sel-hora", "sel-min"].forEach((id) => {
    const el = document.getElementById(id);
    if (el) el.value = "";
  });
}

function closeAgendaModal() {
  document.getElementById("agenda-modal").style.display = "none";
}
function setAgendaAction(val) {
  newScheduleAction = val;
  updateModalUI();
}

function toggleDay(i) {
  const idx = selectedDays.indexOf(i);
  idx > -1 ? selectedDays.splice(idx, 1) : selectedDays.push(i);
  updateModalUI();
}

function updateModalUI() {
  document
    .getElementById("btn-action-on")
    ?.classList.toggle("selected", newScheduleAction === 1);
  document
    .getElementById("btn-action-off")
    ?.classList.toggle("selected", newScheduleAction === 0);
  for (let i = 0; i < 7; i++)
    document
      .getElementById("day-" + i)
      ?.classList.toggle("active", selectedDays.includes(i));
}

function saveAgenda() {
  const device = document.getElementById("agenda-device").value;
  const hora = document.getElementById("sel-hora").value;
  const min = document.getElementById("sel-min").value;
  if (!hora || !min || !selectedDays.length) {
    alert("Selecione o horário e pelo menos um dia.");
    return;
  }
  selectedDays.forEach((day) =>
    schedules.push({
      id: device,
      d: day,
      h: parseInt(hora),
      m: parseInt(min),
      s: newScheduleAction,
      once: false,
    }),
  );
  uploadSchedules();
}

// ── TIMER ──────────────────────────────────────────────────

function openTimerModal(targetId, isGroup = false) {
  currentTargetId = targetId;
  isGroupTarget = isGroup;
  const input = document.getElementById("modal-time-input");
  if (input) {
    input.value = "";
    input.focus();
  }
  document.getElementById("timer-modal").style.display = "flex";
}

function closeModal() {
  document.getElementById("timer-modal").style.display = "none";
  currentTargetId = null;
}
function setModalValue(val) {
  document.getElementById("modal-time-input").value = val;
}

function confirmTimer() {
  const min = parseInt(document.getElementById("modal-time-input").value);
  if (!min || min <= 0) {
    alert("Tempo inválido.");
    return;
  }

  if (isGroupTarget) toggleAll(currentTargetId, "on", min);
  else {
    const t = document.getElementById("switch-" + currentTargetId);
    if (t) t.checked = true;
    sendCommand(currentTargetId, "1", min);
  }

  const off = new Date(Date.now() + min * 60000);
  const prefixos = {
    luzes: "luz_",
    ventiladores: "vent_",
    ares: "ar_",
    externos: "ref",
    tomadas: "tomada_",
  };
  let targets = [];

  if (isGroupTarget) {
    const pref = prefixos[currentTargetId] || "";
    document
      .querySelectorAll(`input[id^="switch-${pref}"]`)
      .forEach((el) => targets.push(el.id.replace("switch-", "")));
  } else targets.push(currentTargetId);

  targets.forEach((devId) =>
    schedules.push({
      id: devId,
      d: off.getDay(),
      h: off.getHours(),
      m: off.getMinutes(),
      s: 0,
      once: true,
    }),
  );
  uploadSchedules();
  closeModal();
  alert(
    `Ligado! Desliga às ${String(off.getHours()).padStart(2, "0")}:${String(off.getMinutes()).padStart(2, "0")}`,
  );
}

function setTimer(id) {
  openTimerModal(id, false);
}

// ── NAVEGAÇÃO ──────────────────────────────────────────────

function handleNavigation() {
  const navigate = () => {
    let hash = window.location.hash.substring(1) || "home";
    if (!["home", "agenda", "groups"].includes(hash)) hash = "home";
    document
      .querySelectorAll(".tab-content")
      .forEach((t) => (t.style.display = "none"));
    const tela = document.getElementById("tab-" + hash);
    if (tela) {
      tela.style.display = "block";
      if (hash === "agenda") loadSchedules();
    }
    document
      .querySelectorAll(".nav-item")
      .forEach((el) => el.classList.remove("active"));
    document.querySelector(`a[href="#${hash}"]`)?.classList.add("active");
  };
  window.addEventListener("hashchange", navigate);
  navigate();
}

function filterRoom(category, btnEl) {
  document
    .querySelectorAll(".room-btn")
    .forEach((b) => b.classList.remove("active"));
  btnEl?.classList.add("active");
  document.querySelectorAll(".device-card").forEach((card) => {
    const show = category === "all" || card.dataset.room === category;
    card.style.display = show ? "block" : "none";
    if (show) {
      card.style.opacity = "0";
      setTimeout(() => (card.style.opacity = "1"), 50);
    }
  });
}

function toggleMenu() {
  const nav = document.getElementById("bottom-nav");
  const overlay = document.getElementById("menu-overlay");
  const icon = document.getElementById("fab-icon");
  nav.classList.toggle("active");
  overlay.classList.toggle("active");
  icon.classList.toggle("fa-bars", !nav.classList.contains("active"));
  icon.classList.toggle("fa-xmark", nav.classList.contains("active"));
}

document.querySelectorAll(".nav-item").forEach((link) => {
  link.addEventListener("click", () =>
    setTimeout(() => {
      const nav = document.getElementById("bottom-nav");
      if (nav?.classList.contains("active")) toggleMenu();
    }, 150),
  );
});

// ── TIME PICKER ────────────────────────────────────────────

function initTimePicker() {
  createPickerColumn("hora", 24);
  createPickerColumn("min", 60);
}

function createPickerColumn(type, count) {
  const container = document.getElementById(
    type === "hora" ? "scroll-hora" : "scroll-min",
  );
  if (!container) return;
  container.innerHTML = "";
  let timer;
  container.addEventListener("scroll", () => {
    clearTimeout(timer);
    timer = setTimeout(() => snapToCenter(type, container), 150);
  });
  for (let i = 0; i < count; i++) {
    const val = String(i).padStart(2, "0");
    const div = document.createElement("div");
    div.className = "picker-item";
    div.innerText = val;
    div.onclick = () => selectItem(type, div, val);
    container.appendChild(div);
  }
}

function snapToCenter(type, container) {
  const items = container.querySelectorAll(".picker-item");
  const index = Math.min(
    Math.max(Math.round(container.scrollTop / 40), 0),
    items.length - 1,
  );
  selectItem(type, items[index], items[index].innerText);
}

function selectItem(type, element, value) {
  const container = document.getElementById(
    type === "hora" ? "scroll-hora" : "scroll-min",
  );
  const input = document.getElementById(
    type === "hora" ? "sel-hora" : "sel-min",
  );
  container
    .querySelectorAll(".picker-item")
    .forEach((el) => el.classList.remove("selected"));
  element.classList.add("selected");
  container.scrollTo({ top: element.offsetTop - 60, behavior: "smooth" });
  if (input) input.value = value;
}

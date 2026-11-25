"use strict";
const messages = document.getElementById("messages");
const form = document.getElementById("chat-form");
const input = document.getElementById("message");
const send = document.getElementById("send");
const progress = document.getElementById("progress");
const approval = document.getElementById("approval");
const approvalDetail = document.getElementById("approval-detail");
let busy = false, currentId = null, lastPending = false;

function bubble(role, message) {
  const item = document.createElement("article");
  item.className = "bubble " + role;
  item.textContent = message;
  messages.appendChild(item);
  messages.scrollTop = messages.scrollHeight;
}
function setBusy(value) {
  busy = value;
  send.disabled = value;
  input.disabled = value;
}
async function api(url, payload) {
  const options = {credentials: "same-origin", cache: "no-store"};
  if (payload !== undefined) {
    options.method = "POST";
    options.headers = {"Content-Type": "application/json"};
    options.body = JSON.stringify(payload);
  }
  const response = await fetch(url, options);
  if (response.status === 401) { location.reload(); throw Error("Sessione scaduta"); }
  const data = await response.json();
  if (!response.ok) throw Error(data.error || "Richiesta non riuscita");
  return data;
}
async function poll() {
  if (!busy) return;
  try {
    const state = await api("/api/state");
    if (currentId !== null && state.request_id !== currentId) {
      throw Error("La conversazione è stata modificata da un'altra scheda");
    }
    if (state.status === "confirmation" && state.pending) {
      approval.hidden = false;
      approvalDetail.textContent = state.pending.tool + "\n" + JSON.stringify(state.pending.args, null, 2);
      progress.textContent = "In attesa della tua autorizzazione";
      lastPending = true;
    } else {
      approval.hidden = true;
      if (state.status === "done") {
        bubble("bot", state.answer || "Operazione terminata.");
        setBusy(false);
        currentId = null;
        progress.textContent = "";
        lastPending = false;
        input.focus();
        return;
      }
      progress.textContent = lastPending ? "Elaborazione della conferma..." : "ALBATROS sta pensando...";
    }
  } catch (error) {
    progress.textContent = error.message;
    setBusy(false);
    approval.hidden = true;
    return;
  }
  setTimeout(poll, 950);
}
form.addEventListener("submit", async (event) => {
  event.preventDefault();
  const text = input.value.trim();
  if (!text || busy) return;
  bubble("user", text);
  setBusy(true);
  input.value = "";
  progress.textContent = "Invio in corso...";
  try {
    const data = await api("/api/send", {question: text});
    currentId = data.request_id;
    lastPending = false;
    poll();
  } catch (error) {
    bubble("bot", "Errore: " + error.message);
    progress.textContent = "";
    setBusy(false);
  }
});
input.addEventListener("keydown", (event) => {
  if (event.key === "Enter" && !event.shiftKey && !event.isComposing) {
    event.preventDefault();
    form.requestSubmit();
  }
});
async function confirm(approved) {
  if (!busy || currentId === null) return;
  document.getElementById("allow").disabled = true;
  document.getElementById("deny").disabled = true;
  try {
    await api("/api/confirm", {request_id: currentId, approved});
    approval.hidden = true;
    progress.textContent = approved ? "Operazione autorizzata..." : "Operazione rifiutata...";
  } catch (error) {
    progress.textContent = error.message;
  } finally {
    document.getElementById("allow").disabled = false;
    document.getElementById("deny").disabled = false;
  }
}
document.getElementById("allow").addEventListener("click", () => confirm(true));
document.getElementById("deny").addEventListener("click", () => confirm(false));
(async function restorePending() {
  try {
    const state = await api("/api/state");
    if (state.status === "working" || state.status === "confirmation") {
      currentId = state.request_id;
      setBusy(true);
      poll();
    }
  } catch (error) { progress.textContent = error.message; }
})();

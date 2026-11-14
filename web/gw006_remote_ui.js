/* Browser controls: all 96 button identities stay on GW006.  No secrets stored. */
(() => {
"use strict";
const $=id=>document.getElementById(id);
const sets=[
 ["Televisore",["Power","Sorgente","Mute","Vol +","Vol −","Can +","Can −","Menu","Home","Indietro","OK","Su","Giù","Sinistra","Destra","Guida","Info","Esci","Play","Pausa","Stop","Indietro veloce","Avanti veloce","Registra","0","1","2","3","4","5","6","7","8","9","Sottotitoli","Impostazioni"]],
 ["Climatizzatore",["Power","Temp +","Temp −","Modalità","Ventola +","Ventola −","Swing","Timer","Notte","Eco","Turbo","Luce","Deumidifica","Riscalda"]],
 ["Decoder",["Power","Home","Menu","Indietro","Guida","Info","OK","Su","Giù","Sinistra","Destra","Can +","Can −","Vol +","Vol −","Mute","1","2","3","4","5","6","7","8","9","0","Play","Pausa","Stop","Sorgente","Esci","Registra"]],
 ["Speaker",["Power","Vol +","Vol −","Mute","Play","Pausa","Successiva","Precedente","Modalità","Bassi +","Bassi −","Alti +","Alti −","Ingresso"]]
];
if(sets.map(x=>x[1].length).join(",")!=="36,14,32,14") throw Error("96 keys expected");
const offsets=[0,36,50,82];
const allowed=secureBrowserGate(location.protocol,globalThis.crypto?.subtle);
let tab=0, selected=-1, client=null, flags=Array(96).fill(false), staged=[], armed=false, pending=false;
const report=s=>{$("msg").textContent=s;};
function redraw() {
 const [label,keys]=sets[tab], offset=offsets[tab], area=$("buttons");
 $("remote-title").textContent=label+" · "+keys.length+" tasti";
 $("count").textContent=flags.slice(offset,offset+keys.length).filter(Boolean).length+" memorizzati";
 document.querySelectorAll(".tab").forEach((b,i)=>b.setAttribute("aria-selected",i===tab?"true":"false"));
 area.replaceChildren();
 keys.forEach((name,i)=>{
   const index=offset+i, b=document.createElement("button"), labelNode=document.createElement("span"),
         dot=document.createElement("span");
   b.className="key";b.type="button";
   b.setAttribute("aria-label",name+(flags[index]?" memorizzato":" vuoto"));
   labelNode.textContent=name;dot.className="dot"+(flags[index]?" saved":"");
   dot.setAttribute("aria-hidden","true");b.append(labelNode,dot);
   let timer=null, long=false;
   const cancel=()=>{if(timer!==null){clearTimeout(timer);timer=null;}};
   b.addEventListener("pointerdown",e=>{
     if(e.button!==0)return;
     long=false;cancel();
     timer=setTimeout(()=>{timer=null;long=true;showMenu(index,name);},3000);
   });
   for(const ev of ["pointerup","pointercancel","pointerleave"])b.addEventListener(ev,cancel);
   b.addEventListener("click",()=>{
     if(long){long=false;return;}
     void send(index);
   });
   b.addEventListener("contextmenu",e=>{e.preventDefault();cancel();long=true;showMenu(index,name);});
   area.append(b);
 });
}
function controls(){
 const active=!!client?.key;
 for(const id of ["key","nonce","pair"]) $(id).disabled=!allowed||active;
 $("refresh").disabled=!active||pending;
 $("disconnect").disabled=!active||pending;
}
async function refresh(){
 if(!client?.key||pending)return;
 pending=true;controls();
 try{
   flags=await client.savedInfo();
   $("state").textContent="Autenticato";
   report("Memoria dei 96 tasti verificata con risposta firmata.");
 }catch(e){
   flags.fill(false);$("state").textContent="Memoria non pronta";
   report("Memoria non disponibile: "+e.message);
 }finally{pending=false;controls();redraw();}
}
async function send(id){
 if(!client?.key){report("Comandi disabilitati: autenticazione necessaria.");return;}
 if(!flags[id]){report("Tasto senza codice memorizzato.");return;}
 try{const n=await client.transmit(id);report("Trasmissione confermata: "+n+" pacchetto/i.");}
 catch(e){report("Esito IR non certo (non riprovare automaticamente): "+e.message);}
}
function showMenu(index,name) {
 selected=index;staged=[];armed=false;
 $("menu-title").textContent="Tasto "+(index+1)+" — "+name;
 $("preview").textContent="Fino a 5 pacchetti IR per tasto.";
 $("learn").disabled=!client?.key;
 $("try").disabled=!client?.key;
 $("delete").disabled=!client?.key||!flags[index];
 $("stopLearn").disabled=true;
 $("save").disabled=true;
 $("discard").disabled=true;
 $("menu").showModal();
}
async function action(fn){
 try{await fn();}catch(e){
   $("preview").textContent="Non confermato: "+e.message;
   report("Operazione non confermata; non ripetere automaticamente.");
 }
}
$("learn").onclick=()=>void action(async()=>{
 if(!client?.key)throw Error("Non autenticato");
 await client.arm();armed=true;staged=[];
 $("preview").textContent="Acquisizione attiva. Premi sul telecomando originale.";
 $("learn").disabled=true;$("stopLearn").disabled=false;
});
$("stopLearn").onclick=()=>void action(async()=>{
 if(!armed)return;
 await client.disarm();armed=false;
 $("stopLearn").disabled=true;
 const first=await client.readCapture();
 staged=first?[first]:[];
 $("preview").textContent=first?
   "Primo pacchetto verificato: ancora nella coda IR. Premi Salva per memorizzare da 1 a 5 pacchetti, oppure Scarta.":
   "Nessun pacchetto ricevuto.";
 $("save").disabled=!first;
 $("discard").disabled=!first;
 $("learn").disabled=!!first;
});
$("save").onclick=()=>void action(async()=>{
 if(!client?.key||selected<0||!staged.length)throw Error("Nessun pacchetto in attesa");
 $("save").disabled=true;
 $("discard").disabled=true;
 let parts=[], savedCount=0;
 try {
   // COMMIT permanent GWCD, then ACK exactly that capture ID. This avoids
   // silently losing captured frames on a network failure or device reboot.
   for(let i=0;i<5;i++){
     const current=staged[0];
     if(!current)break;
     parts.push(current.raw);
     await client.saveCode(selected,parts);
     await client.ackCapture(current.id);
     savedCount++;
     staged=[];
     const next=i===4?null:await client.readCapture();
     if(next)staged=[next];
     if(!next)break;
   }
   flags[selected]=true;redraw();
   $("preview").textContent=savedCount+" pacchetto/i salvati nella memoria permanente. "+
     "È stata verificata la presenza del tasto tramite risposta autenticata.";
   $("learn").disabled=false;
   $("delete").disabled=false;
 } finally {
   $("save").disabled=!staged.length;
   $("discard").disabled=!staged.length;
 }
});
$("discard").onclick=()=>void action(async()=>{
 if(!client?.key||!staged.length)return;
 if(!confirm("Scartare i pacchetti acquisiti senza salvarli?"))return;
 for(let i=0;i<5;i++){
   const current=staged[0];
   if(!current)break;
   await client.ackCapture(current.id);
   const next=await client.readCapture();
   staged=next?[next]:[];
 }
 $("preview").textContent="Cattura scartata esplicitamente, nessun codice salvato.";
 $("save").disabled=true;$("discard").disabled=true;$("learn").disabled=false;
});
$("try").onclick=()=>void action(async()=>{
 if(!client?.key||selected<0)throw Error("Non pronto");
 if(staged.length){
   const r=await client.rpc(WIRE.txFrame,staged[0].raw);
   if(r.command!==WIRE.status||r.payload.length<1||r.payload[0]!==0)
     throw Error("Prova IR fallita");
   $("preview").textContent="Primo pacchetto acquisito trasmesso, NON salvato.";
 } else {
   const n=await client.transmit(selected);
   $("preview").textContent="Trasmissione confermata: "+n+" pacchetto/i salvati.";
 }
});
$("delete").onclick=()=>void action(async()=>{
 if(!client?.key||selected<0||!flags[selected])return;
 if(!confirm("Eliminare il codice IR memorizzato?"))return;
 await client.erase(selected);flags[selected]=false;redraw();
 $("delete").disabled=true;$("preview").textContent="Eliminato con conferma firmata.";
});
$("close").onclick=()=>{
 if(armed||staged.length||pending){report("Prima salva o scarta l'acquisizione in attesa.");return;}
 $("menu").close();
};
$("menu").addEventListener("cancel",e=>{
 if(armed||staged.length||pending){e.preventDefault();report("Prima salva o scarta l'acquisizione in attesa.");}
});
document.querySelectorAll(".tab").forEach((b,i)=>b.onclick=()=>{
 if($("menu").open)return;
 tab=i;redraw();
});
$("pair").onclick=()=>void(async()=>{
 if(!allowed)return;
 const key=$("key").value,nonce=$("nonce").value;
 $("key").value="";$("nonce").value="";
 try{
   client=new GwirBrowserClient(globalThis.fetch.bind(globalThis),
                                globalThis.crypto.subtle,location.protocol);
   await client.pair(key,nonce);
   $("state").textContent="Autenticazione in corso";controls();await refresh();
 }catch(e){
   client?.clear();client=null;controls();
   $("state").textContent="Disconnesso";report("Autenticazione non riuscita: "+e.message);
 }
})();
$("refresh").onclick=()=>void refresh();
$("disconnect").onclick=()=>{
 if(armed||staged.length||pending){report("Termina e salva o scarta l'acquisizione prima di disconnetterti.");return;}
 client?.clear();client=null;flags.fill(false);staged=[];armed=false;
 $("state").textContent="Disconnesso";report("Sessione chiusa; chiavi non salvate.");
 controls();redraw();
};
if(!allowed){
 $("warning").textContent="ACCESSO BLOCCATO: HTTP senza TLS non consente l'immissione sicura delle chiavi. "+
   "Il server Realtek attuale non ha HTTPS. Interfaccia dimostrativa: nessun comando reale disponibile.";
 $("state").textContent="HTTP non sicuro";
}else{
 $("warning").textContent="HTTPS attivo: inserire chiave personale e nonce di avvio "+
   "soltanto se forniti tramite un canale affidabile. Nessuna password salvata.";
}
controls();redraw();
})();
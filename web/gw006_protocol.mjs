/* GW006 IR Universal Browser protocol V1 -- no keys ever stored.
 * This module is usable only from an HTTPS secure context. No WebCrypto
 * fallback, no HTTP secret entry, no weak digest, no localStorage.
 * Existing GWAS 1: HMAC-SHA256 over header+complete GWIR v3 message.
 */
const te = new TextEncoder();
const WIRE = Object.freeze({
  hello:1, arm:2, disarm:3, txFrame:4, read:5, ping:6, stats:7, ack:8,
  saved:9, begin:10, chunk:11, commit:12, del:13, info:14, abort:15,
  helloReply:0x81, capture:0x85, status:0x90, savedStatus:0x91,
  storeStatus:0x92, infoReply:0x93, pong:0x86, statsReply:0x87
});
const MAX_INNER = 20 + 8208;
const MAX_SIGNED = 36 + MAX_INNER + 32;
function asBytes(v) {
  if (!(v instanceof Uint8Array)) throw new TypeError("Expected bytes");
  return v;
}
function cleanHex(s, n) {
  if (typeof s !== "string" || !new RegExp("^[0-9a-fA-F]{"+n*2+"}$").test(s))
    throw new Error("Expected exactly "+n+" bytes in hexadecimal");
  return Uint8Array.from({length:n}, (_,i)=>Number.parseInt(s.slice(i*2,i*2+2),16));
}
function crc32(a, b) {
  let c=0xffffffff;
  for (const bytes of [a,b]) for (const v of bytes) {
    c ^= v;
    for (let j=0;j<8;j++) c=(c>>>1) ^ ((c&1) ? 0xedb88320 : 0);
  }
  return (c ^ 0xffffffff)>>>0;
}
function be32(u,o) { return new DataView(u.buffer,u.byteOffset,u.byteLength).getUint32(o,false); }
function put32(u,o,v) { new DataView(u.buffer,u.byteOffset,u.byteLength).setUint32(o,v>>>0,false); }
function be64(u,o) { return new DataView(u.buffer,u.byteOffset,u.byteLength).getBigUint64(o,false); }
function put64(u,o,v) { new DataView(u.buffer,u.byteOffset,u.byteLength).setBigUint64(o,v,false); }
function encodeGwir(cmd,seq,payload=new Uint8Array(0)) {
  asBytes(payload);
  if (!Number.isInteger(cmd)||cmd<1||cmd>255||!Number.isInteger(seq)||
      seq<1||seq>0xffffffff||payload.length>8208) throw new RangeError("GWIR command");
  const u=new Uint8Array(20+payload.length);
  u.set(te.encode("GWIR")); u[4]=3;u[5]=cmd;
  put32(u,8,seq);put32(u,12,payload.length);u.set(payload,20);
  put32(u,16,crc32(u.subarray(0,16),u.subarray(20)));
  return u;
}
function decodeGwir(u) {
  asBytes(u);
  if (u.length<20||u.length>MAX_INNER||String.fromCharCode(...u.subarray(0,4))!=="GWIR"||
      u[4]!==3||u[6]||u[7]||be32(u,12)!==u.length-20 ||
      crc32(u.subarray(0,16),u.subarray(20))!==be32(u,16)) throw new Error("Invalid GWIR message");
  return {command:u[5],sequence:be32(u,8),payload:u.slice(20)};
}
function secureBrowserGate(protocol, subtle) {
  return protocol==="https:" && !!subtle &&
    typeof subtle.importKey==="function" && typeof subtle.sign==="function" &&
    typeof subtle.verify==="function";
}
async function importKey(hex,subtle=globalThis.crypto?.subtle) {
  if (!subtle || typeof subtle.importKey!=="function") throw new Error("WebCrypto unavailable");
  const key=cleanHex(hex,32);
  if (key.every(x=>x===0)) throw new Error("Zero key forbidden");
  const result=await subtle.importKey("raw",key,{name:"HMAC",hash:"SHA-256"},false,["sign","verify"]);
  key.fill(0);
  return result;
}
async function signGwas(inner,key,nonce,counter,subtle=globalThis.crypto?.subtle) {
  asBytes(inner);asBytes(nonce);
  if (nonce.length!==16||nonce.every(x=>x===0)||counter<1n||
      counter>0x7fffffffffffffffn||inner.length<20||inner.length>MAX_INNER)
    throw new Error("Invalid signing inputs");
  const u=new Uint8Array(36+inner.length+32);
  u.set(te.encode("GWAS")); u[4]=1;u[5]=1;u.set(nonce,8);
  put64(u,24,counter);put32(u,32,inner.length);u.set(inner,36);
  const signed=new Uint8Array(await subtle.sign("HMAC",key,u.subarray(0,u.length-32)));
  if (signed.length!==32) throw new Error("Wrong HMAC length");
  u.set(signed,u.length-32);
  return u;
}
async function verifyGwas(u,key,nonce,minCounter,subtle=globalThis.crypto?.subtle) {
  asBytes(u);asBytes(nonce);
  if (u.length<88||u.length>MAX_SIGNED||nonce.length!==16||
      String.fromCharCode(...u.subarray(0,4))!=="GWAS"||u[4]!==1||u[5]!==2||
      u[6]!==0||u[7]!==0||
      !nonce.every((x,i)=>u[8+i]===x)) throw new Error("Invalid reply envelope");
  const counter=be64(u,24);
  if (counter<=minCounter||counter>0x7fffffffffffffffn) throw new Error("Replayed response");
  const innerLen=be32(u,32);
  if (innerLen<20||innerLen>MAX_INNER||u.length!==36+innerLen+32)
    throw new Error("Invalid response length");
  if (!(await subtle.verify("HMAC",key,u.subarray(36+innerLen),
                            u.subarray(0,36+innerLen))))
    throw new Error("Response signature invalid");
  return {counter,message:decodeGwir(u.subarray(36,36+innerLen))};
}
/* Stateless conversion deliberately does not depend on a server or DOM. */
function statusBitmap(payload) {
  asBytes(payload);
  if (payload.length!==13||payload[0]!==0) throw new Error("Storage not available");
  return Array.from({length:96},(_,i)=>!!(payload[1+(i>>3)]&(1<<(i&7))));
}
/* Exact GWCD v1 writer from pinned gwir_gwcd_store.c: 16-byte header,
 * 1..5 TX-shaped frame records, CRC32 IEEE trailer. */
function encodeGwcd(button,frames) {
  if (!Number.isInteger(button)||button<0||button>=96||
      !Array.isArray(frames)||frames.length<1||frames.length>5)
    throw new RangeError("GWCD button/frame count");
  let bytes=20;
  for(const frame of frames) {
    asBytes(frame);
    if(frame.length<16) throw new RangeError("GWCD frame header");
    const carrier=be32(frame,0), count=new DataView(frame.buffer,frame.byteOffset,frame.byteLength).getUint16(6,false),
          duty=new DataView(frame.buffer,frame.byteOffset,frame.byteLength).getUint16(4,false),
          gap=be32(frame,8);
    if(carrier<25000||carrier>500000||duty<200||duty>500||
       count<1||count>2048||gap>30000000||frame.length!==12+count*4)
      throw new RangeError("GWCD frame limits");
    for(let j=0;j<count;j++) {
      const pulse=be32(frame,12+j*4);
      if(pulse<1||pulse>30000000) throw new RangeError("GWCD pulse limits");
    }
    bytes+=frame.length;
  }
  if(bytes>41040)throw new RangeError("GWCD record too large");
  const record=new Uint8Array(bytes);
  record.set(te.encode("GWCD"),0);
  put32(record,4,1);put32(record,8,button);put32(record,12,frames.length);
  let offset=16;
  for(const frame of frames){record.set(frame,offset);offset+=frame.length;}
  put32(record,bytes-4,crc32(record.subarray(0,bytes-4),new Uint8Array(0)));
  return record;
}

class GwirBrowserClient {
  constructor(fetcher,subtle,protocol) {
    if (!secureBrowserGate(protocol,subtle))
      throw new Error("HTTPS and WebCrypto required before credentials or IR access");
    if (typeof fetcher!=="function") throw new TypeError("fetcher");
    this.fetcher=fetcher; this.subtle=subtle; this.key=null;
    this.nonce=null;this.tx=0n;this.rx=0n;this.seq=0;this.busy=false;this.uploading=false;
  }
  async pair(keyHex,nonceHex) {
    if (this.busy||this.key) throw new Error("Already paired or busy");
    const nonce=cleanHex(nonceHex,16);
    if (nonce.every(x=>x===0)) throw new Error("Zero boot nonce forbidden");
    const key=await importKey(keyHex,this.subtle);
    this.key=key;this.nonce=nonce;this.tx=0n;this.rx=0n;this.seq=0;
  }
  clear() { this.key=null;this.nonce=null;this.busy=false;this.uploading=false;this.tx=0n;this.rx=0n;this.seq=0; }
  async rpc(cmd,payload=new Uint8Array(0)) {
    if(this.uploading) throw new Error("Storage transaction in progress");
    return this._rpc(cmd,payload);
  }
  async _rpc(cmd,payload=new Uint8Array(0)) {
    if (!this.key||!this.nonce||this.busy) throw new Error("Unavailable or busy");
    if (this.tx>=0x7ffffffffffffffen||this.seq===0xffffffff)
      throw new Error("Counters exhausted: stop and securely reboot/re-pair");
    this.busy=true;
    try {
      this.seq++;
      const seq=this.seq;
      const request=encodeGwir(cmd,seq,payload);
      this.tx++; // never reuse a request counter, even on network failure
      const envelope=await signGwas(request,this.key,this.nonce,this.tx,this.subtle);
      const result=await this.fetcher("/api/gwir",{
        method:"POST",
        headers:{"Content-Type":"application/octet-stream"},
        credentials:"omit",cache:"no-store",redirect:"error",
        body:envelope
      });
      if (!result||!result.ok||result.type==="opaque" ||
          !result.headers?.get("content-type")?.toLowerCase().startsWith("application/octet-stream"))
        throw new Error("No verified binary response (operation outcome unknown)");
      const bytes=new Uint8Array(await result.arrayBuffer());
      const reply=await verifyGwas(bytes,this.key,this.nonce,this.rx,this.subtle);
      if (reply.message.sequence!==seq)
        throw new Error("Response sequence mismatch");
      this.rx=reply.counter;
      return reply.message;
    } finally { this.busy=false; }
  }
  async savedInfo() {
    const r=await this.rpc(WIRE.info);
    if (r.command!==WIRE.infoReply) throw new Error("Unexpected info reply");
    return statusBitmap(r.payload);
  }
  async transmit(button) {
    if (!Number.isInteger(button)||button<0||button>95) throw new RangeError("Button");
    const r=await this.rpc(WIRE.saved,Uint8Array.of(button));
    if (r.command!==WIRE.savedStatus||r.payload.length!==2)
      throw new Error("Unexpected send reply");
    if (r.payload[0]!==0) throw new Error("IR send failed, status "+r.payload[0]);
    return r.payload[1];
  }
  async erase(button) {
    if (!Number.isInteger(button)||button<0||button>95) throw new RangeError("Button");
    const r=await this.rpc(WIRE.del,Uint8Array.of(button));
    if (r.command!==WIRE.storeStatus||r.payload.length!==7||r.payload[0]!==0)
      throw new Error("Delete not confirmed");
    return true;
  }
  async saveCode(button, frames) {
    if(this.uploading||this.busy||!this.key) throw new Error("Storage busy or unauthenticated");
    const record=encodeGwcd(button,frames);
    const check=(r,active,received)=>{
      if(r.command!==WIRE.storeStatus||r.payload.length!==7||
         r.payload[0]!==0||r.payload[1]!== (active?button:255) ||
         r.payload[2]!==active||be32(r.payload,3)!==received)
        throw new Error("Storage transaction not confirmed");
    };
    this.uploading=true;
    let beginAttempted=false, commitAttempted=false;
    try {
      const begin=new Uint8Array(5);
      begin[0]=button;put32(begin,1,record.length);
      beginAttempted=true;
      check(await this._rpc(WIRE.begin,begin),1,0);
      const chunkMax=2048;
      for(let off=0;off<record.length;off+=chunkMax) {
        const chunk=record.subarray(off,Math.min(record.length,off+chunkMax));
        const p=new Uint8Array(5+chunk.length);
        p[0]=button;put32(p,1,off);p.set(chunk,5);
        check(await this._rpc(WIRE.chunk,p),1,off+chunk.length);
      }
      commitAttempted=true; // a lost commit reply is NEVER interpreted as failure-to-commit
      check(await this._rpc(WIRE.commit,Uint8Array.of(button)),0,0);
      const info=await this._rpc(WIRE.info);
      if(info.command!==WIRE.infoReply||!statusBitmap(info.payload)[button])
        throw new Error("Saved button missing from authenticated status");
      return record.length;
    }catch(error) {
      if (beginAttempted && !commitAttempted) {
        // BEGIN/CHUNK may already have reached the device. Send a NEW signed
        // command with a new anti-replay counter to wipe the RAM stage.
        // Never retry a failed chunk, since its outcome may be unknown.
        try {
          const r=await this._rpc(WIRE.abort,Uint8Array.of(button));
          const noStage=r.command===WIRE.storeStatus && r.payload.length===7 &&
                        r.payload[1]===255 && r.payload[2]===0 &&
                        be32(r.payload,3)===0;
          if(!noStage || (r.payload[0]!==0 && r.payload[0]!==9))
            throw new Error("Staging abort not confirmed");
        }catch(abortError){
          throw new Error("Save failed ("+error.message+
                          "); RAM staging abort unconfirmed ("+abortError.message+
                          "). Do not retry automatically");
        }
      }
      if(commitAttempted)
        throw new Error("Commit reply not verified ("+error.message+
                        "). The save outcome is UNKNOWN; refresh status and do not retry automatically");
      throw error;
    }finally{
      this.uploading=false;
      record.fill(0);
    }
  }
  async arm() {
    const r=await this.rpc(WIRE.arm);
    if(r.command!==WIRE.status||r.payload[0]!==0)
      throw new Error("Cannot arm IR");
  }
  async disarm() {
    const r=await this.rpc(WIRE.disarm);
    if(r.command!==WIRE.status||r.payload[0]!==0)
      throw new Error("Cannot disarm IR");
  }
  /* RX_READ and ACK are exposed for verified staged capturing; they DO NOT
   * silently save anything. A GWCD encoder + NOR verification is still needed. */
  async readCapture() {
    const r=await this.rpc(WIRE.read);
    if(r.command===WIRE.status&&r.payload[0]===3) return null;
    if(r.command!==WIRE.capture||r.payload.length<20)
      throw new Error("Unexpected capture response");
    const id=be32(r.payload,0);
    if(id===0) throw new Error("Invalid capture id");
    return {id,raw:r.payload.slice(4)};
  }
  async ackCapture(id) {
    if(!Number.isInteger(id)||id<1||id>0xffffffff) throw new RangeError("Capture id");
    const data=new Uint8Array(4);put32(data,0,id);
    const r=await this.rpc(WIRE.ack,data);
    if(r.command!==WIRE.status||r.payload[0]!==0) throw new Error("Capture not acknowledged");
  }
}
export {WIRE,cleanHex,crc32,encodeGwir,decodeGwir,secureBrowserGate,
        importKey,signGwas,verifyGwas,statusBitmap,encodeGwcd,GwirBrowserClient};

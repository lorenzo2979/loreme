import assert from "node:assert/strict";
import {test} from "node:test";
import {webcrypto} from "node:crypto";
import {
 WIRE, cleanHex, crc32, encodeGwir, decodeGwir, secureBrowserGate,
 importKey, signGwas, verifyGwas, statusBitmap, encodeGwcd, GwirBrowserClient
} from "../web/gw006_protocol.mjs";

const subtle=webcrypto.subtle;
const keyHex="c3".repeat(32), nonceHex="e5".repeat(16);
function hdr(bytes,offset) {return new DataView(bytes.buffer,bytes.byteOffset,bytes.byteLength).getUint32(offset,false);}
async function responseFor(req, key, nonce, sequence, counter, opts={}) {
  const payload=opts.payload ?? Uint8Array.of(0,4);
  const inner=encodeGwir(opts.cmd??WIRE.savedStatus,sequence,payload);
  const u=new Uint8Array(36+inner.length+32);
  u.set([71,87,65,83,1,2,0,0],0);
  u.set(nonce,8);
  new DataView(u.buffer).setBigUint64(24,counter,false);
  new DataView(u.buffer).setUint32(32,inner.length,false);
  u.set(inner,36);
  u.set(new Uint8Array(await subtle.sign("HMAC",key,u.subarray(0,u.length-32))),u.length-32);
  return u;
}
test("CRC32 IEEE, GWIR encode/decode and accidental-corruption rejection",()=>{
  assert.equal(crc32(new TextEncoder().encode("123"),new TextEncoder().encode("456789")),0xcbf43926);
  const u=encodeGwir(WIRE.saved,0x10203040,Uint8Array.of(95));
  assert.equal(u.length,21);
  assert.deepEqual(decodeGwir(u),{command:9,sequence:0x10203040,payload:Uint8Array.of(95)});
  u[20]^=1;
  assert.throws(()=>decodeGwir(u),/Invalid/);
  assert.throws(()=>encodeGwir(1,0),/command/);
});
test("HTTP blocked before any key can be accepted; bad keys/nonce rejected",async()=>{
  assert.equal(secureBrowserGate("http:",subtle),false);
  assert.equal(secureBrowserGate("https:",subtle),true);
  assert.throws(()=>new GwirBrowserClient(fetch,subtle,"http:"),/HTTPS/);
  const c=new GwirBrowserClient(async()=>{throw Error("network called");},subtle,"https:");
  await assert.rejects(()=>c.pair("0".repeat(64),nonceHex),/Zero key/);
  await assert.rejects(()=>c.pair(keyHex,"0".repeat(32)),/Zero boot nonce/);
  assert.throws(()=>cleanHex("oops",32),/Expected/);
  assert.equal(c.key,null);
});
test("HMAC signed envelope, reply direction, replay, tampering and wrong nonce",async()=>{
  const key=await importKey(keyHex,subtle);
  const nonce=cleanHex(nonceHex,16);
  const request=await signGwas(encodeGwir(WIRE.ping,3),key,nonce,1n,subtle);
  assert.equal(request.length,88);
  assert.equal(request[5],1);
  assert.equal(await subtle.verify("HMAC",key,request.slice(-32),request.slice(0,-32)),true);
  const good=await responseFor(request,key,nonce,3,1n,{cmd:WIRE.pong,payload:new Uint8Array(0)});
  const r=await verifyGwas(good,key,nonce,0n,subtle);
  assert.equal(r.counter,1n);
  assert.equal(r.message.command,WIRE.pong);
  await assert.rejects(()=>verifyGwas(good,key,nonce,1n,subtle),/Replayed/);
  const bad=good.slice(); bad[40]^=1;
  await assert.rejects(()=>verifyGwas(bad,key,nonce,0n,subtle),/signature/);
  await assert.rejects(()=>verifyGwas(good,key,cleanHex("b0".repeat(16),16),0n,subtle),/envelope/);
  const wrong=good.slice();wrong[5]=1;
  await assert.rejects(()=>verifyGwas(wrong,key,nonce,0n,subtle),/envelope/);
  const malformed=good.slice(0,good.length-1);
  await assert.rejects(()=>verifyGwas(malformed,key,nonce,0n,subtle),/(length|envelope)/);
});
test("96 status bits exactly, signed TX_SAVED and STORE_DELETE",async()=>{
  const bitmap=new Uint8Array(13);bitmap[0]=0;
  for (const i of [0,35,36,49,50,81,82,95])
    bitmap[1+(i>>3)]|=1<<(i&7);
  const statuses=statusBitmap(bitmap);
  assert.equal(statuses.length,96);
  assert.equal(statuses.filter(Boolean).length,8);
  for (const i of [0,35,36,49,50,81,82,95]) assert.equal(statuses[i],true);
  assert.throws(()=>statusBitmap(new Uint8Array(12)),/Storage/);
  let last=0n; let requests=[];
  const key=await importKey(keyHex,subtle); const nonce=cleanHex(nonceHex,16);
  const fetcher=async(url,init)=>{
    assert.equal(url,"/api/gwir");
    assert.equal(init.credentials,"omit");
    assert.equal(init.method,"POST");
    assert.equal(init.headers["Content-Type"],"application/octet-stream");
    const u=init.body;
    assert.equal(await subtle.verify("HMAC",key,u.slice(-32),u.slice(0,-32)),true);
    assert.equal(hdr(u,32)+68,u.length);
    const counter=new DataView(u.buffer,u.byteOffset,u.byteLength).getBigUint64(24);
    assert.ok(counter>last);last=counter;
    const inner=decodeGwir(u.subarray(36,u.length-32));
    requests.push(inner);
    const response=inner.command===WIRE.info ?
      {cmd:WIRE.infoReply,payload:bitmap} :
      inner.command===WIRE.del ?
      {cmd:WIRE.storeStatus,payload:Uint8Array.of(0,255,0,0,0,0,0)} :
      {cmd:WIRE.savedStatus,payload:Uint8Array.of(0,3)};
    const data=await responseFor(u,key,nonce,inner.sequence,BigInt(requests.length),response);
    return {ok:true,type:"basic",headers:{get:()=> "application/octet-stream"},
      arrayBuffer:async()=>data.buffer};
  };
  const client=new GwirBrowserClient(fetcher,subtle,"https:");
  await client.pair(keyHex,nonceHex);
  assert.equal((await client.savedInfo()).filter(Boolean).length,8);
  assert.equal(await client.transmit(95),3);
  assert.equal(await client.erase(0),true);
  assert.deepEqual(requests.map(x=>x.command),[14,9,13]);
  assert.equal(client.tx,3n);assert.equal(client.rx,3n);
  assert.deepEqual(Array.from(requests[1].payload),[95]);
  await assert.rejects(()=>client.transmit(-1),/Button/);
});
test("No retry and no nonce/counter reset on lost network result",async()=>{
  let calls=0;
  const client=new GwirBrowserClient(async()=>{calls++;throw Error("connection lost");},subtle,"https:");
  await client.pair(keyHex,nonceHex);
  await assert.rejects(()=>client.transmit(4),/connection lost/);
  await assert.rejects(()=>client.transmit(4),/connection lost/);
  assert.equal(calls,2);
  assert.equal(client.tx,2n);
  assert.equal(client.rx,0n);
});


test("GWCD v1 serializes 1 and five frames at exact firmware maximum",()=>{
  const one=new Uint8Array(16);
  const dv=new DataView(one.buffer);
  dv.setUint32(0,38000,false);
  dv.setUint16(4,333,false);
  dv.setUint16(6,1,false);
  dv.setUint32(8,25000,false);
  dv.setUint32(12,900,false);
  const rec=encodeGwcd(95,[one]);
  assert.equal(rec.length,36);
  assert.equal(new TextDecoder().decode(rec.subarray(0,4)),"GWCD");
  assert.equal(hdr(rec,4),1);
  assert.equal(hdr(rec,8),95);
  assert.equal(hdr(rec,12),1);
  assert.equal(hdr(rec,32),crc32(rec.subarray(0,32),new Uint8Array(0)));
  const maxFrame=new Uint8Array(12+2048*4),d=new DataView(maxFrame.buffer);
  d.setUint32(0,38000,false);d.setUint16(4,333,false);d.setUint16(6,2048,false);d.setUint32(8,25000,false);
  for(let i=0;i<2048;i++)d.setUint32(12+4*i,1000+i,false);
  const max=encodeGwcd(0,Array(5).fill(maxFrame));
  assert.equal(max.length,41040);
  assert.equal(hdr(max,41036),crc32(max.subarray(0,41036),new Uint8Array(0)));
  assert.throws(()=>encodeGwcd(96,[one]),/GWCD/);
  const bad=one.slice();new DataView(bad.buffer).setUint32(12,0,false);
  assert.throws(()=>encodeGwcd(0,[bad]),/pulse/);
});
test("Signed GWCD staged upload: begin, chunks, commit, readback, no concurrent RPC",async()=>{
 const key=await importKey(keyHex,subtle),nonce=cleanHex(nonceHex,16);
 const frame=new Uint8Array(16),v=new DataView(frame.buffer);
 v.setUint32(0,38000,false);v.setUint16(4,333,false);
 v.setUint16(6,1,false);v.setUint32(8,25000,false);v.setUint32(12,900,false);
 const expected=encodeGwcd(34,[frame]);
 const received=new Uint8Array(expected.length);
 let size=0,offset=0,active=false,commandLog=[],seqCounter=0n;
 const fetcher=async(_url,init)=>{
   const signed=new Uint8Array(init.body);
   const inner=decodeGwir(signed.subarray(36,signed.length-32));
   commandLog.push(inner.command);
   let cmd=WIRE.storeStatus, p=new Uint8Array(7);
   if(inner.command===WIRE.begin){
     assert.equal(inner.payload[0],34);
     size=hdr(inner.payload,1);offset=0;active=true;
   }else if(inner.command===WIRE.chunk){
     assert.equal(inner.payload[0],34);
     assert.equal(hdr(inner.payload,1),offset);
     const chunk=inner.payload.subarray(5);
     received.set(chunk,offset);offset+=chunk.length;
   }else if(inner.command===WIRE.commit){
     assert.equal(size,expected.length);
     assert.deepEqual(received,expected);
     assert.equal(offset,size);
     active=false;
   }else if(inner.command===WIRE.info) {
     cmd=WIRE.infoReply;p=new Uint8Array(13);p[1+(34>>3)]|=1<<(34&7);
   }else throw Error("Unexpected upload opcode");
   if(cmd===WIRE.storeStatus){
     p[0]=0;p[1]=active?34:255;p[2]=active?1:0;
     new DataView(p.buffer).setUint32(3,active?offset:0,false);
   }
   seqCounter++;
   const data=await responseFor(signed,key,nonce,inner.sequence,seqCounter,{cmd,payload:p});
   return {ok:true,headers:{get:()=>"application/octet-stream"},arrayBuffer:async()=>data.buffer};
 };
 const client=new GwirBrowserClient(fetcher,subtle,"https:");
 await client.pair(keyHex,nonceHex);
 assert.equal(await client.saveCode(34,[frame]),36);
 assert.deepEqual(commandLog,[WIRE.begin,WIRE.chunk,WIRE.commit,WIRE.info]);
 assert.equal(client.tx,4n);
 assert.equal(client.uploading,false);
});

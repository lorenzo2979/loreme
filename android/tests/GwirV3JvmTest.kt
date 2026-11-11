import org.gw006.ir.*
import java.io.ByteArrayInputStream
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.io.File

fun main() {
    val frame=IrFrame(38_000,333,25_000,listOf(9000,4500,560,560))
    val payload=GwirV3.txPayload(frame)
    val body=ByteBuffer.allocate(4+payload.size).order(ByteOrder.BIG_ENDIAN)
        .putInt(42).put(payload).array()
    val message=GwirV3.encode(GwirV3.RX_DATA, 5,body)
    val decoded=GwirV3.decode(message)
    check(decoded.command==GwirV3.RX_DATA && decoded.sequence==5L)
    val fromStream=GwirV3.readMessage(ByteArrayInputStream(message))
    check(fromStream.command==decoded.command && fromStream.sequence==decoded.sequence && fromStream.payload.contentEquals(decoded.payload))
    val (token,found)=GwirV3.rxData(decoded.payload)
    check(token==42L && found==frame)
    check(ByteBuffer.wrap(GwirV3.ackPayload(token)).order(ByteOrder.BIG_ENDIAN).int==42)
    val broken=message.clone();broken[22]=(broken[22].toInt() xor 0xFF).toByte()
    check(runCatching { GwirV3.decode(broken) }.isFailure)
    val root=File(System.getProperty("java.io.tmpdir"),"gw006_v3_jvm_test_${System.nanoTime()}")
    val store=AndroidCodeStore(root)
    val inbox=AndroidCaptureInbox(root)
    try {
        check(inbox.record(4,25,frame))
        check(!inbox.record(4,25,frame)) /* repeated after lost ACK */
        check(runCatching {inbox.record(4,25,frame.copy(carrierHz=40000))}.isFailure)
        for (id in 26L..29L) check(inbox.record(4,id,frame.copy(gapAfterUs=25000+id.toInt())))
        check(inbox.pending(4).size==5)
        check(runCatching {inbox.record(4,30,frame)}.isFailure)
        inbox.commit(4,store)
        check(inbox.pending(4).isEmpty())
        check(store.loadButton(4)?.size==5)
        /* An app process restart can read pending frames before ACK. */
        check(inbox.record(5,41,frame))
        val restartedInbox=AndroidCaptureInbox(root)
        check(restartedInbox.pending(5).single().first==41L)
        restartedInbox.commit(5,store)
        check(store.loadButton(5)==listOf(frame))
        for (button in 0 until 96) {
            val frames=List(5) { frame.copy(gapAfterUs=25000+it+button) }
            store.saveButton(button,frames)
            check(store.loadButton(button)==frames)
        }
        val huge=IrFrame(38000,333,30000,List(2048){if(it%2==0) 560 else 550})
        store.saveButton(0,listOf(huge,huge,huge,huge,huge))
        check(store.loadButton(0)?.size==5)
        check(store.loadButton(0)?.get(4)==huge)
        val file=File(root,"ir_button_00.gwcd")
        val data=file.readBytes();data[20]=(data[20].toInt() xor 0x80).toByte();file.writeBytes(data)
        check(runCatching { store.loadButton(0) }.isFailure)
        check(runCatching { store.saveButton(96,listOf(frame)) }.isFailure)
        check(runCatching { store.saveButton(1,List(6){frame}) }.isFailure)
    } finally { root.listFiles()?.forEach{ it.delete() };root.delete() }
    testGwirAuthentication()
    println("PASS Kotlin/JVM: protocol v3, durable staged ACK, 96 slots, five frames, max pulses, corruption")
}

private fun testGwirAuthentication() {
    val key=ByteArray(32){(it+1).toByte()}
    val nonce=ByteArray(16){(0xa0+it).toByte()}
    val client=GwirAuth(key,nonce)
    val server=GwirAuth(key,nonce)
    val inner=GwirV3.encode(6,7,"hi".toByteArray(Charsets.US_ASCII))
    val record=client.wrap(GwirAuth.TO_DEVICE,inner)
    val hex=record.joinToString(""){ "%02x".format(it.toInt() and 255) }
    val expected="4757415301010000a0a1a2a3a4a5a6a7a8a9aaabacadaeaf0000000000000001000000164757495203060000000000070000000200cab0b96869ef14e1a5eec5d9924b9ed11d9a05e34229ca947d174e1f7ba04ad73dbc425a2d"
    check(hex == expected) {"Kotlin/Java HMAC GWAS mismatch against C/OpenSSL test vector"}
    check(server.unwrap(GwirAuth.TO_DEVICE,record).contentEquals(inner))
    check(runCatching{server.unwrap(GwirAuth.TO_DEVICE,record)}.isFailure)
    check(runCatching{server.unwrap(GwirAuth.FROM_DEVICE,record)}.isFailure)
    val altered=record.clone();altered[38]=(altered[38].toInt() xor 1).toByte()
    check(runCatching{GwirAuth(key,nonce).unwrap(GwirAuth.TO_DEVICE,altered)}.isFailure)
    val next=client.wrap(GwirAuth.TO_DEVICE,inner)
    check(server.unwrap(GwirAuth.TO_DEVICE,next).contentEquals(inner))
    val rotatedNonce=nonce.clone();rotatedNonce[0]=(rotatedNonce[0].toInt() xor 1).toByte()
    check(runCatching{GwirAuth(key,rotatedNonce).unwrap(GwirAuth.TO_DEVICE,next)}.isFailure)
    check(runCatching{GwirAuth(ByteArray(32),nonce)}.isFailure)
    client.close()
    check(runCatching{client.wrap(GwirAuth.TO_DEVICE,inner)}.isFailure)
    server.close()
    println("PASS Kotlin/JVM: HMAC-SHA256 GWAS cross-language vector, replay and boot nonce separation")
}

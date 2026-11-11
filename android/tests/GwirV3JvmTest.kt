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
    println("PASS Kotlin/JVM: protocol v3, durable staged ACK, 96 slots, five frames, max pulses, corruption")
}

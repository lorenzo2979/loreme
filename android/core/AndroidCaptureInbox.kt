package org.gw006.ir

import java.io.ByteArrayOutputStream
import java.io.DataOutputStream
import java.io.File
import java.io.FileOutputStream
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.nio.file.Files
import java.nio.file.StandardCopyOption
import java.util.zip.CRC32

/**
 * One CRC-checked, atomically-replaced staging file per button.
 * After RX_DATA, call record(button, captureId, frame), then RX_ACK only
 * once record() finishes. Each capture can be re-delivered after reconnect.
 * A device reset still destroys its RAM queue; capture IDs are uptime-local.
 */
class AndroidCaptureInbox(private val directory: File) {
    init { require(directory.isDirectory || directory.mkdirs()) }
    private fun path(button: Int): File {
        require(button in 0 until 96)
        return File(directory, "capture_%02d.gwci".format(button))
    }

    @Synchronized fun pending(button: Int): List<Pair<Long, IrFrame>> {
        val file=path(button)
        if (!file.exists()) return emptyList()
        require(file.length() in 40..41_100) {"Unexpected capture file size"}
        val bytes=file.readBytes()
        val b=ByteBuffer.wrap(bytes).order(ByteOrder.BIG_ENDIAN)
        val expected=b.getInt(bytes.size-4).toLong() and 0xFFFFFFFFL
        val actual=CRC32().apply {update(bytes,0,bytes.size-4)}.value
        require(expected==actual) {"Corrupt capture inbox"}
        require(b.int==0x47574349 && b.int==1 && b.int==button) // GWCI v1
        val count=b.int
        require(count in 1..5)
        val results=ArrayList<Pair<Long,IrFrame>>(count)
        repeat(count) {
            require(b.position() <= bytes.size-4 - 16)
            val captureId=b.int.toLong() and 0xFFFFFFFFL
            require(captureId!=0L && results.none {it.first==captureId})
            val carrier=b.int
            val duty=b.short.toInt() and 0xffff
            val n=b.short.toInt() and 0xffff
            val gap=b.int
            require(n in 1..2048 && b.position() <= bytes.size-4 - n*4)
            val pulses=List(n){b.int}
            val frame=IrFrame(carrier,duty,gap,pulses)
            frame.validate()
            results.add(captureId to frame)
        }
        require(b.position()==bytes.size-4) {"Trailing data"}
        return results
    }

    @Synchronized fun record(button: Int, captureId: Long, frame: IrFrame): Boolean {
        require(captureId in 1..0xFFFFFFFFL)
        frame.validate()
        val existing=pending(button)
        val previous=existing.find{it.first==captureId}
        if (previous!=null) {
            require(previous.second==frame) {"Conflicting capture ID"}
            return false // already committed to staging file; safe to re-ACK
        }
        require(existing.size<5) {"Five frames already staged"}
        val updated=existing + (captureId to frame)
        val raw=ByteArrayOutputStream()
        DataOutputStream(raw).use{out->
            out.writeInt(0x47574349)
            out.writeInt(1)
            out.writeInt(button)
            out.writeInt(updated.size)
            for ((id,f) in updated) {
                out.writeInt(id.toInt())
                out.write(GwirV3.txPayload(f))
            }
        }
        val data=raw.toByteArray()
        val crc=CRC32().apply{update(data)}.value.toInt()
        val target=path(button)
        val temp=File.createTempFile("gwci_",".tmp",directory)
        try {
            FileOutputStream(temp).use{out->
                out.write(data)
                DataOutputStream(out).writeInt(crc)
                out.fd.sync()
            }
            try {
                Files.move(temp.toPath(),target.toPath(),StandardCopyOption.ATOMIC_MOVE,
                    StandardCopyOption.REPLACE_EXISTING)
            } catch (_:java.nio.file.AtomicMoveNotSupportedException) {
                Files.move(temp.toPath(),target.toPath(),StandardCopyOption.REPLACE_EXISTING)
            }
        } finally {temp.delete()}
        require(pending(button)==updated) {"Capture staging integrity check failed"}
        return true
    }

    /** Safe to repeat after a crash: one staging file, never partial deletion. */
    @Synchronized fun commit(button: Int, codes: AndroidCodeStore) {
        val staged=pending(button)
        require(staged.isNotEmpty())
        codes.saveButton(button,staged.map{it.second})
        require(path(button).delete()) {"Cannot remove staged file"}
    }
}

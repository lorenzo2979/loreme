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
 * Durable 96-button store. Pass Android context.filesDir as `directory`.
 * This is a platform-neutral storage component; no graphical app / APK here.
 * Only call saveButton when a capture is complete (1-5 frames).
 */
class AndroidCodeStore(private val directory: File) {
    init { require(directory.isDirectory || directory.mkdirs()) }
    private fun path(button: Int): File {
        require(button in 0 until 96)
        return File(directory, "ir_button_%02d.gwcd".format(button))
    }

    @Synchronized fun saveButton(button: Int, frames: List<IrFrame>) {
        require(frames.size in 1..5)
        frames.forEach { it.validate() }
        val raw = ByteArrayOutputStream()
        DataOutputStream(raw).use { out ->
            out.writeInt(0x47574344) // GWCD
            out.writeInt(1) // storage version
            out.writeInt(button)
            out.writeInt(frames.size)
            for (f in frames) {
                out.writeInt(f.carrierHz)
                out.writeShort(f.dutyPermille)
                out.writeShort(f.pulsesUs.size)
                out.writeInt(f.gapAfterUs)
                f.pulsesUs.forEach(out::writeInt)
            }
        }
        val bytes = raw.toByteArray()
        val crc = CRC32().apply { update(bytes) }.value.toInt()
        val target = path(button)
        val tmp = File.createTempFile("gwcd_", ".tmp", directory)
        try {
            FileOutputStream(tmp).use { out ->
                out.write(bytes)
                DataOutputStream(out).writeInt(crc)
                out.fd.sync()
            }
            // Rename in same directory; leave old file intact if replacement fails.
            try {
                Files.move(tmp.toPath(), target.toPath(), StandardCopyOption.ATOMIC_MOVE,
                    StandardCopyOption.REPLACE_EXISTING)
            } catch (_: java.nio.file.AtomicMoveNotSupportedException) {
                Files.move(tmp.toPath(), target.toPath(), StandardCopyOption.REPLACE_EXISTING)
            }
        } finally { tmp.delete() }
        require(loadButton(button) == frames) { "Data integrity verification failed" }
    }

    @Synchronized fun loadButton(button: Int): List<IrFrame>? {
        val file=path(button)
        if (!file.exists()) return null
        require(file.length() in 20..45_000) { "Unexpected code file size" }
        val contents = file.readBytes()
        val crc = CRC32().apply { update(contents,0,contents.size-4) }.value
        val b = ByteBuffer.wrap(contents).order(ByteOrder.BIG_ENDIAN)
        val expected = b.getInt(contents.size-4).toLong() and 0xFFFFFFFFL
        require(crc == expected) { "Corrupted code file" }
        require(b.int == 0x47574344 && b.int == 1)
        require(b.int == button)
        val count = b.int
        require(count in 1..5)
        val results = ArrayList<IrFrame>(count)
        repeat(count) {
            require(b.remaining() >= 12+4)
            val carrier=b.int
            val duty=b.short.toInt() and 0xffff
            val n=b.short.toInt() and 0xffff
            val gap=b.int
            require(n in 1..2048 && b.position() <= contents.size-4 - n*4)
            val values=List(n) { b.int }
            val f=IrFrame(carrier,duty,gap,values)
            f.validate()
            results.add(f)
        }
        require(b.position() == contents.size-4) { "Trailing data" }
        return results
    }
}

package org.gw006.ir

import java.io.ByteArrayInputStream
import java.io.DataInputStream
import java.io.EOFException
import java.io.InputStream
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.util.zip.CRC32

/** Platform-neutral core for future Android app; does NOT imply an APK exists. */
data class IrFrame(
    val carrierHz: Int,
    val dutyPermille: Int,
    val gapAfterUs: Int,
    val pulsesUs: List<Int>
) {
    fun validate() {
        require(carrierHz in 25_000..500_000)
        require(dutyPermille in 200..500)
        require(gapAfterUs in 0..30_000_000)
        require(pulsesUs.size in 1..2048)
        require(pulsesUs.all { it in 1..30_000_000 })
    }
}

data class GwirMessage(val command: Int, val sequence: Long, val payload: ByteArray)

object GwirV3 {
    const val VERSION = 3
    const val HEADER_SIZE = 20
    const val MAX_PAYLOAD = 16 + 2048 * 4
    const val RX_READ = 5
    const val RX_DATA = 0x85
    const val RX_ACK = 8
    const val STATUS = 0x90

    fun encode(command: Int, sequence: Long, payload: ByteArray = byteArrayOf()): ByteArray {
        require(command in 0..255 && sequence in 0..0xFFFFFFFFL)
        require(payload.size <= MAX_PAYLOAD)
        val data = ByteArray(HEADER_SIZE + payload.size)
        val b = ByteBuffer.wrap(data).order(ByteOrder.BIG_ENDIAN)
        b.put(byteArrayOf('G'.code.toByte(), 'W'.code.toByte(), 'I'.code.toByte(), 'R'.code.toByte()))
        b.put(VERSION.toByte()).put(command.toByte()).putShort(0)
        b.putInt(sequence.toInt()).putInt(payload.size)
        b.putInt(0)
        b.put(payload)
        val crc = CRC32()
        crc.update(data, 0, 16)
        crc.update(data, HEADER_SIZE, payload.size)
        b.putInt(16, crc.value.toInt())
        return data
    }

    fun decode(data: ByteArray): GwirMessage {
        require(data.size >= HEADER_SIZE)
        val b = ByteBuffer.wrap(data).order(ByteOrder.BIG_ENDIAN)
        require(data[0] == 'G'.code.toByte() && data[1] == 'W'.code.toByte() &&
                data[2] == 'I'.code.toByte() && data[3] == 'R'.code.toByte())
        val ver = b.get(4).toInt() and 0xff
        require(ver == VERSION && b.get(6).toInt() == 0 && b.get(7).toInt() == 0)
        val size = b.getInt(12)
        require(size in 0..MAX_PAYLOAD && data.size == HEADER_SIZE + size)
        val crc = CRC32()
        crc.update(data, 0, 16)
        crc.update(data, HEADER_SIZE, size)
        require(crc.value == (b.getInt(16).toLong() and 0xFFFFFFFFL))
        return GwirMessage(b.get(5).toInt() and 0xff,
            b.getInt(8).toLong() and 0xFFFFFFFFL,
            data.copyOfRange(HEADER_SIZE, data.size))
    }

    /** One TCP read need not be a whole frame. Enforce lengths before allocating. */
    fun readMessage(input: InputStream): GwirMessage {
        val header = ByteArray(HEADER_SIZE)
        DataInputStream(input).readFully(header)
        val b = ByteBuffer.wrap(header).order(ByteOrder.BIG_ENDIAN)
        require(header.copyOfRange(0, 4).contentEquals("GWIR".toByteArray()))
        require((header[4].toInt() and 0xff) == VERSION && header[6].toInt() == 0 && header[7].toInt() == 0)
        val count = b.getInt(12)
        require(count in 0..MAX_PAYLOAD)
        val body = ByteArray(count)
        DataInputStream(input).readFully(body)
        return decode(header + body)
    }

    fun rxData(body: ByteArray): Pair<Long, IrFrame> {
        require(body.size in 20..MAX_PAYLOAD)
        val b = ByteBuffer.wrap(body).order(ByteOrder.BIG_ENDIAN)
        val captureId = b.int.toLong() and 0xFFFFFFFFL
        require(captureId != 0L)
        val carrier = b.int
        val duty = b.short.toInt() and 0xffff
        val n = b.short.toInt() and 0xffff
        val gap = b.int
        require(n in 1..2048 && body.size == 16 + 4*n)
        val pulses = List(n) { b.int }
        val frame = IrFrame(carrier, duty, gap, pulses)
        frame.validate()
        return captureId to frame
    }

    fun txPayload(frame: IrFrame): ByteArray {
        frame.validate()
        val b = ByteBuffer.allocate(12 + frame.pulsesUs.size * 4).order(ByteOrder.BIG_ENDIAN)
        b.putInt(frame.carrierHz).putShort(frame.dutyPermille.toShort())
        b.putShort(frame.pulsesUs.size.toShort()).putInt(frame.gapAfterUs)
        frame.pulsesUs.forEach { b.putInt(it) }
        return b.array()
    }

    fun ackPayload(captureId: Long): ByteArray {
        require(captureId in 1..0xFFFFFFFFL)
        return ByteBuffer.allocate(4).order(ByteOrder.BIG_ENDIAN).putInt(captureId.toInt()).array()
    }
}

package org.gw006.ir

import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.security.MessageDigest
import javax.crypto.Mac
import javax.crypto.spec.SecretKeySpec

/** GWAS v1: HMAC-SHA256 envelope for full GWIR v3 frames.
 *
 * This is a platform-neutral building block, NOT an Android UI, key-provisioning
 * mechanism, verified Keystore integration, or active network listener.
 * Never ship static secrets. Nonce must come from a secure hardware RNG on each
 * target reboot, and the peer must learn it via the future pairing/handshake.
 */
class GwirAuth(secret: ByteArray, bootNonce: ByteArray) : AutoCloseable {
    companion object {
        const val TO_DEVICE = 1
        const val FROM_DEVICE = 2
        const val HEADER = 36
        const val TAG = 32
        const val MAX_RECORD = HEADER + GwirV3.HEADER_SIZE + GwirV3.MAX_PAYLOAD + TAG
    }
    private val secretCopy = secret.copyOf()
    private val nonceCopy = bootNonce.copyOf()
    private var active = true
    private var txCounter = 0L
    private var lastRxCounter = 0L

    init {
        require(secret.size == 32 && secret.any { it != 0.toByte() })
        require(bootNonce.size == 16 && bootNonce.any { it != 0.toByte() })
    }
    private fun checkOpen() { check(active) { "Authentication context closed" } }
    private fun checkDirection(direction: Int) { require(direction == TO_DEVICE || direction == FROM_DEVICE) }
    private fun hmac(bytes: ByteArray, count: Int): ByteArray {
        val mac = Mac.getInstance("HmacSHA256")
        mac.init(SecretKeySpec(secretCopy, "HmacSHA256"))
        mac.update(bytes, 0, count)
        return mac.doFinal()
    }
    fun wrap(direction: Int, inner: ByteArray): ByteArray {
        checkOpen()
        checkDirection(direction)
        require(txCounter < Long.MAX_VALUE)
        require(inner.size in GwirV3.HEADER_SIZE..(GwirV3.HEADER_SIZE + GwirV3.MAX_PAYLOAD))
        GwirV3.decode(inner) // reject even correctly authenticated malformed CRC
        val counter = txCounter + 1
        val result = ByteBuffer.allocate(HEADER + inner.size + TAG).order(ByteOrder.BIG_ENDIAN)
        result.put(byteArrayOf('G'.code.toByte(), 'W'.code.toByte(), 'A'.code.toByte(), 'S'.code.toByte()))
        result.put(1).put(direction.toByte()).putShort(0)
        result.put(nonceCopy).putLong(counter).putInt(inner.size).put(inner)
        val signed = HEADER + inner.size
        val hash = hmac(result.array(), signed)
        require(hash.size == TAG)
        result.put(hash)
        txCounter = counter
        return result.array()
    }
    fun unwrap(expectedDirection: Int, record: ByteArray): ByteArray {
        checkOpen()
        checkDirection(expectedDirection)
        require(record.size in (HEADER + GwirV3.HEADER_SIZE + TAG)..MAX_RECORD)
        require(record[0] == 'G'.code.toByte() && record[1] == 'W'.code.toByte() &&
                record[2] == 'A'.code.toByte() && record[3] == 'S'.code.toByte())
        require(record[4] == 1.toByte() && (record[5].toInt() and 0xff) == expectedDirection)
        require(record[6] == 0.toByte() && record[7] == 0.toByte())
        require(MessageDigest.isEqual(nonceCopy, record.copyOfRange(8, 24)))
        val buf = ByteBuffer.wrap(record).order(ByteOrder.BIG_ENDIAN)
        val counter = buf.getLong(24)
        require(counter > 0 && counter > lastRxCounter) { "replay or invalid counter" }
        val size = buf.getInt(32)
        require(size in GwirV3.HEADER_SIZE..(GwirV3.HEADER_SIZE + GwirV3.MAX_PAYLOAD))
        require(record.size == HEADER + size + TAG)
        val signed = HEADER + size
        require(MessageDigest.isEqual(hmac(record, signed),record.copyOfRange(signed, record.size))) {
            "invalid authentication tag"
        }
        val inner = record.copyOfRange(HEADER, signed)
        GwirV3.decode(inner) // also require valid inner frame/CRC
        lastRxCounter = counter
        return inner
    }
    override fun close() {
        secretCopy.fill(0)
        nonceCopy.fill(0)
        txCounter = 0
        lastRxCounter = 0
        active = false
    }
}

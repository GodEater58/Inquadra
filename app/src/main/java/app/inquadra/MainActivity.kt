package app.inquadra

import android.Manifest
import android.animation.ObjectAnimator
import android.animation.ValueAnimator
import android.content.pm.PackageManager
import android.content.res.ColorStateList
import android.graphics.Bitmap
import android.graphics.Outline
import android.graphics.RectF
import android.net.ConnectivityManager
import android.net.NetworkCapabilities
import android.os.Build
import android.os.Bundle
import android.os.SystemClock
import android.view.View
import android.view.ViewOutlineProvider
import android.widget.TextView
import androidx.activity.result.contract.ActivityResultContracts
import androidx.appcompat.app.AppCompatActivity
import androidx.camera.core.CameraSelector
import androidx.camera.core.Preview
import androidx.camera.lifecycle.ProcessCameraProvider
import androidx.camera.view.PreviewView
import androidx.core.content.ContextCompat
import androidx.core.view.isVisible
import androidx.lifecycle.lifecycleScope
import com.google.android.material.button.MaterialButton
import com.google.android.material.progressindicator.LinearProgressIndicator
import com.google.mlkit.genai.common.DownloadStatus
import com.google.mlkit.genai.common.FeatureStatus
import com.google.mlkit.genai.prompt.Generation
import com.google.mlkit.genai.prompt.GenerativeModel
import com.google.mlkit.genai.prompt.ImagePart
import com.google.mlkit.genai.prompt.TextPart
import com.google.mlkit.genai.prompt.generateContentRequest
import kotlinx.coroutines.Job
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch
import org.json.JSONObject
import java.util.Locale

/**
 * FASE 1 — Test di Gemini Nano: scarica il modello, guarda la fotocamera
 * e propone un'inquadratura.
 */
class MainActivity : AppCompatActivity() {

    // UI
    private lateinit var preview: PreviewView
    private lateinit var overlay: OverlayView
    private lateinit var hint: TextView
    private lateinit var pillDot: View
    private lateinit var pillText: TextView
    private lateinit var scanBar: LinearProgressIndicator
    private lateinit var btnScan: View
    private lateinit var scanIcon: TextView
    private lateinit var btnText: TextView
    private lateinit var logPanel: View
    private lateinit var result: TextView
    // Scheda download
    private lateinit var dlCard: View
    private lateinit var dlTitle: TextView
    private lateinit var dlBar: LinearProgressIndicator
    private lateinit var dlBytes: TextView
    private lateinit var dlPercent: TextView
    private lateinit var dlSpeed: TextView
    private lateinit var dlWarn: TextView
    private lateinit var btnDownload: MaterialButton

    private val model: GenerativeModel by lazy { Generation.getClient() }
    private val prefs by lazy { getSharedPreferences("inquadra", MODE_PRIVATE) }

    // Stato download
    private var downloading = false
    private var dlStartMs = 0L
    private var bytesDone = 0L
    private var bytesTotal = 0L
    private var lastBytes = 0L
    private var lastBytesMs = 0L
    private var speedBps = 0.0
    private var lastEventMs = 0L
    private var ticker: Job? = null
    private var dotPulse: ObjectAnimator? = null

    private val cameraPermission =
        registerForActivityResult(ActivityResultContracts.RequestPermission()) { ok ->
            if (ok) startCamera() else setHint("Serve il permesso fotocamera")
        }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        bindViews()

        // Mirino con angoli arrotondati
        findViewById<View>(R.id.viewfinder).apply {
            outlineProvider = object : ViewOutlineProvider() {
                override fun getOutline(v: View, o: Outline) =
                    o.setRoundRect(0, 0, v.width, v.height, 22f * resources.displayMetrics.density)
            }
            clipToOutline = true
        }

        btnDownload.setOnClickListener { startDownload() }
        btnScan.setOnClickListener { analyzeScene() }
        btnText.setOnClickListener { textTest() }
        findViewById<View>(R.id.btnLog).setOnClickListener { logPanel.isVisible = !logPanel.isVisible }
        findViewById<View>(R.id.btnCloseLog).setOnClickListener { logPanel.isVisible = false }

        bytesTotal = prefs.getLong("bytesTotal", 0L)

        runCatching {
            val f = java.io.File(filesDir, "last_crash.txt")
            if (f.exists()) {
                log("⚠️ CRASH PRECEDENTE:\n" + f.readText()); f.delete(); logPanel.isVisible = true
            }
        }
        log("Dispositivo: ${Build.MANUFACTURER} ${Build.MODEL} — Android ${Build.VERSION.RELEASE}")

        if (ContextCompat.checkSelfPermission(this, Manifest.permission.CAMERA)
            == PackageManager.PERMISSION_GRANTED
        ) startCamera() else cameraPermission.launch(Manifest.permission.CAMERA)

        checkModel()
    }

    private fun bindViews() {
        preview = findViewById(R.id.preview)
        overlay = findViewById(R.id.overlay)
        hint = findViewById(R.id.hint)
        pillDot = findViewById(R.id.pillDot)
        pillText = findViewById(R.id.pillText)
        scanBar = findViewById(R.id.scanBar)
        btnScan = findViewById(R.id.btnScan)
        scanIcon = findViewById(R.id.scanIcon)
        btnText = findViewById(R.id.btnText)
        logPanel = findViewById(R.id.logPanel)
        result = findViewById(R.id.result)
        dlCard = findViewById(R.id.dlCard)
        dlTitle = findViewById(R.id.dlTitle)
        dlBar = findViewById(R.id.dlBar)
        dlBytes = findViewById(R.id.dlBytes)
        dlPercent = findViewById(R.id.dlPercent)
        dlSpeed = findViewById(R.id.dlSpeed)
        dlWarn = findViewById(R.id.dlWarn)
        btnDownload = findViewById(R.id.btnDownload)
    }

    // ---------- Fotocamera ----------

    private fun startCamera() {
        val future = ProcessCameraProvider.getInstance(this)
        future.addListener({
            try {
                val provider = future.get()
                val p = Preview.Builder().build().also { it.setSurfaceProvider(preview.surfaceProvider) }
                provider.unbindAll()
                provider.bindToLifecycle(this, CameraSelector.DEFAULT_BACK_CAMERA, p)
            } catch (e: Throwable) {
                log("Fotocamera errore: $e")
            }
        }, ContextCompat.getMainExecutor(this))
    }

    // ---------- Stato modello ----------

    private enum class Pill { CHECK, DOWNLOAD, READY, ERROR }

    private fun setPill(state: Pill, text: String) {
        val color = when (state) {
            Pill.CHECK -> R.color.muted
            Pill.DOWNLOAD -> R.color.warn
            Pill.READY -> R.color.ok
            Pill.ERROR -> R.color.err
        }
        pillDot.backgroundTintList = ColorStateList.valueOf(ContextCompat.getColor(this, color))
        pillText.text = text
        dotPulse?.cancel()
        pillDot.alpha = 1f
        if (state == Pill.DOWNLOAD || state == Pill.CHECK) {
            dotPulse = ObjectAnimator.ofFloat(pillDot, View.ALPHA, 1f, 0.2f).apply {
                duration = 700
                repeatMode = ValueAnimator.REVERSE
                repeatCount = ValueAnimator.INFINITE
                start()
            }
        }
    }

    private fun checkModel() = lifecycleScope.launch {
        setPill(Pill.CHECK, "Controllo…")
        try {
            when (val s = model.checkStatus()) {
                FeatureStatus.AVAILABLE -> ready()
                FeatureStatus.DOWNLOADABLE -> {
                    setPill(Pill.DOWNLOAD, "Da scaricare")
                    showDownloadCard(idle = true)
                    setHint("Scarica il modello per iniziare")
                }
                FeatureStatus.DOWNLOADING -> startDownload()
                FeatureStatus.UNAVAILABLE -> {
                    setPill(Pill.ERROR, "Non disponibile")
                    setHint("Gemini Nano non è disponibile su questo telefono")
                }
                else -> log("Stato sconosciuto: $s")
            }
            runCatching { log("Modello: ${model.getBaseModelName()} — limite token: ${model.getTokenLimit()}") }
        } catch (e: Throwable) {
            setPill(Pill.ERROR, "Errore")
            setHint("Errore controllo modello (vedi registro)")
            log("checkStatus: $e")
        }
    }

    // ---------- Download ----------

    private fun showDownloadCard(idle: Boolean) {
        dlCard.isVisible = true
        btnDownload.isVisible = idle
        dlBar.isVisible = !idle
        dlBytes.isVisible = !idle
        dlPercent.isVisible = !idle
        dlSpeed.isVisible = !idle
        dlTitle.text = if (idle) "Scarica Gemini Nano" else "Download Gemini Nano"
        updateNetworkWarning()
    }

    private fun startDownload() {
        if (downloading) return
        downloading = true
        dlStartMs = SystemClock.elapsedRealtime()
        lastBytesMs = dlStartMs
        lastEventMs = dlStartMs
        setPill(Pill.DOWNLOAD, "Download")
        setHint("Download del modello in corso…")
        showDownloadCard(idle = false)
        startTicker()

        lifecycleScope.launch {
            try {
                model.download().collect { d ->
                    lastEventMs = SystemClock.elapsedRealtime()
                    when (d) {
                        is DownloadStatus.DownloadStarted -> {
                            if (d.bytesToDownload > 0) {
                                bytesTotal = d.bytesToDownload
                                prefs.edit().putLong("bytesTotal", bytesTotal).apply()
                            }
                            log("Download avviato: ${fmtBytes(bytesTotal)}")
                        }
                        is DownloadStatus.DownloadProgress -> onBytes(d.totalBytesDownloaded)
                        is DownloadStatus.DownloadCompleted -> ready()
                        is DownloadStatus.DownloadFailed -> downloadError(d.e.message ?: "errore")
                        else -> {}
                    }
                }
            } catch (e: Throwable) {
                log("download(): $e")
                // Non è fatale: AICore può continuare in background, il ticker ricontrolla.
            }
        }
    }

    private fun onBytes(done: Long) {
        val now = SystemClock.elapsedRealtime()
        val dt = (now - lastBytesMs) / 1000.0
        if (dt >= 1.0) {
            val inst = (done - lastBytes) / dt
            speedBps = if (speedBps == 0.0) inst else speedBps * 0.7 + inst * 0.3
            lastBytes = done
            lastBytesMs = now
        }
        bytesDone = done
        renderDownload()
    }

    private fun renderDownload() {
        val elapsed = (SystemClock.elapsedRealtime() - dlStartMs) / 1000
        val silent = (SystemClock.elapsedRealtime() - lastEventMs) / 1000
        if (bytesTotal > 0 && bytesDone > 0) {
            val pct = (bytesDone * 100 / bytesTotal).toInt().coerceIn(0, 100)
            if (dlBar.isIndeterminate) {
                dlBar.isVisible = false; dlBar.isIndeterminate = false; dlBar.isVisible = true
            }
            dlBar.setProgressCompat(pct, true)
            dlPercent.text = "$pct%"
            dlBytes.text = "${fmtBytes(bytesDone)} / ${fmtBytes(bytesTotal)}"
        } else {
            dlPercent.text = ""
            dlBytes.text = if (bytesDone > 0) "${fmtBytes(bytesDone)} scaricati"
            else "In attesa che il sistema avvii il download…"
        }
        val parts = mutableListOf("⏱ ${fmtTime(elapsed)}")
        if (speedBps > 0 && silent < 10) {
            parts += "${fmtBytes(speedBps.toLong())}/s"
            if (bytesTotal > bytesDone) parts += "mancano ~${fmtTime(((bytesTotal - bytesDone) / speedBps).toLong())}"
        } else if (silent >= 15) {
            parts += "nessun dato da ${silent}s"
        }
        dlSpeed.text = parts.joinToString("  ·  ")
    }

    /** Ogni secondo aggiorna la scheda; ogni 5 s chiede ad AICore lo stato reale. */
    private fun startTicker() {
        ticker?.cancel()
        ticker = lifecycleScope.launch {
            var n = 0
            while (downloading) {
                renderDownload()
                updateNetworkWarning()
                if (n % 5 == 4) {
                    val s = runCatching { model.checkStatus() }.getOrNull()
                    if (s == FeatureStatus.AVAILABLE) { ready(); break }
                }
                n++
                delay(1000)
            }
        }
    }

    private fun downloadError(msg: String) {
        downloading = false
        setPill(Pill.ERROR, "Errore")
        showDownloadCard(idle = true)
        btnDownload.text = "Riprova"
        setHint("Download non riuscito")
        log("Download fallito: $msg")
    }

    private fun updateNetworkWarning() {
        val cm = getSystemService(ConnectivityManager::class.java)
        val caps = cm?.getNetworkCapabilities(cm.activeNetwork)
        val msg = when {
            caps == null -> "⚠️ Nessuna connessione a internet"
            !caps.hasTransport(NetworkCapabilities.TRANSPORT_WIFI) ->
                "⚠️ Sei su rete mobile: il modello potrebbe scaricarsi solo in Wi-Fi"
            else -> null
        }
        dlWarn.isVisible = msg != null
        dlWarn.text = msg ?: ""
    }

    private fun ready() {
        downloading = false
        ticker?.cancel()
        dlCard.isVisible = false
        setPill(Pill.READY, "Nano pronto")
        setHint("Inquadra e tocca ✨")
        btnScan.isEnabled = true
        btnText.isEnabled = true
    }

    // ---------- Test ----------

    private fun textTest() = lifecycleScope.launch {
        btnText.isEnabled = false
        setHint("Test testo in corso…")
        val t0 = SystemClock.elapsedRealtime()
        try {
            val r = model.generateContent("Rispondi in una frase: cos'è la regola dei terzi in fotografia?")
            val ms = SystemClock.elapsedRealtime() - t0
            log("TESTO OK ($ms ms):\n${r.candidates.firstOrNull()?.text}")
            setHint("Test testo OK in $ms ms — apri ≡")
        } catch (e: Throwable) {
            log("TESTO ERRORE: ${errInfo(e)}")
            setHint("Test testo fallito — apri ≡")
        }
        btnText.isEnabled = true
    }

    private var scanJob: Job? = null

    private fun analyzeScene() {
        // Secondo tocco = annulla
        scanJob?.let { if (it.isActive) { it.cancel(); return } }
        scanJob = lifecycleScope.launch { runScan() }
    }

    private suspend fun runScan() {
        val frame = preview.bitmap ?: run { log("Nessun fotogramma dalla fotocamera."); return }
        overlay.show(null, "")
        scanBar.isVisible = true
        val spin = ObjectAnimator.ofFloat(scanIcon, View.ROTATION, 0f, 360f).apply {
            duration = 1200; repeatCount = ValueAnimator.INFINITE; start()
        }
        val small = scaleDown(frame, 512)
        log("Immagine inviata: ${small.width}×${small.height}")
        val prompt = """
            You are a professional photographer looking through a phone camera.
            Find the single most photogenic subject in this image and the best crop to photograph it.
            Reply ONLY with JSON, no other text:
            {"subject":"short poetic description, max 4 words",
             "box":[left,top,right,bottom],
             "zoom":1.0,
             "tip":"one short composition tip in Italian"}
            box values are integers 0-1000 relative to image width/height.
            zoom is how much to zoom in (1.0 to 5.0) so the crop fills the frame.
        """.trimIndent()

        val t0 = SystemClock.elapsedRealtime()
        var firstTokenMs = -1L
        val sb = StringBuilder()
        // Timer visibile
        val timer = lifecycleScope.launch {
            while (true) {
                val sec = (SystemClock.elapsedRealtime() - t0) / 1000
                hint.text = if (firstTokenMs < 0) "Analizzo la scena… ${sec}s  (tocca ✨ per annullare)"
                else "Il modello sta scrivendo… ${sec}s"
                delay(500)
            }
        }
        try {
            val request = generateContentRequest(ImagePart(small), TextPart(prompt)) {
                temperature = 0.2f
                topK = 10
                candidateCount = 1
                maxOutputTokens = 200
            }
            val done = kotlinx.coroutines.withTimeoutOrNull(120_000) {
                model.generateContentStream(request).collect { chunk ->
                    if (firstTokenMs < 0) {
                        firstTokenMs = SystemClock.elapsedRealtime() - t0
                        log("Primo token dopo $firstTokenMs ms")
                    }
                    sb.append(chunk.candidates.firstOrNull()?.text.orEmpty())
                }
                true
            }
            timer.cancel()
            val ms = SystemClock.elapsedRealtime() - t0
            if (done == null) {
                log("TIMEOUT dopo 120 s. Testo ricevuto: \"$sb\"")
                setHint("Nessuna risposta in 2 minuti — apri ≡")
            } else {
                log("IMMAGINE OK ($ms ms, primo token $firstTokenMs ms):\n$sb")
                parseAndShow(sb.toString(), ms)
            }
        } catch (e: kotlinx.coroutines.CancellationException) {
            timer.cancel()
            log("Analisi annullata dopo ${(SystemClock.elapsedRealtime() - t0) / 1000}s. Testo: \"$sb\"")
            setHint("Annullato")
        } catch (e: Throwable) {
            timer.cancel()
            setHint("Analisi non riuscita — apri ≡")
            log("IMMAGINE ERRORE: ${errInfo(e)}")
        } finally {
            spin.cancel(); scanIcon.rotation = 0f
            scanBar.isVisible = false
        }
    }

    private fun parseAndShow(raw: String, ms: Long) {
        try {
            val json = JSONObject(raw.substring(raw.indexOf('{'), raw.lastIndexOf('}') + 1))
            val subject = json.optString("subject")
            val tip = json.optString("tip")
            val zoom = json.optDouble("zoom", 1.0)
            val box = json.optJSONArray("box")
            val rect = if (box != null && box.length() == 4) RectF(
                (box.getDouble(0) / 1000).toFloat().coerceIn(0f, 1f),
                (box.getDouble(1) / 1000).toFloat().coerceIn(0f, 1f),
                (box.getDouble(2) / 1000).toFloat().coerceIn(0f, 1f),
                (box.getDouble(3) / 1000).toFloat().coerceIn(0f, 1f),
            ) else null
            overlay.show(rect, subject)
            setHint("${String.format(Locale.ITALY, "%.1f", zoom)}× · $tip")
        } catch (e: Exception) {
            setHint("Risposta in $ms ms ma non valida — apri ≡")
        }
    }

    // ---------- Utility ----------

    private fun setHint(text: String) {
        hint.text = text
        hint.alpha = 0f
        hint.animate().alpha(1f).setDuration(200).start()
    }

    private fun scaleDown(b: Bitmap, max: Int): Bitmap {
        val s = max.toFloat() / maxOf(b.width, b.height)
        if (s >= 1f) return b
        return Bitmap.createScaledBitmap(b, (b.width * s).toInt(), (b.height * s).toInt(), true)
    }

    private fun fmtBytes(b: Long): String = when {
        b >= 1_000_000_000 -> String.format(Locale.ITALY, "%.2f GB", b / 1e9)
        b >= 1_000_000 -> String.format(Locale.ITALY, "%.0f MB", b / 1e6)
        b >= 1_000 -> String.format(Locale.ITALY, "%.0f KB", b / 1e3)
        else -> "$b B"
    }

    private fun fmtTime(s: Long): String =
        if (s >= 3600) String.format(Locale.ITALY, "%d:%02d:%02d", s / 3600, (s % 3600) / 60, s % 60)
        else String.format(Locale.ITALY, "%d:%02d", s / 60, s % 60)

    private fun errInfo(e: Throwable): String {
        val code = (e as? com.google.mlkit.genai.common.GenAiException)?.errorCode
        val sb = StringBuilder("$e")
        if (code != null) sb.append("\n  codice errore: $code")
        var c = e.cause
        while (c != null) { sb.append("\n  causa: $c"); c = c.cause }
        return sb.toString()
    }

    private fun log(msg: String) {
        result.text = if (result.text.isEmpty()) msg else "$msg\n\n${result.text}"
    }

    override fun onDestroy() {
        super.onDestroy()
        runCatching { model.close() }
    }
}

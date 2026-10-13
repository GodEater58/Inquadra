package app.inquadra

import android.app.Application
import java.io.File

/** Salva l'ultimo crash su file, così l'app lo mostra al riavvio. */
class App : Application() {
    override fun onCreate() {
        super.onCreate()
        val previous = Thread.getDefaultUncaughtExceptionHandler()
        Thread.setDefaultUncaughtExceptionHandler { t, e ->
            runCatching {
                File(filesDir, "last_crash.txt").writeText(
                    "Thread: ${t.name}\n" + android.util.Log.getStackTraceString(e).take(4000)
                )
            }
            previous?.uncaughtException(t, e)
        }
    }
}

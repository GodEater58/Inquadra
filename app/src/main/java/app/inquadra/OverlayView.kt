package app.inquadra

import android.animation.ValueAnimator
import android.content.Context
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
import android.graphics.RectF
import android.util.AttributeSet
import android.view.View

/** Griglia dei terzi + riquadro giallo suggerito con etichetta. */
class OverlayView(context: Context, attrs: AttributeSet?) : View(context, attrs) {

    private var box: RectF? = null
    private var label: String = ""
    private var appear = 1f
    private val d = resources.displayMetrics.density

    private val gridPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        color = Color.argb(70, 255, 255, 255)
        strokeWidth = 1f * d
    }
    private val boxPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        color = Color.parseColor("#FFD60A")
        style = Paint.Style.STROKE
        strokeWidth = 3f * d
    }
    private val dimPaint = Paint().apply { color = Color.argb(90, 0, 0, 0) }
    private val labelBg = Paint(Paint.ANTI_ALIAS_FLAG).apply { color = Color.argb(170, 0, 0, 0) }
    private val textPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        color = Color.parseColor("#FFD60A")
        textSize = 13f * d
        isFakeBoldText = true
    }

    fun show(normBox: RectF?, text: String) {
        box = normBox
        label = text
        if (normBox != null) {
            ValueAnimator.ofFloat(0f, 1f).apply {
                duration = 350
                addUpdateListener { appear = it.animatedValue as Float; invalidate() }
                start()
            }
        } else invalidate()
    }

    override fun onDraw(canvas: Canvas) {
        super.onDraw(canvas)
        val w = width.toFloat(); val h = height.toFloat()
        // griglia dei terzi
        for (i in 1..2) {
            canvas.drawLine(w * i / 3, 0f, w * i / 3, h, gridPaint)
            canvas.drawLine(0f, h * i / 3, w, h * i / 3, gridPaint)
        }
        val b = box ?: return
        val r = RectF(b.left * w, b.top * h, b.right * w, b.bottom * h)
        // scurisce leggermente fuori dal riquadro
        dimPaint.alpha = (90 * appear).toInt()
        canvas.drawRect(0f, 0f, w, r.top, dimPaint)
        canvas.drawRect(0f, r.bottom, w, h, dimPaint)
        canvas.drawRect(0f, r.top, r.left, r.bottom, dimPaint)
        canvas.drawRect(r.right, r.top, w, r.bottom, dimPaint)
        // riquadro con leggero "zoom in" all'apparizione
        val inset = (1f - appear) * 24f * d
        val rr = RectF(r.left - inset, r.top - inset, r.right + inset, r.bottom + inset)
        boxPaint.alpha = (255 * appear).toInt()
        canvas.drawRoundRect(rr, 14f * d, 14f * d, boxPaint)
        if (label.isNotEmpty()) {
            val pad = 8f * d
            val tw = textPaint.measureText(label)
            val lx = r.left + pad
            val ly = r.bottom - pad
            canvas.drawRoundRect(
                lx - pad / 2, ly - textPaint.textSize - pad / 2,
                lx + tw + pad / 2, ly + pad / 1.5f, 8f * d, 8f * d, labelBg
            )
            canvas.drawText(label, lx, ly, textPaint)
        }
    }
}

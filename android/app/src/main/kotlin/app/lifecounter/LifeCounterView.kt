package app.lifecounter

import android.content.Context
import android.graphics.Canvas
import android.graphics.LinearGradient
import android.graphics.Paint
import android.graphics.Path
import android.graphics.RectF
import android.graphics.Shader
import android.graphics.Typeface
import android.os.SystemClock
import android.view.MotionEvent
import android.view.View
import life.application.LifeAction
import life.application.LifeCommand
import life.application.LifeEngine
import life.domain.PLAYER_COUNT
import life.ui.LayoutConfig
import life.ui.Orientation
import life.ui.Palette
import life.ui.Point
import life.ui.QuadLayout
import life.ui.Rect
import life.ui.ResetConfirmation
import life.ui.TapInput

// 4人用ライフカウンターの画面。状態は LifeEngine、判定は QuadLayout / ResetConfirmation に任せ、
// ここは「描く」と「指の位置をコマンドへ渡す」だけを行う。
// 判定領域（QuadLayout）は隙間なく並べ、見た目だけカード状に余白と角丸を付ける。
class LifeCounterView(context: Context) : View(context) {
    private val engine = LifeEngine()
    private val confirmation = ResetConfirmation()
    private val tapInput = TapInput(confirmation)
    private val palette = Palette.DEFAULT
    private var layout: QuadLayout? = null

    private val dp = resources.displayMetrics.density
    private fun argb(rgb: Int, alpha: Float = 1f): Int =
        ((alpha * 255f).toInt() shl 24) or (rgb and 0xFFFFFF)

    private val paint = Paint(Paint.ANTI_ALIAS_FLAG)
    private val cardRects = Array(PLAYER_COUNT) { RectF() }
    private val cardShaders = arrayOfNulls<Shader>(PLAYER_COUNT)
    private val path = Path()
    private val arc = RectF()

    private val valueFace = Typeface.create(Typeface.DEFAULT, 600, false)
    private val labelFace = Typeface.create(Typeface.DEFAULT, 700, false)

    // 押した瞬間の光り。短時間で消える。
    private var flash: LifeCommand? = null
    private var flashStartMs = 0L

    private val expireRunnable = Runnable { expirePending() }

    override fun onSizeChanged(w: Int, h: Int, oldw: Int, oldh: Int) {
        val l = if (w > 0 && h > 0) QuadLayout(LayoutConfig.forScreen(w, h)) else null
        layout = l
        if (l == null) return
        val gap = CARD_GAP_DP * dp
        for (id in 0 until PLAYER_COUNT) {
            val p = l.panel(id)
            val card = cardRects[id]
            card.set(p.x + gap, p.y + gap, p.right - gap, p.bottom - gap)
            // プレイヤー自身から見て上が明るく、下が暗い。上段は180°回転しているので向きを逆にする。
            val light = argb(palette.panelLight[id])
            val dark = argb(palette.panelDark[id])
            cardShaders[id] = if (l.orientationOf(id) == Orientation.UpsideDown) {
                LinearGradient(0f, card.bottom, 0f, card.top, light, dark, Shader.TileMode.CLAMP)
            } else {
                LinearGradient(0f, card.top, 0f, card.bottom, light, dark, Shader.TileMode.CLAMP)
            }
        }
    }

    // 指ごとに独立して1回だけ発火する。押しっぱなしでの連続加算はしない（DOWN系のみ発火）。
    // 4人が同時に別領域を押しても、それぞれ処理される。
    // リセットの2回目は、1回目の指が離れてからのみ受け付ける（TapInput）。
    override fun onTouchEvent(event: MotionEvent): Boolean {
        val l = layout ?: return false
        when (event.actionMasked) {
            MotionEvent.ACTION_DOWN, MotionEvent.ACTION_POINTER_DOWN -> {
                val index = event.actionIndex
                val command = l.hitTest(Point(event.getX(index).toInt(), event.getY(index).toInt())) ?: return true
                val confirmed = tapInput.pointerDown(event.getPointerId(index), command, event.eventTime)
                if (confirmed != null) engine.apply(confirmed)
                flash = command
                flashStartMs = SystemClock.uptimeMillis()
                // 値の変化・リセット待機の開始/解除のいずれも再描画で反映する。
                invalidate()
                scheduleExpiry()
            }
            MotionEvent.ACTION_UP, MotionEvent.ACTION_POINTER_UP ->
                tapInput.pointerUp(event.getPointerId(event.actionIndex))
            MotionEvent.ACTION_CANCEL -> tapInput.cancelAll()
        }
        return true
    }

    // どの指が残っているか分からなくなるため、フォーカスを失ったら全て離した扱いにする。
    override fun onWindowFocusChanged(hasWindowFocus: Boolean) {
        super.onWindowFocusChanged(hasWindowFocus)
        if (!hasWindowFocus) tapInput.cancelAll()
    }

    // 触られなくても猶予切れで待機表示を戻す。待機中のプレイヤーのうち最も早い期限に更新を予約し、
    // 実行後に残りを再予約する（他人の操作で自分の期限が延びないようにする）。
    private fun scheduleExpiry() {
        removeCallbacks(expireRunnable)
        val next = confirmation.nextExpiryMs() ?: return
        postDelayed(expireRunnable, maxOf(next - SystemClock.uptimeMillis(), 0L))
    }

    private fun expirePending() {
        confirmation.tick(SystemClock.uptimeMillis())
        invalidate()
        scheduleExpiry()
    }

    override fun onDetachedFromWindow() {
        removeCallbacks(expireRunnable)
        super.onDetachedFromWindow()
    }

    override fun onDraw(canvas: Canvas) {
        val l = layout ?: return
        canvas.drawColor(argb(palette.background))
        val values = engine.snapshot()
        val now = SystemClock.uptimeMillis()
        confirmation.tick(now)  // 描画が期限切れの待機色を出さないようにする
        for (id in 0 until PLAYER_COUNT) drawPlayer(canvas, l, id, values[id], now)
        if (flash != null && now - flashStartMs < FLASH_MS) postInvalidateOnAnimation()
    }

    // 押した直後 1.0 → 0.0。対象の操作でなければ 0。
    private fun flashLevel(id: Int, action: LifeAction, now: Long): Float {
        val f = flash ?: return 0f
        if (f.playerId != id || f.action != action) return 0f
        return (1f - (now - flashStartMs).toFloat() / FLASH_MS).coerceIn(0f, 1f)
    }

    private fun drawPlayer(canvas: Canvas, l: QuadLayout, id: Int, value: Int, now: Long) {
        val upsideDown = l.orientationOf(id) == Orientation.UpsideDown
        val card = cardRects[id]

        paint.reset(); paint.isAntiAlias = true
        paint.shader = cardShaders[id]
        canvas.drawRoundRect(card, CARD_RADIUS_DP * dp, CARD_RADIUS_DP * dp, paint)
        paint.shader = null

        drawStepButton(canvas, l.incrementButton(id), plus = true, flashLevel(id, LifeAction.Increment, now))
        drawStepButton(canvas, l.decrementButton(id), plus = false, flashLevel(id, LifeAction.Decrement, now))
        drawReset(canvas, l.resetButton(id), upsideDown, confirmation.isPending(id), flashLevel(id, LifeAction.Reset, now))
        drawText(canvas, l.labelArea(id), "P${id + 1}", upsideDown, labelFace, 0.40f, 0.8f, 0.70f, 0.08f)
        drawText(canvas, l.numberArea(id), value.toString(), upsideDown, valueFace, 0.86f, 1f, 0.92f, 0f)
    }

    // ± は円形ボタン。記号は線で描くので回転しても崩れない。
    private fun drawStepButton(canvas: Canvas, r: Rect, plus: Boolean, flashLevel: Float) {
        if (r.width <= 0 || r.height <= 0) return
        val cx = r.x + r.width / 2f
        val cy = r.y + r.height / 2f
        val radius = minOf(r.width, r.height) / 2f * 0.74f

        paint.reset(); paint.isAntiAlias = true
        paint.color = argb(0xFFFFFF, 0.18f + 0.30f * flashLevel)
        canvas.drawCircle(cx, cy, radius, paint)

        paint.color = argb(palette.text)
        paint.style = Paint.Style.STROKE
        paint.strokeWidth = radius * 0.16f
        paint.strokeCap = Paint.Cap.ROUND
        val arm = radius * 0.42f
        canvas.drawLine(cx - arm, cy, cx + arm, cy, paint)
        if (plus) canvas.drawLine(cx, cy - arm, cx, cy + arm, paint)
    }

    // リセットは丸い矢印アイコンのピル。待機中（ダブルタップ1回目）は黄色になる。
    private fun drawReset(canvas: Canvas, r: Rect, upsideDown: Boolean, armed: Boolean, flashLevel: Float) {
        if (r.width <= 0 || r.height <= 0) return
        val inset = r.height * 0.12f
        val pill = RectF(r.x + inset, r.y + inset, r.right - inset, r.bottom - inset)
        val radius = pill.height() / 2f

        paint.reset(); paint.isAntiAlias = true
        paint.color = if (armed) argb(palette.resetArmed) else argb(0xFFFFFF, 0.18f + 0.30f * flashLevel)
        canvas.drawRoundRect(pill, radius, radius, paint)

        val cx = pill.centerX()
        val cy = pill.centerY()
        val ir = pill.height() * 0.24f
        paint.color = if (armed) argb(palette.onResetArmed) else argb(palette.text)
        paint.style = Paint.Style.STROKE
        paint.strokeWidth = ir * 0.28f
        paint.strokeCap = Paint.Cap.ROUND
        arc.set(cx - ir, cy - ir, cx + ir, cy + ir)
        canvas.save()
        if (upsideDown) canvas.rotate(180f, cx, cy)
        val startDeg = -50.0
        val sweepDeg = 280.0
        canvas.drawArc(arc, startDeg.toFloat(), sweepDeg.toFloat(), false, paint)
        // 矢じり: 弧の終点で進行方向（時計回りの接線）を向く三角形
        val endRad = Math.toRadians(startDeg + sweepDeg)
        val nx = Math.cos(endRad).toFloat()   // 半径方向
        val ny = Math.sin(endRad).toFloat()
        val tx = -ny                          // 進行方向（時計回り）
        val ty = nx
        val ex = cx + ir * nx
        val ey = cy + ir * ny
        val head = ir * 0.62f
        path.reset()
        path.moveTo(ex + tx * head, ey + ty * head)
        path.lineTo(ex - nx * head * 0.9f, ey - ny * head * 0.9f)
        path.lineTo(ex + nx * head * 0.9f, ey + ny * head * 0.9f)
        path.close()
        paint.style = Paint.Style.FILL
        canvas.drawPath(path, paint)
        canvas.restore()
    }

    // 領域の中央に、はみ出さない最大の大きさで描く。上行は180°回転（各プレイヤーの視点で読める向き）。
    // heightRatio は領域の高さに対する文字の最大比率、widthRatio は領域の幅に対する最大比率。
    private fun drawText(
        canvas: Canvas, r: Rect, s: String, upsideDown: Boolean, face: Typeface,
        heightRatio: Float, alpha: Float, widthRatio: Float, letterSpacing: Float,
    ) {
        if (r.width <= 0 || r.height <= 0) return
        paint.reset(); paint.isAntiAlias = true
        paint.typeface = face
        paint.textAlign = Paint.Align.CENTER
        paint.letterSpacing = letterSpacing
        paint.color = argb(palette.text, alpha)
        paint.textSize = 100f
        val fitWidth = r.width * widthRatio / paint.measureText(s) * 100f
        paint.textSize = minOf(r.height * heightRatio, fitWidth)
        val cx = r.x + r.width / 2f
        val cy = r.y + r.height / 2f
        val baseline = cy - (paint.fontMetrics.ascent + paint.fontMetrics.descent) / 2f
        canvas.save()
        if (upsideDown) canvas.rotate(180f, cx, cy)
        canvas.drawText(s, cx, baseline, paint)
        canvas.restore()
    }

    private companion object {
        const val CARD_GAP_DP = 6f      // カード間の隙間は両側で12dp
        const val CARD_RADIUS_DP = 28f
        const val FLASH_MS = 220L
    }
}

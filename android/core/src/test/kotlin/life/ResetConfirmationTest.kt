package life

import life.application.LifeAction
import life.application.LifeCommand
import life.ui.DEFAULT_RESET_WINDOW_MS
import life.ui.ResetConfirmation
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFalse
import kotlin.test.assertNotNull
import kotlin.test.assertNull
import kotlin.test.assertTrue

// リセットのダブルタップ確定（SPEC.md、2026-10-11 決定）。待機はプレイヤーごと（Android版）。
class ResetConfirmationTest {
    private fun reset(id: Int) = LifeCommand(id, LifeAction.Reset)

    @Test fun default_window_is_one_second() = assertEquals(1000L, DEFAULT_RESET_WINDOW_MS)

    @Test fun increment_and_decrement_pass_through_immediately() {
        val c = ResetConfirmation()
        assertEquals(LifeAction.Increment, c.accept(LifeCommand(0, LifeAction.Increment), 0)?.action)
        assertNotNull(c.accept(LifeCommand(0, LifeAction.Decrement), 10))
    }

    @Test fun first_reset_tap_is_held_and_second_confirms() {
        val c = ResetConfirmation()
        assertNull(c.accept(reset(2), 1000))
        assertTrue(c.isPending(2))
        val confirmed = c.accept(reset(2), 1200)
        assertEquals(reset(2), confirmed)
        assertFalse(c.isPending(2))
    }

    @Test fun second_tap_just_inside_the_window_confirms() {
        val c = ResetConfirmation()
        c.accept(reset(0), 0)
        assertNotNull(c.accept(reset(0), 999))
    }

    @Test fun second_tap_exactly_at_the_window_starts_over() {
        val c = ResetConfirmation()
        assertNull(c.accept(reset(1), 0))
        assertNull(c.accept(reset(1), 1000)) // ちょうど1000msは猶予切れ → 1回目として数え直す
        assertTrue(c.isPending(1))
        assertNotNull(c.accept(reset(1), 1700))
    }

    @Test fun second_tap_must_be_the_same_player() {
        val c = ResetConfirmation()
        assertNull(c.accept(reset(0), 0))
        assertNull(c.accept(reset(3), 100)) // 別プレイヤーのリセットでは P1 は確定しない
        assertTrue(c.isPending(0) && c.isPending(3)) // 待機は各自が独立して持つ（CYD版との差分）
        assertNotNull(c.accept(reset(0), 300))
        assertTrue(c.isPending(3))
    }

    @Test fun own_other_action_cancels_the_pending_reset() {
        val c = ResetConfirmation()
        c.accept(reset(1), 0)
        assertNotNull(c.accept(LifeCommand(1, LifeAction.Increment), 100))
        assertFalse(c.isPending(1))
        assertNull(c.accept(reset(1), 200)) // 1回目からやり直し
    }

    @Test fun other_players_action_does_not_cancel_the_pending_reset() {
        val c = ResetConfirmation()
        c.accept(reset(1), 0)
        c.accept(LifeCommand(2, LifeAction.Increment), 100) // 4人同時利用で他人の操作が入っても
        assertTrue(c.isPending(1))
        assertNotNull(c.accept(reset(1), 200))
    }

    @Test fun tick_expires_the_pending_reset() {
        val c = ResetConfirmation()
        c.accept(reset(0), 0)
        c.tick(999); assertTrue(c.isPending(0))
        c.tick(1000); assertFalse(c.isPending(0))
    }

    @Test fun invalid_player_never_becomes_pending() {
        val c = ResetConfirmation()
        assertNull(c.accept(reset(-1), 0))
        assertNull(c.accept(reset(4), 10))
        for (id in -1..4) assertFalse(c.isPending(id))
    }

    // レビュー AR02: 他人の操作で自分の期限が延びない。待機は各自が独立に満了する。
    @Test fun next_expiry_is_the_earliest_pending_deadline() {
        val c = ResetConfirmation()
        assertNull(c.nextExpiryMs())
        c.accept(reset(0), 0)
        c.accept(LifeCommand(1, LifeAction.Increment), 900) // 他人の操作は P1 の期限を動かさない
        assertEquals(1000L, c.nextExpiryMs())
        c.accept(reset(2), 400)
        assertEquals(1000L, c.nextExpiryMs()) // 最も早い方
        c.tick(1000)
        assertEquals(1400L, c.nextExpiryMs()) // P1 が満了し、残りの P3 の期限になる
        c.tick(1400)
        assertNull(c.nextExpiryMs())
    }

    @Test fun four_players_expire_independently() {
        val c = ResetConfirmation()
        for (id in 0..3) c.accept(reset(id), id * 100L)
        for (id in 0..3) assertTrue(c.isPending(id))
        c.tick(1000); assertFalse(c.isPending(0)); assertTrue(c.isPending(1))
        c.tick(1100); assertFalse(c.isPending(1)); assertTrue(c.isPending(2))
        c.tick(1300); assertFalse(c.isPending(2)); assertFalse(c.isPending(3))
        assertNull(c.nextExpiryMs())
    }

    @Test fun confirming_clears_that_players_deadline_only() {
        val c = ResetConfirmation()
        c.accept(reset(0), 0); c.accept(reset(1), 300)
        assertNotNull(c.accept(reset(0), 500))
        assertEquals(1300L, c.nextExpiryMs())
    }
}

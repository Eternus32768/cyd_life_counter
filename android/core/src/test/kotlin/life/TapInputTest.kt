package life

import life.application.LifeAction
import life.application.LifeCommand
import life.ui.ResetConfirmation
import life.ui.TapInput
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFalse
import kotlin.test.assertNotNull
import kotlin.test.assertNull
import kotlin.test.assertTrue

// レビュー AJ02: 1回目の指が離れてから2回目を受け付ける。
class TapInputTest {
    private fun reset(id: Int) = LifeCommand(id, LifeAction.Reset)
    private fun inc(id: Int) = LifeCommand(id, LifeAction.Increment)

    private fun newInput(): Pair<TapInput, ResetConfirmation> {
        val confirmation = ResetConfirmation()
        return TapInput(confirmation) to confirmation
    }

    @Test fun normal_double_tap_confirms() {
        val (input, _) = newInput()
        assertNull(input.pointerDown(0, reset(2), 0))
        input.pointerUp(0)
        assertEquals(reset(2), input.pointerDown(0, reset(2), 300))
    }

    @Test fun second_finger_while_first_is_still_down_is_ignored() {
        val (input, c) = newInput()
        assertNull(input.pointerDown(0, reset(1), 0))
        assertNull(input.pointerDown(1, reset(1), 200)) // 1本目を離さずに2本目 → 確定しない
        assertTrue(c.isPending(1))                      // 待機は維持される
        assertEquals(1000L, c.nextExpiryMs())           // 期限も延びない
    }

    @Test fun after_ignoring_a_held_second_finger_a_release_and_retap_still_confirms() {
        val (input, _) = newInput()
        input.pointerDown(0, reset(1), 0)
        input.pointerDown(1, reset(1), 200)  // 無視
        input.pointerUp(0)
        input.pointerUp(1)
        assertNotNull(input.pointerDown(0, reset(1), 500))
    }

    @Test fun releasing_the_first_finger_lets_another_finger_confirm() {
        val (input, _) = newInput()
        input.pointerDown(0, reset(3), 0)
        input.pointerUp(0)
        assertNotNull(input.pointerDown(1, reset(3), 400)) // 別の指でもよい
    }

    @Test fun other_players_are_not_blocked_by_a_held_finger() {
        val (input, c) = newInput()
        input.pointerDown(0, reset(0), 0)  // P1 の1回目、指は押したまま
        assertNull(input.pointerDown(1, reset(2), 100)) // P3 の1回目
        assertTrue(c.isPending(0) && c.isPending(2))
        input.pointerUp(1)
        assertNotNull(input.pointerDown(1, reset(2), 300)) // P3 は確定できる
        assertTrue(c.isPending(0))
    }

    @Test fun step_buttons_are_never_blocked() {
        val (input, _) = newInput()
        input.pointerDown(0, reset(1), 0)
        assertEquals(inc(1), input.pointerDown(1, inc(1), 100))
        assertEquals(inc(2), input.pointerDown(2, inc(2), 120))
    }

    @Test fun pointer_id_reuse_does_not_block_a_later_confirmation() {
        val (input, c) = newInput()
        input.pointerDown(0, reset(1), 0)
        input.pointerUp(0)                              // 番号0の指が離れる
        input.pointerDown(0, inc(2), 200)               // 番号0が別の指に再利用され、押したままになる
        assertNotNull(input.pointerDown(1, reset(1), 300)) // それでも P2 の2回目は確定できる
        assertFalse(c.isPending(1))
    }

    @Test fun held_past_the_window_then_new_tap_starts_over() {
        val (input, c) = newInput()
        input.pointerDown(0, reset(0), 0)
        // 指を離さないまま猶予が切れる
        assertNull(input.pointerDown(1, reset(0), 1500)) // 期限切れ後なので新しい1回目
        assertTrue(c.isPending(0))
        input.pointerUp(1)
        assertNotNull(input.pointerDown(1, reset(0), 1700))
    }

    @Test fun cancel_all_forgets_held_fingers() {
        val (input, _) = newInput()
        input.pointerDown(0, reset(1), 0)
        input.cancelAll()                               // ACTION_CANCEL 等。離した扱いにする
        assertNotNull(input.pointerDown(1, reset(1), 300))
    }

    @Test fun invalid_player_is_ignored_safely() {
        val (input, c) = newInput()
        assertNull(input.pointerDown(0, reset(-1), 0))
        assertNull(input.pointerDown(1, reset(4), 10))
        for (id in -1..4) assertFalse(c.isPending(id))
    }
}

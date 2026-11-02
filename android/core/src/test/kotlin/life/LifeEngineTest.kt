package life

import life.application.LifeAction
import life.application.LifeCommand
import life.application.LifeEngine
import life.domain.INITIAL_VALUE
import life.domain.PLAYER_COUNT
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFalse
import kotlin.test.assertTrue

// SPEC.md「状態と操作」/ tests/ACCEPTANCE.md の自動テスト項目に対応。
class LifeEngineTest {
    private fun inc(id: Int) = LifeCommand(id, LifeAction.Increment)
    private fun dec(id: Int) = LifeCommand(id, LifeAction.Decrement)
    private fun reset(id: Int) = LifeCommand(id, LifeAction.Reset)

    private fun expectAll(engine: LifeEngine, vararg expected: Int) {
        assertEquals(expected.toList(), engine.snapshot())
    }

    @Test fun initial_state_is_zero_for_all_players() = expectAll(LifeEngine(), 0, 0, 0, 0)

    @Test fun increment_and_decrement_affect_only_target_player() {
        val e = LifeEngine()
        assertTrue(e.apply(inc(0))); expectAll(e, 1, 0, 0, 0)
        assertTrue(e.apply(dec(0))); expectAll(e, 0, 0, 0, 0)
        assertTrue(e.apply(dec(0))); expectAll(e, -1, 0, 0, 0) // 0で下げ止めしない
    }

    @Test fun each_player_is_independent() {
        val e = LifeEngine()
        for (id in 0 until PLAYER_COUNT) repeat(id + 1) { assertTrue(e.apply(inc(id))) }
        expectAll(e, 1, 2, 3, 4)
    }

    @Test fun reset_clears_only_target_player() {
        val e = LifeEngine()
        e.apply(inc(0)); e.apply(inc(1)); e.apply(inc(1)); e.apply(dec(2)); e.apply(inc(3))
        expectAll(e, 1, 2, -1, 1)
        assertTrue(e.apply(reset(1))); expectAll(e, 1, 0, -1, 1)
        assertTrue(e.apply(reset(2))); expectAll(e, 1, 0, 0, 1)
    }

    @Test fun reset_is_idempotent_and_accepted() {
        val e = LifeEngine()
        assertTrue(e.apply(reset(3))); expectAll(e, 0, 0, 0, 0)
    }

    @Test fun operations_work_after_reset() {
        val e = LifeEngine()
        e.apply(inc(2)); e.apply(reset(2))
        assertTrue(e.apply(inc(2))); assertEquals(1, e.valueOf(2))
        assertTrue(e.apply(dec(2))); assertTrue(e.apply(dec(2)))
        assertEquals(-1, e.valueOf(2))
    }

    @Test fun invalid_player_id_does_not_change_state() {
        val e = LifeEngine()
        e.apply(inc(0))
        for (id in intArrayOf(-1, PLAYER_COUNT, 99, -12345, 1 shl 20)) {
            assertFalse(e.apply(inc(id)))
            assertFalse(e.apply(dec(id)))
            assertFalse(e.apply(reset(id)))
            assertEquals(INITIAL_VALUE, e.valueOf(id))
        }
        expectAll(e, 1, 0, 0, 0)
    }

    @Test fun saturates_at_int_max() {
        val e = LifeEngine(Int.MAX_VALUE)
        assertFalse(e.apply(inc(0)))
        assertEquals(Int.MAX_VALUE, e.valueOf(0)) // オーバーフローせず変化なし
        assertTrue(e.apply(dec(0))); assertEquals(Int.MAX_VALUE - 1, e.valueOf(0))
        assertTrue(e.apply(inc(0))); assertEquals(Int.MAX_VALUE, e.valueOf(0))
    }

    @Test fun saturates_at_int_min() {
        val e = LifeEngine(Int.MIN_VALUE)
        assertFalse(e.apply(dec(3)))
        assertEquals(Int.MIN_VALUE, e.valueOf(3)) // アンダーフローせず変化なし
        assertTrue(e.apply(inc(3))); assertEquals(Int.MIN_VALUE + 1, e.valueOf(3))
    }

    @Test fun reset_returns_to_initial_value_from_extremes() {
        val e = LifeEngine(Int.MAX_VALUE)
        assertTrue(e.apply(reset(1)))
        assertEquals(INITIAL_VALUE, e.valueOf(1))
        assertEquals(Int.MAX_VALUE, e.valueOf(0))
    }

    @Test fun snapshot_is_immutable_copy() {
        val e = LifeEngine()
        val before = e.snapshot()
        e.apply(inc(0))
        assertEquals(listOf(0, 0, 0, 0), before) // 取得後の操作は反映されない
    }
}

package life

import life.application.LifeAction
import life.domain.PLAYER_COUNT
import life.ui.LayoutConfig
import life.ui.Orientation
import life.ui.Point
import life.ui.QuadLayout
import life.ui.Rect
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFalse
import kotlin.test.assertNotNull
import kotlin.test.assertNull
import kotlin.test.assertTrue

// SPEC.md「UI」/ ACCEPTANCE「UIヒットテスト」に対応。CYD実機(320×240)とタブレット想定の両方で検査する。
class LayoutTest {
    private val sizes = listOf(320 to 240, 1920 to 1200, 2560 to 1600, 1280 to 800, 1024 to 600)

    private fun layoutOf(w: Int, h: Int) = QuadLayout(LayoutConfig.forScreen(w, h))
    private fun forEachSize(block: (QuadLayout, Int, Int) -> Unit) =
        sizes.forEach { (w, h) -> block(layoutOf(w, h), w, h) }

    private fun center(r: Rect) = Point(r.x + r.width / 2, r.y + r.height / 2)

    @Test fun panels_tile_the_screen_without_gap_or_overlap() = forEachSize { l, w, h ->
        var covered = 0
        for (y in 0 until h) for (x in 0 until w) {
            if ((0 until PLAYER_COUNT).count { l.panel(it).contains(Point(x, y)) } == 1) covered++
        }
        assertEquals(w * h, covered)
    }

    @Test fun panel_positions_follow_p1_to_p4_order() = forEachSize { l, w, h ->
        assertTrue(l.panel(0).contains(Point(10, 10)))
        assertTrue(l.panel(1).contains(Point(w - 10, 10)))
        assertTrue(l.panel(2).contains(Point(10, h - 10)))
        assertTrue(l.panel(3).contains(Point(w - 10, h - 10)))
    }

    @Test fun plus_minus_are_vertically_stacked_on_the_outer_edge() = forEachSize { l, _, _ ->
        for (id in 0 until PLAYER_COUNT) {
            val panel = l.panel(id); val plus = l.incrementButton(id); val minus = l.decrementButton(id)
            assertEquals(plus.x, minus.x)
            assertEquals(plus.width, minus.width)
            assertFalse(plus.overlaps(minus))
            assertEquals(panel.height, plus.height + minus.height)
            if (id % 2 == 1) assertEquals(panel.right, plus.right) else assertEquals(panel.x, plus.x)
        }
    }

    @Test fun plus_is_on_the_player_side_of_the_strip() = forEachSize { l, _, _ ->
        for (id in 0 until PLAYER_COUNT) {
            val plus = l.incrementButton(id); val minus = l.decrementButton(id); val panel = l.panel(id)
            if (l.orientationOf(id) == Orientation.UpsideDown) {
                assertTrue(minus.y < plus.y)
                assertEquals(panel.y, minus.y)
                assertEquals(panel.bottom, plus.bottom)
            } else {
                assertTrue(plus.y < minus.y)
                assertEquals(panel.y, plus.y)
                assertEquals(panel.bottom, minus.bottom)
            }
        }
    }

    @Test fun top_row_panels_are_upside_down() {
        val l = layoutOf(1920, 1200)
        assertEquals(Orientation.UpsideDown, l.orientationOf(0))
        assertEquals(Orientation.UpsideDown, l.orientationOf(1))
        assertEquals(Orientation.Normal, l.orientationOf(2))
        assertEquals(Orientation.Normal, l.orientationOf(3))
        assertEquals(Orientation.Normal, l.orientationOf(-1))
        assertEquals(Orientation.Normal, l.orientationOf(PLAYER_COUNT))
    }

    @Test fun hit_test_follows_the_swapped_plus_minus_on_the_top_row() = forEachSize { l, _, _ ->
        for (id in 0..1) {
            val hit = l.hitTest(center(l.incrementButton(id)))
            assertEquals(LifeAction.Increment, hit?.action)
            assertTrue(center(l.incrementButton(id)).y > center(l.decrementButton(id)).y)
        }
    }

    @Test fun reset_sits_in_the_header_band_toward_the_screen_center() = forEachSize { l, _, _ ->
        for (id in 0 until PLAYER_COUNT) {
            val header = l.headerArea(id); val reset = l.resetButton(id); val label = l.labelArea(id)
            assertEquals(header.y, reset.y)
            assertEquals(header.height, reset.height)
            assertTrue(reset.x >= header.x && reset.right <= header.right)
            if (id % 2 == 1) assertTrue(reset.x < label.x) else assertTrue(reset.x > label.x)
            assertFalse(reset.overlaps(l.incrementButton(id)))
            assertFalse(reset.overlaps(l.decrementButton(id)))
        }
    }

    @Test fun neighbouring_reset_buttons_keep_a_gap() = forEachSize { l, _, _ ->
        assertTrue(l.resetButton(0).right < l.resetButton(1).x)
        assertTrue(l.resetButton(2).right < l.resetButton(3).x)
    }

    @Test fun header_splits_into_label_and_reset_and_number_takes_the_rest() = forEachSize { l, _, _ ->
        for (id in 0 until PLAYER_COUNT) {
            val value = l.valueArea(id); val header = l.headerArea(id); val number = l.numberArea(id)
            assertEquals(value.width, header.width)
            assertEquals(value.height, header.height + number.height)
            assertEquals(value.width, number.width)
            assertFalse(header.overlaps(number))
            assertFalse(l.labelArea(id).overlaps(l.resetButton(id)))
            assertTrue(number.height > header.height) // 数値を最大限大きく
            if (id / 2 == 1) assertEquals(value.bottom, header.bottom) else assertEquals(value.y, header.y)
        }
    }

    @Test fun number_area_never_overlaps_any_button() = forEachSize { l, _, _ ->
        for (id in 0 until PLAYER_COUNT) for (other in 0 until PLAYER_COUNT) {
            val n = l.numberArea(id)
            assertFalse(n.overlaps(l.incrementButton(other)))
            assertFalse(n.overlaps(l.decrementButton(other)))
            assertFalse(n.overlaps(l.resetButton(other)))
        }
    }

    @Test fun hit_test_maps_each_button_to_its_player_and_action() = forEachSize { l, _, _ ->
        for (id in 0 until PLAYER_COUNT) {
            val cases = listOf(
                l.incrementButton(id) to LifeAction.Increment,
                l.decrementButton(id) to LifeAction.Decrement,
                l.resetButton(id) to LifeAction.Reset,
            )
            for ((rect, action) in cases) {
                val hit = assertNotNull(l.hitTest(center(rect)))
                assertEquals(id, hit.playerId)
                assertEquals(action, hit.action)
            }
        }
    }

    @Test fun hit_test_ignores_points_outside_the_screen() = forEachSize { l, w, h ->
        for (p in listOf(Point(-1, 0), Point(0, -1), Point(w, 0), Point(0, h), Point(-100, -100), Point(w + 50, h + 50))) {
            assertNull(l.hitTest(p))
        }
    }

    @Test fun hit_test_ignores_the_number_area() = forEachSize { l, _, _ ->
        for (id in 0 until PLAYER_COUNT) {
            val n = l.numberArea(id)
            for (p in listOf(center(n), Point(n.x, n.y), Point(n.right - 1, n.y),
                Point(n.x, n.bottom - 1), Point(n.right - 1, n.bottom - 1))) {
                assertNull(l.hitTest(p))
            }
        }
    }

    @Test fun hit_test_result_always_matches_the_rect_containing_the_point() = forEachSize { l, w, h ->
        for (y in 0 until h) for (x in 0 until w) {
            val p = Point(x, y)
            val hit = l.hitTest(p) ?: continue
            assertTrue(l.panel(hit.playerId).contains(p))
            val rect = when (hit.action) {
                LifeAction.Increment -> l.incrementButton(hit.playerId)
                LifeAction.Decrement -> l.decrementButton(hit.playerId)
                LifeAction.Reset -> l.resetButton(hit.playerId)
            }
            assertTrue(rect.contains(p))
        }
    }

    @Test fun buttons_of_different_players_never_overlap() = forEachSize { l, _, _ ->
        val rects = (0 until PLAYER_COUNT).flatMap { listOf(l.incrementButton(it), l.decrementButton(it), l.resetButton(it)) }
        for (i in rects.indices) for (j in i + 1 until rects.size) assertFalse(rects[i].overlaps(rects[j]))
    }

    @Test fun layout_stays_valid_on_unusual_screen_sizes() {
        for ((w, h) in listOf(320 to 240, 240 to 320, 480 to 320, 64 to 48, 33 to 25, 1201 to 801)) {
            val l = layoutOf(w, h)
            for (id in 0 until PLAYER_COUNT) {
                val panel = l.panel(id)
                assertTrue(panel.width > 0 && panel.height > 0)
                for (r in listOf(l.incrementButton(id), l.decrementButton(id), l.resetButton(id),
                    l.numberArea(id), l.labelArea(id))) {
                    assertTrue(r.width > 0 && r.height > 0)
                    assertTrue(r.x >= panel.x && r.right <= panel.right)
                    assertTrue(r.y >= panel.y && r.bottom <= panel.bottom)
                }
            }
        }
    }

    @Test fun invalid_player_id_returns_empty_rects() {
        val l = layoutOf(1920, 1200)
        for (id in intArrayOf(-1, PLAYER_COUNT, 1000)) {
            for (r in listOf(l.panel(id), l.incrementButton(id), l.decrementButton(id), l.resetButton(id), l.numberArea(id))) {
                assertEquals(0, r.width)
            }
            assertFalse(l.panel(id).contains(Point(0, 0)))
        }
    }
}

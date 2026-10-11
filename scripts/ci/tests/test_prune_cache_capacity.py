"""验证容量驱逐只在需要空间时发生，并保留本轮及明确保护的缓存。"""

from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from prune_cache_capacity import eviction_plan


class CacheCapacityTests(unittest.TestCase):
    def test_pressure_can_evict_original_nightly_cache_by_access_time(self):
        entries = [dict(id=1, key="ccache-linux-nightly-100", size_in_bytes=30, last_accessed_at="2026-01-01"),
                   dict(id=2, key="ccache-linux-fuzz-101", size_in_bytes=30, last_accessed_at="2026-01-02"),
                   dict(id=3, key="ccache-linux-release-200", size_in_bytes=30, last_accessed_at="2026-01-03"),
                   dict(id=4, key="vcpkg-linux-current", size_in_bytes=10, last_accessed_at="2025-01-01"),
                   dict(id=5, key="unrelated-cache", size_in_bytes=10, last_accessed_at="2024-01-01")]
        victims, remaining = eviction_plan(entries, 100, 20, "200", {"vcpkg-linux-current"})
        self.assertEqual([entry["id"] for entry in victims], [1])
        self.assertEqual(remaining, 80)
        self.assertEqual(eviction_plan(entries, 200, 0, "200", set())[0], [])

    def test_insufficient_eligible_space_does_not_delete_protected_or_unrelated_entries(self):
        entries = [dict(id=1, key="ccache-linux-nightly-200", size_in_bytes=70),
                   dict(id=2, key="vcpkg-linux-current", size_in_bytes=30),
                   dict(id=3, key="other-cache", size_in_bytes=40)]
        victims, remaining = eviction_plan(entries, 100, 20, "200", {"vcpkg-linux-current"})
        self.assertEqual(victims, [])
        self.assertEqual(remaining, 140)

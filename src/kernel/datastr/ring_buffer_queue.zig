const std = @import("std");

pub fn FixedMPSCRingBuffer(comptime T: type, comptime CAPACITY: usize) type {
    if (CAPACITY == 0 or (CAPACITY & (CAPACITY - 1)) != 0)
        @compileError("CAPACITY must be power of two and > 0");

    return struct {
        const Self = @This();

        const Slot = struct {
            seq: std.atomic.Value(u64),
            val: T,
        };

        slots: [CAPACITY]Slot = undefined,
        head: std.atomic.Value(u64) = std.atomic.Value(u64).init(0),
        tail: std.atomic.Value(u64) = std.atomic.Value(u64).init(0),

        /// Must be called once before use.
        pub fn init() Self {
			var ret = Self{};
            var i: usize = 0;
            while (i < CAPACITY) : (i += 1) {
                ret.slots[i].seq = std.atomic.Value(u64).init(@intCast(i));
            }
			return ret;
        }

        /// Multi-producer push. Returns `true` if success, `false` if full.
        pub fn push(self: *Self, value: T) bool {
            const pos = self.tail.fetchAdd(1, .acq_rel);
            const idx: usize = @as(usize, @intCast(pos)) & (CAPACITY - 1);
            const slot = &self.slots[idx];

            const seq = slot.seq.load(.acquire);
            const dif =@as(i64, @intCast(seq)) -  @as(i64, @intCast(pos));
            if (dif != 0) {
                // slot not ready yet -> queue full
                return false;
            }

            slot.val = value; // copy
            slot.seq.store(pos + 1, .release);
            return true;
        }

        /// Single-consumer pop. Returns `?T` (null if empty).
        pub fn pop(self: *Self) ?T {
            const pos = self.head.load(.unordered);
            const idx: usize = @as(usize, @intCast(pos)) & (CAPACITY - 1);
            const slot = &self.slots[idx];

            const seq = slot.seq.load(.acquire);
            const dif = @as(i64, @intCast(seq)) -  @as(i64, @intCast(pos + 1));
            if (dif != 0) {
                // empty
                return null;
            }

            const v = slot.val;
            slot.seq.store(pos + CAPACITY, .release);
            _ = self.head.fetchAdd(1, .acq_rel);
            return v;
        }

        pub fn is_empty(self: *Self) bool {
            return self.head.load(.acquire) == self.tail.load(.acquire);
        }
    };
}

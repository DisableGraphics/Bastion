pub const LinearCongruentialGenerator = struct {
	state: u64,
	pub fn init(seed: u64) LinearCongruentialGenerator {
		return .{.state = seed};
	}

	pub fn next(self:* LinearCongruentialGenerator) u16 {
		self.state = self.state *% 1103515245 +% 12345;
    	return @truncate(self.state/65536 % 32768);
	}
};
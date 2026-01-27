const limine = @import("limine");
const main = @import("../../../main.zig");
const serial = @import("../../../serial/serial.zig");
const spin = @import("../../../sync/spinlock.zig");

pub export var start_marker: limine.RequestsStartMarker linksection(".limine_requests_start") = .{};
pub export var end_marker: limine.RequestsEndMarker linksection(".limine_requests_end") = .{};

pub export var base_revision: limine.BaseRevision linksection(".limine_requests") = .init(3);
pub export var framebuffer_request: limine.FramebufferRequest linksection(".limine_requests") = .{};
pub export var memory_map_request: limine.MemoryMapRequest linksection(".limine_requests") = .{};
pub export var mp_request: limine.MpRequest linksection(".limine_requests") = .{};
pub export var hhdm_request: limine.HhdmRequest linksection(".limine_requests") = .{};
pub export var rsdp_request: limine.RsdpRequest linksection(".limine_requests") = .{};
pub export var module_request: limine.ModuleRequest linksection(".limine_requests") = .{};


pub const SmpInfo = packed struct {
	processor_id: u32,
	lapic_id: u32,
	reserved: u64,
	goto_address: ?*anyopaque,
	extra_argument: u64,
};




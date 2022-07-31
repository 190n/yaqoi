const std = @import("std");
const stb_image = @cImport({
    @cInclude("stb_image.h");
});

pub const STBImageResult = struct {
    data: []u8,
    x: usize,
    y: usize,
    channels_in_file: u3,
};

pub fn load(filename: [*:0]const u8, desired_channels: ?u3) !STBImageResult {
    var x: c_int = undefined;
    var y: c_int = undefined;
    var channels_in_file: c_int = undefined;
    const ptr = stb_image.stbi_load(filename, &x, &y, &channels_in_file, desired_channels orelse 0);
    return STBImageResult{
        .data = ptr[0..(@intCast(usize, x) * @intCast(usize, y) * @intCast(u3, channels_in_file))],
        .x = @intCast(usize, x),
        .y = @intCast(usize, y),
        .channels_in_file = @intCast(u3, channels_in_file),
    };
}

test "foo" {
    _ = load;
}

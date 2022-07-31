const std = @import("std");
const stb_image = @cImport({
    @cInclude("stb_image.h");
});

pub const STBImageResult = struct {
    data: []u8,
    x: usize,
    y: usize,
    channels_in_file: u3,

    pub fn deinit(self: *STBImageResult) void {
        std.heap.c_allocator.free(self.data);
        self.data.len = 0;
    }
};

pub const IOCallbacks = extern struct {
    read: fn (user: ?*anyopaque, data: ?[*]u8, size: c_int) callconv(.C) c_int,
    skip: fn (user: ?*anyopaque, n: c_int) callconv(.C) void,
    eof: fn (user: ?*anyopaque) callconv(.C) c_int,
};

fn makeCallbacks(comptime Context: type) IOCallbacks {
    if (!@hasDecl(Context, "reader")) @compileError("type " ++ @typeName(Context) ++ " does not implement .reader()");
    if (!@hasDecl(Context, "seekableStream")) @compileError("type " ++ @typeName(Context) ++ " does not implement .seekableStream()");

    const callbacks = struct {
        fn opaqueToContext(user: ?*anyopaque) *Context {
            return @ptrCast(*Context, @alignCast(@alignOf(Context), user));
        }

        pub fn read(user: ?*anyopaque, data: ?[*]u8, size: c_int) callconv(.C) c_int {
            const context = opaqueToContext(user);
            return @intCast(c_int, context.reader().read(data.?[0..@intCast(usize, size)]) catch 0);
        }

        pub fn skip(user: ?*anyopaque, n: c_int) callconv(.C) void {
            const context = opaqueToContext(user);
            context.seekableStream().seekBy(n) catch unreachable;
        }

        pub fn eof(user: ?*anyopaque) callconv(.C) c_int {
            const context = opaqueToContext(user);
            var byte: [1]u8 = undefined;
            // errors are counted as EOF because continuing to read probably isn't desired, and
            // stb_image will already report errors from too-early EOFs that cause files to be
            // invalid
            const amount_read = context.reader().read(&byte) catch return 1;
            if (amount_read == 0) {
                // nothing read means EOF
                return 1;
            } else {
                // skip back, and now we're not at EOF
                context.seekableStream().seekBy(-1) catch return 1;
                return 0;
            }
        }
    };

    return .{
        .read = callbacks.read,
        .skip = callbacks.skip,
        .eof = callbacks.eof,
    };
}

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

test "I/O callbacks" {
    const cbs = makeCallbacks(std.io.FixedBufferStream([]const u8));
    const in_buf = "hello world";
    var out_buf: [in_buf.len]u8 = undefined;
    var stream = std.io.fixedBufferStream(in_buf);

    // it should read 5 bytes and not be at EOF
    try std.testing.expectEqual(@as(c_int, 5), cbs.read(&stream, &out_buf, 5));
    try std.testing.expectEqualSlices(u8, "hello", out_buf[0..5]);
    try std.testing.expectEqual(@as(c_int, 0), cbs.eof(&stream));

    // it should read 6 bytes and be at EOF
    try std.testing.expectEqual(@as(c_int, 6), cbs.read(&stream, &out_buf, out_buf.len));
    try std.testing.expectEqualSlices(u8, " world", out_buf[0..6]);
    try std.testing.expect(cbs.eof(&stream) != 0);

    // it should read zero bytes and still be at EOF
    try std.testing.expectEqual(@as(c_int, 0), cbs.read(&stream, &out_buf, out_buf.len));
    try std.testing.expect(cbs.eof(&stream) != 0);

    // it should seek back, then not be at EOF, then read 3 bytes and return to EOF
    cbs.skip(&stream, -3);
    try std.testing.expectEqual(@as(c_int, 0), cbs.eof(&stream));
    try std.testing.expectEqual(@as(c_int, 3), cbs.read(&stream, &out_buf, out_buf.len));
    try std.testing.expectEqualSlices(u8, "rld", out_buf[0..3]);
    try std.testing.expect(cbs.eof(&stream) != 0);
}

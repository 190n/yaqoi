const std = @import("std");
const Stats = @import("./stats.zig");
const QoiEncoder = @This();

pub const Channels = enum(u8) { rgb = 3, rgba = 4 };

pub const Colorspace = enum(u8) { srgb_gamma = 0, srgb_linear = 1 };

pub const Pixel = extern struct { r: u8, g: u8, b: u8, a: u8 };

pub const Config = struct {
    width: u32,
    height: u32,
    channels: Channels,
    colorspace: Colorspace,
};

/// generate a struct identical to T except with all fields in reverse order
fn Reverse(comptime T: type) type {
    comptime var fields: []const std.builtin.Type.StructField = &.{};
    for (@typeInfo(T).Struct.fields) |f| {
        fields = [1]std.builtin.Type.StructField{f} ++ fields;
    }
    return @Type(.{
        .Struct = .{
            .layout = @typeInfo(T).Struct.layout,
            .decls = &.{},
            .fields = fields,
            .is_tuple = @typeInfo(T).Struct.is_tuple,
        },
    });
}

const PixelDifference = struct {
    r: i9,
    g: i9,
    b: i9,
    a: i9,

    /// calculate y - x
    pub fn init(x: Pixel, y: Pixel) PixelDifference {
        return .{
            .r = @as(i9, y.r) - @as(i9, x.r),
            .g = @as(i9, y.g) - @as(i9, x.g),
            .b = @as(i9, y.b) - @as(i9, x.b),
            .a = @as(i9, y.a) - @as(i9, x.a),
        };
    }
};

pub fn chunkToBytes(chunk: anytype) [@bitSizeOf(@TypeOf(chunk)) / 8]u8 {
    const T = @TypeOf(chunk);
    const IntegerType = std.meta.Int(.unsigned, @bitSizeOf(T));
    const int = @bitCast(IntegerType, chunk);
    const big_endian = std.mem.nativeToBig(IntegerType, int);
    return @bitCast([@bitSizeOf(T) / 8]u8, big_endian);
}

const Header = packed struct {
    magic0: u8 = 'q',
    magic1: u8 = 'o',
    magic2: u8 = 'i',
    magic3: u8 = 'f',
    width: u32,
    height: u32,
    channels: Channels,
    colorspace: Colorspace,

    const FlipHeader = Reverse(@This());

    pub fn init(width: u32, height: u32, channels: Channels, colorspace: Colorspace) FlipHeader {
        return .{
            .width = width,
            .height = height,
            .channels = channels,
            .colorspace = colorspace,
        };
    }
};

test "Header" {
    try std.testing.expectEqual(@as(usize, 112), @bitSizeOf(Header));
    try std.testing.expectEqual([_]u8{
        'q', 'o', 'i', 'f', // magic
        0x00, 0x00, 0x00, 0xff, // width
        0x00, 0x00, 0x00, 0x80, // height
        3, // channels
        1, // colorspace
    }, chunkToBytes(Header.init(0xff, 0x80, .rgb, .srgb_linear)));
}

const ChunkRgb = packed struct {
    tag: u8 = 0b11111110,
    r: u8,
    g: u8,
    b: u8,

    const FlipChunkRgb = Reverse(@This());

    pub fn init(pixel: Pixel) FlipChunkRgb {
        return .{
            .r = pixel.r,
            .g = pixel.g,
            .b = pixel.b,
        };
    }
};

test "ChunkRgb" {
    try std.testing.expectEqual([_]u8{
        0b11111110,
        0xff,
        0x80,
        0x00,
    }, chunkToBytes(ChunkRgb.init(Pixel{ .r = 0xff, .g = 0x80, .b = 0x00, .a = 0xff })));
}

const ChunkRgba = packed struct {
    tag: u8 = 0b11111111,
    r: u8,
    g: u8,
    b: u8,
    a: u8,

    const FlipChunkRgba = Reverse(@This());

    pub fn init(pixel: Pixel) FlipChunkRgba {
        return .{
            .r = pixel.r,
            .g = pixel.g,
            .b = pixel.b,
            .a = pixel.a,
        };
    }
};

test "ChunkRgba" {
    try std.testing.expectEqual([_]u8{
        0b11111111,
        0xff,
        0xaa,
        0x55,
        0x00,
    }, chunkToBytes(ChunkRgba.init(Pixel{ .r = 0xff, .g = 0xaa, .b = 0x55, .a = 0x00 })));
}

const ChunkIndex = packed struct {
    tag: u2 = 0b00,
    index: u6,

    const FlipChunkIndex = Reverse(@This());

    pub fn init(index: u6) FlipChunkIndex {
        return .{ .index = index };
    }
};

test "ChunkIndex" {
    try std.testing.expectEqual([_]u8{35}, chunkToBytes(ChunkIndex.init(35)));
}

const ChunkDiff = packed struct {
    tag: u2 = 0b01,
    dr: u2,
    dg: u2,
    db: u2,

    const FlipChunkDiff = Reverse(@This());

    /// create a diff chunk representing the given pixel difference if the difference is small
    /// enough to be represented, or null otherwise
    pub fn init(diff: PixelDifference) ?FlipChunkDiff {
        if (diff.a != 0) return null;
        for ([_]i9{ diff.r, diff.g, diff.b }) |channel| {
            if (channel < -2 or channel > 1) return null;
        }

        return .{
            .dr = @intCast(u2, diff.r + 2),
            .dg = @intCast(u2, diff.g + 2),
            .db = @intCast(u2, diff.b + 2),
        };
    }
};

test "ChunkDiff" {
    try std.testing.expectEqual([_]u8{
        (0b01 << 6) | (2 << 4) | (0 << 2) | 3,
    }, chunkToBytes(ChunkDiff.init(PixelDifference.init(
        // diff:    0      -2        1         0
        // encoded: 2       0        3       n/a
        Pixel{ .r = 5, .g = 8, .b = 10, .a = 255 },
        Pixel{ .r = 5, .g = 6, .b = 11, .a = 255 },
    )).?));

    // should fail: alpha channels not identical
    try std.testing.expectEqual(@as(?Reverse(ChunkDiff), null), ChunkDiff.init(PixelDifference{
        .r = 0,
        .g = 0,
        .b = 0,
        .a = 1,
    }));

    // should fail: red channel difference too big
    try std.testing.expectEqual(@as(?Reverse(ChunkDiff), null), ChunkDiff.init(PixelDifference{
        .r = 3,
        .g = 0,
        .b = 0,
        .a = 0,
    }));
}

const ChunkLuma = packed struct {
    tag: u2 = 0b10,
    dg: u6,
    dr_dg: u4,
    db_dg: u4,

    const FlipChunkLuma = Reverse(@This());

    /// create an ChunkLuma representing the given pixel difference if the difference is small
    /// enough to be represented
    pub fn init(diff: PixelDifference) ?FlipChunkLuma {
        if (diff.a != 0) return null;
        if (diff.g < -32 or diff.g > 31) return null;
        const dr_dg = @as(i10, diff.r) - @as(i10, diff.g);
        const db_dg = @as(i10, diff.b) - @as(i10, diff.g);
        inline for ([_]i10{ dr_dg, db_dg }) |channel| {
            if (channel < -8 or channel > 7) return null;
        }
        return .{
            .dg = @intCast(u6, diff.g + 32),
            .dr_dg = @intCast(u4, dr_dg + 8),
            .db_dg = @intCast(u4, db_dg + 8),
        };
    }
};

test "ChunkLuma" {
    try std.testing.expectEqual([_]u8{
        (0b10 << 6) | 63,
        (15 << 4) | 0,
    }, chunkToBytes(ChunkLuma.init(PixelDifference.init(
        // diff:     38       31       23         0
        // encoded:  15       63        0       n/a
        Pixel{ .r = 200, .g = 50, .b = 10, .a = 255 },
        Pixel{ .r = 238, .g = 81, .b = 33, .a = 255 },
    )).?));

    // fail: alpha difference
    try std.testing.expectEqual(@as(?Reverse(ChunkLuma), null), ChunkLuma.init(PixelDifference{
        .r = 0,
        .g = 0,
        .b = 0,
        .a = 1,
    }));

    // fail: dg
    try std.testing.expectEqual(@as(?Reverse(ChunkLuma), null), ChunkLuma.init(PixelDifference{
        .r = 32,
        .g = 32,
        .b = 32,
        .a = 0,
    }));

    // fail: dr_dg
    try std.testing.expectEqual(@as(?Reverse(ChunkLuma), null), ChunkLuma.init(PixelDifference{
        .r = -9,
        .g = 0,
        .b = 0,
        .a = 0,
    }));

    // fail: db_dg
    try std.testing.expectEqual(@as(?Reverse(ChunkLuma), null), ChunkLuma.init(PixelDifference{
        .r = 0,
        .g = 0,
        .b = 8,
        .a = 0,
    }));

    // fail: dr_dg, and would overflow if we didn't cast to i10
    try std.testing.expectEqual(@as(?Reverse(ChunkLuma), null), ChunkLuma.init(PixelDifference{
        .r = 255,
        .g = -32,
        .b = 0,
        .a = 0,
    }));
}

const ChunkRun = packed struct {
    tag: u2 = 0b11,
    run_length: u6,

    const FlipChunkRun = Reverse(@This());

    pub fn init(run_length: u6) FlipChunkRun {
        std.debug.assert(run_length >= 1 and run_length <= 62);
        return .{
            .run_length = run_length - 1,
        };
    }
};

test "ChunkRun" {
    try std.testing.expectEqual([_]u8{(0b11 << 6) | 27}, chunkToBytes(ChunkRun.init(28)));
}

fn hashPixel(pixel: Pixel) u6 {
    return @truncate(
        u6,
        (3 *% pixel.r) +% (5 *% pixel.g) +% (7 *% pixel.b) +% (11 *% pixel.a),
    );
}

test "hashPixel" {
    try std.testing.expectEqual(@as(u6, 46), hashPixel(Pixel{
        .r = 85,
        .g = 134,
        .b = 173,
        .a = 2,
    }));
}

const end_marker = [1]u8{0} ** 7 ++ [1]u8{1};

stats: Stats = .{},
track_stats: bool,
config: Config,
seen_pixels: [64]Pixel = std.mem.zeroes([64]Pixel),
last_pixel: Pixel = .{ .r = 0, .g = 0, .b = 0, .a = 255 },
run_length: u8 = 0,

pub fn init(track_stats: bool, config: Config) QoiEncoder {
    return QoiEncoder{
        .track_stats = track_stats,
        .config = config,
    };
}

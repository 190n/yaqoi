const std = @import("std");
const Stats = @import("./stats.zig");
const QOIEncoder = @This();

pub const Channels = enum(u8) { rgb = 3, rgba = 4 };

pub const Colorspace = enum(u8) { srgb_gamma = 0, srgb_linear = 1 };

pub const Pixel = extern struct { r: u8, g: u8, b: u8, a: u8 };

pub const Config = struct {
    width: u32,
    height: u32,
    channels: Channels,
    colorspace: Colorspace,
};

const PixelDifference = struct {
    r: i9,
    g: i9,
    b: i9,
    a: i9,

    /// calculate y - x
    pub fn init(x: Pixel, y: Pixel) PixelDifference {
        return .{ .r = y.r - x.r, .g = y.g - x.g, .b = y.b - x.b, .a = y.a - x.a };
    }
};

const Header = packed struct {
    magic: [4]u8 = "qoif".*,
    width: u32,
    height: u32,
    channels: Channels,
    colorspace: Colorspace,

    pub fn init(width: u32, height: u32, channels: Channels, colorspace: Colorspace) Header {
        return .{
            .width = std.mem.nativeToBig(u32, width),
            .height = std.mem.nativeToBig(u32, height),
            .channels = channels,
            .colorspace = colorspace,
        };
    }
};

test "Header" {
    try std.testing.expectEqual(@as(usize, 14), @sizeOf(Header));
    try std.testing.expectEqual([_]u8{
        'q', 'o', 'i', 'f', // magic
        0x00, 0x00, 0x00, 0xff, // width
        0x00, 0x00, 0x00, 0x80, // height
        3, // channels
        1, // colorspace
    }, @bitCast([14]u8, Header.init(0xff, 0x80, .rgb, .srgb_linear)));
}

const OpRGB = packed struct {
    tag: u8 = 0b11111110,
    r: u8,
    g: u8,
    b: u8,

    pub fn init(pixel: Pixel) OpRGB {
        return .{ .r = pixel.r, .g = pixel.g, .b = pixel.b };
    }
};

const OpRGBA = packed struct {
    tag: u8 = 0b11111111,
    r: u8,
    g: u8,
    b: u8,
    a: u8,

    pub fn init(pixel: Pixel) OpRGB {
        return .{ .r = pixel.r, .g = pixel.g, .b = pixel.b, .a = pixel.a };
    }
};

const OpIndex = packed struct {
    tag: u2 = 0b00,
    index: u6,
};

const OpDiff = packed struct {
    tag: u2 = 0b01,
    dr: u2,
    dg: u2,
    db: u2,

    /// create an OpDiff representing the given pixel difference if the difference is small enough
    /// to be represented
    pub fn init(diff: PixelDifference) ?OpDiff {
        if (diff.a != 0) return null;
        inline for ([_]i9{ diff.r, diff.g, diff.b }) |channel| {
            if (channel < -2 or channel > 1) return null;
        }

        return OpDiff{
            .dr = @intCast(u2, diff.r + 2),
            .dg = @intCast(u2, diff.g + 2),
            .db = @intCast(u2, diff.b + 2),
        };
    }
};

const OpLuma = packed struct {
    tag: u2 = 0b10,
    dg: u6,
    dr_dg: u4,
    db_dg: u4,

    /// create an OpLuma representing the given pixel difference if the difference is small enough
    /// to be represented
    pub fn init(diff: PixelDifference) ?OpLuma {
        if (diff.a != 0) return null;
        if (diff.g < -32 or diff.g > 31) return null;
        const dr_dg = diff.r - diff.g;
        const db_dg = diff.b - diff.g;
        inline for ([_]i9{ dr_dg, db_dg }) |channel| {
            if (channel < -8 or channel > 7) return null;
        }
        return OpLuma{
            .dg = @intCast(u6, diff.g + 32),
            .dr_dg = @intCast(u4, dr_dg + 8),
            .db_dg = @intCast(u4, db_dg + 8),
        };
    }
};

const OpRun = packed struct {
    tag: u2 = 0b11,
    run_length: u6,

    pub fn init(run_length: u7) OpRun {
        std.debug.assert(run_length >= 1 and run_length <= 62);
        return OpRun{
            .run_length = @intCast(u6, run_length - 1),
        };
    }
};

const end_marker = [1]u8{0} ** 7 ++ [1]u8{1};

stats: Stats = .{},
track_stats: bool,
config: Config,
seen_pixels: [64]Pixel = std.mem.zeroes([64]Pixel),
last_pixel: Pixel = .{ .r = 0, .g = 0, .b = 0, .a = 255 },
run_length: u8 = 0,

pub fn init(track_stats: bool, config: Config) QOIEncoder {
    return QOIEncoder{
        .track_stats = track_stats,
        .config = config,
    };
}

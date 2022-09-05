const clap = @import("clap");
const std = @import("std");
const stb_image = @import("./stb_image.zig");
const QoiEncoder = @import("./qoi_encoder.zig");
const Stats = @import("./stats.zig");

const EncoderThread = struct {
    thread: std.Thread,
    encoder: QoiEncoder,
    pixels: []const QoiEncoder.Pixel,
    bytes: std.ArrayList(u8),

    pub fn run(self: *EncoderThread) !void {
        var buf = std.io.bufferedWriter(self.bytes.writer());
        for (self.pixels) |p| {
            try self.encoder.addPixel(buf.writer(), p);
        }
        try self.encoder.finish(buf.writer());
        try buf.flush();
    }
};

fn addStructs(comptime T: type, dst: *T, src: T) void {
    inline for (@typeInfo(T).Struct.fields) |f| {
        switch (@typeInfo(f.field_type)) {
            .Int, .Float => @field(dst, f.name) += @field(src, f.name),
            .Struct => addStructs(f.field_type, &@field(dst, f.name), @field(src, f.name)),
            else => @compileError("bad field type passed to addStructs: " ++ @typeName(f.field_type)),
        }
    }
}

pub fn main() !void {
    var arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena.deinit();
    const ally = arena.allocator();
    stb_image.allocator = ally;

    const stderr = std.io.getStdErr().writer();

    const params = comptime clap.parseParamsComptime(
        \\-h, --help               display help
        \\-l, --linear-srgb        indicate that output file is linear sRGB, not gamma (no actual conversion is done)
        \\-v, --verbose            show encoding statistics
        \\-i, --input <filename>   input file, default stdin
        \\-o, --output <filename>  output file, default stdout
        \\-t, --threads <number>   number of threads to use, default 1
    );

    var diag = clap.Diagnostic{};
    var res = clap.parse(clap.Help, &params, comptime .{
        .filename = clap.parsers.string,
        .number = clap.parsers.int(usize, 10),
    }, .{
        .diagnostic = &diag,
    }) catch |err| {
        diag.report(stderr, err) catch {};
        std.os.exit(1);
    };
    defer res.deinit();

    if (res.args.help) {
        return clap.help(stderr, clap.Help, &params, .{});
    }

    const input = if (res.args.input) |filename| std.fs.cwd().openFile(filename, .{}) catch |err| {
        std.log.err("opening input '{s}': {s}", .{ filename, @errorName(err) });
        std.os.exit(1);
    } else std.io.getStdIn();
    defer input.close();

    const output = if (res.args.output) |filename| std.fs.cwd().createFile(filename, .{
        // if input is a file, match its permissions, otherwise use default
        // note that this is also modified by the umask
        .mode = if ((try input.metadata()).kind() == .File) (try input.mode()) else std.fs.File.default_mode,
    }) catch |err| {
        std.log.err("opening output '{s}': {s}", .{ filename, @errorName(err) });
        std.os.exit(1);
    } else std.io.getStdOut();
    defer output.close();

    const linear_srgb = res.args.@"linear-srgb";
    const verbose = res.args.verbose;
    const num_threads = res.args.threads orelse 1;

    var result = stb_image.load(&input, .rgba);
    defer result.deinit();
    if (result == .err) {
        const quote = if (res.args.input) |_| "'" else "";
        const filename = res.args.input orelse "[stdin]";
        std.log.err("reading input {s}{s}{s}: {s}", .{
            quote,
            filename,
            quote,
            result.err,
        });
        std.os.exit(1);
    }

    const config = QoiEncoder.Config{
        .width = std.math.cast(u32, result.ok.x) orelse {
            std.log.err("image dimensions too large", .{});
            std.os.exit(1);
        },
        .height = std.math.cast(u32, result.ok.y) orelse {
            std.log.err("image dimensions too large", .{});
            std.os.exit(1);
        },
        .channels = switch (result.ok.channels_in_file) {
            .grey, .rgb => .rgb,
            .grey_alpha, .rgba => .rgba,
        },
        .colorspace = if (linear_srgb) .srgb_linear else .srgb_gamma,
    };

    const pixels = @ptrCast([*]const QoiEncoder.Pixel, result.ok.data.ptr)[0..(result.ok.x * result.ok.y)];

    const threads = try ally.alloc(EncoderThread, num_threads);
    defer ally.free(threads);

    var timer = try std.time.Timer.start();

    for (threads) |*t, i| {
        const start = pixels.len * i / num_threads;
        const end = pixels.len * (i + 1) / num_threads;
        t.* = .{
            .thread = undefined,
            .encoder = QoiEncoder.init(verbose, config),
            .pixels = pixels[start..end],
            .bytes = std.ArrayList(u8).init(ally),
        };
        t.thread = try std.Thread.spawn(.{}, EncoderThread.run, .{t});
    }

    // finish encoding
    for (threads) |*t| {
        t.thread.join();
    }

    const elapsed_ns = timer.read();

    // write every section of the image
    try output.writeAll(&QoiEncoder.chunkToBytes(threads[0].encoder.header));
    for (threads) |t| {
        try output.writeAll(t.bytes.items);
    }
    try output.writeAll(&QoiEncoder.end_marker);

    if (verbose) {
        var sum = Stats{};
        for (threads) |t| {
            addStructs(Stats, &sum, t.encoder.stats);
        }
        try std.json.stringify(.{
            .ns = elapsed_ns,
            .mpix_per_s = @intToFloat(f64, sum.total_pixels) / @intToFloat(f64, elapsed_ns) * 1000,
            .bpp = @intToFloat(f64, sum.total_bits) / @intToFloat(f64, sum.total_pixels),
            .encoding = sum,
        }, .{ .whitespace = .{} }, std.io.getStdErr().writer());
    }
}

test {
    std.testing.refAllDecls(@This());
    std.testing.refAllDecls(QoiEncoder);
}

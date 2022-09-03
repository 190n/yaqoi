const clap = @import("clap");
const std = @import("std");
const stb_image = @import("./stb_image.zig");
const QoiEncoder = @import("./qoi_encoder.zig");

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
    const threads = res.args.threads orelse 1;
    _ = verbose;
    _ = threads;

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

    const header = QoiEncoder.Header.init(
        std.math.cast(u32, result.ok.x) orelse {
            std.log.err("image dimensions too large", .{});
            std.os.exit(1);
        },
        std.math.cast(u32, result.ok.y) orelse {
            std.log.err("image dimensions too large", .{});
            std.os.exit(1);
        },
        switch (result.ok.channels_in_file) {
            .grey, .rgb => .rgb,
            .grey_alpha, .rgba => .rgba,
        },
        if (linear_srgb) .srgb_linear else .srgb_gamma,
    );
    try output.writeAll(&QoiEncoder.chunkToBytes(header));

    const pixels = @ptrCast([*]QoiEncoder.Pixel, result.ok.data.ptr)[0..(result.ok.x * result.ok.y)];
    var encoder = QoiEncoder.init(false, header);
    for (pixels) |p| {
        try encoder.addPixel(output.writer(), p);
    }
    try encoder.finish(output.writer());
    try output.writeAll(&QoiEncoder.end_marker);
}

test {
    std.testing.refAllDecls(@This());
    std.testing.refAllDecls(QoiEncoder);
}

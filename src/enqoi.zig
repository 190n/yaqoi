const clap = @import("clap");
const std = @import("std");
const stb_image = @import("./stb_image.zig");
const QOIEncoder = @import("./qoi_encoder.zig");

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
    _ = linear_srgb;
    _ = verbose;
    _ = threads;

    var result = stb_image.load(&input, null);
    defer result.deinit();
    if (result == .ok) {
        std.log.info("{}x{}, {} channels", .{
            result.ok.x,
            result.ok.y,
            @enumToInt(result.ok.channels),
        });
    } else {
        const quote = if (res.args.input) |_| "'" else "";
        const filename = res.args.input orelse "[stdin]";
        std.log.err("reading input {s}{s}{s}: {s}", .{
            quote,
            filename,
            quote,
            result.err,
        });
    }
}

test {
    std.testing.refAllDecls(@This());
}

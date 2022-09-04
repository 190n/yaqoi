const std = @import("std");

pub fn build(b: *std.build.Builder) void {
    // Standard target options allows the person running `zig build` to choose
    // what target to build for. Here we do not override the defaults, which
    // means any target is allowed, and the default is native. Other options
    // for restricting supported target set are available.
    const target = b.standardTargetOptions(.{});

    // Standard release options allow the person running `zig build` to select
    // between Debug, ReleaseSafe, ReleaseFast, and ReleaseSmall.
    const mode = b.standardReleaseOptions();

    const exe = b.addExecutable("enqoi", "src/enqoi.zig");
    // TODO only strip in release builds (if zig doesn't already do that)
    // exe.strip = true; // since we disable a lot of stb_image stuff
    const exe_tests = b.addTestExe("enqoi-tests", "src/enqoi.zig");

    for ([_]*std.build.LibExeObjStep{ exe, exe_tests }) |step| {
        step.setTarget(target);
        step.setBuildMode(mode);
        step.addPackagePath("clap", "vendor/zig-clap/clap.zig");
        step.addIncludePath("vendor/stb");
        step.linkLibC();
        step.addCSourceFile("src/stbi.c", &.{});
        step.install();
    }

    const run_cmd = exe.run();
    run_cmd.step.dependOn(b.getInstallStep());
    run_cmd.expected_exit_code = null;
    if (b.args) |args| {
        run_cmd.addArgs(args);
    }

    const run_step = b.step("run", "Run the app");
    run_step.dependOn(&run_cmd.step);

    const test_step = b.step("test", "Run unit tests");
    test_step.dependOn(&exe_tests.run().step);

    const format_step = b.step("format", "Format all source files");
    const zig_fmt = b.addSystemCommand(&.{ "sh", "-c", "zig fmt build.zig src/*.zig" });
    format_step.dependOn(&zig_fmt.step);
    const clang_format = b.addSystemCommand(&.{ "sh", "-c", "clang-format -i -style=file src/*.[ch]" });
    format_step.dependOn(&clang_format.step);
}
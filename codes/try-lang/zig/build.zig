const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const exe = b.addExecutable(.{
        .name = "zig",
        .root_source_file = .{ .path = "src/main.zig" },
        .target = target,
        .optimize = optimize,
    });
    b.installArtifact(exe);

    // these are build-time constants injected into the build
    const exe_options = b.addOptions();
    exe.addOptions("build_options", exe_options);
    exe_options.addOption(u32, "buildtime_answer_to_the_ultimate_question", 42);
    exe_options.addOption([]const u8, "ping", "pong");
}

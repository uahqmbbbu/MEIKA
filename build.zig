const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const exe = b.addExecutable(.{
        .name = "meika-convert",
        .root_module = b.createModule(.{
            .root_source_file = null,
            .target = target,
            .optimize = optimize,
            .link_libcpp = true,
        }),
    });
    exe.root_module.addIncludePath(b.path("."));   // meika headers
    exe.root_module.addCSourceFile(.{ .file = b.path("convert/entrance.cpp"), .flags = &.{"-std=c++17"} });

    b.installArtifact(exe);
}

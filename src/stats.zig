total_pixels: usize = 0,
total_bits: usize = 0,
pixels_per_op: PixelsPerOp = .{},

const PixelsPerOp = struct {
    rgb: usize = 0,
    rgba: usize = 0,
    index: usize = 0,
    diff: usize = 0,
    luma: usize = 0,
    run: usize = 0,
};

const std = @import("std");

// In C we can do stuff like this:
// #define CHOICES \
//    C(FOO) \
//    C(BAR) \
//    C(BAZ)
// Then you can generate an enum with this #define-list:
// enum choices {
//    #define C(c) c,
//    CHOICES
//    #undef C
// };
// Enum-to-string conversion can be done like this:
// static const char* choice_names[] = {
//    #define C(c) #c,
//    CHOICES
//    #undef C
// };
// Or you can do it with switch/case:
// switch (choice) {
//    #define C(c) case c: return #c;
//    CHOICES
//    #undef C
// }
// In zig @tagName() converts an enum value to a string
fn eval0() void {
    const Choice = enum {
        foo,
        bar,
        baz,
    };

    const choice_list = [_]Choice{
        Choice.foo,
        Choice.bar,
        Choice.baz,
        Choice.bar,
    };
    for (choice_list, 0..) |choice, i| {
        std.debug.print("Choice #{d} is {s}\n", .{ i + 1, @tagName(choice) });
    }

    // and it works with unions!
    const ChoiceWithData = union(enum) {
        foo: i32,
        bar: f32,
        baz: bool,
    };

    // const choice_with_data_list = [_]ChoiceWithData{
    //    ChoiceWithData{.foo = 420},
    //    ChoiceWithData{.bar = 666.67},
    //    ChoiceWithData{.baz = true},
    // };
    const choice_with_data_list = [_]ChoiceWithData{ // shorter syntax!
        .{ .foo = 420 },
        .{ .bar = 666.67 },
        .{ .baz = true },
    };
    for (choice_with_data_list, 0..) |choice_with_data, i| {
        std.debug.print("Choice (with data) #{d} is a {s}-tag having the value {}\n", .{ i + 1, @tagName(choice_with_data), choice_with_data });
    }
}

// #define-lists in C can generally be used to define lists of mixed-type data,
// and re-use the list in multiple contexts at compile-time. How does Zig
// compare?
const eval1_tuple_definitions = .{
    .{ 1, 10, "ones" },
    .{ 2, 20, "twos" },
};
fn eval1() void {
    std.debug.print("tuples debug view; n={d}; n[0]={d}; {}\n", .{
        eval1_tuple_definitions.len,
        eval1_tuple_definitions[0].len,
        eval1_tuple_definitions,
    });

    // TODO illustrate what I can do with eval1_tuple_definitions compared to /
    // exceeding C?
}

// #defines can be (ab)used as local/inline functions in C. Zig doesn't have
// local functions (non-closure/non-capturing local functions might enter the
// language at some point?), but it's still possible to have local struct types
// with functions.
fn eval2() void {
    const x = 42;
    const y = 10;
    const z = x * y;
    const Counter = struct {
        count0: i32 = 0,
        acc1: i32 = 0,
        pub fn inc0(self: anytype) void { // XXX I get `error: use of undeclared identifier 'Counter'` if I try `Counter` instead of `anytype`; why?
            self.count0 += 1;
        }
        pub fn add1(self: anytype, n: i32) void {
            self.acc1 += n;
        }
        pub fn report(self: anytype) void {
            // we cannot "capture" mutable variables, but consts are fine (and
            // consts are more powerful than usual in Zig)
            std.debug.print("counter is now {} (const \"capture\": {}×{}={})\n", .{ self, x, y, z });
        }
    };
    var c = Counter{};

    c.report();
    c.inc0();
    c.inc0();
    c.add1(400);
    c.add1(20);
    c.inc0();
    c.report();
}

const Eval3Err = error{
    IsNotDivisibleBySeven,
};

fn is_divisible_by_7(v: i32) bool {
    return @rem(v, 7) == 0;
}

fn must_be_divisible_by_7(v: i32) anyerror!void {
    if (!is_divisible_by_7(v)) {
        return error.IsNotDivisibleBySeven;
    }
}

fn check_error_at_location(x: anyerror!void, loc: std.builtin.SourceLocation) void {
    if (x) {
        std.debug.print("nothing to see here at {s}:{d}\n", .{ loc.file, loc.line });
    } else |err| {
        std.debug.print("'{s}'-error at {s}:{d}\n", .{ @errorName(err), loc.file, loc.line });
    }
}

fn eval3() void {
    // This test is inspired by __FILE__/__LINE__ macro-stuff in C. `@src()`
    // has to be typed/passed explicitly from here to know where we are in the
    // source, but it's good it's short and not like @getSourceLocationObject().
    // NOTE: the error-type can also have a stack trace, but this is only
    // available in debug/safe builds, and disabled in fast/small release
    // builds.
    check_error_at_location(must_be_divisible_by_7(21), @src());
    check_error_at_location(must_be_divisible_by_7(20), @src());
}

fn eval4_apply(comptime v: anytype) void {
    const things = [_]i32{ 11, 222 };
    for (things) |i| {
        v.do_thing_with_thing(i);
    }
    const other_things = .{ 3333, 4242 };
    inline for (other_things) |i| {
        v.do_thing_with_thing(i);
    }
}

fn eval4() void {
    // "always_inline" (and also C-macros) can be used to "inject code" into
    // the middle of a function without any performance degradation. This type
    // of "generics" is easily supported by Zig:
    const T0 = struct {
        t0_stuff: i32,
        pub fn do_thing_with_thing(self: anytype, i: i32) void {
            std.debug.print("T0({}) is doing thing {d}\n", .{ self, i });
        }
    };
    const T1 = struct {
        t1_is_awesome: bool,
        pub fn do_thing_with_thing(self: anytype, i: i32) void {
            std.debug.print("T1({}) is doin' thing {d}\n", .{ self, i });
        }
    };
    eval4_apply(T0{ .t0_stuff = 666 });
    eval4_apply(T1{ .t1_is_awesome = true });
}

const Eval5Err = error{
    Bang,
};
fn eval5_bang(x: i32) !void {
    const good_xs = [_]i32{ 420, 666 };
    for (good_xs) |good_x| {
        if (x == good_x) {
            std.debug.print("e5 {d} is good\n", .{good_x});
            return;
        }
    }
    std.debug.print("e5 {d} is BANG\n", .{x});
    return error.Bang;
}

fn eval5_inner() !void {
    // "try" allows errors to bubble up; in C I suppose the alternatives are
    // things like:
    //  - error handling bottled into a macro, like:
    //     `#define CHKERR if (err) return -1;`
    //  - same but explicit (not inlined with a macro)
    //  - error handling using goto
    defer {
        std.debug.print("(also hello from a \"normal\" defer block in eval5_inner()\n", .{});
    }
    errdefer {
        std.debug.print("eval5_inner() is terminating early due to error (caught by errdefer block)\n", .{});
    }
    try eval5_bang(420);
    try eval5_bang(666);
    try eval5_bang(1);
    try eval5_bang(2); // never reached
}

fn eval5() void {
    if (eval5_inner()) {
        // OK
    } else |err| {
        std.debug.print("caught {s} error\n", .{@errorName(err)});
    }
}

fn eval6_passed(buf: []u8) void {
    std.debug.print("passed buf.len={d}\n", .{buf.len});
}

fn eval6() void {
    var buf: [420666]u8 = undefined;
    std.debug.print("buf.len={d}\n", .{buf.len});
    eval6_passed(&buf);
}

fn SAFE_CALL(comptime f: anytype) void {
    f();
}

fn BANG_CALL(comptime f: anytype) void {
    f() catch |err| std.debug.print("ERR={s}", .{@errorName(err)});
}

fn T(comptime f: anytype) void {
    const rt = @typeInfo(@typeInfo(@TypeOf(f)).Fn.return_type orelse unreachable);
    const ff = comptime switch (rt) {
        std.builtin.Type.Void => &SAFE_CALL,
        else => &BANG_CALL,
    };
    std.debug.print("T {{{{\n", .{});
    ff(f);
    std.debug.print("}}}} T\n", .{});
}

var allocator0: std.mem.Allocator = undefined;
var allocator1: std.mem.Allocator = undefined;

fn make_baked_allocator_type(comptime allocator_ptr: *std.mem.Allocator) type {
    return struct {
        xs: []i32 = &[_]i32{},
        pub fn baked_alloc(self: anytype, n: usize) !void {
            self.xs = try allocator_ptr.*.alloc(@TypeOf(self.xs[0]), n);
            std.debug.print("xs is now {d} elements\n", .{self.xs.len});
        }
    };
}

const TraceAllocator = struct {
    const Self = @This();
    a: std.mem.Allocator,
    pub fn allocator(self: *Self) std.mem.Allocator {
        return .{
            .ptr = self,
            .vtable = &.{
                .alloc = alloc,
                .resize = resize,
                .free = free,
            },
        };
    }

    pub fn alloc(
        ctx: *anyopaque,
        n: usize,
        log2_ptr_align: u8,
        ret_addr: usize,
    ) ?[*]u8 {
        const self = @ptrCast(*Self, @alignCast(@alignOf(Self), ctx));
        std.debug.print("TraceAllocator::alloc({d},{d})\n", .{ n, log2_ptr_align });
        return self.a.vtable.alloc(self.a.ptr, n, log2_ptr_align, ret_addr);
    }

    pub fn resize(
        ctx: *anyopaque,
        buf: []u8,
        log2_buf_align: u8,
        new_len: usize,
        ret_addr: usize,
    ) bool {
        const self = @ptrCast(*Self, @alignCast(@alignOf(Self), ctx));
        std.debug.print("TraceAllocator::resize({d},{d})\n", .{ log2_buf_align, new_len });
        return self.a.vtable.resize(self.a.ptr, buf, log2_buf_align, new_len, ret_addr);
    }

    pub fn free(
        ctx: *anyopaque,
        buf: []u8,
        log2_buf_align: u8,
        ret_addr: usize,
    ) void {
        const self = @ptrCast(*Self, @alignCast(@alignOf(Self), ctx));
        std.debug.print("TraceAllocator::free()\n", .{});
        self.a.vtable.free(self.a.ptr, buf, log2_buf_align, ret_addr);
    }
};

fn eval7() !void {
    // zig has std.mem.Allocator, which is nice, but having each value that
    // needs an allocator store one (16 bytes) or a pointer to a shared one (8
    // bytes) might seem a bit too fine-grained and excessive? (e.g. see
    // std.ArrayList). the problem with C is that:
    //  - the only standard is malloc()/free() (and mmap() etc to a lesser
    //    extent)
    //  - designing libraries to NOT depend on malloc()/free() is challenging
    //    and non-standard; you could go full-on fnptr interface, but sometimes
    //    macros are more convenient, but this requires recompiling your
    //    dependency (which is not always feasible).
    // this example shows that there's a middle ground between "ONE ALLOCATOR
    // TO RULE THEM ALL" and "ONE ALLOCATOR PER VALUE": the _address_ of an
    // allocator is known at comptime, so this is baked into the type, but the
    // allocator is still initialized at runtime.
    const T0 = make_baked_allocator_type(&allocator0);
    const T1 = make_baked_allocator_type(&allocator1);

    var t0 = T0{};
    var t1 = T1{};

    var test_slice: []i32 = undefined;
    std.debug.print("sizeof(t0)={d} / sizeof(t1)={d} (types should only contain a single slice, which, for reference, should be {d} bytes)\n", .{
        @sizeOf(@TypeOf(t0)),
        @sizeOf(@TypeOf(t1)),
        @sizeOf(@TypeOf(test_slice)),
    });

    var X0 = std.heap.GeneralPurposeAllocator(.{}){};

    var X1 = std.heap.GeneralPurposeAllocator(.{}){};
    var Y1 = TraceAllocator{ .a = X1.allocator() };

    // notice we're initializing global allocators AFTER depending on them; it
    // should only be the `baked_alloc()` calls below that actually uses the
    // allocator, so we're safe until then.
    allocator0 = X0.allocator();
    allocator1 = Y1.allocator();

    std.debug.print("(NOTE: trace allocator should only \"see 6's\" after division with type size:)\n", .{});
    try t0.baked_alloc(42);
    try t1.baked_alloc(66);
    try t0.baked_alloc(420);
    try t1.baked_alloc(666);
}

pub fn main() !void {
    T(eval0);
    T(eval1);
    T(eval2);
    T(eval3);
    T(eval4);
    T(eval5);
    T(eval6);
    T(eval7);
}

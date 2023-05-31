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

fn T(comptime f: anytype) void {
    // T() is half-way there to being like a C-macro, because `f` is
    // "duck-typed" at compile-time (even if "comptime" is removed).
    //std.debug.print("T {s} {{{{\n", .{@typeName(@TypeOf(f))}); // functions don't know their name; @typeName@(TypeOf(f)) evaluates to "fn() void"
    std.debug.print("\nT {{{{\n", .{}); // "{{" escapes to "{"
    f();
    std.debug.print("}}}} T\n", .{});
}

// TODO can I mimic a stb_ds.h-style dynamic array? i.e. len, cap and data are
// all stored in the same allocation? and "null" is an empty array?

pub fn main() !void {
    T(eval0);
    T(eval1);
    T(eval2);
    T(eval3);
    T(eval4);
    T(eval5);
}

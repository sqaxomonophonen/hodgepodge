const DEFAULT_TOKENIZER_SETUP = {
    whitespace           : " \t",
    newline              : "\n",
    parentheses          : [["(",")"],["[","]"],["{","}"],["<",">"]],
    chain                : "..",
    comment              : "--",
    statement_separator  : ";",
    javascript_escape    : "#",
    state_tag            : "&",
    quotes               : ['"',"'","`"],
    assignment           : "=",
    named_argument       : ":",
    local_variable       : "$",
    global_variable      : "%", // XXX no?
    macro                : "*",
    number_decimal       : ".",
    //number_negative      : "-",
    //number_digit         : [["0","9"]],
    // XXX should number syntax be hardcoded...?
    identifier_set       : ["az","AZ","_","?","!"], // XXX?
};

function resize_typed_array(xs,n) {
    if (n === xs.length) return xs;
    if (n < xs.length) return xs.subarray(0,n);
    let grown_xs = new xs.constructor(n);
    grown_xs.set(xs);
    return grown_xs;
}

function string_to_codepoints(s) {
    let codepoints = new Uint32Array(s.length);
    let n=0;
    for (const c of s) codepoints[n++] = c.codePointAt(0);
    return resize_typed_array(codepoints,n);
}

class FatString {
    constructor(codepoints) {
        this.codepoints = codepoints || (new Uint32Array());
        // TODO rgb, last modified ts/user, source locs
    }

    get_length() { return this.codepoints.length; }
    set_length(n) {
        this.codepoints = resize_typed_array(this.codepoints, n);
        // TODO rgb, last modified ts/user, ...?
    }

    codepoint_at(index) { return this.codepoints[index]; }
         char_at(index) { return String.fromCodePoint(this.codepoint_at(index)); }

    substring(i0,i1) {
        return new this.constructor(
            this.codepoints.subarray(i0,i1)
        );
    }
}


function new_tokenizer(fat_string, setup) {
    if (setup === undefined) setup = DEFAULT_TOKENIZER_SETUP;
    const punctuation_arr = ("parentheses,chain,comment,statement_separator,"+
          "javascript_escape,assignment,named_argument,"+
          "local_variable,global_variable,macro").split(",");
    const punctuation_set = new Set(punctuation_arr);
    const digit_range = string_to_codepoints("09");
    const minus_sign = string_to_codepoints("-");
    let setup_codepoints = {};
    for (const key in setup) {
        function maprec(v) {
            if (typeof v === "string") return string_to_codepoints(v);
            if (v instanceof Array)    return v.map(maprec);
            throw new Error("invalid input: " + JSON.stringify(v));
        };
        setup_codepoints[key] = maprec(setup[key]);
    }

    let cursor = 0;
    let line = 1;
    let line_cursor0 = cursor;
    //let parenthesis_stack = []; // XXX belongs in "parser"?

    function skip_whitespace() {
        for (;;++cursor) {
            const c = fat_string.codepoint_at(cursor)
            if (setup_codepoints.whitespace.indexOf(c) >= 0) continue;
            if (c === setup_codepoints.newline[0]) { ++line; continue; }
            break;
        }
        //while (setup_codepoints.whitespace.indexOf(fat_string.codepoint_at(cursor)) >= 0) ++cursor;
    }

    function match_codepoints(codepoints) {
        for (let i=0; i<codepoints.length; ++i) if (codepoints[i] !== fat_string.codepoint_at[cursor+i]) return false;
        return true;
    }

    function match_codepoints_rec(def) {
        const n = def.length;
        if (n === 0) return null;
        if (typeof def[0] === "number") return match_codepoints(def);
        let r = null;
        for (let i=0; i<def.length; ++i) {
            let subdef = def[i];
            let m = match_codepoints_rec(subdef);
            if (m === true) {
                if (r !== null) throw new Error("multiple punctuation matches: bad tokenizer setup? def=" + JSON.stringify(def));
                r = [i];
            } else if (m) r = [i, ...m];
        }
        return r;
    }

    return () => {
        for (;;) {
            if (cursor >= fat_string.get_length()) return null;
            skip_whitespace();

            const cursor0 = cursor;
            const token_rest = () => ({
                string: fat_string.substring(cursor0, cursor),
                line,
                col: (1+cursor0-line_cursor0),
            });
            let get_codepoint = () => fat_string.codepoint_at(cursor);

            // try matching punctuation
            for (const name of punctuation_arr) {
                let p = match_codepoints_rec(setup_codepoints[name]);
                if (p) {
                    console.log("TODO MATCH:"+name+":"+p); // TODO return token.. advance cursor...
                    process.exit(1);
                }
            }

            for (;;) { // try matching identifier
                let match = false;
                const c = fat_string.codepoint_at(cursor);
                const cp = get_codepoint();
                for (let d of setup_codepoints.identifier_set) {
                    if ((d.length === 1 && d[0] === cp) || ((d.length === 2) && (d[0] <= cp && cp <= d[1]))) {
                        match = true;
                        break;
                    }
                }
                if (match) {
                    ++cursor;
                } else if (cursor === cursor0) {
                    break;
                } else if (cursor > cursor0) {
                    return { type:"identifier", ...token_rest() };
                } else {
                    throw new Error("unreachable");
                }
            }

            for (;;) { // try matching number
                const cp = get_codepoint();
                if (cursor === cursor0 && cp === minus_sign[0]) { ++cursor; continue; }
                if (digit_range[0] <= cp && cp <= digit_range[1])  { ++cursor; continue; }
                if (cp === setup_codepoints.number_decimal) { ++cursor; continue; } // XXX conflict with "chain" operator
                if (cursor > cursor0) return { type:"number", ...token_rest() };
                break;
            }

            // TODO strings
            // TODO comment
            throw new Error("unexpected input " + fat_string.char_at(cursor));
        }
        throw new Error("unreachable");
    };
}

const T = new_tokenizer(new FatString(string_to_codepoints("foo 42 .. bar # 3*3 # .. baz \"bd*4\"")));
for (;;) {
    /*
    let [t,e] = T();
    if (e) throw e;
    */
    let t = T();
    if (!t) break;
    console.log(t);
}


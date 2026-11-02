/* Made by rio_hostHeader (rio -H) from the Rio module that declares these host procs: don't edit.
   A struct parameter comes as 2 words: rio_getStruct(vm, a + i, &s, sizeof s) copies it into s.
   Register the module's source with its procs (_SOURCE, below: rio_addSource), and each proc with
   its _SIG (_HOST_PROCS lists them all). */
#include <stdint.h>
#include "rio.h"


#define core_HOST_PROCS(X)

#define core_HASH 0xa1ab2e37u /* (rio_hashText of the source) */
#define core_SOURCE \
  "# core.rio: the builtins, module core, declared (docs/spec/builtins.md says what they do). Programs\n" \
  "# don't import it: the builtins are always there, and as core.name too. It's what they are, written in Rio, and tests/host.c compiles\n" \
  "# it, so each declaration is checked against the builtin (its parameters and result), and every\n" \
  "# builtin is declared here or in the comments below.\n" \
  "\n" \
  "# ---------------------------------------------------------------- types\n" \
  "\n" \
  "Int* :: host struct      # 32 bits, signed; arithmetic wraps\n" \
  "Float* :: host struct    # 32 bits (IEEE single)\n" \
  "Bool* :: host struct     # true or false, stored as an Int (0 or 1)\n" \
  "String* :: host struct   # a read-only view of bytes: (address, length)\n" \
  "Blob* :: host struct     # a writable view of bytes: (address, length)\n" \
  "\n" \
  "# how format writes values (b.format(settings, ...)); each field's 0 is as print writes it. Built in,\n" \
  "# as declared here (tests/host.c checks it's the same)\n" \
  "Format* :: struct\n" \
  "  width: Int              # numbers: at least this many bytes, padded with spaces (negative: on the right)\n" \
  "  digits: Int             # numbers: at least this many digits before the point, padded with zeros\n" \
  "  places: Int             # Floats: exactly this many digits after the point, rounded\n" \
  "  base: Int               # Ints: 2 to 16, of the bits (-1 is ffffffff in base 16)\n" \
  "  textWidth: Int          # text, Bools and enums' names: as width does for numbers\n" \
  "end\n" \
  "\n" \
  "# a..<b and a..=b: the Ints from first to last (inclusive: empty when last < first). Printed first..=last.\n" \
  "# Built in, as declared here (tests/host.c checks it's the same), with these methods:\n" \
  "#   Range.len* :: host proc() -> Int            # how many values: last - first + 1, or 0\n" \
  "#   Range.has* :: host proc(i: Int) -> Bool     # first <= i <= last\n" \
  "Range* :: struct\n" \
  "  first: Int\n" \
  "  last: Int\n" \
  "end\n" \
  "\n" \
  "# ---------------------------------------------------------------- math\n" \
  "\n" \
  "abs* :: host proc(x: Int) -> Int\n" \
  "abs* :: host proc(x: Float) -> Float\n" \
  "min* :: host proc(a, b: Int) -> Int\n" \
  "min* :: host proc(a, b: Float) -> Float\n" \
  "max* :: host proc(a, b: Int) -> Int\n" \
  "max* :: host proc(a, b: Float) -> Float\n" \
  "clamp* :: host proc(x, lo, hi: Int) -> Int      # max(lo, min(x, hi))\n" \
  "clamp* :: host proc(x, lo, hi: Float) -> Float\n" \
  "\n" \
  "sqrt* :: host proc(x: Float) -> Float\n" \
  "sin* :: host proc(x: Float) -> Float\n" \
  "cos* :: host proc(x: Float) -> Float\n" \
  "tan* :: host proc(x: Float) -> Float\n" \
  "asin* :: host proc(x: Float) -> Float\n" \
  "acos* :: host proc(x: Float) -> Float\n" \
  "atan* :: host proc(x: Float) -> Float\n" \
  "atan2* :: host proc(y, x: Float) -> Float\n" \
  "exp* :: host proc(x: Float) -> Float\n" \
  "ln* :: host proc(x: Float) -> Float      # the natural log\n" \
  "pow* :: host proc(x, y: Float) -> Float\n" \
  "fmod* :: host proc(x, y: Float) -> Float\n" \
  "floor* :: host proc(x: Float) -> Float\n" \
  "ceil* :: host proc(x: Float) -> Float\n" \
  "round* :: host proc(x: Float) -> Float   # halves away from zero\n" \
  "\n" \
  "# ---------------------------------------------------------------- checks and data\n" \
  "\n" \
  "assert* :: host proc(condition: Bool)                # a runtime error at this line when false\n" \
  "assert* :: host proc(condition: Bool, why: String)\n" \
  "hexBlob* :: host proc(digits: String) -> Blob        # a literal only: decoded while compiling\n" \
  "\n" \
  "# ---------------------------------------------------------------- conversions\n" \
  "\n" \
  "Int.toFloat* :: host proc() -> Float\n" \
  "Int.toBool* :: host proc() -> Bool       # n != 0\n" \
  "Float.toInt* :: host proc() -> Int       # toward zero; saturates; NaN is 0\n" \
  "Bool.toInt* :: host proc() -> Int        # 0 or 1\n" \
  "Blob.asString* :: host proc() -> String  # the same bytes, read-only (a Blob[..]'s, or a Blob[N]'s, too)\n" \
  "\n" \
  "# text as a number: nil if it isn't one (docs/spec/expressions.md#text-as-numbers); base 2 to 36, left\n" \
  "# out or 0 for 10\n" \
  "String.toInt* :: host proc(base?: Int) -> ?Int\n" \
  "String.toFloat* :: host proc() -> ?Float\n" \
  "Blob.toInt* :: host proc(base?: Int) -> ?Int\n" \
  "Blob.toFloat* :: host proc() -> ?Float\n" \
  "(Blob[..]).toInt* :: host proc(base?: Int) -> ?Int\n" \
  "(Blob[..]).toFloat* :: host proc() -> ?Float\n" \
  "\n" \
  "# ---------------------------------------------------------------- bytes\n" \
  "\n" \
  "String.len* :: host proc() -> Int\n" \
  "String.cap* :: host proc() -> Int\n" \
  "Blob.len* :: host proc() -> Int\n" \
  "Blob.cap* :: host proc() -> Int\n" \
  "\n" \
  "# Blob[..] is a byte builder view, for text\n" \
  "(Blob[..]).len* :: host proc() -> Int\n" \
  "(Blob[..]).cap* :: host proc() -> Int\n" \
  "(Blob[..]).push* :: host proc(byte: Int) -> Bool          # false when full\n" \
  "(Blob[..]).pushAll* :: host proc(text: String) -> Bool    # all or nothing\n" \
  "(Blob[..]).pop* :: host proc() -> Int                     # a runtime error when empty\n" \
  "(Blob[..]).clear* :: host proc()\n" \
  "(Blob[..]).remove* :: host proc(i: Int)                   # keeps the order\n" \
  "(Blob[..]).swapRemove* :: host proc(i: Int)               # the last element takes its place\n" \
  "\n" \
  "# an int of bits (a constant: 8, 16, 24 or 32; negative: signed) at byte at, little-endian; a runtime\n" \
  "# error past the end (a byte builder's: its length)\n" \
  "String.getInt* :: host proc(at: Int, bits: Int) -> Int\n" \
  "Blob.getInt* :: host proc(at: Int, bits: Int) -> Int\n" \
  "Blob.setInt* :: host proc(at: Int, bits: Int, value: Int)   # its low bits\n" \
  "(Blob[..]).getInt* :: host proc(at: Int, bits: Int) -> Int\n" \
  "(Blob[..]).setInt* :: host proc(at: Int, bits: Int, value: Int)\n" \
  "\n" \
  "# ---------------------------------------------------------------- sequences, for any element type T\n" \
  "#\n" \
  "# Each is checked as the others are, with T standing for a type of its own. A T by value is fine here:\n" \
  "# these are signatures only, and the builtins are compiled as they always were.\n" \
  "\n" \
  "([]T).len* :: host proc[T]() -> Int                  # also on [N]T, [..N]T and [..]T\n" \
  "([..]T).cap* :: host proc[T]() -> Int                # also on [N]T and []T\n" \
  "([..]T).push* :: host proc[T](value: T) -> Bool       # false when full\n" \
  "([..]T).pushAll* :: host proc[T](values: []T) -> Bool # all or nothing\n" \
  "([..]T).pop* :: host proc[T]() -> T                  # a runtime error when empty\n" \
  "([..]T).clear* :: host proc[T]()\n" \
  "([..]T).remove* :: host proc[T](i: Int)              # keeps the order\n" \
  "([..]T).swapRemove* :: host proc[T](i: Int)          # the last element takes its place\n" \
  "([]T).swap* :: host proc[T](i, j: Int)               # also on [N]T, [..N]T and [..]T; not String\n" \
  "\n" \
  "# ---------------------------------------------------------------- not declarable yet\n" \
  "#\n" \
  "# These take any printable or packable value, any enum E, a type, or any number of values: they'd need\n" \
  "# constraints, which generics don't have. Written in a pretend syntax: [T: C] after a name makes it\n" \
  "# generic over types meeting C, and ..T is any number of Ts.\n" \
  "#\n" \
  "#   log*[T: Printable] :: host proc(values: ..T)     # any values; one line, spaces\n" \
  "#                                                     # between\n" \
  "#   (Blob[..]).print*[T: Printable] :: host proc(values: ..T) -> Bool   # as text, all or nothing\n" \
  "#   (Blob[..]).format*[T: Printable] :: host proc(settings: Format, values: ..T) -> Bool\n" \
  "#                                                     # as print, as settings say, inside values too\n" \
  "#   Blob.pack*[T: Packable] :: host proc(at: Int, value: T)   # its bytes at byte at, as in memory\n" \
  "#   (Blob[..]).pack*[T: Packable] :: host proc(value: T) -> Bool   # appended, all or nothing\n" \
  "#   String.unpack*[T: Packable] :: host proc(at: Int, into: &T)    # also Blob's, (Blob[..])'s;\n" \
  "#                                                     # into a []T or [..]T too (its elements), no &\n" \
  "#                                                     # Packable: no views or proc values inside;\n" \
  "#                                                     # a []T or [..]T packs its elements\n" \
  "#   sizeOf*[T] :: host proc(t: T) -> Int          # T's bytes, a constant: sizeOf(T) or sizeOf(x)\n" \
  "#                                                     # (not a view's; a struct's is what pack writes)\n" \
  "#   offsetOf*[S: struct] :: host proc(s: S, path) -> Int   # offsetOf(S, .a.b): the byte where the\n" \
  "#                                                     # field starts (a bitfield's: its lowest bit's)\n" \
  "#   bitOffsetOf*[S: struct] :: host proc(s: S, path) -> Int   # the same, in bits\n" \
  "#   E.fromInt*[E: enum] :: host proc(n: Int) -> E    # a runtime error if n isn't one of E's values\n" \
  "#   E.has*[E: enum] :: host proc(n: Int) -> Bool\n" \
  "#   (E).toInt*[E: enum] :: host proc() -> Int\n"

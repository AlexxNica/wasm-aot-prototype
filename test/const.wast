;; Test that the binary encoding of the dump matches that of the original
;; RUN: sexpr-dump %s > %t1
;; RUN: %sexpr-wasm -d %t1 > %t2
;; RUN: %sexpr-wasm -d %s | diff - %t2
;; Test that round-tripping is stable
;; RUN: sexpr-dump %t1 | diff %t1 -
(module
  (func
    (i32.const 0)
    (i32.const -2147483648)
    (i32.const 4294967295)
    (i32.const -0x80000000)
    (i32.const 0xffffffff)
    (i64.const 0)
    (i64.const -9223372036854775808)
    (i64.const 18446744073709551615)
    (i64.const -0x8000000000000000)
    (i64.const 0xffffffffffffffff)
    (f32.const 0.0)
    (f32.const 1e23)
    (f32.const 1.234567e-5)
    (f64.const 0.0)
    (f64.const -0.987654321)
    (f64.const 6.283185307179586)))

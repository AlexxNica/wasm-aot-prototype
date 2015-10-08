;; Test that the binary encoding of the dump matches that of the original
;; RUN: sexpr_dump %s > %t1
;; RUN: sexpr-wasm -d %t1 > %t2
;; RUN: sexpr-wasm -d %s | diff - %t2
;; Test that round-tripping is stable
;; RUN: sexpr_dump %t1 | diff %t1 -
(module
  (func
    (f32.neg (f32.const 0))
    (f64.neg (f64.const 0))
    (f32.abs (f32.const 0))
    (f64.abs (f64.const 0))
    (f32.sqrt (f32.const 0))
    (f64.sqrt (f64.const 0))
    (i32.clz (i32.const 0))
    (i64.clz (i64.const 0))
    (i32.ctz (i32.const 0))
    (i64.ctz (i64.const 0))
    (i32.popcnt (i32.const 0))
    (i64.popcnt (i64.const 0))
    (f32.ceil (f32.const 0))
    (f64.ceil (f64.const 0))
    (f32.floor (f32.const 0))
    (f64.floor (f64.const 0))
    (f32.trunc (f32.const 0))
    (f64.trunc (f64.const 0))
    (f32.nearest (f32.const 0))
    (f64.nearest (f64.const 0))))

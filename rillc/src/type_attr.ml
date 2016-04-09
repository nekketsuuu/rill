(*
 * Copyright yutopp 2015 - .
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *)

type ref_val_t =
    Ref
  | Val
  | XRef
  | RefValUndef

type mut_t =
    Immutable
  | Const
  | Mutable
  | MutUndef

type lifetime_t =
    Static
  | ThreadLocal
  | Scoped of Lifetime.t
  | LifetimeUndef

type attr_t = {
  ta_ref_val    : ref_val_t;
  ta_mut        : mut_t;
}


let undef = {
  ta_ref_val = RefValUndef;
  ta_mut = MutUndef;
}


let int_of_mut m = match m with
  | Immutable -> 0
  | Const -> 1
  | Mutable -> 2
  | _ -> failwith ""

let mut_of_int n = match n with
  | 0 -> Immutable
  | 1 -> Const
  | 2 -> Mutable
  | _ -> failwith ""

let mut_strong a b =
  mut_of_int (min (int_of_mut a) (int_of_mut b))
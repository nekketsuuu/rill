open Type_sets

module L = Llvm
module LE = Llvm_executionengine

type t = {
  cg_ctx        : Llvm_codegen.ctx_t;
  exec_engine   : LE.llexecutionengine;
}


module JITCounter =
  struct
    let counter = ref Int64.zero

    let generate_fresh_name () =
      let c = !counter in
      counter := Int64.succ !counter;

      "__rill_jit_tmp_expr_" ^ (Int64.to_string c)
  end


let initialize type_gen =
  if not (LE.initialize ()) then
    failwith "[ICE] Couldn't initialize LLVM backend";

  let module CgCtx = Llvm_codegen.Ctx in
  let codegen_ctx =
    Llvm_codegen.make_default_context ~opt_type_gen:(Some type_gen) () in
  Llvm_codegen.inject_builtins codegen_ctx;

  let llmod = codegen_ctx.CgCtx.ir_module in
  let jit_engine = LE.create llmod in
  {
    cg_ctx = codegen_ctx;
    exec_engine = jit_engine;
  }


let invoke_function engine fname ret_ty type_sets =
  let {
    Type.ty_cenv = cenv;
    _
  } = Type.as_unique ret_ty in
  let open Ctypes in
  ignore cenv;
  if true then
    begin
      let cfunc_ty = Ctypes.void @-> returning Ctypes.int64_t in
      let jit_f =
        LE.get_function_address fname
                                (Foreign.funptr cfunc_ty)
                                engine.exec_engine
      in
      let tv = jit_f () in
      Printf.printf "=========> %s\n" (Int64.to_string tv);
    end else
    begin
      failwith "not supported"
    end


let execute engine expr_node expr_ty type_sets =
  let module CgCtx = Llvm_codegen.Ctx in
  (* TODO: add cache... *)
  Printf.printf "JIT ==== execute!!!\n";

  (* save the module which has previous definitions to JIT engine *)
  LE.add_module engine.cg_ctx.CgCtx.ir_module engine.exec_engine;

  (* generate a new module to define a temporary function for CTFE *)
  Llvm_codegen.regenerate_module engine.cg_ctx;

  (* alias *)
  let ir_ctx = engine.cg_ctx.CgCtx.ir_context in
  let ir_mod = engine.cg_ctx.CgCtx.ir_module in
  let ir_builder = engine.cg_ctx.CgCtx.ir_builder in

  (**)
  let expr_llty = Llvm_codegen.lltype_of_typeinfo ~bb:None expr_ty engine.cg_ctx in

  (**)
  let tmp_expr_fname = JITCounter.generate_fresh_name () in

  (* declare temporary funtion : unit -> (typeof expr) *)
  let f_ty = L.function_type expr_llty [||] in
  let f = L.declare_function tmp_expr_fname f_ty ir_mod in

  let bb = L.append_block ir_ctx "entry" f in
  L.position_at_end bb ir_builder;

  (* generate a LLVM value from the expression *)
  let expr_llval =
    Llvm_codegen.code_generate_as_value ~bb:(Some bb) expr_node engine.cg_ctx in
  ignore @@ L.build_ret expr_llval ir_builder;

  Llvm_analysis.assert_valid_function f;
  L.dump_value f;   (* debug *)

  (**)
  LE.add_module ir_mod engine.exec_engine;

  (**)
  invoke_function engine tmp_expr_fname expr_ty type_sets;

  (* Remove the module for this tmporary function from execution engine.
   * However, the module will be used until next time.
   *)
  LE.remove_module ir_mod engine.exec_engine;
  L.delete_function f;

  L.dump_module engine.cg_ctx.CgCtx.ir_module;   (* debug *)
  ()

#pragma once

#ifndef _TRITON_SELECTION_GENERATOR_H_
#define _TRITON_SELECTION_GENERATOR_H_

#include "triton/ir/visitor.h"
#include "triton/codegen/analysis/layout.h"
#include <functional>

// forward
namespace llvm{
  class Type;
  class Value;
  class BasicBlock;
  class Attribute;
  class Instruction;
  class Constant;
  class LLVMContext;
  class Module;
  class ConstantFolder;
  class IRBuilderDefaultInserter;
  template <typename T, typename Inserter>
  class IRBuilder;
  class ArrayType;
  class Function;
}

namespace triton{

namespace ir{
class attribute;
class load_inst;
class store_inst;
}

namespace codegen{

// forward
namespace analysis{
class liveness;
class tiles;
class align;
class allocation;
class cts;
class axes;
class layouts;
class swizzle;
}
// typedef
typedef llvm::IRBuilder<llvm::ConstantFolder,
                        llvm::IRBuilderDefaultInserter> Builder;
typedef llvm::LLVMContext LLVMContext;
typedef llvm::Type Type;
typedef llvm::Value Value;
typedef llvm::Attribute Attribute;
typedef llvm::BasicBlock BasicBlock;
typedef llvm::Module Module;
typedef llvm::Instruction Instruction;
typedef llvm::Constant Constant;
typedef llvm::ArrayType ArrayType;
typedef llvm::Function Function;
typedef std::vector<Value*> indices_t;
class target;

}
}

namespace triton{
namespace codegen{

struct distributed_axis {
  int contiguous;
  std::vector<Value*> values;
  Value* thread_id;
};

class generator: public ir::visitor, public analysis::layout_visitor {
private:
  void init_idx(ir::value *x);
  Instruction* add_barrier();
  Value* shared_off(const std::vector<unsigned>& shapes, const std::vector<int>& order, indices_t idx);
  void finalize_shared_layout(analysis::shared_layout*);
  void finalize_function(ir::function*);
  void finalize_phi_node(ir::phi_node*);

private:
  Type *cvt(ir::type *ty);
  llvm::Attribute cvt(ir::attribute attr);

public:
  generator(analysis::axes *a_axes,
            analysis::layouts *layouts,
            analysis::align *alignment,
            analysis::allocation *alloc,
            analysis::swizzle *swizzle,
            target *tgt,
            unsigned num_warps);

  void visit_value(ir::value* v);
  void visit_phi_node(ir::phi_node*);
  void visit_binary_operator(ir::binary_operator*);
  void visit_getelementptr_inst(ir::getelementptr_inst*);
  void visit_icmp_inst(ir::icmp_inst*);
  void visit_fcmp_inst(ir::fcmp_inst*);
  void visit_cast_inst(ir::cast_inst*);
  void visit_return_inst(ir::return_inst*);
  void visit_cond_branch_inst(ir::cond_branch_inst*);
  void visit_uncond_branch_inst(ir::uncond_branch_inst*);
  void visit_load_inst(ir::load_inst*);
  void visit_unmasked_load_inst(ir::unmasked_load_inst*);
  void visit_masked_load_inst(ir::masked_load_inst*);
  void visit_store_inst(ir::store_inst*);
  void visit_unmasked_store_inst(ir::unmasked_store_inst*);
  void visit_masked_store_inst(ir::masked_store_inst*);
  void visit_reshape_inst(ir::reshape_inst*);
  void visit_splat_inst(ir::splat_inst*);
  void visit_broadcast_inst(ir::broadcast_inst*);
  void visit_downcast_inst(ir::downcast_inst*);
  void visit_exp_inst(ir::exp_inst*);
  void visit_log_inst(ir::log_inst*);
  void visit_get_program_id_inst(ir::get_program_id_inst*);
  void visit_get_num_programs_inst(ir::get_num_programs_inst*);
  void visit_atomic_cas_inst(ir::atomic_cas_inst*);
  void visit_atomic_exch_inst(ir::atomic_exch_inst*);
  void visit_atomic_add_inst(ir::atomic_add_inst*);
  void visit_mma884(ir::dot_inst*, ir::value *A, ir::value *B, ir::value *D, unsigned NK);
  void visit_mma16816(ir::dot_inst*, ir::value *A, ir::value *B, ir::value *D, unsigned NK);
  void visit_fmadot(ir::dot_inst*, ir::value *A, ir::value *B, ir::value *D, unsigned NK, Type *c_ty, Function *f_mul_add);
  void visit_dot_inst(ir::dot_inst*);
  void visit_trans_inst(ir::trans_inst*);
  void visit_sqrt_inst(ir::sqrt_inst*);
  void visit_reduce1d_inst(ir::reduce_inst*, std::function<Value*(Value*,Value*)>, Value*);
  void visit_reducend_inst(ir::reduce_inst*, std::function<Value*(Value*,Value*)>, Value*);
  void visit_reduce_inst(ir::reduce_inst*);
  void visit_select_inst(ir::select_inst*);
  void visit_recoalesce_inst(ir::recoalesce_inst*);
  void visit_masked_load_async_inst(ir::masked_load_async_inst*);
  void visit_copy_to_shared_inst(ir::copy_to_shared_inst*);
  void visit_copy_from_shared_inst(ir::copy_from_shared_inst*);
  void visit_barrier_inst(ir::barrier_inst*);
  void visit_async_wait_inst(ir::async_wait_inst*);
  void visit_make_range_dyn(ir::make_range_dyn*);
  void visit_make_range(ir::make_range*);
  void visit_make_range_sta(ir::make_range_sta*);
  void visit_undef_value(ir::undef_value*);
  void visit_constant_int(ir::constant_int*);
  void visit_constant_fp(ir::constant_fp*);
  void visit_alloc_const(ir::alloc_const*);
  void visit_function(ir::function*);
  void visit_basic_block(ir::basic_block*);
  void visit_argument(ir::argument*);
  void visit(ir::module &, llvm::Module &);

  // layouts
  void visit_layout_mma(analysis::mma_layout*);
  void visit_layout_scanline(analysis::scanline_layout*);
  void visit_layout_shared(analysis::shared_layout*);


private:
  LLVMContext *ctx_;
  Builder* builder_;
  Module *mod_;

  analysis::axes *a_axes_;
  analysis::swizzle *swizzle_;
  std::map<unsigned, distributed_axis> axes_;
  target *tgt_;
  analysis::layouts *layouts_;
  analysis::align *alignment_;
  analysis::allocation *alloc_;
  Value *shmem_;
  unsigned num_warps_;
  std::set<ir::value*> seen_;

  std::map<analysis::data_layout*, Value*> offset_a_m_;
  std::map<analysis::data_layout*, Value*> offset_a_k_;
  std::map<analysis::data_layout*, Value*> offset_b_k_;
  std::map<analysis::data_layout*, Value*> offset_b_n_;

  std::map<analysis::data_layout*, Value*> shared_ptr_;
  std::map<analysis::data_layout*, Value*> shared_pre_ptr_;
  std::map<analysis::data_layout*, Value*> shared_next_ptr_;
  std::map<analysis::data_layout*, Value*> shared_off_;


  std::map<ir::value*, Value*> shmems_;
  std::map<ir::value*, Value*> shoffs_;
  std::map<ir::value*, std::vector<indices_t>> idxs_;
  std::map<ir::value*, std::map<indices_t, Value*>> vals_;
  std::map<ir::value*, BasicBlock *> bbs_;
  std::map<ir::value*, std::vector<int>> ords_;

};

}
}

#endif

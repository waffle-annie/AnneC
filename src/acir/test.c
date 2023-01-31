#include <acir/acir.h>
#include <annec_anchor.h>
#include "cli.h"

AnchAllocator *allocator;
AnchCharWriteStream *wsStdout;
AnchCharWriteStream *wsStderr;

int main(int argc, char *argv[]) {
  AnchFileWriteStream valueWsStdout = {0};
  AnchFileWriteStream_InitWith(&valueWsStdout, stdout);
  wsStdout = &valueWsStdout.stream;
  
  AnchFileWriteStream valueWsStderr = {0};
  AnchFileWriteStream_InitWith(&valueWsStderr, stderr);
  wsStderr = &valueWsStderr.stream;

  AnchDefaultAllocator defaultAllocator;
  AnchDefaultAllocator_Init(&defaultAllocator);

  AnchStatsAllocator statsAllocator;
  AnchStatsAllocator_Init(&statsAllocator, &defaultAllocator);

  allocator = &statsAllocator.base;

  AcirValueType valueTuint64 = { .type = ACIR_VALUE_TYPE_BASIC, .basic = ACIR_BASIC_VALUE_TYPE_UINT64 };

  const AcirValueType *Tuint64 = &valueTuint64;
  
  AcirInstr instrs[] = {
    (AcirInstr){ 0, ACIR_OPCODE_SET, Tuint64, &instrs[1],
      .out = (AcirOperand){ ACIR_OPERAND_TYPE_BINDING, .idx = 0 },
      .val = (AcirOperand){ ACIR_OPERAND_TYPE_IMMEDIATE, Tuint64, .imm.uint64 = 64 }, },
    (AcirInstr){ 1, ACIR_OPCODE_SET, Tuint64, &instrs[2],
      .out = (AcirOperand){ ACIR_OPERAND_TYPE_BINDING, .idx = 1 },
      .val = (AcirOperand){ ACIR_OPERAND_TYPE_IMMEDIATE, Tuint64, .imm.uint64 = 12 }, },
    (AcirInstr){ 2, ACIR_OPCODE_ADD, Tuint64, &instrs[3],
      .out = (AcirOperand){ ACIR_OPERAND_TYPE_BINDING, .idx = 2 },
      .lhs = (AcirOperand){ ACIR_OPERAND_TYPE_BINDING, .idx = 1 },
      .rhs = (AcirOperand){ ACIR_OPERAND_TYPE_BINDING, .idx = 0 }, },
    (AcirInstr){ 3, ACIR_OPCODE_RET, Tuint64, NULL,
      .val = (AcirOperand){ ACIR_OPERAND_TYPE_BINDING, .idx = 2 }, },
  };

#define SEPARATOR "======================="
  puts(ANSI_GRAY "\n" SEPARATOR "[ " ANSI_MAGENTA "Generated IR" ANSI_GRAY " ]" SEPARATOR ANSI_RESET);
  for(int i = 0; i < 4; ++i) {
    AcirInstr_Print(wsStdout, &instrs[i]);
    AnchWriteString(wsStdout, "\n");
  }

  puts(ANSI_GRAY "\n" SEPARATOR "=[ " ANSI_MAGENTA "Validation" ANSI_GRAY " ]=" SEPARATOR ANSI_RESET);
  AcirFunction func = { .type = NULL, .code = instrs, .name = "main" };
  int errorCount = AcirFunction_Validate(&func, allocator);

  if(errorCount > 0) {
    AnchWriteFormat(wsStderr, ANSI_RED "\n%d Errors, Aborting.\n" ANSI_RESET, errorCount);
    return 0;
  } else {
    AnchWriteFormat(wsStdout, ANSI_GREEN "\nNo Errors.\n" ANSI_RESET);
  }

  AcirFunction funcOptimized = { .type = NULL, .code = NULL, .name = "main" };

  AcirBuilder builder;
  AcirBuilder_Init(&builder, &funcOptimized, allocator);

  AcirOptimizer optimizer;
  AcirOptimizer_Init(&optimizer, &(const AcirOptimizer_InitInfo){
    .allocator = allocator,
    .source = &func,
    .builder = &builder
  });

  AcirOptimizer_Analyze(&optimizer);
  // AcirOptimizer_ConstantFold(&optimizer);

  puts(ANSI_GRAY "\n" SEPARATOR "[ " ANSI_MAGENTA "Optimized IR" ANSI_GRAY " ]" SEPARATOR ANSI_RESET);
  for(int i = 0; i < builder.instrCount; ++i) {
    AcirInstr_Print(wsStdout, &builder.instrs[i]);
    AnchWriteString(wsStdout, "\n");
  }

  AcirBuilder_Free(&builder);
  AcirOptimizer_Free(&optimizer);
}

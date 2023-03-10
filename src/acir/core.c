#include <acir/acir.h>
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include "annec_anchor.h"
#include "../cli.h"

#define COMMA ,

#define TMP(A, B, C, D, E, F) { #A, #B, C, D, E, F }
static const struct {
  const char *name;
  const char *mnemonic;
  int operandCount;
  const char *signature;
  const char *description;
  bool consteval;
} AcirOpcode_Info[] = { ACIR_OPCODES_ENUM(TMP, COMMA) };
#undef TMP

#define TMP(A, B, C) { #A, #B, C }
static const struct {
  const char *name;
  const char *mnemonic;
  const char *description;
} AcirBasicValueType_Info[] = { ACIR_BASIC_VALUE_TYPES_ENUM(TMP, COMMA) };
#undef TMP

#undef COMMA

const char *AcirOperandType_Name(AcirOperandType type) {
  switch(type) {
  case ACIR_OPERAND_TYPE_BINDING: return "binding";
  case ACIR_OPERAND_TYPE_IMMEDIATE: return "immediate";
  default: return NULL;
  }
}

const char *AcirOpcode_Name(AcirOpcode opcode) {
  assert(opcode < ACIR_OPCODE_MAX_);
  return AcirOpcode_Info[opcode].name;
}

const char *AcirOpcode_Mnemonic(AcirOpcode opcode) {
  if(opcode >= ACIR_OPCODE_MAX_) return NULL; // ANSI_RED "<bad opcode>" ANSI_RESET;
  return AcirOpcode_Info[opcode].mnemonic;
}

int AcirOpcode_OperandCount(AcirOpcode opcode) {
  if(opcode >= ACIR_OPCODE_MAX_) return -1;
  return AcirOpcode_Info[opcode].operandCount;
}

const char *AcirOpcode_Signature(AcirOpcode opcode) {
  if(opcode >= ACIR_OPCODE_MAX_) return NULL; // ANSI_RED "<bad opcode>" ANSI_RESET;
  return AcirOpcode_Info[opcode].signature;
}

const char *AcirOpcode_Description(AcirOpcode opcode) {
  if(opcode >= ACIR_OPCODE_MAX_) return NULL; // ANSI_RED "<bad opcode>" ANSI_RESET;
  return AcirOpcode_Info[opcode].description;
}

bool AcirOpcode_ConstEval(AcirOpcode opcode) {
  if(opcode >= ACIR_OPCODE_MAX_) return NULL; // false;
  return AcirOpcode_Info[opcode].consteval;
}


const char *AcirBasicValueType_Name(AcirBasicValueType type) {
  if(type >= ACIR_BASIC_VALUE_TYPE_MAX_) NULL; // return ANSI_RED "<bad value type>" ANSI_RESET;
  return AcirBasicValueType_Info[type].name;
}

const char *AcirBasicValueType_Mnemonic(AcirBasicValueType type) {
  if(type >= ACIR_BASIC_VALUE_TYPE_MAX_) return NULL; // ANSI_RED "<bad value type>" ANSI_RESET;
  return AcirBasicValueType_Info[type].mnemonic;
}

const char *AcirBasicValueType_Description(AcirBasicValueType type) {
  if(type >= ACIR_BASIC_VALUE_TYPE_MAX_) return NULL; // ANSI_RED "<bad value type>" ANSI_RESET;
  return AcirBasicValueType_Info[type].description;
}

void AcirInstr_Print(AnchCharWriteStream *out, const AcirInstr *self) {
  if(!self) { AnchWriteFormat(out, ANSI_RED "<null instruction>" ANSI_RESET); return; }
  
  if(self->opcode == ACIR_OPCODE_SET) {
    AnchWriteFormat(out, ANSI_GRAY "%zu" ANSI_RESET " | ", self->index);
    AcirOperand_Print(out, &self->out);
    AnchWriteString(out, ".");
    AcirValueType_Print(out, self->type);
    AnchWriteString(out, " = ");
    AcirOperand_Print(out, &self->val);
  } else {
    AnchWriteFormat(out, ANSI_GRAY "%zu" ANSI_RESET " | " ANSI_BLUE "%s" ANSI_RESET ".",
      self->index, AcirOpcode_Mnemonic(self->opcode));
    
    AcirValueType_Print(out, self->type);
    AnchWriteString(out, " ");

    int operandCount = AcirOpcode_OperandCount(self->opcode);
    if(operandCount == 1) {
      AcirOperand_Print(out, &self->val);
    } else if(operandCount == 2) {
      AcirOperand_Print(out, &self->val);
      AnchWriteString(out, ", ");
      AcirOperand_Print(out, &self->out);
    } else if(operandCount == 3) {
      AcirOperand_Print(out, &self->lhs);
      AnchWriteString(out, ", ");
      AcirOperand_Print(out, &self->rhs);
      AnchWriteString(out, ", ");
      AcirOperand_Print(out, &self->out);
    } else {
      AnchWriteFormat(out, ANSI_RED "<bad operand count (%d)>" ANSI_RESET, operandCount);
    }
  }

  if(self->next == ACIR_INSTR_NULL_INDEX)
    AnchWriteString(out, ANSI_GRAY  " -> end" ANSI_RESET);
  else if(self->next != self->index + 1)
    AnchWriteFormat(out, ANSI_GRAY " -> %zu" ANSI_RESET, self->next);
}

void AcirOperand_Print(AnchCharWriteStream *out, const AcirOperand *self) {
  if(!self) { AnchWriteFormat(out, ANSI_RED "<null operand>" ANSI_RESET); return; } 
  if(self->type == ACIR_OPERAND_TYPE_IMMEDIATE) {
    AcirValueType_Print(out, self->imm.type);
    AnchWriteString(out, "#" ANSI_GREEN);
    if(!self->imm.type) {
      AnchWriteFormat(out, ANSI_RED "0x%016zx", *(size_t*)&self->imm);
    } else if(self->imm.type->type != ACIR_VALUE_TYPE_BASIC) {
      AnchWriteFormat(out, ANSI_RED "<not basic type (%d)>", self->imm.type->type);
      AnchWriteFormat(out, ANSI_RED "0x%016zx", *(size_t*)&self->imm);
    } else switch(self->imm.type->basic) {
    case ACIR_BASIC_VALUE_TYPE_SINT64: AnchWriteFormat(out, "%zu", self->imm.sint64); break;
    case ACIR_BASIC_VALUE_TYPE_UINT64: AnchWriteFormat(out, "%zu", self->imm.uint64); break;
    case ACIR_BASIC_VALUE_TYPE_SINT32: AnchWriteFormat(out, "%d", self->imm.sint32); break;
    case ACIR_BASIC_VALUE_TYPE_UINT32: AnchWriteFormat(out, "%u", self->imm.uint32); break;
    case ACIR_BASIC_VALUE_TYPE_SINT16: AnchWriteFormat(out, "%d", self->imm.sint16); break;
    case ACIR_BASIC_VALUE_TYPE_UINT16: AnchWriteFormat(out, "%u", self->imm.uint16); break;
    case ACIR_BASIC_VALUE_TYPE_SINT8: AnchWriteFormat(out, "%d", self->imm.sint8); break;
    case ACIR_BASIC_VALUE_TYPE_UINT8: AnchWriteFormat(out, "%u", self->imm.uint8); break;
    case ACIR_BASIC_VALUE_TYPE_FLOAT32: AnchWriteFormat(out, "%f", self->imm.float32); break;
    case ACIR_BASIC_VALUE_TYPE_FLOAT64: AnchWriteFormat(out, "%f", self->imm.float64); break;
    case ACIR_BASIC_VALUE_TYPE_BOOL: AnchWriteString(out, self->imm.boolean ? "true" : "false"); break;
    case ACIR_BASIC_VALUE_TYPE_VOID: AnchWriteString(out, "void"); break;
    default:
      AnchWriteFormat(out, ANSI_RED "<bad basic value type (%d)>", self->imm.type->basic);
      AnchWriteFormat(out, ANSI_RED "0x%016zx", *(size_t*)&self->imm);
    }
    AnchWriteString(out, ANSI_RESET);
  } else if(self->type == ACIR_OPERAND_TYPE_BINDING) {
    AnchWriteFormat(out, "$" ANSI_YELLOW "%zu" ANSI_RESET, self->idx);
  } else {
    AnchWriteFormat(out, ANSI_RED "<bad operand type (%d)>" ANSI_RESET, self->type);
  }
}

void AcirValueType_Print(AnchCharWriteStream *out, const AcirValueType *self) {
  if(!self) { AnchWriteFormat(out, ANSI_RED "<null value type>" ANSI_RESET); return; } 
  if(self->type == ACIR_VALUE_TYPE_BASIC) {
    AnchWriteFormat(out, ANSI_GRAY "%s" ANSI_RESET, AcirBasicValueType_Mnemonic(self->basic));
  } else if(self->type == ACIR_VALUE_TYPE_POINTER) {
    AnchWriteFormat(out, ANSI_GRAY "*" ANSI_RESET);
    AcirValueType_Print(out, self->pointer);
  } else {
    AnchWriteFormat(out, ANSI_RED "<bad value type (%d)>" ANSI_RESET, self->type);
  }
}

void AcirFunction_Print(const AcirFunction *self, AnchCharWriteStream *out) {
  for(const AcirInstr *instr = &self->instrs[self->code]; ; instr = &self->instrs[instr->next]) {
    AcirInstr_Print(wsStdout, instr);
    AnchWriteString(wsStdout, "\n");
    if(instr->next == ACIR_INSTR_NULL_INDEX) break;
  }
}

bool AcirBasicValueType_Equals(const AcirValueType *self, const AcirValueType *other) {
  assert(self != NULL);
  assert(other != NULL);
  if(self == other) return true;
  if(self->type != other->type) return false;
  if(self->type == ACIR_VALUE_TYPE_BASIC) return self->basic == other->basic;
  if(self->type == ACIR_VALUE_TYPE_POINTER) return AcirBasicValueType_Equals(self->pointer, other->pointer);
  if(self->type == ACIR_VALUE_TYPE_FUNCTION) {
    if(self->function->argumentCount != other->function->argumentCount) return false;
    if(!AcirBasicValueType_Equals(self->function->returnType, other->function->returnType)) return false;
    for(size_t i = 0; i < self->function->argumentCount; ++i) {
      if(!AcirBasicValueType_Equals(self->function->argumentTypes[i], other->function->argumentTypes[i]))
        return false;
    }
    return true;
  }
  return false;
}

static bool strprefix(const char *str, const char *pre) {
  return strncmp(pre, str, strlen(pre)) == 0;
}

typedef struct {
  bool exists;
  AcirValueType type;
} ValidationContext_Binding_;

typedef struct {
  size_t bindingCount;
  ValidationContext_Binding_ *bindings;
  AnchAllocator *allocator;
  int errorCount;
} ValidationContext_;

static ValidationContext_Binding_ *ValidationContext_GetBinding(const ValidationContext_ *self, size_t index) {
  assert(self != NULL);
  if(index >= self->bindingCount) return NULL;
  return &self->bindings[index];
}

static ValidationContext_Binding_ *ValidationContext_GetOrCreateBinding(ValidationContext_ *self, size_t index) {
  assert(self != NULL);
  if(index < self->bindingCount) return &self->bindings[index];

  size_t oldBindingCount = self->bindingCount;
  self->bindingCount = index + 1;

  if(self->bindings == NULL) {
    self->bindings = AnchAllocator_AllocZero(self->allocator,
      sizeof(ValidationContext_Binding_) * self->bindingCount);
  } else {
    self->bindings = AnchAllocator_Realloc(self->allocator,
      self->bindings, sizeof(ValidationContext_Binding_) * self->bindingCount);
    memset(self->bindings + oldBindingCount, 0, sizeof(ValidationContext_Binding_) * (self->bindingCount - oldBindingCount));
  }
  
  return &self->bindings[index];
}

static const AcirValueType *ValidationContext_TypeOf(const ValidationContext_ *self, const AcirOperand *op) {
  assert(self != NULL);
  assert(op != NULL);
  switch(op->type) {
    case ACIR_OPERAND_TYPE_BINDING: {
      ValidationContext_Binding_ *binding = ValidationContext_GetBinding(self, op->idx);
      if(binding == NULL) return NULL;
      return &binding->type;
    } break;
    case ACIR_OPERAND_TYPE_IMMEDIATE: return op->imm.type;
    default: return NULL;
  }
}

static void ValidationContext_Error_(ValidationContext_ *self, const AcirInstr *instr, const char *format, ...) {
  self->errorCount += 1;

  va_list va;
  va_start(va, format);

  const char *notes = NULL;
  if(*format == '!') {
    ++format;
    notes = format;
    while(*format != ';') ++format;
    ++format;
  }

  AnchWriteString(wsStderr, ANSI_RED "\nError: " ANSI_RESET);
  AnchWriteFormatV(wsStderr, format, va);
  AnchWriteString(wsStderr, "\n");
  AcirInstr_Print(wsStderr, instr);
  AnchWriteString(wsStderr, "\n");
  if(notes) AnchWriteString(wsStderr, "\n");
  while(notes && *notes != ';') {
    AnchWriteString(wsStderr, ANSI_GRAY "Note: " ANSI_RESET);
    switch(*notes) {
      case 'E':
        AnchWriteString(wsStderr, "expected `");
        AcirValueType_Print(wsStderr, va_arg(va, const AcirValueType *));
        AnchWriteString(wsStderr, "`, but got `");
        AcirValueType_Print(wsStderr, va_arg(va, const AcirValueType *));
        AnchWriteString(wsStderr, "` instead.");
        break;
      case 'G':
        AnchWriteString(wsStderr, "got `");
        AcirValueType_Print(wsStderr, va_arg(va, const AcirValueType *));
        AnchWriteString(wsStderr, "`");
        break;
      case 'A':
        AnchWriteString(wsStderr, "`");
        AcirValueType_Print(wsStderr, va_arg(va, const AcirValueType *));
        AnchWriteString(wsStderr, "` is not allowed.");
        break;
      case 'I':
        AnchWriteFormat(wsStderr, "Instruction count is %zu", va_arg(va, size_t));
        break;
      case 'O': {
        const AcirOperand *op = va_arg(va, const AcirOperand *);
        AnchWriteFormat(wsStderr, "got %s `", AcirOperandType_Name(op->type));
        AcirValueType_Print(wsStderr, ValidationContext_TypeOf(self, op));
        AnchWriteString(wsStderr, "`");
      } break;
      case 's':
        AnchWriteFormat(wsStderr, "instruction signature: `" ANSI_GRAY "%s" ANSI_RESET "`",
          AcirOpcode_Signature(instr->opcode));
        break;
      default:
        AnchWriteFormat(wsStderr, ANSI_RED "<bad note type ('%c' %d)>" ANSI_RESET, *notes, *notes);
    }
    AnchWriteString(wsStderr, "\n");
    ++notes;
  }

  if(notes) AnchWriteString(wsStderr, "\n");
  va_end(va);
}

static void ValidationContext_CheckInstr_(ValidationContext_ *self, const AcirInstr *instr) {
  const char *sig = AcirOpcode_Signature(instr->opcode);
  assert(sig != NULL);

  while(isspace(*sig)) ++sig;

  char generic = 0;
  if(*sig == '.') {
    ++sig;
    generic = *sig;
    ++sig;
    while(isspace(*sig)) ++sig;
    while(*sig == '!') {
      ++sig;

      size_t length = 0;
      const char *start = sig;
      while(isalpha(*sig)) { ++sig; ++length; }
      while(isspace(*sig)) ++sig;

      for(size_t i = 0; i < ACIR_BASIC_VALUE_TYPE_MAX_; ++i) {
        const char *mnemonic = AcirBasicValueType_Mnemonic(i);
        assert(mnemonic != NULL);
        if(length == strlen(mnemonic) && strncmp(mnemonic, start, length) == 0) {
          AcirValueType type = { ACIR_VALUE_TYPE_BASIC, .basic = i };
          if(AcirBasicValueType_Equals(instr->type, &type)) {
            ValidationContext_Error_(self, instr, "!A;generic instruction does not allow provided type.", &type);
          }

          break;
        }
      }
    }
  }
  
  while(isspace(*sig)) ++sig;
  
  int index = 0;
  while(*sig) {
    while(isspace(*sig)) ++sig;

    bool writable = false;
    bool lvalue = false;
    bool pointer = false;
    
    if(*sig == 'w') { writable = true; ++sig; }
    if(*sig == 'l') { lvalue = true; ++sig; }
    if(*sig == '*') { pointer = true; ++sig; }
    
    size_t length = 0;
    const char *start = sig;
    while(isalpha(*sig)) { ++sig; ++length; }
    while(isspace(*sig)) ++sig;
    bool last = *sig != ',';
    ++sig;

    const AcirValueType *typeRef;
    AcirValueType typeValue;
    AcirValueType pointerTypeValue;
    
    if(length == 1 && generic && *start == generic) {
      typeRef = instr->type;
    } else if(strncmp("any", start, 3) != 0) {
      for(size_t i = 0; i < ACIR_BASIC_VALUE_TYPE_MAX_; ++i) {
        const char *mnemonic = AcirBasicValueType_Mnemonic(i);
        assert(mnemonic != NULL);
        if(length == strlen(mnemonic) && strncmp(mnemonic, start, length) == 0) {
          typeValue = (AcirValueType){ ACIR_VALUE_TYPE_BASIC, .basic = i };
          typeRef = &typeValue;
          break;
        }
      }
    }

    if(pointer) {
      pointerTypeValue = (AcirValueType){ ACIR_VALUE_TYPE_POINTER, .pointer = typeRef };
      typeRef = &pointerTypeValue;
    }

    const AcirOperand *op = NULL;
    const char *opname = "<unknown>";
    if(index == 0) { op = &instr->val; opname = "Value"; }
    if(index == 1 && last) { op = &instr->out; opname = "Output"; }
    if(index == 1 && !last) { op = &instr->rhs; opname = "Second value"; }
    if(index == 2) { op = &instr->out; opname = "Output"; }

    ValidationContext_Binding_ *binding = op->type == ACIR_OPERAND_TYPE_BINDING
      ? ValidationContext_GetBinding(self, op->idx) : NULL;
    
    if(writable || lvalue) {
      if(op->type != ACIR_OPERAND_TYPE_BINDING) {
        ValidationContext_Error_(self, instr, "!Os;%s argument (#%d) must be either writable or an lvalue.",
          opname, index + 1, op);
      } else {
        if(binding != NULL && binding->exists)
            ValidationContext_Error_(self, instr,
              ANSI_RESET "binding $"
              ANSI_YELLOW "%d"
              ANSI_RESET " might be set multiple times.", op->idx);
        
        binding = ValidationContext_GetOrCreateBinding(self, op->idx);
        binding->exists = true;
        binding->type = *typeRef;
      }
    } else {
      if(op->type == ACIR_OPERAND_TYPE_BINDING && (binding == NULL || !binding->exists))
          ValidationContext_Error_(self, instr, "binding $%d doesn't exist.", op->idx);
    }

    const AcirValueType *opType = ValidationContext_TypeOf(self, op);
    if(opType == NULL) {
      ValidationContext_Error_(self, instr, "%s argument (#%d) doesn't have a type.",
        opname, index + 1);
    } else if(!AcirBasicValueType_Equals(typeRef, opType)) {
      ValidationContext_Error_(self, instr, "!Es;%s argument (#%d) did not match type.",
        opname, index + 1, typeRef, opType);
    }

    if(instr->opcode == ACIR_OPCODE_RET && instr->next != ACIR_INSTR_NULL_INDEX) {
      ValidationContext_Error_(self, instr, "the `" ANSI_BLUE "ret" ANSI_RESET "` instruction has to be last.");
    }

    ++index;
    if(last) break;
  }
}

int AcirFunction_Validate(AcirFunction *self, AnchAllocator *allocator) {
  assert(self != NULL);
  
  ValidationContext_ context = {0};
  context.allocator = allocator;

  for(const AcirInstr *instr = &self->instrs[self->code]; instr != NULL;) {
    ValidationContext_CheckInstr_(&context, instr);
    if(instr->next == ACIR_INSTR_NULL_INDEX) break;
    if(instr->next >= self->instrCount)
      ValidationContext_Error_(&context, instr,
        "!I;Non-existent next instruction %zu.", instr->next, self->instrCount);
    instr = &self->instrs[instr->next];
  }

  if(context.bindingCount > 0)
    AnchAllocator_Free(context.allocator, context.bindings);
  
  return context.errorCount;
}

void AcirBuilder_Init(AcirBuilder *self, AcirFunction *target, AnchAllocator *allocator) {
  assert(self != NULL);
  assert(target != NULL);
  self->target = target;
  self->allocator = allocator;
  self->target->instrCount = 0;
  self->target->instrs = NULL;
  self->instrs = NULL;
}

void AcirBuilder_Free(AcirBuilder *self) {
  assert(self != NULL);
  if(self->target->instrCount > 0)
    AnchAllocator_Free(self->allocator, self->instrs);
  self->target->instrCount = 0;
  self->target->instrs = NULL;
  self->target->code = ACIR_INSTR_NULL_INDEX;
  self->allocator = NULL;
  self->target = NULL;
  self->instrs = NULL;
}

AcirInstr *AcirBuilder_Add(AcirBuilder *self, size_t index) {
  assert(self != NULL);

  if(index < self->target->instrCount) return &self->instrs[index];

  size_t oldInstrCount = self->target->instrCount;
  self->target->instrCount = index + 1;

  if(oldInstrCount == 0) {
    self->target->instrs = self->instrs =
      AnchAllocator_AllocZero(self->allocator, sizeof(AcirInstr));
    
    self->target->code = index;
  } else {
    self->target->instrs = self->instrs =
      AnchAllocator_Realloc(self->allocator, self->instrs,
        sizeof(AcirInstr) * self->target->instrCount);
    
    memset(self->instrs + oldInstrCount, 0,
      sizeof(AcirInstr) * (self->target->instrCount - oldInstrCount));
  }
  
  return &self->instrs[index];
}

void AcirBuilder_BuildNormalized(const AcirBuilder *self, AcirBuilder *target) {
  size_t index = 0;
  for(AcirInstr *instr = &self->instrs[self->target->code]; ; instr = &self->instrs[instr->next]) {
    AcirInstr *tinstr = AcirBuilder_Add(target, index);
    *tinstr = *instr;
    tinstr->index = index;
    if(instr->next == ACIR_INSTR_NULL_INDEX) break;
    tinstr->next = ++index;
  }
}

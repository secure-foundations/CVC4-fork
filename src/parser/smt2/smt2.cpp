/*********************                                                        */
/*! \file smt2.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Andrew Reynolds, Kshitij Bansal, Morgan Deters
 ** This file is part of the CVC4 project.
 ** Copyright (c) 2009-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved.  See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief Definitions of SMT2 constants.
 **
 ** Definitions of SMT2 constants.
 **/
#include "parser/smt2/smt2.h"

#include <algorithm>

#include "base/check.h"
#include "expr/type.h"
#include "options/options.h"
#include "parser/antlr_input.h"
#include "parser/parser.h"
#include "parser/smt2/smt2_input.h"
#include "printer/sygus_print_callback.h"
#include "util/bitvector.h"

// ANTLR defines these, which is really bad!
#undef true
#undef false

namespace CVC4 {
namespace parser {

Smt2::Smt2(api::Solver* solver, Input* input, bool strictMode, bool parseOnly)
    : Parser(solver, input, strictMode, parseOnly),
      d_logicSet(false),
      d_seenSetLogic(false)
{
  if (!strictModeEnabled())
  {
    addTheory(Smt2::THEORY_CORE);
  }
}

void Smt2::addArithmeticOperators() {
  addOperator(api::PLUS, "+");
  addOperator(api::MINUS, "-");
  // api::MINUS is converted to api::UMINUS if there is only a single operand
  Parser::addOperator(api::UMINUS);
  addOperator(api::MULT, "*");
  addOperator(api::LT, "<");
  addOperator(api::LEQ, "<=");
  addOperator(api::GT, ">");
  addOperator(api::GEQ, ">=");

  if (!strictModeEnabled())
  {
    // NOTE: this operator is non-standard
    addOperator(api::POW, "^");
  }
}

void Smt2::addTranscendentalOperators()
{
  addOperator(api::EXPONENTIAL, "exp");
  addOperator(api::SINE, "sin");
  addOperator(api::COSINE, "cos");
  addOperator(api::TANGENT, "tan");
  addOperator(api::COSECANT, "csc");
  addOperator(api::SECANT, "sec");
  addOperator(api::COTANGENT, "cot");
  addOperator(api::ARCSINE, "arcsin");
  addOperator(api::ARCCOSINE, "arccos");
  addOperator(api::ARCTANGENT, "arctan");
  addOperator(api::ARCCOSECANT, "arccsc");
  addOperator(api::ARCSECANT, "arcsec");
  addOperator(api::ARCCOTANGENT, "arccot");
  addOperator(api::SQRT, "sqrt");
}

void Smt2::addQuantifiersOperators()
{
  if (!strictModeEnabled())
  {
    addOperator(api::INST_CLOSURE, "inst-closure");
  }
}

void Smt2::addBitvectorOperators() {
  addOperator(api::BITVECTOR_CONCAT, "concat");
  addOperator(api::BITVECTOR_NOT, "bvnot");
  addOperator(api::BITVECTOR_AND, "bvand");
  addOperator(api::BITVECTOR_OR, "bvor");
  addOperator(api::BITVECTOR_NEG, "bvneg");
  addOperator(api::BITVECTOR_PLUS, "bvadd");
  addOperator(api::BITVECTOR_MULT, "bvmul");
  addOperator(api::BITVECTOR_UDIV, "bvudiv");
  addOperator(api::BITVECTOR_UREM, "bvurem");
  addOperator(api::BITVECTOR_SHL, "bvshl");
  addOperator(api::BITVECTOR_LSHR, "bvlshr");
  addOperator(api::BITVECTOR_ULT, "bvult");
  addOperator(api::BITVECTOR_NAND, "bvnand");
  addOperator(api::BITVECTOR_NOR, "bvnor");
  addOperator(api::BITVECTOR_XOR, "bvxor");
  addOperator(api::BITVECTOR_XNOR, "bvxnor");
  addOperator(api::BITVECTOR_COMP, "bvcomp");
  addOperator(api::BITVECTOR_SUB, "bvsub");
  addOperator(api::BITVECTOR_SDIV, "bvsdiv");
  addOperator(api::BITVECTOR_SREM, "bvsrem");
  addOperator(api::BITVECTOR_SMOD, "bvsmod");
  addOperator(api::BITVECTOR_ASHR, "bvashr");
  addOperator(api::BITVECTOR_ULE, "bvule");
  addOperator(api::BITVECTOR_UGT, "bvugt");
  addOperator(api::BITVECTOR_UGE, "bvuge");
  addOperator(api::BITVECTOR_SLT, "bvslt");
  addOperator(api::BITVECTOR_SLE, "bvsle");
  addOperator(api::BITVECTOR_SGT, "bvsgt");
  addOperator(api::BITVECTOR_SGE, "bvsge");
  addOperator(api::BITVECTOR_REDOR, "bvredor");
  addOperator(api::BITVECTOR_REDAND, "bvredand");
  addOperator(api::BITVECTOR_TO_NAT, "bv2nat");

  addIndexedOperator(api::BITVECTOR_EXTRACT, api::BITVECTOR_EXTRACT, "extract");
  addIndexedOperator(api::BITVECTOR_REPEAT, api::BITVECTOR_REPEAT, "repeat");
  addIndexedOperator(
      api::BITVECTOR_ZERO_EXTEND, api::BITVECTOR_ZERO_EXTEND, "zero_extend");
  addIndexedOperator(
      api::BITVECTOR_SIGN_EXTEND, api::BITVECTOR_SIGN_EXTEND, "sign_extend");
  addIndexedOperator(
      api::BITVECTOR_ROTATE_LEFT, api::BITVECTOR_ROTATE_LEFT, "rotate_left");
  addIndexedOperator(
      api::BITVECTOR_ROTATE_RIGHT, api::BITVECTOR_ROTATE_RIGHT, "rotate_right");
  addIndexedOperator(api::INT_TO_BITVECTOR, api::INT_TO_BITVECTOR, "int2bv");
}

void Smt2::addDatatypesOperators()
{
  Parser::addOperator(api::APPLY_CONSTRUCTOR);
  Parser::addOperator(api::APPLY_TESTER);
  Parser::addOperator(api::APPLY_SELECTOR);

  if (!strictModeEnabled())
  {
    addOperator(api::DT_SIZE, "dt.size");
  }
}

void Smt2::addStringOperators() {
  defineVar(
      "re.all",
      getSolver()->mkTerm(api::REGEXP_STAR, getSolver()->mkRegexpSigma()));
  addOperator(api::STRING_CONCAT, "str.++");
  addOperator(api::STRING_LENGTH, "str.len");
  addOperator(api::STRING_SUBSTR, "str.substr");
  addOperator(api::STRING_STRCTN, "str.contains");
  addOperator(api::STRING_CHARAT, "str.at");
  addOperator(api::STRING_STRIDOF, "str.indexof");
  addOperator(api::STRING_STRREPL, "str.replace");
  if (!strictModeEnabled())
  {
    addOperator(api::STRING_TOLOWER, "str.tolower");
    addOperator(api::STRING_TOUPPER, "str.toupper");
    addOperator(api::STRING_REV, "str.rev");
  }
  addOperator(api::STRING_PREFIX, "str.prefixof");
  addOperator(api::STRING_SUFFIX, "str.suffixof");
  addOperator(api::STRING_FROM_CODE, "str.from_code");
  addOperator(api::STRING_IS_DIGIT, "str.is_digit");
  // at the moment, we only use this syntax for smt2.6.1
  if (getLanguage() == language::input::LANG_SMTLIB_V2_6_1
      || getLanguage() == language::input::LANG_SYGUS_V2)
  {
    addOperator(api::STRING_ITOS, "str.from_int");
    addOperator(api::STRING_STOI, "str.to_int");
    addOperator(api::STRING_IN_REGEXP, "str.in_re");
    addOperator(api::STRING_TO_REGEXP, "str.to_re");
    addOperator(api::STRING_TO_CODE, "str.to_code");
    addOperator(api::STRING_STRREPLALL, "str.replace_all");
  }
  else
  {
    addOperator(api::STRING_ITOS, "int.to.str");
    addOperator(api::STRING_STOI, "str.to.int");
    addOperator(api::STRING_IN_REGEXP, "str.in.re");
    addOperator(api::STRING_TO_REGEXP, "str.to.re");
    addOperator(api::STRING_TO_CODE, "str.code");
    addOperator(api::STRING_STRREPLALL, "str.replaceall");
  }

  addOperator(api::REGEXP_CONCAT, "re.++");
  addOperator(api::REGEXP_UNION, "re.union");
  addOperator(api::REGEXP_INTER, "re.inter");
  addOperator(api::REGEXP_STAR, "re.*");
  addOperator(api::REGEXP_PLUS, "re.+");
  addOperator(api::REGEXP_OPT, "re.opt");
  addOperator(api::REGEXP_RANGE, "re.range");
  addOperator(api::REGEXP_LOOP, "re.loop");
  addOperator(api::REGEXP_COMPLEMENT, "re.comp");
  addOperator(api::REGEXP_DIFF, "re.diff");
  addOperator(api::STRING_LT, "str.<");
  addOperator(api::STRING_LEQ, "str.<=");
}

void Smt2::addFloatingPointOperators() {
  addOperator(api::FLOATINGPOINT_FP, "fp");
  addOperator(api::FLOATINGPOINT_EQ, "fp.eq");
  addOperator(api::FLOATINGPOINT_ABS, "fp.abs");
  addOperator(api::FLOATINGPOINT_NEG, "fp.neg");
  addOperator(api::FLOATINGPOINT_PLUS, "fp.add");
  addOperator(api::FLOATINGPOINT_SUB, "fp.sub");
  addOperator(api::FLOATINGPOINT_MULT, "fp.mul");
  addOperator(api::FLOATINGPOINT_DIV, "fp.div");
  addOperator(api::FLOATINGPOINT_FMA, "fp.fma");
  addOperator(api::FLOATINGPOINT_SQRT, "fp.sqrt");
  addOperator(api::FLOATINGPOINT_REM, "fp.rem");
  addOperator(api::FLOATINGPOINT_RTI, "fp.roundToIntegral");
  addOperator(api::FLOATINGPOINT_MIN, "fp.min");
  addOperator(api::FLOATINGPOINT_MAX, "fp.max");
  addOperator(api::FLOATINGPOINT_LEQ, "fp.leq");
  addOperator(api::FLOATINGPOINT_LT, "fp.lt");
  addOperator(api::FLOATINGPOINT_GEQ, "fp.geq");
  addOperator(api::FLOATINGPOINT_GT, "fp.gt");
  addOperator(api::FLOATINGPOINT_ISN, "fp.isNormal");
  addOperator(api::FLOATINGPOINT_ISSN, "fp.isSubnormal");
  addOperator(api::FLOATINGPOINT_ISZ, "fp.isZero");
  addOperator(api::FLOATINGPOINT_ISINF, "fp.isInfinite");
  addOperator(api::FLOATINGPOINT_ISNAN, "fp.isNaN");
  addOperator(api::FLOATINGPOINT_ISNEG, "fp.isNegative");
  addOperator(api::FLOATINGPOINT_ISPOS, "fp.isPositive");
  addOperator(api::FLOATINGPOINT_TO_REAL, "fp.to_real");

  addIndexedOperator(api::FLOATINGPOINT_TO_FP_GENERIC,
                     api::FLOATINGPOINT_TO_FP_GENERIC,
                     "to_fp");
  addIndexedOperator(api::FLOATINGPOINT_TO_FP_UNSIGNED_BITVECTOR,
                     api::FLOATINGPOINT_TO_FP_UNSIGNED_BITVECTOR,
                     "to_fp_unsigned");
  addIndexedOperator(
      api::FLOATINGPOINT_TO_UBV, api::FLOATINGPOINT_TO_UBV, "fp.to_ubv");
  addIndexedOperator(
      api::FLOATINGPOINT_TO_SBV, api::FLOATINGPOINT_TO_SBV, "fp.to_sbv");

  if (!strictModeEnabled())
  {
    addIndexedOperator(api::FLOATINGPOINT_TO_FP_IEEE_BITVECTOR,
                       api::FLOATINGPOINT_TO_FP_IEEE_BITVECTOR,
                       "to_fp_bv");
    addIndexedOperator(api::FLOATINGPOINT_TO_FP_FLOATINGPOINT,
                       api::FLOATINGPOINT_TO_FP_FLOATINGPOINT,
                       "to_fp_fp");
    addIndexedOperator(api::FLOATINGPOINT_TO_FP_REAL,
                       api::FLOATINGPOINT_TO_FP_REAL,
                       "to_fp_real");
    addIndexedOperator(api::FLOATINGPOINT_TO_FP_SIGNED_BITVECTOR,
                       api::FLOATINGPOINT_TO_FP_SIGNED_BITVECTOR,
                       "to_fp_signed");
  }
}

void Smt2::addSepOperators() {
  addOperator(api::SEP_STAR, "sep");
  addOperator(api::SEP_PTO, "pto");
  addOperator(api::SEP_WAND, "wand");
  addOperator(api::SEP_EMP, "emp");
  Parser::addOperator(api::SEP_STAR);
  Parser::addOperator(api::SEP_PTO);
  Parser::addOperator(api::SEP_WAND);
  Parser::addOperator(api::SEP_EMP);
}

void Smt2::addTheory(Theory theory) {
  switch(theory) {
  case THEORY_ARRAYS:
    addOperator(api::SELECT, "select");
    addOperator(api::STORE, "store");
    break;

  case THEORY_BITVECTORS:
    addBitvectorOperators();
    break;

  case THEORY_CORE:
    defineType("Bool", d_solver->getBooleanSort());
    defineVar("true", d_solver->mkTrue());
    defineVar("false", d_solver->mkFalse());
    addOperator(api::AND, "and");
    addOperator(api::DISTINCT, "distinct");
    addOperator(api::EQUAL, "=");
    addOperator(api::IMPLIES, "=>");
    addOperator(api::ITE, "ite");
    addOperator(api::NOT, "not");
    addOperator(api::OR, "or");
    addOperator(api::XOR, "xor");
    break;

  case THEORY_REALS_INTS:
    defineType("Real", d_solver->getRealSort());
    addOperator(api::DIVISION, "/");
    addOperator(api::TO_INTEGER, "to_int");
    addOperator(api::IS_INTEGER, "is_int");
    addOperator(api::TO_REAL, "to_real");
    // falling through on purpose, to add Ints part of Reals_Ints
    CVC4_FALLTHROUGH;
  case THEORY_INTS:
    defineType("Int", d_solver->getIntegerSort());
    addArithmeticOperators();
    addOperator(api::INTS_DIVISION, "div");
    addOperator(api::INTS_MODULUS, "mod");
    addOperator(api::ABS, "abs");
    addIndexedOperator(api::DIVISIBLE, api::DIVISIBLE, "divisible");
    break;

  case THEORY_REALS:
    defineType("Real", d_solver->getRealSort());
    addArithmeticOperators();
    addOperator(api::DIVISION, "/");
    if (!strictModeEnabled())
    {
      addOperator(api::ABS, "abs");
    }
    break;

  case THEORY_TRANSCENDENTALS:
    defineVar("real.pi", d_solver->mkTerm(api::PI));
    addTranscendentalOperators();
    break;

  case THEORY_QUANTIFIERS: addQuantifiersOperators(); break;

  case THEORY_SETS:
    defineVar("emptyset", d_solver->mkEmptySet(d_solver->getNullSort()));
    // the Boolean sort is a placeholder here since we don't have type info
    // without type annotation
    defineVar("univset", d_solver->mkUniverseSet(d_solver->getBooleanSort()));

    addOperator(api::UNION, "union");
    addOperator(api::INTERSECTION, "intersection");
    addOperator(api::SETMINUS, "setminus");
    addOperator(api::SUBSET, "subset");
    addOperator(api::MEMBER, "member");
    addOperator(api::SINGLETON, "singleton");
    addOperator(api::INSERT, "insert");
    addOperator(api::CARD, "card");
    addOperator(api::COMPLEMENT, "complement");
    addOperator(api::JOIN, "join");
    addOperator(api::PRODUCT, "product");
    addOperator(api::TRANSPOSE, "transpose");
    addOperator(api::TCLOSURE, "tclosure");
    break;

  case THEORY_DATATYPES:
  {
    const std::vector<api::Sort> types;
    defineType("Tuple", d_solver->mkTupleSort(types));
    addDatatypesOperators();
    break;
  }

  case THEORY_STRINGS:
    defineType("String", d_solver->getStringSort());
    defineType("RegLan", d_solver->getRegExpSort());
    defineType("Int", d_solver->getIntegerSort());

    if (getLanguage() == language::input::LANG_SMTLIB_V2_6_1)
    {
      defineVar("re.none", d_solver->mkRegexpEmpty());
    }
    else
    {
      defineVar("re.nostr", d_solver->mkRegexpEmpty());
    }
    defineVar("re.allchar", d_solver->mkRegexpSigma());

    addStringOperators();
    break;

  case THEORY_UF:
    Parser::addOperator(api::APPLY_UF);

    if (!strictModeEnabled() && d_logic.hasCardinalityConstraints())
    {
      addOperator(api::CARDINALITY_CONSTRAINT, "fmf.card");
      addOperator(api::CARDINALITY_VALUE, "fmf.card.val");
    }
    break;

  case THEORY_FP:
    defineType("RoundingMode", d_solver->getRoundingmodeSort());
    defineType("Float16", d_solver->mkFloatingPointSort(5, 11));
    defineType("Float32", d_solver->mkFloatingPointSort(8, 24));
    defineType("Float64", d_solver->mkFloatingPointSort(11, 53));
    defineType("Float128", d_solver->mkFloatingPointSort(15, 113));

    defineVar("RNE", d_solver->mkRoundingMode(api::ROUND_NEAREST_TIES_TO_EVEN));
    defineVar("roundNearestTiesToEven",
              d_solver->mkRoundingMode(api::ROUND_NEAREST_TIES_TO_EVEN));
    defineVar("RNA", d_solver->mkRoundingMode(api::ROUND_NEAREST_TIES_TO_AWAY));
    defineVar("roundNearestTiesToAway",
              d_solver->mkRoundingMode(api::ROUND_NEAREST_TIES_TO_AWAY));
    defineVar("RTP", d_solver->mkRoundingMode(api::ROUND_TOWARD_POSITIVE));
    defineVar("roundTowardPositive",
              d_solver->mkRoundingMode(api::ROUND_TOWARD_POSITIVE));
    defineVar("RTN", d_solver->mkRoundingMode(api::ROUND_TOWARD_NEGATIVE));
    defineVar("roundTowardNegative",
              d_solver->mkRoundingMode(api::ROUND_TOWARD_NEGATIVE));
    defineVar("RTZ", d_solver->mkRoundingMode(api::ROUND_TOWARD_ZERO));
    defineVar("roundTowardZero",
              d_solver->mkRoundingMode(api::ROUND_TOWARD_ZERO));

    addFloatingPointOperators();
    break;
    
  case THEORY_SEP:
    // the Boolean sort is a placeholder here since we don't have type info
    // without type annotation
    defineVar("sep.nil", d_solver->mkSepNil(d_solver->getBooleanSort()));

    addSepOperators();
    break;
    
  default:
    std::stringstream ss;
    ss << "internal error: unsupported theory " << theory;
    throw ParserException(ss.str());
  }
}

void Smt2::addOperator(api::Kind kind, const std::string& name)
{
  Debug("parser") << "Smt2::addOperator( " << kind << ", " << name << " )"
                  << std::endl;
  Parser::addOperator(kind);
  operatorKindMap[name] = kind;
}

void Smt2::addIndexedOperator(api::Kind tKind,
                              api::Kind opKind,
                              const std::string& name)
{
  Parser::addOperator(tKind);
  d_indexedOpKindMap[name] = opKind;
}

api::Kind Smt2::getOperatorKind(const std::string& name) const
{
  // precondition: isOperatorEnabled(name)
  return operatorKindMap.find(name)->second;
}

bool Smt2::isOperatorEnabled(const std::string& name) const {
  return operatorKindMap.find(name) != operatorKindMap.end();
}

bool Smt2::isTheoryEnabled(Theory theory) const {
  switch(theory) {
  case THEORY_ARRAYS:
    return d_logic.isTheoryEnabled(theory::THEORY_ARRAYS);
  case THEORY_BITVECTORS:
    return d_logic.isTheoryEnabled(theory::THEORY_BV);
  case THEORY_CORE:
    return true;
  case THEORY_DATATYPES:
    return d_logic.isTheoryEnabled(theory::THEORY_DATATYPES);
  case THEORY_INTS:
    return d_logic.isTheoryEnabled(theory::THEORY_ARITH) &&
      d_logic.areIntegersUsed() && ( !d_logic.areRealsUsed() );
  case THEORY_REALS:
    return d_logic.isTheoryEnabled(theory::THEORY_ARITH) &&
      ( !d_logic.areIntegersUsed() ) && d_logic.areRealsUsed();
  case THEORY_REALS_INTS:
    return d_logic.isTheoryEnabled(theory::THEORY_ARITH) &&
      d_logic.areIntegersUsed() && d_logic.areRealsUsed();
  case THEORY_QUANTIFIERS:
    return d_logic.isQuantified();
  case THEORY_SETS:
    return d_logic.isTheoryEnabled(theory::THEORY_SETS);
  case THEORY_STRINGS:
    return d_logic.isTheoryEnabled(theory::THEORY_STRINGS);
  case THEORY_UF:
    return d_logic.isTheoryEnabled(theory::THEORY_UF);
  case THEORY_FP:
    return d_logic.isTheoryEnabled(theory::THEORY_FP);
  case THEORY_SEP:
    return d_logic.isTheoryEnabled(theory::THEORY_SEP);
  default:
    std::stringstream ss;
    ss << "internal error: unsupported theory " << theory;
    throw ParserException(ss.str());
  }
}

bool Smt2::logicIsSet() {
  return d_logicSet;
}

api::Term Smt2::getExpressionForNameAndType(const std::string& name,
                                            api::Sort t)
{
  if (isAbstractValue(name))
  {
    return mkAbstractValue(name);
  }
  return Parser::getExpressionForNameAndType(name, t);
}

api::Term Smt2::mkIndexedConstant(const std::string& name,
                                  const std::vector<uint64_t>& numerals)
{
  if (isTheoryEnabled(THEORY_FP))
  {
    if (name == "+oo")
    {
      return d_solver->mkPosInf(numerals[0], numerals[1]);
    }
    else if (name == "-oo")
    {
      return d_solver->mkNegInf(numerals[0], numerals[1]);
    }
    else if (name == "NaN")
    {
      return d_solver->mkNaN(numerals[0], numerals[1]);
    }
    else if (name == "+zero")
    {
      return d_solver->mkPosZero(numerals[0], numerals[1]);
    }
    else if (name == "-zero")
    {
      return d_solver->mkNegZero(numerals[0], numerals[1]);
    }
  }

  if (isTheoryEnabled(THEORY_BITVECTORS) && name.find("bv") == 0)
  {
    std::string bvStr = name.substr(2);
    return d_solver->mkBitVector(numerals[0], bvStr, 10);
  }

  // NOTE: Theory parametric constants go here

  parseError(std::string("Unknown indexed literal `") + name + "'");
  return api::Term();
}

api::Op Smt2::mkIndexedOp(const std::string& name,
                          const std::vector<uint64_t>& numerals)
{
  const auto& kIt = d_indexedOpKindMap.find(name);
  if (kIt != d_indexedOpKindMap.end())
  {
    api::Kind k = (*kIt).second;
    if (numerals.size() == 1)
    {
      return d_solver->mkOp(k, numerals[0]);
    }
    else if (numerals.size() == 2)
    {
      return d_solver->mkOp(k, numerals[0], numerals[1]);
    }
  }

  parseError(std::string("Unknown indexed function `") + name + "'");
  return api::Op();
}

api::Term Smt2::bindDefineFunRec(
    const std::string& fname,
    const std::vector<std::pair<std::string, api::Sort>>& sortedVarNames,
    api::Sort t,
    std::vector<api::Term>& flattenVars)
{
  std::vector<api::Sort> sorts;
  for (const std::pair<std::string, api::Sort>& svn : sortedVarNames)
  {
    sorts.push_back(svn.second);
  }

  // make the flattened function type, add bound variables
  // to flattenVars if the defined function was given a function return type.
  api::Sort ft = mkFlatFunctionType(sorts, t, flattenVars);

  // allow overloading
  return bindVar(fname, ft, ExprManager::VAR_FLAG_NONE, true);
}

void Smt2::pushDefineFunRecScope(
    const std::vector<std::pair<std::string, api::Sort>>& sortedVarNames,
    api::Term func,
    const std::vector<api::Term>& flattenVars,
    std::vector<api::Term>& bvs,
    bool bindingLevel)
{
  pushScope(bindingLevel);

  // bound variables are those that are explicitly named in the preamble
  // of the define-fun(s)-rec command, we define them here
  for (const std::pair<std::string, api::Sort>& svn : sortedVarNames)
  {
    api::Term v = bindBoundVar(svn.first, svn.second);
    bvs.push_back(v);
  }

  bvs.insert(bvs.end(), flattenVars.begin(), flattenVars.end());
}

void Smt2::reset() {
  d_logicSet = false;
  d_seenSetLogic = false;
  d_logic = LogicInfo();
  operatorKindMap.clear();
  d_lastNamedTerm = std::pair<api::Term, std::string>();
  this->Parser::reset();

  if( !strictModeEnabled() ) {
    addTheory(Smt2::THEORY_CORE);
  }
}

void Smt2::resetAssertions() {
  // Remove all declarations except the ones at level 0.
  while (this->scopeLevel() > 0) {
    this->popScope();
  }
}

Smt2::SynthFunFactory::SynthFunFactory(
    Smt2* smt2,
    const std::string& fun,
    bool isInv,
    api::Sort range,
    std::vector<std::pair<std::string, api::Sort>>& sortedVarNames)
    : d_smt2(smt2), d_fun(fun), d_isInv(isInv)
{
  if (range.isNull())
  {
    smt2->parseError("Must supply return type for synth-fun.");
  }
  if (range.isFunction())
  {
    smt2->parseError("Cannot use synth-fun with function return type.");
  }
  std::vector<api::Sort> varSorts;
  for (const std::pair<std::string, api::Sort>& p : sortedVarNames)
  {
    varSorts.push_back(p.second);
  }
  Debug("parser-sygus") << "Define synth fun : " << fun << std::endl;
  api::Sort synthFunType =
      varSorts.size() > 0 ? d_smt2->getSolver()->mkFunctionSort(varSorts, range)
                          : range;

  // we do not allow overloading for synth fun
  d_synthFun = d_smt2->bindBoundVar(fun, synthFunType);
  // set the sygus type to be range by default, which is overwritten below
  // if a grammar is provided
  d_sygusType = range;

  d_smt2->pushScope(true);
  d_sygusVars = d_smt2->bindBoundVars(sortedVarNames);
}

Smt2::SynthFunFactory::~SynthFunFactory() { d_smt2->popScope(); }

std::unique_ptr<Command> Smt2::SynthFunFactory::mkCommand(api::Sort grammar)
{
  Debug("parser-sygus") << "...read synth fun " << d_fun << std::endl;
  return std::unique_ptr<Command>(new SynthFunCommand(
      d_fun,
      d_synthFun.getExpr(),
      grammar.isNull() ? d_sygusType.getType() : grammar.getType(),
      d_isInv,
      api::termVectorToExprs(d_sygusVars)));
}

std::unique_ptr<Command> Smt2::invConstraint(
    const std::vector<std::string>& names)
{
  checkThatLogicIsSet();
  Debug("parser-sygus") << "Sygus : define sygus funs..." << std::endl;
  Debug("parser-sygus") << "Sygus : read inv-constraint..." << std::endl;

  if (names.size() != 4)
  {
    parseError(
        "Bad syntax for inv-constraint: expected 4 "
        "arguments.");
  }

  std::vector<api::Term> terms;
  for (const std::string& name : names)
  {
    if (!isDeclared(name))
    {
      std::stringstream ss;
      ss << "Function " << name << " in inv-constraint is not defined.";
      parseError(ss.str());
    }

    terms.push_back(getVariable(name));
  }

  return std::unique_ptr<Command>(
      new SygusInvConstraintCommand(api::termVectorToExprs(terms)));
}

Command* Smt2::setLogic(std::string name, bool fromCommand)
{
  if (fromCommand)
  {
    if (d_seenSetLogic)
    {
      parseError("Only one set-logic is allowed.");
    }
    d_seenSetLogic = true;

    if (logicIsForced())
    {
      // If the logic is forced, we ignore all set-logic requests from commands.
      return new EmptyCommand();
    }
  }

  if (sygus_v1())
  {
    // non-smt2-standard sygus logic names go here (http://sygus.seas.upenn.edu/files/sygus.pdf Section 3.2)
    if(name == "Arrays") {
      name = "A";
    }else if(name == "Reals") {
      name = "LRA";
    }
  }

  d_logicSet = true;
  d_logic = name;

  // if sygus is enabled, we must enable UF, datatypes, integer arithmetic and
  // higher-order
  if(sygus()) {
    if (!d_logic.isQuantified())
    {
      warning("Logics in sygus are assumed to contain quantifiers.");
      warning("Omit QF_ from the logic to avoid this warning.");
    }
  }

  // Core theory belongs to every logic
  addTheory(THEORY_CORE);

  if(d_logic.isTheoryEnabled(theory::THEORY_UF)) {
    addTheory(THEORY_UF);
  }

  if(d_logic.isTheoryEnabled(theory::THEORY_ARITH)) {
    if(d_logic.areIntegersUsed()) {
      if(d_logic.areRealsUsed()) {
        addTheory(THEORY_REALS_INTS);
      } else {
        addTheory(THEORY_INTS);
      }
    } else if(d_logic.areRealsUsed()) {
      addTheory(THEORY_REALS);
    }

    if (d_logic.areTranscendentalsUsed())
    {
      addTheory(THEORY_TRANSCENDENTALS);
    }
  }

  if(d_logic.isTheoryEnabled(theory::THEORY_ARRAYS)) {
    addTheory(THEORY_ARRAYS);
  }

  if(d_logic.isTheoryEnabled(theory::THEORY_BV)) {
    addTheory(THEORY_BITVECTORS);
  }

  if(d_logic.isTheoryEnabled(theory::THEORY_DATATYPES)) {
    addTheory(THEORY_DATATYPES);
  }

  if(d_logic.isTheoryEnabled(theory::THEORY_SETS)) {
    addTheory(THEORY_SETS);
  }

  if(d_logic.isTheoryEnabled(theory::THEORY_STRINGS)) {
    addTheory(THEORY_STRINGS);
  }

  if(d_logic.isQuantified()) {
    addTheory(THEORY_QUANTIFIERS);
  }

  if (d_logic.isTheoryEnabled(theory::THEORY_FP)) {
    addTheory(THEORY_FP);
  }

  if (d_logic.isTheoryEnabled(theory::THEORY_SEP)) {
    addTheory(THEORY_SEP);
  }

  Command* cmd =
      new SetBenchmarkLogicCommand(sygus() ? d_logic.getLogicString() : name);
  cmd->setMuted(!fromCommand);
  return cmd;
} /* Smt2::setLogic() */

bool Smt2::sygus() const
{
  InputLanguage ilang = getLanguage();
  return ilang == language::input::LANG_SYGUS
         || ilang == language::input::LANG_SYGUS_V2;
}
bool Smt2::sygus_v1() const
{
  return getLanguage() == language::input::LANG_SYGUS;
}

void Smt2::setInfo(const std::string& flag, const SExpr& sexpr) {
  // TODO: ???
}

void Smt2::setOption(const std::string& flag, const SExpr& sexpr) {
  // TODO: ???
}

void Smt2::checkThatLogicIsSet()
{
  if (!logicIsSet())
  {
    if (strictModeEnabled())
    {
      parseError("set-logic must appear before this point.");
    }
    else
    {
      Command* cmd = nullptr;
      if (logicIsForced())
      {
        cmd = setLogic(getForcedLogic(), false);
      }
      else
      {
        warning("No set-logic command was given before this point.");
        warning("CVC4 will make all theories available.");
        warning(
            "Consider setting a stricter logic for (likely) better "
            "performance.");
        warning("To suppress this warning in the future use (set-logic ALL).");

        cmd = setLogic("ALL", false);
      }
      preemptCommand(cmd);
    }
  }
}

/* The include are managed in the lexer but called in the parser */
// Inspired by http://www.antlr3.org/api/C/interop.html

static bool newInputStream(const std::string& filename, pANTLR3_LEXER lexer) {
  Debug("parser") << "Including " << filename << std::endl;
  // Create a new input stream and take advantage of built in stream stacking
  // in C target runtime.
  //
  pANTLR3_INPUT_STREAM    in;
#ifdef CVC4_ANTLR3_OLD_INPUT_STREAM
  in = antlr3AsciiFileStreamNew((pANTLR3_UINT8) filename.c_str());
#else /* CVC4_ANTLR3_OLD_INPUT_STREAM */
  in = antlr3FileStreamNew((pANTLR3_UINT8) filename.c_str(), ANTLR3_ENC_8BIT);
#endif /* CVC4_ANTLR3_OLD_INPUT_STREAM */
  if( in == NULL ) {
    Debug("parser") << "Can't open " << filename << std::endl;
    return false;
  }
  // Same thing as the predefined PUSHSTREAM(in);
  lexer->pushCharStream(lexer, in);
  // restart it
  //lexer->rec->state->tokenStartCharIndex      = -10;
  //lexer->emit(lexer);

  // Note that the input stream is not closed when it EOFs, I don't bother
  // to do it here, but it is up to you to track streams created like this
  // and destroy them when the whole parse session is complete. Remember that you
  // don't want to do this until all tokens have been manipulated all the way through
  // your tree parsers etc as the token does not store the text it just refers
  // back to the input stream and trying to get the text for it will abort if you
  // close the input stream too early.

  //TODO what said before
  return true;
}

void Smt2::includeFile(const std::string& filename) {
  // security for online version
  if(!canIncludeFile()) {
    parseError("include-file feature was disabled for this run.");
  }

  // Get the lexer
  AntlrInput* ai = static_cast<AntlrInput*>(getInput());
  pANTLR3_LEXER lexer = ai->getAntlr3Lexer();
  // get the name of the current stream "Does it work inside an include?"
  const std::string inputName = ai->getInputStreamName();

  // Find the directory of the current input file
  std::string path;
  size_t pos = inputName.rfind('/');
  if(pos != std::string::npos) {
    path = std::string(inputName, 0, pos + 1);
  }
  path.append(filename);
  if(!newInputStream(path, lexer)) {
    parseError("Couldn't open include file `" + path + "'");
  }
}
bool Smt2::isAbstractValue(const std::string& name)
{
  return name.length() >= 2 && name[0] == '@' && name[1] != '0'
         && name.find_first_not_of("0123456789", 1) == std::string::npos;
}

api::Term Smt2::mkAbstractValue(const std::string& name)
{
  assert(isAbstractValue(name));
  // remove the '@'
  return d_solver->mkAbstractValue(name.substr(1));
}

void Smt2::mkSygusConstantsForType(const api::Sort& type,
                                   std::vector<api::Term>& ops)
{
  if( type.isInteger() ){
    ops.push_back(d_solver->mkReal(0));
    ops.push_back(d_solver->mkReal(1));
  }else if( type.isBitVector() ){
    uint32_t sz = type.getBVSize();
    ops.push_back(d_solver->mkBitVector(sz, 0));
    ops.push_back(d_solver->mkBitVector(sz, 1));
  }else if( type.isBoolean() ){
    ops.push_back(d_solver->mkTrue());
    ops.push_back(d_solver->mkFalse());
  }
  //TODO : others?
}

//  This method adds N operators to ops[index], N names to cnames[index] and N type argument vectors to cargs[index] (where typically N=1)
//  This method may also add new elements pairwise into datatypes/sorts/ops/cnames/cargs in the case of non-flat gterms.
void Smt2::processSygusGTerm(
    CVC4::SygusGTerm& sgt,
    int index,
    std::vector<CVC4::Datatype>& datatypes,
    std::vector<api::Sort>& sorts,
    std::vector<std::vector<ParseOp>>& ops,
    std::vector<std::vector<std::string>>& cnames,
    std::vector<std::vector<std::vector<api::Sort>>>& cargs,
    std::vector<bool>& allow_const,
    std::vector<std::vector<std::string>>& unresolved_gterm_sym,
    const std::vector<api::Term>& sygus_vars,
    std::map<api::Sort, api::Sort>& sygus_to_builtin,
    std::map<api::Sort, api::Term>& sygus_to_builtin_expr,
    api::Sort& ret,
    bool isNested)
{
  if (sgt.d_gterm_type == SygusGTerm::gterm_op)
  {
    Debug("parser-sygus") << "Add " << sgt.d_op << " to datatype "
                          << index << std::endl;
    api::Kind oldKind;
    api::Kind newKind = api::UNDEFINED_KIND;
    //convert to UMINUS if one child of MINUS
    if (sgt.d_children.size() == 1 && sgt.d_op.d_kind == api::MINUS)
    {
      oldKind = api::MINUS;
      newKind = api::UMINUS;
    }
    if (newKind != api::UNDEFINED_KIND)
    {
      Debug("parser-sygus")
          << "Replace " << sgt.d_op.d_kind << " with " << newKind << std::endl;
      sgt.d_op.d_kind = newKind;
      std::string oldName = api::kindToString(oldKind);
      std::string newName = api::kindToString(newKind);
      size_t pos = 0;
      if((pos = sgt.d_name.find(oldName, pos)) != std::string::npos){
        sgt.d_name.replace(pos, oldName.length(), newName);
      }
    }
    ops[index].push_back(sgt.d_op);
    cnames[index].push_back( sgt.d_name );
    cargs[index].push_back(std::vector<api::Sort>());
    for( unsigned i=0; i<sgt.d_children.size(); i++ ){
      std::stringstream ss;
      ss << datatypes[index].getName() << "_" << ops[index].size() << "_arg_" << i;
      std::string sub_dname = ss.str();
      //add datatype for child
      api::Sort null_type;
      pushSygusDatatypeDef( null_type, sub_dname, datatypes, sorts, ops, cnames, cargs, allow_const, unresolved_gterm_sym );
      int sub_dt_index = datatypes.size()-1;
      //process child
      api::Sort sub_ret;
      processSygusGTerm( sgt.d_children[i], sub_dt_index, datatypes, sorts, ops, cnames, cargs, allow_const, unresolved_gterm_sym,
                         sygus_vars, sygus_to_builtin, sygus_to_builtin_expr, sub_ret, true );
      //process the nested gterm (either pop the last datatype, or flatten the argument)
      api::Sort tt = processSygusNestedGTerm(sub_dt_index,
                                             sub_dname,
                                             datatypes,
                                             sorts,
                                             ops,
                                             cnames,
                                             cargs,
                                             allow_const,
                                             unresolved_gterm_sym,
                                             sygus_to_builtin,
                                             sygus_to_builtin_expr,
                                             sub_ret);
      cargs[index].back().push_back(tt);
    }
  }
  else if (sgt.d_gterm_type == SygusGTerm::gterm_constant)
  {
    if( sgt.getNumChildren()!=0 ){
      parseError("Bad syntax for Sygus Constant.");
    }
    std::vector<api::Term> consts;
    mkSygusConstantsForType( sgt.d_type, consts );
    Debug("parser-sygus") << "...made " << consts.size() << " constants." << std::endl;
    for( unsigned i=0; i<consts.size(); i++ ){
      std::stringstream ss;
      ss << consts[i];
      Debug("parser-sygus") << "...add for constant " << ss.str() << std::endl;
      ParseOp constOp;
      constOp.d_expr = consts[i];
      ops[index].push_back(constOp);
      cnames[index].push_back( ss.str() );
      cargs[index].push_back(std::vector<api::Sort>());
    }
    allow_const[index] = true;
  }
  else if (sgt.d_gterm_type == SygusGTerm::gterm_variable
           || sgt.d_gterm_type == SygusGTerm::gterm_input_variable)
  {
    if( sgt.getNumChildren()!=0 ){
      parseError("Bad syntax for Sygus Variable.");
    }
    Debug("parser-sygus") << "...process " << sygus_vars.size() << " variables." << std::endl;
    for( unsigned i=0; i<sygus_vars.size(); i++ ){
      if (sygus_vars[i].getSort() == sgt.d_type)
      {
        std::stringstream ss;
        ss << sygus_vars[i];
        Debug("parser-sygus") << "...add for variable " << ss.str() << std::endl;
        ParseOp varOp;
        varOp.d_expr = sygus_vars[i];
        ops[index].push_back(varOp);
        cnames[index].push_back( ss.str() );
        cargs[index].push_back(std::vector<api::Sort>());
      }
    }
  }
  else if (sgt.d_gterm_type == SygusGTerm::gterm_nested_sort)
  {
    ret = sgt.d_type;
  }
  else if (sgt.d_gterm_type == SygusGTerm::gterm_unresolved)
  {
    if( isNested ){
      if( isUnresolvedType(sgt.d_name) ){
        ret = getSort(sgt.d_name);
      }else{
        //nested, unresolved symbol...fail
        std::stringstream ss;
        ss << "Cannot handle nested unresolved symbol " << sgt.d_name << std::endl;
        parseError(ss.str());
      }
    }else{
      //will resolve when adding constructors
      unresolved_gterm_sym[index].push_back(sgt.d_name);
    }
  }
  else if (sgt.d_gterm_type == SygusGTerm::gterm_ignore)
  {
    // do nothing
  }
}

bool Smt2::pushSygusDatatypeDef(
    api::Sort t,
    std::string& dname,
    std::vector<CVC4::Datatype>& datatypes,
    std::vector<api::Sort>& sorts,
    std::vector<std::vector<ParseOp>>& ops,
    std::vector<std::vector<std::string>>& cnames,
    std::vector<std::vector<std::vector<api::Sort>>>& cargs,
    std::vector<bool>& allow_const,
    std::vector<std::vector<std::string>>& unresolved_gterm_sym)
{
  sorts.push_back(t);
  datatypes.push_back(Datatype(getExprManager(), dname));
  ops.push_back(std::vector<ParseOp>());
  cnames.push_back(std::vector<std::string>());
  cargs.push_back(std::vector<std::vector<api::Sort>>());
  allow_const.push_back(false);
  unresolved_gterm_sym.push_back(std::vector< std::string >());
  return true;
}

bool Smt2::popSygusDatatypeDef(
    std::vector<CVC4::Datatype>& datatypes,
    std::vector<api::Sort>& sorts,
    std::vector<std::vector<ParseOp>>& ops,
    std::vector<std::vector<std::string>>& cnames,
    std::vector<std::vector<std::vector<api::Sort>>>& cargs,
    std::vector<bool>& allow_const,
    std::vector<std::vector<std::string>>& unresolved_gterm_sym)
{
  sorts.pop_back();
  datatypes.pop_back();
  ops.pop_back();
  cnames.pop_back();
  cargs.pop_back();
  allow_const.pop_back();
  unresolved_gterm_sym.pop_back();
  return true;
}

api::Sort Smt2::processSygusNestedGTerm(
    int sub_dt_index,
    std::string& sub_dname,
    std::vector<CVC4::Datatype>& datatypes,
    std::vector<api::Sort>& sorts,
    std::vector<std::vector<ParseOp>>& ops,
    std::vector<std::vector<std::string>>& cnames,
    std::vector<std::vector<std::vector<api::Sort>>>& cargs,
    std::vector<bool>& allow_const,
    std::vector<std::vector<std::string>>& unresolved_gterm_sym,
    std::map<api::Sort, api::Sort>& sygus_to_builtin,
    std::map<api::Sort, CVC4::api::Term>& sygus_to_builtin_expr,
    api::Sort sub_ret)
{
  api::Sort t = sub_ret;
  Debug("parser-sygus") << "Argument is ";
  if( t.isNull() ){
    //then, it is the datatype we constructed, which should have a single constructor
    t = mkUnresolvedType(sub_dname);
    Debug("parser-sygus") << "inline flattening of (auxiliary, local) datatype " << t << std::endl;
    Debug("parser-sygus") << ": to compute type, construct ground term witnessing the grammar, #cons=" << cargs[sub_dt_index].size() << std::endl;
    if( cargs[sub_dt_index].empty() ){
      parseError(std::string("Internal error : datatype for nested gterm does not have a constructor."));
    }
    ParseOp op = ops[sub_dt_index][0];
    api::Sort curr_t;
    if (!op.d_expr.isNull()
        && (op.d_expr.isConst() || cargs[sub_dt_index][0].empty()))
    {
      api::Term sop = op.d_expr;
      curr_t = sop.getSort();
      Debug("parser-sygus")
          << ": it is constant/0-arg cons " << sop << " with type "
          << sop.getSort() << ", debug=" << sop.isConst() << " "
          << cargs[sub_dt_index][0].size() << std::endl;
      // only cache if it is a singleton datatype (has unique expr)
      if (ops[sub_dt_index].size() == 1)
      {
        sygus_to_builtin_expr[t] = sop;
        // store that term sop has dedicated sygus type t
        if (d_sygus_bound_var_type.find(sop) == d_sygus_bound_var_type.end())
        {
          d_sygus_bound_var_type[sop] = t;
        }
      }
    }
    else
    {
      std::vector<api::Term> children;
      for( unsigned i=0; i<cargs[sub_dt_index][0].size(); i++ ){
        std::map<api::Sort, CVC4::api::Term>::iterator it =
            sygus_to_builtin_expr.find(cargs[sub_dt_index][0][i]);
        if( it==sygus_to_builtin_expr.end() ){
          if( sygus_to_builtin.find( cargs[sub_dt_index][0][i] )==sygus_to_builtin.end() ){
            std::stringstream ss;
            ss << "Missing builtin type for type " << cargs[sub_dt_index][0][i] << "!" << std::endl;
            ss << "Builtin types are currently : " << std::endl;
            for (std::map<api::Sort, api::Sort>::iterator itb =
                     sygus_to_builtin.begin();
                 itb != sygus_to_builtin.end();
                 ++itb)
            {
              ss << "  " << itb->first << " -> " << itb->second << std::endl;
            }
            parseError(ss.str());
          }
          api::Sort bt = sygus_to_builtin[cargs[sub_dt_index][0][i]];
          Debug("parser-sygus") << ":  child " << i << " introduce type elem for " << cargs[sub_dt_index][0][i] << " " << bt << std::endl;
          std::stringstream ss;
          ss << t << "_x_" << i;
          api::Term bv = bindBoundVar(ss.str(), bt);
          children.push_back( bv );
          d_sygus_bound_var_type[bv] = cargs[sub_dt_index][0][i];
        }else{
          Debug("parser-sygus") << ":  child " << i << " existing sygus to builtin expr : " << it->second << std::endl;
          children.push_back( it->second );
        }
      }
      api::Term e = applyParseOp(op, children);
      Debug("parser-sygus") << ": constructed " << e << ", which has type "
                            << e.getSort() << std::endl;
      curr_t = e.getSort();
      sygus_to_builtin_expr[t] = e;
    }
    sorts[sub_dt_index] = curr_t;
    sygus_to_builtin[t] = curr_t;
  }else{
    Debug("parser-sygus") << "simple argument " << t << std::endl;
    Debug("parser-sygus") << "...removing " << datatypes.back().getName() << std::endl;
    //otherwise, datatype was unecessary
    //pop argument datatype definition
    popSygusDatatypeDef( datatypes, sorts, ops, cnames, cargs, allow_const, unresolved_gterm_sym );
  }
  return t;
}

void Smt2::setSygusStartIndex(const std::string& fun,
                              int startIndex,
                              std::vector<CVC4::Datatype>& datatypes,
                              std::vector<api::Sort>& sorts,
                              std::vector<std::vector<ParseOp>>& ops)
{
  if( startIndex>0 ){
    CVC4::Datatype tmp_dt = datatypes[0];
    api::Sort tmp_sort = sorts[0];
    std::vector<ParseOp> tmp_ops;
    tmp_ops.insert( tmp_ops.end(), ops[0].begin(), ops[0].end() );
    datatypes[0] = datatypes[startIndex];
    sorts[0] = sorts[startIndex];
    ops[0].clear();
    ops[0].insert( ops[0].end(), ops[startIndex].begin(), ops[startIndex].end() );
    datatypes[startIndex] = tmp_dt;
    sorts[startIndex] = tmp_sort;
    ops[startIndex].clear();
    ops[startIndex].insert( ops[startIndex].begin(), tmp_ops.begin(), tmp_ops.end() );
  }else if( startIndex<0 ){
    std::stringstream ss;
    ss << "warning: no symbol named Start for synth-fun " << fun << std::endl;
    warning(ss.str());
  }
}

void Smt2::mkSygusDatatype(CVC4::Datatype& dt,
                           std::vector<ParseOp>& ops,
                           std::vector<std::string>& cnames,
                           std::vector<std::vector<api::Sort>>& cargs,
                           std::vector<std::string>& unresolved_gterm_sym,
                           std::map<api::Sort, api::Sort>& sygus_to_builtin)
{
  Debug("parser-sygus") << "Making sygus datatype " << dt.getName() << std::endl;
  Debug("parser-sygus") << "  add constructors..." << std::endl;
  
  Debug("parser-sygus") << "SMT2 sygus parser : Making constructors for sygus datatype " << dt.getName() << std::endl;
  Debug("parser-sygus") << "  add constructors..." << std::endl;
  // size of cnames changes, this loop must check size
  for (unsigned i = 0; i < cnames.size(); i++)
  {
    bool is_dup = false;
    bool is_dup_op = false;
    for (unsigned j = 0; j < i; j++)
    {
      if( ops[i]==ops[j] ){
        is_dup_op = true;
        if( cargs[i].size()==cargs[j].size() ){
          is_dup = true;
          for( unsigned k=0; k<cargs[i].size(); k++ ){
            if( cargs[i][k]!=cargs[j][k] ){
              is_dup = false;
              break;
            }
          }
        }
        if( is_dup ){
          break;
        }
      }
    }
    Debug("parser-sygus") << "SYGUS CONS " << i << " : ";
    if( is_dup ){
      Debug("parser-sygus") << "--> Duplicate gterm : " << ops[i] << std::endl;
      ops.erase( ops.begin() + i, ops.begin() + i + 1 );
      cnames.erase( cnames.begin() + i, cnames.begin() + i + 1 );
      cargs.erase( cargs.begin() + i, cargs.begin() + i + 1 );
      i--;
    }
    else
    {
      std::shared_ptr<SygusPrintCallback> spc;
      if (is_dup_op)
      {
        Debug("parser-sygus") << "--> Duplicate gterm operator : " << ops[i]
                              << std::endl;
        // make into define-fun
        std::vector<api::Sort> ltypes;
        for (unsigned j = 0, size = cargs[i].size(); j < size; j++)
        {
          ltypes.push_back(sygus_to_builtin[cargs[i][j]]);
        }
        std::vector<api::Term> largs;
        api::Term lbvl = makeSygusBoundVarList(dt, i, ltypes, largs);

        // make the let_body
        api::Term body = applyParseOp(ops[i], largs);
        // replace by lambda
        ParseOp pLam;
        pLam.d_expr = d_solver->mkTerm(api::LAMBDA, lbvl, body);
        ops[i] = pLam;
        Debug("parser-sygus") << "  ...replace op : " << ops[i] << std::endl;
        // callback prints as the expression
        spc = std::make_shared<printer::SygusExprPrintCallback>(
            body.getExpr(), api::termVectorToExprs(largs));
      }
      else
      {
        api::Term sop = ops[i].d_expr;
        if (!sop.isNull() && sop.getSort().isBitVector() && sop.isConst())
        {
          Debug("parser-sygus") << "--> Bit-vector constant " << sop << " ("
                                << cnames[i] << ")" << std::endl;
          // Since there are multiple output formats for bit-vectors and
          // we are required by sygus standards to print in the exact input
          // format given by the user, we use a print callback to custom print
          // the given name.
          spc = std::make_shared<printer::SygusNamedPrintCallback>(cnames[i]);
        }
        else if (!sop.isNull() && sop.getKind() == api::VARIABLE)
        {
          Debug("parser-sygus") << "--> Defined function " << ops[i]
                                << std::endl;
          // turn f into (lammbda (x) (f x))
          // in a degenerate case, ops[i] may be a defined constant,
          // in which case we do not replace by a lambda.
          if (sop.getSort().isFunction())
          {
            std::vector<api::Sort> ftypes =
                sop.getSort().getFunctionDomainSorts();
            std::vector<api::Term> largs;
            api::Term lbvl = makeSygusBoundVarList(dt, i, ftypes, largs);
            largs.insert(largs.begin(), sop);
            api::Term body = d_solver->mkTerm(api::APPLY_UF, largs);
            ops[i].d_expr = d_solver->mkTerm(api::LAMBDA, lbvl, body);
            Debug("parser-sygus") << "  ...replace op : " << ops[i]
                                  << std::endl;
          }
          else
          {
            Debug("parser-sygus") << "  ...replace op : " << ops[i]
                                  << std::endl;
          }
          // keep a callback to say it should be printed with the defined name
          spc = std::make_shared<printer::SygusNamedPrintCallback>(cnames[i]);
        }
        else
        {
          Debug("parser-sygus") << "--> Default case " << ops[i] << std::endl;
        }
      }
      // must rename to avoid duplication
      std::stringstream ss;
      ss << dt.getName() << "_" << i << "_" << cnames[i];
      cnames[i] = ss.str();
      Debug("parser-sygus") << "  construct the datatype " << cnames[i] << "..."
                            << std::endl;

      // Add the sygus constructor, either using the expression operator of
      // ops[i], or the kind.
      if (!ops[i].d_expr.isNull())
      {
        dt.addSygusConstructor(ops[i].d_expr.getExpr(),
                               cnames[i],
                               api::sortVectorToTypes(cargs[i]),
                               spc);
      }
      else if (ops[i].d_kind != api::NULL_EXPR)
      {
        dt.addSygusConstructor(extToIntKind(ops[i].d_kind),
                               cnames[i],
                               api::sortVectorToTypes(cargs[i]),
                               spc);
      }
      else
      {
        std::stringstream ess;
        ess << "unexpected parse operator for sygus constructor" << ops[i];
        parseError(ess.str());
      }
      Debug("parser-sygus") << "  finished constructing the datatype"
                            << std::endl;
    }
  }

  Debug("parser-sygus") << "  add constructors for unresolved symbols..." << std::endl;
  if( !unresolved_gterm_sym.empty() ){
    std::vector<api::Sort> types;
    Debug("parser-sygus") << "...resolve " << unresolved_gterm_sym.size() << " symbols..." << std::endl;
    for( unsigned i=0; i<unresolved_gterm_sym.size(); i++ ){
      Debug("parser-sygus") << "  resolve : " << unresolved_gterm_sym[i] << std::endl;
      if( isUnresolvedType(unresolved_gterm_sym[i]) ){
        Debug("parser-sygus") << "    it is an unresolved type." << std::endl;
        api::Sort t = getSort(unresolved_gterm_sym[i]);
        if( std::find( types.begin(), types.end(), t )==types.end() ){
          types.push_back( t );
          //identity element
          api::Sort bt = dt.getSygusType();
          Debug("parser-sygus") << ":  make identity function for " << bt << ", argument type " << t << std::endl;

          std::stringstream ss;
          ss << t << "_x";
          api::Term var = bindBoundVar(ss.str(), bt);
          std::vector<api::Term> lchildren;
          lchildren.push_back(d_solver->mkTerm(api::BOUND_VAR_LIST, var));
          lchildren.push_back(var);
          api::Term id_op = d_solver->mkTerm(api::LAMBDA, lchildren);

          // empty sygus callback (should not be printed)
          std::shared_ptr<SygusPrintCallback> sepc =
              std::make_shared<printer::SygusEmptyPrintCallback>();

          //make the sygus argument list
          std::vector<api::Sort> id_carg;
          id_carg.push_back( t );
          dt.addSygusConstructor(id_op.getExpr(),
                                 unresolved_gterm_sym[i],
                                 api::sortVectorToTypes(id_carg),
                                 sepc);

          //add to operators
          ParseOp idOp;
          idOp.d_expr = id_op;
          ops.push_back(idOp);
        }
      }else{
        std::stringstream ss;
        ss << "Unhandled sygus constructor " << unresolved_gterm_sym[i];
        throw ParserException(ss.str());
      }
    }
  }
}

api::Term Smt2::makeSygusBoundVarList(CVC4::Datatype& dt,
                                      unsigned i,
                                      const std::vector<api::Sort>& ltypes,
                                      std::vector<api::Term>& lvars)
{
  for (unsigned j = 0, size = ltypes.size(); j < size; j++)
  {
    std::stringstream ss;
    ss << dt.getName() << "_x_" << i << "_" << j;
    api::Term v = bindBoundVar(ss.str(), ltypes[j]);
    lvars.push_back(v);
  }
  return d_solver->mkTerm(api::BOUND_VAR_LIST, lvars);
}

void Smt2::addSygusConstructorTerm(
    Datatype& dt,
    api::Term term,
    std::map<api::Term, api::Sort>& ntsToUnres) const
{
  Trace("parser-sygus2") << "Add sygus cons term " << term << std::endl;
  // At this point, we should know that dt is well founded, and that its
  // builtin sygus operators are well-typed.
  // Now, purify each occurrence of a non-terminal symbol in term, replace by
  // free variables. These become arguments to constructors. Notice we must do
  // a tree traversal in this function, since unique paths to the same term
  // should be treated as distinct terms.
  // Notice that let expressions are forbidden in the input syntax of term, so
  // this does not lead to exponential behavior with respect to input size.
  std::vector<api::Term> args;
  std::vector<api::Sort> cargs;
  api::Term op = purifySygusGTerm(term, ntsToUnres, args, cargs);
  std::stringstream ssCName;
  ssCName << op.getKind();
  Trace("parser-sygus2") << "Purified operator " << op
                         << ", #args/cargs=" << args.size() << "/"
                         << cargs.size() << std::endl;
  std::shared_ptr<SygusPrintCallback> spc;
  // callback prints as the expression
  spc = std::make_shared<printer::SygusExprPrintCallback>(
      op.getExpr(), api::termVectorToExprs(args));
  if (!args.empty())
  {
    api::Term lbvl = d_solver->mkTerm(api::BOUND_VAR_LIST, args);
    // its operator is a lambda
    op = d_solver->mkTerm(api::LAMBDA, lbvl, op);
  }
  Trace("parser-sygus2") << "addSygusConstructor:  operator " << op
                         << std::endl;
  dt.addSygusConstructor(
      op.getExpr(), ssCName.str(), api::sortVectorToTypes(cargs), spc);
}

api::Term Smt2::purifySygusGTerm(api::Term term,
                                 std::map<api::Term, api::Sort>& ntsToUnres,
                                 std::vector<api::Term>& args,
                                 std::vector<api::Sort>& cargs) const
{
  Trace("parser-sygus2-debug")
      << "purifySygusGTerm: " << term
      << " #nchild=" << term.getExpr().getNumChildren() << std::endl;
  std::map<api::Term, api::Sort>::iterator itn = ntsToUnres.find(term);
  if (itn != ntsToUnres.end())
  {
    api::Term ret = d_solver->mkVar(term.getSort());
    Trace("parser-sygus2-debug")
        << "...unresolved non-terminal, intro " << ret << std::endl;
    args.push_back(ret.getExpr());
    cargs.push_back(itn->second);
    return ret;
  }
  std::vector<api::Term> pchildren;
  bool childChanged = false;
  for (unsigned i = 0, nchild = term.getNumChildren(); i < nchild; i++)
  {
    Trace("parser-sygus2-debug")
        << "......purify child " << i << " : " << term[i] << std::endl;
    api::Term ptermc = purifySygusGTerm(term[i], ntsToUnres, args, cargs);
    pchildren.push_back(ptermc);
    childChanged = childChanged || ptermc != term[i];
  }
  if (!childChanged)
  {
    Trace("parser-sygus2-debug") << "...no child changed" << std::endl;
    return term;
  }
  api::Term nret = d_solver->mkTerm(term.getOp(), pchildren);
  Trace("parser-sygus2-debug")
      << "...child changed, return " << nret << std::endl;
  return nret;
}

void Smt2::addSygusConstructorVariables(Datatype& dt,
                                        const std::vector<api::Term>& sygusVars,
                                        api::Sort type) const
{
  // each variable of appropriate type becomes a sygus constructor in dt.
  for (unsigned i = 0, size = sygusVars.size(); i < size; i++)
  {
    api::Term v = sygusVars[i];
    if (v.getSort() == type)
    {
      std::stringstream ss;
      ss << v;
      std::vector<api::Sort> cargs;
      dt.addSygusConstructor(
          v.getExpr(), ss.str(), api::sortVectorToTypes(cargs));
    }
  }
}

InputLanguage Smt2::getLanguage() const
{
  return getExprManager()->getOptions().getInputLanguage();
}

void Smt2::parseOpApplyTypeAscription(ParseOp& p, api::Sort type)
{
  Debug("parser") << "parseOpApplyTypeAscription : " << p << " " << type
                  << std::endl;
  // (as const (Array T1 T2))
  if (p.d_kind == api::STORE_ALL)
  {
    if (!type.isArray())
    {
      std::stringstream ss;
      ss << "expected array constant term, but cast is not of array type"
         << std::endl
         << "cast type: " << type;
      parseError(ss.str());
    }
    p.d_type = type;
    return;
  }
  if (p.d_expr.isNull())
  {
    Trace("parser-overloading")
        << "Getting variable expression with name " << p.d_name << " and type "
        << type << std::endl;
    // get the variable expression for the type
    if (isDeclared(p.d_name, SYM_VARIABLE))
    {
      p.d_expr = getExpressionForNameAndType(p.d_name, type);
      p.d_name = std::string("");
    }
    if (p.d_expr.isNull())
    {
      std::stringstream ss;
      ss << "Could not resolve expression with name " << p.d_name
         << " and type " << type << std::endl;
      parseError(ss.str());
    }
  }
  Trace("parser-qid") << "Resolve ascription " << type << " on " << p.d_expr;
  Trace("parser-qid") << " " << p.d_expr.getKind() << " " << p.d_expr.getSort();
  Trace("parser-qid") << std::endl;
  // otherwise, we process the type ascription
  p.d_expr =
      applyTypeAscription(api::Term(p.d_expr), api::Sort(type)).getExpr();
}

api::Term Smt2::parseOpToExpr(ParseOp& p)
{
  Debug("parser") << "parseOpToExpr: " << p << std::endl;
  api::Term expr;
  if (p.d_kind != api::NULL_EXPR || !p.d_type.isNull())
  {
    parseError(
        "Bad syntax for qualified identifier operator in term position.");
  }
  else if (!p.d_expr.isNull())
  {
    expr = p.d_expr;
  }
  else if (!isDeclared(p.d_name, SYM_VARIABLE))
  {
    if (sygus_v1() && p.d_name[0] == '-'
        && p.d_name.find_first_not_of("0123456789", 1) == std::string::npos)
    {
      // allow unary minus in sygus version 1
      expr = d_solver->mkReal(p.d_name);
    }
    else
    {
      std::stringstream ss;
      ss << "Symbol " << p.d_name << " is not declared.";
      parseError(ss.str());
    }
  }
  else
  {
    expr = getExpressionForName(p.d_name);
  }
  assert(!expr.isNull());
  return expr;
}

api::Term Smt2::applyParseOp(ParseOp& p, std::vector<api::Term>& args)
{
  bool isBuiltinOperator = false;
  // the builtin kind of the overall return expression
  api::Kind kind = api::NULL_EXPR;
  // First phase: process the operator
  if (Debug.isOn("parser"))
  {
    Debug("parser") << "applyParseOp: " << p << " to:" << std::endl;
    for (std::vector<api::Term>::iterator i = args.begin(); i != args.end();
         ++i)
    {
      Debug("parser") << "++ " << *i << std::endl;
    }
  }
  api::Op op;
  if (p.d_kind != api::NULL_EXPR)
  {
    // It is a special case, e.g. tupSel or array constant specification.
    // We have to wait until the arguments are parsed to resolve it.
  }
  else if (!p.d_expr.isNull())
  {
    // An explicit operator, e.g. an apply function
    api::Kind fkind = getKindForFunction(p.d_expr);
    if (fkind != api::UNDEFINED_KIND)
    {
      // Some operators may require a specific kind.
      // Testers are handled differently than other indexed operators,
      // since they require a kind.
      kind = fkind;
      Debug("parser") << "Got function kind " << kind << " for expression "
                      << std::endl;
    }
    args.insert(args.begin(), p.d_expr);
  }
  else if (!p.d_op.isNull())
  {
    // it was given an operator
    op = p.d_op;
  }
  else
  {
    isBuiltinOperator = isOperatorEnabled(p.d_name);
    if (isBuiltinOperator)
    {
      // a builtin operator, convert to kind
      kind = getOperatorKind(p.d_name);
    }
    else
    {
      // A non-built-in function application, get the expression
      checkDeclaration(p.d_name, CHECK_DECLARED, SYM_VARIABLE);
      api::Term v = getVariable(p.d_name);
      if (!v.isNull())
      {
        checkFunctionLike(v);
        kind = getKindForFunction(v);
        args.insert(args.begin(), v);
      }
      else
      {
        // Overloaded symbol?
        // Could not find the expression. It may be an overloaded symbol,
        // in which case we may find it after knowing the types of its
        // arguments.
        std::vector<api::Sort> argTypes;
        for (std::vector<api::Term>::iterator i = args.begin(); i != args.end();
             ++i)
        {
          argTypes.push_back((*i).getSort());
        }
        api::Term fop = getOverloadedFunctionForTypes(p.d_name, argTypes);
        if (!fop.isNull())
        {
          checkFunctionLike(fop);
          kind = getKindForFunction(fop);
          args.insert(args.begin(), fop);
        }
        else
        {
          parseError(
              "Cannot find unambiguous overloaded function for argument "
              "types.");
        }
      }
    }
  }
  // Second phase: apply the arguments to the parse op
  ExprManager* em = getExprManager();
  // handle special cases
  if (p.d_kind == api::STORE_ALL && !p.d_type.isNull())
  {
    if (args.size() != 1)
    {
      parseError("Too many arguments to array constant.");
    }
    api::Term constVal = args[0];
    if (!constVal.isConst())
    {
      // To parse array constants taking reals whose values are specified by
      // rationals, e.g. ((as const (Array Int Real)) (/ 1 3)), we must handle
      // the fact that (/ 1 3) is the division of constants 1 and 3, and not
      // the resulting constant rational value. Thus, we must construct the
      // resulting rational here. This also is applied for integral real values
      // like 5.0 which are converted to (/ 5 1) to distinguish them from
      // integer constants. We must ensure numerator and denominator are
      // constant and the denominator is non-zero.
      if (constVal.getKind() == api::DIVISION && constVal[0].isConst()
          && constVal[1].isConst()
          && !constVal[1].getExpr().getConst<Rational>().isZero())
      {
        std::stringstream sdiv;
        sdiv << constVal[0] << "/" << constVal[1];
        constVal = d_solver->mkReal(sdiv.str());
      }
      if (!constVal.isConst())
      {
        std::stringstream ss;
        ss << "expected constant term inside array constant, but found "
           << "nonconstant term:" << std::endl
           << "the term: " << constVal;
        parseError(ss.str());
      }
    }
    if (!p.d_type.getArrayElementSort().isComparableTo(constVal.getSort()))
    {
      std::stringstream ss;
      ss << "type mismatch inside array constant term:" << std::endl
         << "array type:          " << p.d_type << std::endl
         << "expected const type: " << p.d_type.getArrayElementSort()
         << std::endl
         << "computed const type: " << constVal.getSort();
      parseError(ss.str());
    }
    api::Term ret = d_solver->mkConstArray(p.d_type, constVal);
    Debug("parser") << "applyParseOp: return store all " << ret << std::endl;
    return ret;
  }
  else if (p.d_kind == api::APPLY_SELECTOR && !p.d_expr.isNull())
  {
    // tuple selector case
    Integer x = p.d_expr.getExpr().getConst<Rational>().getNumerator();
    if (!x.fitsUnsignedInt())
    {
      parseError("index of tupSel is larger than size of unsigned int");
    }
    unsigned int n = x.toUnsignedInt();
    if (args.size() != 1)
    {
      parseError("tupSel should only be applied to one tuple argument");
    }
    api::Sort t = args[0].getSort();
    if (!t.isTuple())
    {
      parseError("tupSel applied to non-tuple");
    }
    size_t length = t.getTupleLength();
    if (n >= length)
    {
      std::stringstream ss;
      ss << "tuple is of length " << length << "; cannot access index " << n;
      parseError(ss.str());
    }
    const Datatype& dt = ((DatatypeType)t.getType()).getDatatype();
    api::Term ret = d_solver->mkTerm(
        api::APPLY_SELECTOR, api::Term(dt[0][n].getSelector()), args[0]);
    Debug("parser") << "applyParseOp: return selector " << ret << std::endl;
    return ret;
  }
  else if (p.d_kind != api::NULL_EXPR)
  {
    // it should not have an expression or type specified at this point
    if (!p.d_expr.isNull() || !p.d_type.isNull())
    {
      std::stringstream ss;
      ss << "Could not process parsed qualified identifier kind " << p.d_kind;
      parseError(ss.str());
    }
    // otherwise it is a simple application
    kind = p.d_kind;
  }
  else if (isBuiltinOperator)
  {
    Trace("ajr-temp") << "mkTerm builtin operator" << std::endl;
    if (!em->getOptions().getUfHo()
        && (kind == api::EQUAL || kind == api::DISTINCT))
    {
      // need --uf-ho if these operators are applied over function args
      for (std::vector<api::Term>::iterator i = args.begin(); i != args.end();
           ++i)
      {
        if ((*i).getSort().isFunction())
        {
          parseError(
              "Cannot apply equalty to functions unless --uf-ho is set.");
        }
      }
    }
    if (!strictModeEnabled() && (kind == api::AND || kind == api::OR)
        && args.size() == 1)
    {
      // Unary AND/OR can be replaced with the argument.
      Debug("parser") << "applyParseOp: return unary " << args[0] << std::endl;
      return args[0];
    }
    else if (kind == api::MINUS && args.size() == 1)
    {
      api::Term ret = d_solver->mkTerm(api::UMINUS, args[0]);
      Debug("parser") << "applyParseOp: return uminus " << ret << std::endl;
      return ret;
    }
    api::Term ret = d_solver->mkTerm(kind, args);
    Debug("parser") << "applyParseOp: return default builtin " << ret
                    << std::endl;
    return ret;
  }

  if (args.size() >= 2)
  {
    // may be partially applied function, in this case we use HO_APPLY
    api::Sort argt = args[0].getSort();
    if (argt.isFunction())
    {
      unsigned arity = argt.getFunctionArity();
      if (args.size() - 1 < arity)
      {
        if (!em->getOptions().getUfHo())
        {
          parseError("Cannot partially apply functions unless --uf-ho is set.");
        }
        Debug("parser") << "Partial application of " << args[0];
        Debug("parser") << " : #argTypes = " << arity;
        Debug("parser") << ", #args = " << args.size() - 1 << std::endl;
        api::Term ret = d_solver->mkTerm(api::HO_APPLY, args);
        Debug("parser") << "applyParseOp: return curry higher order " << ret
                        << std::endl;
        // must curry the partial application
        return ret;
      }
    }
  }
  if (!op.isNull())
  {
    api::Term ret = d_solver->mkTerm(op, args);
    Debug("parser") << "applyParseOp: return op : " << ret << std::endl;
    return ret;
  }
  if (kind == api::NULL_EXPR)
  {
    // should never happen in the new API
    parseError("do not know how to process parse op");
  }
  Debug("parser") << "Try default term construction for kind " << kind
                  << " #args = " << args.size() << "..." << std::endl;
  api::Term ret = d_solver->mkTerm(kind, args);
  Debug("parser") << "applyParseOp: return : " << ret << std::endl;
  return ret;
}

api::Term Smt2::setNamedAttribute(api::Term& expr, const SExpr& sexpr)
{
  if (!sexpr.isKeyword())
  {
    parseError("improperly formed :named annotation");
  }
  std::string name = sexpr.getValue();
  checkUserSymbol(name);
  // ensure expr is a closed subterm
  if (expr.getExpr().hasFreeVariable())
  {
    std::stringstream ss;
    ss << ":named annotations can only name terms that are closed";
    parseError(ss.str());
  }
  // check that sexpr is a fresh function symbol, and reserve it
  reserveSymbolAtAssertionLevel(name);
  // define it
  api::Term func = bindVar(name, expr.getSort(), ExprManager::VAR_FLAG_DEFINED);
  // remember the last term to have been given a :named attribute
  setLastNamedTerm(expr, name);
  return func;
}

api::Term Smt2::mkAnd(const std::vector<api::Term>& es)
{
  if (es.size() == 0)
  {
    return d_solver->mkTrue();
  }
  else if (es.size() == 1)
  {
    return es[0];
  }
  else
  {
    return d_solver->mkTerm(api::AND, es);
  }
}

}  // namespace parser
}/* CVC4 namespace */

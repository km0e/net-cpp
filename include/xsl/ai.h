#pragma once
#ifndef XSL_AI
#  define XSL_AI
// abstract interface
#  include "xsl/ai/dev.h"
#  include "xsl/def.h"
XSL_NB
namespace ai {
  using _ai::ABRL;
  using _ai::ABRWL;
  using _ai::ABWL;
  using _ai::AsyncDevice;
  using _ai::AsyncReadDeviceLike;
  using _ai::AsyncReadWriteDeviceLike;
  using _ai::AsyncWritable;
  using _ai::AsyncWriteDeviceLike;
  using _ai::BRL;
  using _ai::BRWL;
  using _ai::BWL;
  using _ai::read_poly_resolve;
  using _ai::ReadDeviceLike;
  using _ai::write_poly_resolve;
  using _ai::WriteDeviceLike;
}  // namespace ai
using _ai::ABR;
using _ai::ABW;
using _ai::Result;
XSL_NE

#endif

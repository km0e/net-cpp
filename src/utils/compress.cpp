#include "xsl/ai/dev.h"
#include "xsl/wheel/def.h"

#include <brotli/encode.h>
XSL_WHEEL_NB
struct BrotliCompressorParameters {
  uint32_t mode = 0;
  uint32_t quality = 11;
  uint32_t lgwin = 22;
};

class BrotliCompressor {
public:
  using value_type = std::byte;
  BrotliCompressor() : state(BrotliEncoderCreateInstance(nullptr, nullptr, nullptr)) {}
  ~BrotliCompressor() { BrotliEncoderDestroyInstance(state); }
  void set_parameter(const BrotliCompressorParameters& parameters) {
    BrotliEncoderSetParameter(state, BROTLI_PARAM_MODE, parameters.mode);
    BrotliEncoderSetParameter(state, BROTLI_PARAM_QUALITY, parameters.quality);
    BrotliEncoderSetParameter(state, BROTLI_PARAM_LGWIN, parameters.lgwin);
  }

  ai::Result read(std::span<value_type> buf) {}

private:
  BrotliEncoderState* state;
};

static_assert(ai::ReadDeviceLike<BrotliCompressor, std::byte>);
XSL_WHEEL_NE

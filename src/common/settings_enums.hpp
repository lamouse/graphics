#pragma once

#include <cstdint>

namespace settings::enums
{
    enum class VSyncMode : uint32_t { Immediate, Mailbox, Fifo, FifoRelaxed };

    enum class AnisotropyMode : uint32_t { Automatic, Default, X2, X4, X8, X16 };

    enum class AstcDecodeMode : uint32_t { Cpu, Gpu, CpuAsynchronous };

    enum class AstcRecompression : uint32_t { Uncompressed, Bc1, Bc3 };

    enum class ScalingFilter : uint32_t { NearestNeighbor, Bilinear, Bicubic, Gaussian, ScaleForce, Fsr, MaxEnum };

    enum class VramUsageMode : uint32_t { Conservative, Aggressive };

    enum class ShaderBackend : uint32_t { Glsl, Glasm, SpirV };

    enum class AspectRatio : uint32_t { R16_9, R4_3, R21_9, R16_10, R32_9, Stretch };

    enum class LogLevel : uint32_t { debug, info, warn, trace, error, critical, off };
}


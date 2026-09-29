#pragma once

/// Mode blending untuk rendering Object3D
enum class BlendMode : int {
    Opaque     = 0,  ///< Tidak transparan, depth write aktif (default)
    AlphaBlend = 1,  ///< Alpha blending normal (SrcAlpha / InvSrcAlpha)
    Add        = 2,  ///< Additive blending (efek api, cahaya)
    Multiply   = 3,  ///< Multiply blending (efek bayangan gelap)
    Count      = 4
};

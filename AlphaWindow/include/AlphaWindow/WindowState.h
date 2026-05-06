#pragma once

namespace AW {

enum class WindowState {
    AW_Uninitialized,
    AW_Created,
    AW_Running,
    AW_Minimized,
    AW_Closing,
    AW_Destroyed
};

} // namespace AW

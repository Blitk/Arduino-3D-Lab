#pragma once

#include "emulator/CPU.hpp"
#include "emulator/InstructionDecoder.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

namespace arduino3d::emulator {

enum class StepStatus {
    Executed,
    InvalidInstruction,
    ProgramCounterOutOfRange,
    InvalidCpuState
};

struct StepResult {
    StepStatus status{StepStatus::Executed};
    std::uint16_t pcBefore{0};
    std::uint16_t word{0};
    std::string message;

    bool ok() const noexcept {
        return status == StepStatus::Executed;
    }
};

enum class RunStatus {
    LimitReached,
    InvalidInstruction,
    ProgramCounterOutOfRange,
    InvalidCpuState
};

struct RunResult {
    RunStatus status{RunStatus::LimitReached};
    std::size_t instructionsExecuted{0};
    StepResult lastStep{};

    bool ok() const noexcept {
        return status == RunStatus::LimitReached;
    }
};

class AVR {
public:
    explicit AVR(CPUState& cpu) noexcept;

    StepResult step();
    RunResult run(std::size_t instructionLimit);

    CPUState& cpu() noexcept;
    const CPUState& cpu() const noexcept;

private:
    void execute(const DecodedInstruction& instruction);

    CPUState& cpu_;
};

} // namespace arduino3d::emulator

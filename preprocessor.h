// Copyright (c) Open Enclave SDK contributors.
// Licensed under the MIT License.

#ifndef PREPROCESSOR_H
#define PREPROCESSOR_H

#include <string>
#include <vector>

#include "ast.h"

struct DirectiveState
{
    Directive command;
    bool condition;

    DirectiveState(Directive command_, bool condition_)
    {
        command = command_;
        condition = condition_;
    }
};

class Preprocessor
{
    std::vector<DirectiveState> stack_;
    const std::vector<std::string> defines_;

  public:
    Preprocessor(const std::vector<std::string>& defines) : defines_(defines)
    {
    }

    ~Preprocessor()
    {
        stack_.clear();
    }

    bool process(Directive cmd, const std::string& arg = "")
    {
        bool result = false;

        switch (cmd)
        {
            case Ifdef:
            {
                DirectiveState state(cmd, false);
                if (is_defined(arg))
                    state.condition = true;

                stack_.push_back(state);
                result = true;
                break;
            }
            case Ifndef:
            {
                DirectiveState state(cmd, false);
                if (!is_defined(arg))
                    state.condition = true;

                stack_.push_back(state);
                result = true;
                break;
            }
            case Else:
            {
                DirectiveState& current_state = stack_.back();
                /* Verify that the current state is ifdef or ifndef. */
                if (current_state.command != Ifdef &&
                    current_state.command != Ifndef)
                    break;

                current_state.command = cmd;
                current_state.condition = !current_state.condition;
                result = true;
                break;
            }
            case Endif:
            {
                /* Ensure the stack is not empty. */
                if (stack_.empty())
                    break;

                DirectiveState& current_state = stack_.back();
                /* Verify that the current state is ifdef, ifndef, or else. */
                if (current_state.command != Ifdef &&
                    current_state.command != Ifndef &&
                    current_state.command != Else)
                    break;

                stack_.pop_back();
                result = true;
                break;
            }
            default:
            {
                /* Do nothing. */
            }
        }

        return result;
    }

    bool is_defined(const std::string& name)
    {
        bool found = false;
        for (auto& define : defines_)
        {
            if (name == define)
            {
                found = true;
                break;
            }
        }
        return found;
    }

    /* Determine if the code needs to be included based on the state of
     * preprocessor. */
    bool is_included()
    {
        for (auto& s : stack_)
        {
            if (!s.condition)
                return false;
        }
        return true;
    }

    /* Determine if there is an open control block. */
    bool is_closed()
    {
        return stack_.empty();
    }
};

#endif // PREPROCESSOR_H

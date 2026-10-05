# MEDISENSE-AI Agent Guide

## Scope
- Suhani owns only `src/symptoms/symptom_hash.h`, `src/symptoms/symptom_hash.c`, and `tests/symptoms/symptom_hash_test.c`.
- Do not implement or modify the history, triage, or main modules except for explicitly requested integration placeholders.
- Keep the symptom module independent and expose it through its public header.

## Collaboration
- Teach the work in stages and wait for Suhani to confirm understanding before moving to the next stage.
- Explain each function's purpose, inputs, output, and logic before presenting its implementation.
- Prefer small, focused changes, clear C names, input validation, and careful memory ownership.
- Use separate chaining for hash collisions unless the project specification requires another method.
- Test the symptom module independently and explain what each test verifies.

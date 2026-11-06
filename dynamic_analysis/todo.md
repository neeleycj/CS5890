## 1. Set Up AddressSanitizer

- Follow the official documentation to configure Clang AddressSanitizer.

- Create dynamic_analysis/dyn_analysis.sh to build your project with ASan instrumentation and execute the test cases.

- Ensure the script can be reproduced from a clean project checkout with the required tools installed.

## 2. Design and Execute Diverse Test Cases

- Design a set of test inputs and execution scenarios covering:

- Normal inputs: Exercise expected program functionality.

- Boundary inputs: Test minimum, maximum, empty, and out-of-range values.

- Malformed inputs: Test invalid formats, truncated data, and inconsistent fields.

- Failure scenarios: Exercise error handling, interrupted operations, and resource cleanup.

---
Select test cases relevant to your project. Focus on exercising different program behaviors and execution paths, rather than simply increasing the number of inputs.

Place your test inputs and supporting scripts (if necessary) under dynamic_analysis/tests/. Execute the tests with ASan enabled and examine the reported diagnostics.
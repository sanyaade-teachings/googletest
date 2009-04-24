// Copyright 2005, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// A unit test for Google Test itself.  This verifies that the basic
// constructs of Google Test work.
//
// Author: wan@google.com (Zhanyong Wan)

#include <gtest/gtest-spi.h>
#include <gtest/gtest.h>

// Indicates that this translation unit is part of Google Test's
// implementation.  It must come before gtest-internal-inl.h is
// included, or there will be a compiler error.  This trick is to
// prevent a user from accidentally including gtest-internal-inl.h in
// his code.
#define GTEST_IMPLEMENTATION_ 1
#include "src/gtest-internal-inl.h"
#undef GTEST_IMPLEMENTATION_

#include <stdlib.h>

#if GTEST_HAS_PTHREAD
#include <pthread.h>
#endif  // GTEST_HAS_PTHREAD

#if GTEST_OS_LINUX
#include <string.h>
#include <signal.h>
#include <string>
#include <vector>
#endif  // GTEST_OS_LINUX

using testing::ScopedFakeTestPartResultReporter;
using testing::TestPartResultArray;

namespace posix = ::testing::internal::posix;
using testing::internal::String;

// Tests catching fatal failures.

// A subroutine used by the following test.
void TestEq1(int x) {
  ASSERT_EQ(1, x);
}

// This function calls a test subroutine, catches the fatal failure it
// generates, and then returns early.
void TryTestSubroutine() {
  // Calls a subrountine that yields a fatal failure.
  TestEq1(2);

  // Catches the fatal failure and aborts the test.
  //
  // The testing::Test:: prefix is necessary when calling
  // HasFatalFailure() outside of a TEST, TEST_F, or test fixture.
  if (testing::Test::HasFatalFailure()) return;

  // If we get here, something is wrong.
  FAIL() << "This should never be reached.";
}

TEST(PassingTest, PassingTest1) {
}

TEST(PassingTest, PassingTest2) {
}

// Tests catching a fatal failure in a subroutine.
TEST(FatalFailureTest, FatalFailureInSubroutine) {
  printf("(expecting a failure that x should be 1)\n");

  TryTestSubroutine();
}

// Tests catching a fatal failure in a nested subroutine.
TEST(FatalFailureTest, FatalFailureInNestedSubroutine) {
  printf("(expecting a failure that x should be 1)\n");

  // Calls a subrountine that yields a fatal failure.
  TryTestSubroutine();

  // Catches the fatal failure and aborts the test.
  //
  // When calling HasFatalFailure() inside a TEST, TEST_F, or test
  // fixture, the testing::Test:: prefix is not needed.
  if (HasFatalFailure()) return;

  // If we get here, something is wrong.
  FAIL() << "This should never be reached.";
}

// Tests HasFatalFailure() after a failed EXPECT check.
TEST(FatalFailureTest, NonfatalFailureInSubroutine) {
  printf("(expecting a failure on false)\n");
  EXPECT_TRUE(false);  // Generates a nonfatal failure
  ASSERT_FALSE(HasFatalFailure());  // This should succeed.
}

// Tests interleaving user logging and Google Test assertions.
TEST(LoggingTest, InterleavingLoggingAndAssertions) {
  static const int a[4] = {
    3, 9, 2, 6
  };

  printf("(expecting 2 failures on (3) >= (a[i]))\n");
  for (int i = 0; i < static_cast<int>(sizeof(a)/sizeof(*a)); i++) {
    printf("i == %d\n", i);
    EXPECT_GE(3, a[i]);
  }
}

// Tests the SCOPED_TRACE macro.

// A helper function for testing SCOPED_TRACE.
void SubWithoutTrace(int n) {
  EXPECT_EQ(1, n);
  ASSERT_EQ(2, n);
}

// Another helper function for testing SCOPED_TRACE.
void SubWithTrace(int n) {
  SCOPED_TRACE(testing::Message() << "n = " << n);

  SubWithoutTrace(n);
}

// Tests that SCOPED_TRACE() obeys lexical scopes.
TEST(SCOPED_TRACETest, ObeysScopes) {
  printf("(expected to fail)\n");

  // There should be no trace before SCOPED_TRACE() is invoked.
  ADD_FAILURE() << "This failure is expected, and shouldn't have a trace.";

  {
    SCOPED_TRACE("Expected trace");
    // After SCOPED_TRACE(), a failure in the current scope should contain
    // the trace.
    ADD_FAILURE() << "This failure is expected, and should have a trace.";
  }

  // Once the control leaves the scope of the SCOPED_TRACE(), there
  // should be no trace again.
  ADD_FAILURE() << "This failure is expected, and shouldn't have a trace.";
}

// Tests that SCOPED_TRACE works inside a loop.
TEST(SCOPED_TRACETest, WorksInLoop) {
  printf("(expected to fail)\n");

  for (int i = 1; i <= 2; i++) {
    SCOPED_TRACE(testing::Message() << "i = " << i);

    SubWithoutTrace(i);
  }
}

// Tests that SCOPED_TRACE works in a subroutine.
TEST(SCOPED_TRACETest, WorksInSubroutine) {
  printf("(expected to fail)\n");

  SubWithTrace(1);
  SubWithTrace(2);
}

// Tests that SCOPED_TRACE can be nested.
TEST(SCOPED_TRACETest, CanBeNested) {
  printf("(expected to fail)\n");

  SCOPED_TRACE("");  // A trace without a message.

  SubWithTrace(2);
}

// Tests that multiple SCOPED_TRACEs can be used in the same scope.
TEST(SCOPED_TRACETest, CanBeRepeated) {
  printf("(expected to fail)\n");

  SCOPED_TRACE("A");
  ADD_FAILURE()
      << "This failure is expected, and should contain trace point A.";

  SCOPED_TRACE("B");
  ADD_FAILURE()
      << "This failure is expected, and should contain trace point A and B.";

  {
    SCOPED_TRACE("C");
    ADD_FAILURE() << "This failure is expected, and should contain "
                  << "trace point A, B, and C.";
  }

  SCOPED_TRACE("D");
  ADD_FAILURE() << "This failure is expected, and should contain "
                << "trace point A, B, and D.";
}

TEST(DisabledTestsWarningTest,
     DISABLED_AlsoRunDisabledTestsFlagSuppressesWarning) {
  // This test body is intentionally empty.  Its sole purpose is for
  // verifying that the --gtest_also_run_disabled_tests flag
  // suppresses the "YOU HAVE 12 DISABLED TESTS" warning at the end of
  // the test output.
}

// Tests using assertions outside of TEST and TEST_F.
//
// This function creates two failures intentionally.
void AdHocTest() {
  printf("The non-test part of the code is expected to have 2 failures.\n\n");
  EXPECT_TRUE(false);
  EXPECT_EQ(2, 3);
}

// Runs all TESTs, all TEST_Fs, and the ad hoc test.
int RunAllTests() {
  AdHocTest();
  return RUN_ALL_TESTS();
}

// Tests non-fatal failures in the fixture constructor.
class NonFatalFailureInFixtureConstructorTest : public testing::Test {
 protected:
  NonFatalFailureInFixtureConstructorTest() {
    printf("(expecting 5 failures)\n");
    ADD_FAILURE() << "Expected failure #1, in the test fixture c'tor.";
  }

  ~NonFatalFailureInFixtureConstructorTest() {
    ADD_FAILURE() << "Expected failure #5, in the test fixture d'tor.";
  }

  virtual void SetUp() {
    ADD_FAILURE() << "Expected failure #2, in SetUp().";
  }

  virtual void TearDown() {
    ADD_FAILURE() << "Expected failure #4, in TearDown.";
  }
};

TEST_F(NonFatalFailureInFixtureConstructorTest, FailureInConstructor) {
  ADD_FAILURE() << "Expected failure #3, in the test body.";
}

// Tests fatal failures in the fixture constructor.
class FatalFailureInFixtureConstructorTest : public testing::Test {
 protected:
  FatalFailureInFixtureConstructorTest() {
    printf("(expecting 2 failures)\n");
    Init();
  }

  ~FatalFailureInFixtureConstructorTest() {
    ADD_FAILURE() << "Expected failure #2, in the test fixture d'tor.";
  }

  virtual void SetUp() {
    ADD_FAILURE() << "UNEXPECTED failure in SetUp().  "
                  << "We should never get here, as the test fixture c'tor "
                  << "had a fatal failure.";
  }

  virtual void TearDown() {
    ADD_FAILURE() << "UNEXPECTED failure in TearDown().  "
                  << "We should never get here, as the test fixture c'tor "
                  << "had a fatal failure.";
  }
 private:
  void Init() {
    FAIL() << "Expected failure #1, in the test fixture c'tor.";
  }
};

TEST_F(FatalFailureInFixtureConstructorTest, FailureInConstructor) {
  ADD_FAILURE() << "UNEXPECTED failure in the test body.  "
                << "We should never get here, as the test fixture c'tor "
                << "had a fatal failure.";
}

// Tests non-fatal failures in SetUp().
class NonFatalFailureInSetUpTest : public testing::Test {
 protected:
  virtual ~NonFatalFailureInSetUpTest() {
    Deinit();
  }

  virtual void SetUp() {
    printf("(expecting 4 failures)\n");
    ADD_FAILURE() << "Expected failure #1, in SetUp().";
  }

  virtual void TearDown() {
    FAIL() << "Expected failure #3, in TearDown().";
  }
 private:
  void Deinit() {
    FAIL() << "Expected failure #4, in the test fixture d'tor.";
  }
};

TEST_F(NonFatalFailureInSetUpTest, FailureInSetUp) {
  FAIL() << "Expected failure #2, in the test function.";
}

// Tests fatal failures in SetUp().
class FatalFailureInSetUpTest : public testing::Test {
 protected:
  virtual ~FatalFailureInSetUpTest() {
    Deinit();
  }

  virtual void SetUp() {
    printf("(expecting 3 failures)\n");
    FAIL() << "Expected failure #1, in SetUp().";
  }

  virtual void TearDown() {
    FAIL() << "Expected failure #2, in TearDown().";
  }
 private:
  void Deinit() {
    FAIL() << "Expected failure #3, in the test fixture d'tor.";
  }
};

TEST_F(FatalFailureInSetUpTest, FailureInSetUp) {
  FAIL() << "UNEXPECTED failure in the test function.  "
         << "We should never get here, as SetUp() failed.";
}

#if GTEST_OS_WINDOWS

// This group of tests verifies that Google Test handles SEH and C++
// exceptions correctly.

// A function that throws an SEH exception.
static void ThrowSEH() {
  int* p = NULL;
  *p = 0;  // Raises an access violation.
}

// Tests exceptions thrown in the test fixture constructor.
class ExceptionInFixtureCtorTest : public testing::Test {
 protected:
  ExceptionInFixtureCtorTest() {
    printf("(expecting a failure on thrown exception "
           "in the test fixture's constructor)\n");

    ThrowSEH();
  }

  virtual ~ExceptionInFixtureCtorTest() {
    Deinit();
  }

  virtual void SetUp() {
    FAIL() << "UNEXPECTED failure in SetUp().  "
           << "We should never get here, as the test fixture c'tor threw.";
  }

  virtual void TearDown() {
    FAIL() << "UNEXPECTED failure in TearDown().  "
           << "We should never get here, as the test fixture c'tor threw.";
  }
 private:
  void Deinit() {
    FAIL() << "UNEXPECTED failure in the d'tor.  "
           << "We should never get here, as the test fixture c'tor threw.";
  }
};

TEST_F(ExceptionInFixtureCtorTest, ExceptionInFixtureCtor) {
  FAIL() << "UNEXPECTED failure in the test function.  "
         << "We should never get here, as the test fixture c'tor threw.";
}

// Tests exceptions thrown in SetUp().
class ExceptionInSetUpTest : public testing::Test {
 protected:
  virtual ~ExceptionInSetUpTest() {
    Deinit();
  }

  virtual void SetUp() {
    printf("(expecting 3 failures)\n");

    ThrowSEH();
  }

  virtual void TearDown() {
    FAIL() << "Expected failure #2, in TearDown().";
  }
 private:
  void Deinit() {
    FAIL() << "Expected failure #3, in the test fixture d'tor.";
  }
};

TEST_F(ExceptionInSetUpTest, ExceptionInSetUp) {
  FAIL() << "UNEXPECTED failure in the test function.  "
         << "We should never get here, as SetUp() threw.";
}

// Tests that TearDown() and the test fixture d'tor are always called,
// even when the test function throws an exception.
class ExceptionInTestFunctionTest : public testing::Test {
 protected:
  virtual ~ExceptionInTestFunctionTest() {
    Deinit();
  }

  virtual void TearDown() {
    FAIL() << "Expected failure #2, in TearDown().";
  }
 private:
  void Deinit() {
    FAIL() << "Expected failure #3, in the test fixture d'tor.";
  }
};

// Tests that the test fixture d'tor is always called, even when the
// test function throws an SEH exception.
TEST_F(ExceptionInTestFunctionTest, SEH) {
  printf("(expecting 3 failures)\n");

  ThrowSEH();
}

#if GTEST_HAS_EXCEPTIONS

// Tests that the test fixture d'tor is always called, even when the
// test function throws a C++ exception.  We do this only when
// GTEST_HAS_EXCEPTIONS is non-zero, i.e. C++ exceptions are enabled.
TEST_F(ExceptionInTestFunctionTest, CppException) {
  throw 1;
}

// Tests exceptions thrown in TearDown().
class ExceptionInTearDownTest : public testing::Test {
 protected:
  virtual ~ExceptionInTearDownTest() {
    Deinit();
  }

  virtual void TearDown() {
    throw 1;
  }
 private:
  void Deinit() {
    FAIL() << "Expected failure #2, in the test fixture d'tor.";
  }
};

TEST_F(ExceptionInTearDownTest, ExceptionInTearDown) {
  printf("(expecting 2 failures)\n");
}

#endif  // GTEST_HAS_EXCEPTIONS

#endif  // GTEST_OS_WINDOWS

// The MixedUpTestCaseTest test case verifies that Google Test will fail a
// test if it uses a different fixture class than what other tests in
// the same test case use.  It deliberately contains two fixture
// classes with the same name but defined in different namespaces.

// The MixedUpTestCaseWithSameTestNameTest test case verifies that
// when the user defines two tests with the same test case name AND
// same test name (but in different namespaces), the second test will
// fail.

namespace foo {

class MixedUpTestCaseTest : public testing::Test {
};

TEST_F(MixedUpTestCaseTest, FirstTestFromNamespaceFoo) {}
TEST_F(MixedUpTestCaseTest, SecondTestFromNamespaceFoo) {}

class MixedUpTestCaseWithSameTestNameTest : public testing::Test {
};

TEST_F(MixedUpTestCaseWithSameTestNameTest,
       TheSecondTestWithThisNameShouldFail) {}

}  // namespace foo

namespace bar {

class MixedUpTestCaseTest : public testing::Test {
};

// The following two tests are expected to fail.  We rely on the
// golden file to check that Google Test generates the right error message.
TEST_F(MixedUpTestCaseTest, ThisShouldFail) {}
TEST_F(MixedUpTestCaseTest, ThisShouldFailToo) {}

class MixedUpTestCaseWithSameTestNameTest : public testing::Test {
};

// Expected to fail.  We rely on the golden file to check that Google Test
// generates the right error message.
TEST_F(MixedUpTestCaseWithSameTestNameTest,
       TheSecondTestWithThisNameShouldFail) {}

}  // namespace bar

// The following two test cases verify that Google Test catches the user
// error of mixing TEST and TEST_F in the same test case.  The first
// test case checks the scenario where TEST_F appears before TEST, and
// the second one checks where TEST appears before TEST_F.

class TEST_F_before_TEST_in_same_test_case : public testing::Test {
};

TEST_F(TEST_F_before_TEST_in_same_test_case, DefinedUsingTEST_F) {}

// Expected to fail.  We rely on the golden file to check that Google Test
// generates the right error message.
TEST(TEST_F_before_TEST_in_same_test_case, DefinedUsingTESTAndShouldFail) {}

class TEST_before_TEST_F_in_same_test_case : public testing::Test {
};

TEST(TEST_before_TEST_F_in_same_test_case, DefinedUsingTEST) {}

// Expected to fail.  We rely on the golden file to check that Google Test
// generates the right error message.
TEST_F(TEST_before_TEST_F_in_same_test_case, DefinedUsingTEST_FAndShouldFail) {
}

// Used for testing EXPECT_NONFATAL_FAILURE() and EXPECT_FATAL_FAILURE().
int global_integer = 0;

// Tests that EXPECT_NONFATAL_FAILURE() can reference global variables.
TEST(ExpectNonfatalFailureTest, CanReferenceGlobalVariables) {
  global_integer = 0;
  EXPECT_NONFATAL_FAILURE({
    EXPECT_EQ(1, global_integer) << "Expected non-fatal failure.";
  }, "Expected non-fatal failure.");
}

// Tests that EXPECT_NONFATAL_FAILURE() can reference local variables
// (static or not).
TEST(ExpectNonfatalFailureTest, CanReferenceLocalVariables) {
  int m = 0;
  static int n;
  n = 1;
  EXPECT_NONFATAL_FAILURE({
    EXPECT_EQ(m, n) << "Expected non-fatal failure.";
  }, "Expected non-fatal failure.");
}

// Tests that EXPECT_NONFATAL_FAILURE() succeeds when there is exactly
// one non-fatal failure and no fatal failure.
TEST(ExpectNonfatalFailureTest, SucceedsWhenThereIsOneNonfatalFailure) {
  EXPECT_NONFATAL_FAILURE({
    ADD_FAILURE() << "Expected non-fatal failure.";
  }, "Expected non-fatal failure.");
}

// Tests that EXPECT_NONFATAL_FAILURE() fails when there is no
// non-fatal failure.
TEST(ExpectNonfatalFailureTest, FailsWhenThereIsNoNonfatalFailure) {
  printf("(expecting a failure)\n");
  EXPECT_NONFATAL_FAILURE({
  }, "");
}

// Tests that EXPECT_NONFATAL_FAILURE() fails when there are two
// non-fatal failures.
TEST(ExpectNonfatalFailureTest, FailsWhenThereAreTwoNonfatalFailures) {
  printf("(expecting a failure)\n");
  EXPECT_NONFATAL_FAILURE({
    ADD_FAILURE() << "Expected non-fatal failure 1.";
    ADD_FAILURE() << "Expected non-fatal failure 2.";
  }, "");
}

// Tests that EXPECT_NONFATAL_FAILURE() fails when there is one fatal
// failure.
TEST(ExpectNonfatalFailureTest, FailsWhenThereIsOneFatalFailure) {
  printf("(expecting a failure)\n");
  EXPECT_NONFATAL_FAILURE({
    FAIL() << "Expected fatal failure.";
  }, "");
}

// Tests that EXPECT_NONFATAL_FAILURE() fails when the statement being
// tested returns.
TEST(ExpectNonfatalFailureTest, FailsWhenStatementReturns) {
  printf("(expecting a failure)\n");
  EXPECT_NONFATAL_FAILURE({
    return;
  }, "");
}

#if GTEST_HAS_EXCEPTIONS

// Tests that EXPECT_NONFATAL_FAILURE() fails when the statement being
// tested throws.
TEST(ExpectNonfatalFailureTest, FailsWhenStatementThrows) {
  printf("(expecting a failure)\n");
  try {
    EXPECT_NONFATAL_FAILURE({
      throw 0;
    }, "");
  } catch(int) {  // NOLINT
  }
}

#endif  // GTEST_HAS_EXCEPTIONS

// Tests that EXPECT_FATAL_FAILURE() can reference global variables.
TEST(ExpectFatalFailureTest, CanReferenceGlobalVariables) {
  global_integer = 0;
  EXPECT_FATAL_FAILURE({
    ASSERT_EQ(1, global_integer) << "Expected fatal failure.";
  }, "Expected fatal failure.");
}

// Tests that EXPECT_FATAL_FAILURE() can reference local static
// variables.
TEST(ExpectFatalFailureTest, CanReferenceLocalStaticVariables) {
  static int n;
  n = 1;
  EXPECT_FATAL_FAILURE({
    ASSERT_EQ(0, n) << "Expected fatal failure.";
  }, "Expected fatal failure.");
}

// Tests that EXPECT_FATAL_FAILURE() succeeds when there is exactly
// one fatal failure and no non-fatal failure.
TEST(ExpectFatalFailureTest, SucceedsWhenThereIsOneFatalFailure) {
  EXPECT_FATAL_FAILURE({
    FAIL() << "Expected fatal failure.";
  }, "Expected fatal failure.");
}

// Tests that EXPECT_FATAL_FAILURE() fails when there is no fatal
// failure.
TEST(ExpectFatalFailureTest, FailsWhenThereIsNoFatalFailure) {
  printf("(expecting a failure)\n");
  EXPECT_FATAL_FAILURE({
  }, "");
}

// A helper for generating a fatal failure.
void FatalFailure() {
  FAIL() << "Expected fatal failure.";
}

// Tests that EXPECT_FATAL_FAILURE() fails when there are two
// fatal failures.
TEST(ExpectFatalFailureTest, FailsWhenThereAreTwoFatalFailures) {
  printf("(expecting a failure)\n");
  EXPECT_FATAL_FAILURE({
    FatalFailure();
    FatalFailure();
  }, "");
}

// Tests that EXPECT_FATAL_FAILURE() fails when there is one non-fatal
// failure.
TEST(ExpectFatalFailureTest, FailsWhenThereIsOneNonfatalFailure) {
  printf("(expecting a failure)\n");
  EXPECT_FATAL_FAILURE({
    ADD_FAILURE() << "Expected non-fatal failure.";
  }, "");
}

// Tests that EXPECT_FATAL_FAILURE() fails when the statement being
// tested returns.
TEST(ExpectFatalFailureTest, FailsWhenStatementReturns) {
  printf("(expecting a failure)\n");
  EXPECT_FATAL_FAILURE({
    return;
  }, "");
}

#if GTEST_HAS_EXCEPTIONS

// Tests that EXPECT_FATAL_FAILURE() fails when the statement being
// tested throws.
TEST(ExpectFatalFailureTest, FailsWhenStatementThrows) {
  printf("(expecting a failure)\n");
  try {
    EXPECT_FATAL_FAILURE({
      throw 0;
    }, "");
  } catch(int) {  // NOLINT
  }
}

#endif  // GTEST_HAS_EXCEPTIONS

// This #ifdef block tests the output of typed tests.
#if GTEST_HAS_TYPED_TEST

template <typename T>
class TypedTest : public testing::Test {
};

TYPED_TEST_CASE(TypedTest, testing::Types<int>);

TYPED_TEST(TypedTest, Success) {
  EXPECT_EQ(0, TypeParam());
}

TYPED_TEST(TypedTest, Failure) {
  EXPECT_EQ(1, TypeParam()) << "Expected failure";
}

#endif  // GTEST_HAS_TYPED_TEST

// This #ifdef block tests the output of type-parameterized tests.
#if GTEST_HAS_TYPED_TEST_P

template <typename T>
class TypedTestP : public testing::Test {
};

TYPED_TEST_CASE_P(TypedTestP);

TYPED_TEST_P(TypedTestP, Success) {
  EXPECT_EQ(0, TypeParam());
}

TYPED_TEST_P(TypedTestP, Failure) {
  EXPECT_EQ(1, TypeParam()) << "Expected failure";
}

REGISTER_TYPED_TEST_CASE_P(TypedTestP, Success, Failure);

typedef testing::Types<unsigned char, unsigned int> UnsignedTypes;
INSTANTIATE_TYPED_TEST_CASE_P(Unsigned, TypedTestP, UnsignedTypes);

#endif  // GTEST_HAS_TYPED_TEST_P

#if GTEST_HAS_DEATH_TEST

// We rely on the golden file to verify that tests whose test case
// name ends with DeathTest are run first.

TEST(ADeathTest, ShouldRunFirst) {
}

#if GTEST_HAS_TYPED_TEST

// We rely on the golden file to verify that typed tests whose test
// case name ends with DeathTest are run first.

template <typename T>
class ATypedDeathTest : public testing::Test {
};

typedef testing::Types<int, double> NumericTypes;
TYPED_TEST_CASE(ATypedDeathTest, NumericTypes);

TYPED_TEST(ATypedDeathTest, ShouldRunFirst) {
}

#endif  // GTEST_HAS_TYPED_TEST

#if GTEST_HAS_TYPED_TEST_P


// We rely on the golden file to verify that type-parameterized tests
// whose test case name ends with DeathTest are run first.

template <typename T>
class ATypeParamDeathTest : public testing::Test {
};

TYPED_TEST_CASE_P(ATypeParamDeathTest);

TYPED_TEST_P(ATypeParamDeathTest, ShouldRunFirst) {
}

REGISTER_TYPED_TEST_CASE_P(ATypeParamDeathTest, ShouldRunFirst);

INSTANTIATE_TYPED_TEST_CASE_P(My, ATypeParamDeathTest, NumericTypes);

#endif  // GTEST_HAS_TYPED_TEST_P

#endif  // GTEST_HAS_DEATH_TEST

// Tests various failure conditions of
// EXPECT_{,NON}FATAL_FAILURE{,_ON_ALL_THREADS}.
class ExpectFailureTest : public testing::Test {
 protected:
  enum FailureMode {
    FATAL_FAILURE,
    NONFATAL_FAILURE
  };
  static void AddFailure(FailureMode failure) {
    if (failure == FATAL_FAILURE) {
      FAIL() << "Expected fatal failure.";
    } else {
      ADD_FAILURE() << "Expected non-fatal failure.";
    }
  }
};

TEST_F(ExpectFailureTest, ExpectFatalFailure) {
  // Expected fatal failure, but succeeds.
  printf("(expecting 1 failure)\n");
  EXPECT_FATAL_FAILURE(SUCCEED(), "Expected fatal failure.");
  // Expected fatal failure, but got a non-fatal failure.
  printf("(expecting 1 failure)\n");
  EXPECT_FATAL_FAILURE(AddFailure(NONFATAL_FAILURE), "Expected non-fatal "
                       "failure.");
  // Wrong message.
  printf("(expecting 1 failure)\n");
  EXPECT_FATAL_FAILURE(AddFailure(FATAL_FAILURE), "Some other fatal failure "
                       "expected.");
}

TEST_F(ExpectFailureTest, ExpectNonFatalFailure) {
  // Expected non-fatal failure, but succeeds.
  printf("(expecting 1 failure)\n");
  EXPECT_NONFATAL_FAILURE(SUCCEED(), "Expected non-fatal failure.");
  // Expected non-fatal failure, but got a fatal failure.
  printf("(expecting 1 failure)\n");
  EXPECT_NONFATAL_FAILURE(AddFailure(FATAL_FAILURE), "Expected fatal failure.");
  // Wrong message.
  printf("(expecting 1 failure)\n");
  EXPECT_NONFATAL_FAILURE(AddFailure(NONFATAL_FAILURE), "Some other non-fatal "
                          "failure.");
}

#if GTEST_IS_THREADSAFE && GTEST_HAS_PTHREAD

class ExpectFailureWithThreadsTest : public ExpectFailureTest {
 protected:
  static void AddFailureInOtherThread(FailureMode failure) {
    pthread_t tid;
    pthread_create(&tid,
                   NULL,
                   ExpectFailureWithThreadsTest::FailureThread,
                   &failure);
    pthread_join(tid, NULL);
  }
 private:
  static void* FailureThread(void* attr) {
    FailureMode* failure = static_cast<FailureMode*>(attr);
    AddFailure(*failure);
    return NULL;
  }
};

TEST_F(ExpectFailureWithThreadsTest, ExpectFatalFailure) {
  // We only intercept the current thread.
  printf("(expecting 2 failures)\n");
  EXPECT_FATAL_FAILURE(AddFailureInOtherThread(FATAL_FAILURE),
                       "Expected fatal failure.");
}

TEST_F(ExpectFailureWithThreadsTest, ExpectNonFatalFailure) {
  // We only intercept the current thread.
  printf("(expecting 2 failures)\n");
  EXPECT_NONFATAL_FAILURE(AddFailureInOtherThread(NONFATAL_FAILURE),
                          "Expected non-fatal failure.");
}

typedef ExpectFailureWithThreadsTest ScopedFakeTestPartResultReporterTest;

// Tests that the ScopedFakeTestPartResultReporter only catches failures from
// the current thread if it is instantiated with INTERCEPT_ONLY_CURRENT_THREAD.
TEST_F(ScopedFakeTestPartResultReporterTest, InterceptOnlyCurrentThread) {
  printf("(expecting 2 failures)\n");
  TestPartResultArray results;
  {
    ScopedFakeTestPartResultReporter reporter(
        ScopedFakeTestPartResultReporter::INTERCEPT_ONLY_CURRENT_THREAD,
        &results);
    AddFailureInOtherThread(FATAL_FAILURE);
    AddFailureInOtherThread(NONFATAL_FAILURE);
  }
  // The two failures should not have been intercepted.
  EXPECT_EQ(0, results.size()) << "This shouldn't fail.";
}

#endif  // GTEST_IS_THREADSAFE && GTEST_HAS_PTHREAD

TEST_F(ExpectFailureTest, ExpectFatalFailureOnAllThreads) {
  // Expected fatal failure, but succeeds.
  printf("(expecting 1 failure)\n");
  EXPECT_FATAL_FAILURE_ON_ALL_THREADS(SUCCEED(), "Expected fatal failure.");
  // Expected fatal failure, but got a non-fatal failure.
  printf("(expecting 1 failure)\n");
  EXPECT_FATAL_FAILURE_ON_ALL_THREADS(AddFailure(NONFATAL_FAILURE),
                                      "Expected non-fatal failure.");
  // Wrong message.
  printf("(expecting 1 failure)\n");
  EXPECT_FATAL_FAILURE_ON_ALL_THREADS(AddFailure(FATAL_FAILURE),
                                      "Some other fatal failure expected.");
}

TEST_F(ExpectFailureTest, ExpectNonFatalFailureOnAllThreads) {
  // Expected non-fatal failure, but succeeds.
  printf("(expecting 1 failure)\n");
  EXPECT_NONFATAL_FAILURE_ON_ALL_THREADS(SUCCEED(), "Expected non-fatal "
                                         "failure.");
  // Expected non-fatal failure, but got a fatal failure.
  printf("(expecting 1 failure)\n");
  EXPECT_NONFATAL_FAILURE_ON_ALL_THREADS(AddFailure(FATAL_FAILURE),
                                         "Expected fatal failure.");
  // Wrong message.
  printf("(expecting 1 failure)\n");
  EXPECT_NONFATAL_FAILURE_ON_ALL_THREADS(AddFailure(NONFATAL_FAILURE),
                                         "Some other non-fatal failure.");
}


// Two test environments for testing testing::AddGlobalTestEnvironment().

class FooEnvironment : public testing::Environment {
 public:
  virtual void SetUp() {
    printf("%s", "FooEnvironment::SetUp() called.\n");
  }

  virtual void TearDown() {
    printf("%s", "FooEnvironment::TearDown() called.\n");
    FAIL() << "Expected fatal failure.";
  }
};

class BarEnvironment : public testing::Environment {
 public:
  virtual void SetUp() {
    printf("%s", "BarEnvironment::SetUp() called.\n");
  }

  virtual void TearDown() {
    printf("%s", "BarEnvironment::TearDown() called.\n");
    ADD_FAILURE() << "Expected non-fatal failure.";
  }
};

GTEST_DEFINE_bool_(internal_skip_environment_and_ad_hoc_tests, false,
                   "This flag causes the program to skip test environment "
                   "tests and ad hoc tests.");

// The main function.
//
// The idea is to use Google Test to run all the tests we have defined (some
// of them are intended to fail), and then compare the test results
// with the "golden" file.
int main(int argc, char **argv) {
  testing::GTEST_FLAG(print_time) = false;

  // We just run the tests, knowing some of them are intended to fail.
  // We will use a separate Python script to compare the output of
  // this program with the golden file.
  testing::InitGoogleTest(&argc, argv);
  if (argc >= 2 &&
      String(argv[1]) == "--gtest_internal_skip_environment_and_ad_hoc_tests")
    GTEST_FLAG(internal_skip_environment_and_ad_hoc_tests) = true;

#if GTEST_HAS_DEATH_TEST
  if (testing::internal::GTEST_FLAG(internal_run_death_test) != "") {
    // Skip the usual output capturing if we're running as the child
    // process of an threadsafe-style death test.
#if GTEST_OS_WINDOWS
    posix::FReopen("nul:", "w", stdout);
#else
    posix::FReopen("/dev/null", "w", stdout);
#endif  // GTEST_OS_WINDOWS
    return RUN_ALL_TESTS();
  }
#endif  // GTEST_HAS_DEATH_TEST

  if (GTEST_FLAG(internal_skip_environment_and_ad_hoc_tests))
    return RUN_ALL_TESTS();

  // Registers two global test environments.
  // The golden file verifies that they are set up in the order they
  // are registered, and torn down in the reverse order.
  testing::AddGlobalTestEnvironment(new FooEnvironment);
  testing::AddGlobalTestEnvironment(new BarEnvironment);

  return RunAllTests();
}

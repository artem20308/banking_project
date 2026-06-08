# Лабораторная работа 5
https://coveralls.io/repos
## В данной лабораторной работе нам предстояло изучить фреймворки для тестирования на примере `GTest`.

###  Разберем, что в себя включает каждый файл данной лабораторной работы: 

## 1 `.github/workflows/ci.yml`
Полное содержимое файла:
```sh
name: CI

on: [push, pull_request]

jobs:
  build-and-test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
        with:
          submodules: true
      - name: Install lcov
        run: sudo apt-get install -y lcov
      - name: Configure
        run: cmake -H. -B build -DBUILD_TESTS=ON -DCOLLECT_COVERAGE=ON
      - name: Build
        run: cmake --build build
      - name: Test
        run: ./build/check
      - name: Coverage
        run: |
          lcov --capture --directory build --output-file coverage_raw.info --ignore-errors mismatch,unexecuted
          lcov --extract coverage_raw.info '*/banking/Account.cpp' '*/banking/Transaction.cpp' --output-file coverage.info
          lcov --remove coverage.info '/usr/*' --output-file coverage.info --ignore-errors unused
          lcov --list coverage.info
      
      - name: Coveralls
        uses: coverallsapp/github-action@v2
        with:
          github-token: ${{ secrets.GITHUB_TOKEN }}
          file: coverage.info
```
`name: CI` задает имя исполняемого процесса<br>
`on: [push, pull_request]` триггерится, когда совершает пуш или пулл-реквест в ветку


## 2 Корневой `CMakeLists.txt`

Содержимое целиком:
```sh
cmake_minimum_required(VERSION 3.10)
project(banking_project)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_TESTS "Build tests" OFF)
option(COLLECT_COVERAGE "Collect coverage" OFF)

add_subdirectory(banking)

if(BUILD_TESTS)
  enable_testing()
  
  # Скачиваем Google Test
  include(FetchContent)
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.12.1
  )
  FetchContent_MakeAvailable(googletest)

  file(GLOB TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp")
  add_executable(check ${TEST_SOURCES})
  target_link_libraries(check banking gtest_main gmock_main)

  if(COLLECT_COVERAGE)
    target_compile_options(check PRIVATE -O0 -g --coverage)
    target_link_options(check PRIVATE --coverage)
  endif()

  add_test(NAME check COMMAND check)
endif()
```




## 3`banking/CMakeLists.txt`
Содержимое файла:
```sh
cmake_minimum_required(VERSION 3.10)
project(banking)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(banking STATIC Account.cpp Transaction.cpp)
target_include_directories(banking PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(COLLECT_COVERAGE)
  target_compile_options(banking PRIVATE -O0 -g --coverage)
  target_link_options(banking PRIVATE --coverage)
endif()
```

## 4 `banking/Account.h`
```#pragma once

class Account {
 public:
  Account(int id, int balance);
  virtual ~Account();

  virtual int GetBalance() const;
  virtual void ChangeBalance(int diff);
  virtual void Lock();
  virtual void Unlock();

  int id() const { return id_; }

 private:
  int id_;
  int balance_;
  bool is_locked_;
};
```


## 5 `banking/Account.cpp`
```sh
#include "Account.h"
#include <stdexcept>

Account::Account(int id, int balance)
    : id_(id), balance_(balance), is_locked_(false) {}

Account::~Account() {}

int Account::GetBalance() const { return balance_; }

void Account::ChangeBalance(int diff) {
  if (!is_locked_) throw std::runtime_error("at first lock the account");
  balance_ += diff;
}

void Account::Lock() {
  if (is_locked_) throw std::runtime_error("already locked");
  is_locked_ = true;
}

void Account::Unlock() { is_locked_ = false; }
```


## 6 `banking/Transaction.h`
```sh
#pragma once

class Account;

class Transaction {
 public:
  Transaction();
  virtual ~Transaction();

  bool Make(Account& from, Account& to, int sum);
  int fee() const { return fee_; }
  void set_fee(int fee) { fee_ = fee; }

 private:
  void Credit(Account& accout, int sum);
  bool Debit(Account& accout, int sum);
  virtual void SaveToDataBase(Account& from, Account& to, int sum);

  int fee_;
};
```

## 7 `banking/Transaction.cpp`
```sh
#include "Transaction.h"
#include <cassert>
#include <iostream>
#include <stdexcept>
#include "Account.h"

namespace {
struct Guard {
  Guard(Account& account) : account_(&account) { account_->Lock(); }
  ~Guard() { account_->Unlock(); }
 private:
  Account* account_;
};
}

Transaction::Transaction() : fee_(1) {}
Transaction::~Transaction() {}

bool Transaction::Make(Account& from, Account& to, int sum) {
  if (from.id() == to.id()) throw std::logic_error("invalid action");
  if (sum < 0) throw std::invalid_argument("sum can't be negative");
  if (sum < 100) throw std::logic_error("too small");
  if (fee_ * 2 > sum) return false;

  Guard guard_from(from);
  Guard guard_to(to);

  Credit(to, sum);
  bool success = Debit(from, sum + fee_);
  if (!success) to.ChangeBalance(-sum);

  SaveToDataBase(from, to, sum);
  return success;
}

void Transaction::Credit(Account& accout, int sum) {
  assert(sum > 0);
  accout.ChangeBalance(sum);
}

bool Transaction::Debit(Account& accout, int sum) {
  assert(sum > 0);
  if (accout.GetBalance() > sum) {
    accout.ChangeBalance(-sum);
    return true;
  }
  return false;
}

void Transaction::SaveToDataBase(Account& from, Account& to, int sum) {
  std::cout << from.id() << " send to " << to.id() << " $" << sum << std::endl;
}
```


## 8 `tests/mock_classes.h`
```sh
#pragma once
#include <gmock/gmock.h>
#include "Account.h"
#include "Transaction.h"

class MockAccount : public Account {
public:
    MockAccount(int id, int balance) : Account(id, balance) {}
    MOCK_METHOD(int, GetBalance, (), (const, override));
    MOCK_METHOD(void, ChangeBalance, (int diff), (override));
    MOCK_METHOD(void, Lock, (), (override));
    MOCK_METHOD(void, Unlock, (), (override));
};

class MockTransaction : public Transaction {
public:
    MOCK_METHOD(void, SaveToDataBase, (Account& from, Account& to, int sum), (override));
};
```

## 9 `tests/test_account.cpp`
Разберем по блокам.
```sh
#include <gtest/gtest.h>
#include "Account.h"

class AccountBalanceTest : public testing::TestWithParam<int> {
public:
    Account* acc;
    void SetUp() override { acc = new Account(1, GetParam()); }
    void TearDown() override { delete acc; }
};

TEST_P(AccountBalanceTest, InitialBalance) {
    EXPECT_EQ(acc->GetBalance(), GetParam());
}

INSTANTIATE_TEST_SUITE_P(
    DifferentBalances,
    AccountBalanceTest,
    testing::Values(100, 0, 1000, 5000)
);

class AccountBehaviourTest : public testing::Test {
public:
    Account* acc;
    void SetUp() override { acc = new Account(123, 1000); }
    void TearDown() override { delete acc; }
};

TEST_F(AccountBehaviourTest, LockUnlock) {
    EXPECT_NO_THROW(acc->Lock());
    EXPECT_THROW(acc->Lock(), std::runtime_error);
    acc->Unlock();
    EXPECT_NO_THROW(acc->Lock());
}

TEST_F(AccountBehaviourTest, ChangeBalanceWhenUnlocked) {
    EXPECT_THROW(acc->ChangeBalance(50), std::runtime_error);
}

TEST_F(AccountBehaviourTest, ChangeBalanceWhenLocked) {
    acc->Lock();
    EXPECT_NO_THROW(acc->ChangeBalance(200));
    EXPECT_EQ(acc->GetBalance(), 1200);
}

TEST_F(AccountBehaviourTest, IdIsCorrect) {
    EXPECT_EQ(acc->id(), 123);
}
```


## 10 `tests/test_transaction.cpp`
```sh
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "Transaction.h"
#include "mock_classes.h"

using ::testing::StrictMock;
using ::testing::Return;
using ::testing::_;
using ::testing::InSequence;

class TransactionTest : public testing::Test {
public:
    StrictMock<MockTransaction>* trans;
    StrictMock<MockAccount>* from;
    StrictMock<MockAccount>* to;
    
    void SetUp() override {
        trans = new StrictMock<MockTransaction>();
        from = new StrictMock<MockAccount>(1, 1000);
        to = new StrictMock<MockAccount>(2, 1000);
    }
    
    void TearDown() override {
        delete trans;
        delete from;
        delete to;
    }
};

TEST_F(TransactionTest, DefaultFee) {
    Transaction t;
    EXPECT_EQ(t.fee(), 1);
}

TEST_F(TransactionTest, SetFee) {
    Transaction t;
    t.set_fee(10);
    EXPECT_EQ(t.fee(), 10);
}

TEST_F(TransactionTest, SameAccount) {
    Transaction t;
    EXPECT_THROW(t.Make(*from, *from, 200), std::logic_error);
}

TEST_F(TransactionTest, NegativeSum) {
    Transaction t;
    EXPECT_THROW(t.Make(*from, *to, -100), std::invalid_argument);
}

TEST_F(TransactionTest, TooSmall) {
    Transaction t;
    EXPECT_THROW(t.Make(*from, *to, 70), std::logic_error);
}

TEST_F(TransactionTest, FeeExceeds) {
    Transaction t;
    t.set_fee(60);
    EXPECT_FALSE(t.Make(*from, *to, 100));
}

TEST_F(TransactionTest, Successful) {
    int sum = 200;
    trans->set_fee(1);
    
    InSequence seq;
    EXPECT_CALL(*from, Lock());
    EXPECT_CALL(*to, Lock());
    EXPECT_CALL(*to, ChangeBalance(sum));
    EXPECT_CALL(*from, GetBalance()).WillOnce(Return(1000));
    EXPECT_CALL(*from, ChangeBalance(-(sum + 1)));
    EXPECT_CALL(*trans, SaveToDataBase(_, _, sum));
    EXPECT_CALL(*to, Unlock());
    EXPECT_CALL(*from, Unlock());
    
    EXPECT_TRUE(trans->Make(*from, *to, sum));
}

TEST_F(TransactionTest, NotEnoughFunds) {
    int sum = 200;
    trans->set_fee(1);
    
    InSequence seq;
    EXPECT_CALL(*from, Lock());
    EXPECT_CALL(*to, Lock());
    EXPECT_CALL(*to, ChangeBalance(sum));
    EXPECT_CALL(*from, GetBalance()).WillOnce(Return(100));
    EXPECT_CALL(*to, ChangeBalance(-sum));
    EXPECT_CALL(*trans, SaveToDataBase(_, _, sum));
    EXPECT_CALL(*to, Unlock());
    EXPECT_CALL(*from, Unlock());
    
    EXPECT_FALSE(trans->Make(*from, *to, sum));
}

TEST(RealTransaction, SaveToDataBaseExecution) {
    Account from(1, 1000);
    Account to(2, 1000);
    Transaction t;
    t.set_fee(1);
    
    testing::internal::CaptureStdout();
    EXPECT_TRUE(t.Make(from, to, 200));
    std::string output = testing::internal::GetCapturedStdout();
    
    EXPECT_EQ(from.GetBalance(), 799);
    EXPECT_EQ(to.GetBalance(), 1200);
    EXPECT_EQ(output, "1 send to 2 $200\n");
}
```


## Результат работы команд по компиляции и тестрироавнию работы программы:

`cmake -H. -B build -DBUILD_TESTS=ON -DCOLLECT_COVERAGE=ON`:
```sh
-- The C compiler identification is GNU 11.4.0
-- The CXX compiler identification is GNU 11.4.0
-- Detecting C compiler ABI info
-- Detecting C compiler ABI info - done
-- Check for working C compiler: /usr/bin/cc - skipped
-- Detecting C compile features
-- Detecting C compile features - done
-- Detecting CXX compiler ABI info
-- Detecting CXX compiler ABI info - done
-- Check for working CXX compiler: /usr/bin/c++ - skipped
-- Detecting CXX compile features
-- Detecting CXX compile features - done
-- Found Python: /usr/bin/python3.10 (found version "3.10.12") found components: Interpreter 
-- Looking for pthread.h
-- Looking for pthread.h - found
-- Performing Test CMAKE_HAVE_LIBC_PTHREAD
-- Performing Test CMAKE_HAVE_LIBC_PTHREAD - Success
-- Found Threads: TRUE  
-- Configuring done
-- Generating done
-- Build files have been written to: /home/ubuntu/Desktop/artem20308/workspace/projects/banking_project/build


```

`cmake --build build`:
```sh
[  7%] Building CXX object _deps/googletest-build/googletest/CMakeFiles/gtest.dir/src/gtest-all.cc.o
[ 14%] Linking CXX static library ../../../lib/libgtest.a
[ 14%] Built target gtest
[ 21%] Building CXX object _deps/googletest-build/googlemock/CMakeFiles/gmock.dir/src/gmock-all.cc.o
[ 28%] Linking CXX static library ../../../lib/libgmock.a
[ 28%] Built target gmock
[ 35%] Building CXX object _deps/googletest-build/googlemock/CMakeFiles/gmock_main.dir/src/gmock_main.cc.o
[ 42%] Linking CXX static library ../../../lib/libgmock_main.a
[ 42%] Built target gmock_main
[ 50%] Building CXX object banking/CMakeFiles/banking.dir/Account.cpp.o
[ 57%] Building CXX object banking/CMakeFiles/banking.dir/Transaction.cpp.o
[ 64%] Linking CXX static library libbanking.a
[ 64%] Built target banking
[ 71%] Building CXX object _deps/googletest-build/googletest/CMakeFiles/gtest_main.dir/src/gtest_main.cc.o
[ 78%] Linking CXX static library ../../../lib/libgtest_main.a
[ 78%] Built target gtest_main
[ 85%] Building CXX object CMakeFiles/check.dir/tests/test_account.cpp.o
[ 92%] Building CXX object CMakeFiles/check.dir/tests/test_transaction.cpp.o
[100%] Linking CXX executable check
[100%] Built target check
```

`./build/check`:
```sh
Running main() from /home/ubuntu/Desktop/artem20308/workspace/projects/banking_project/build/_deps/googletest-src/googletest/src/gtest_main.cc
[==========] Running 17 tests from 4 test suites.
[----------] Global test environment set-up.
[----------] 4 tests from AccountBehaviourTest
[ RUN      ] AccountBehaviourTest.LockUnlock
[       OK ] AccountBehaviourTest.LockUnlock (0 ms)
[ RUN      ] AccountBehaviourTest.ChangeBalanceWhenUnlocked
[       OK ] AccountBehaviourTest.ChangeBalanceWhenUnlocked (0 ms)
[ RUN      ] AccountBehaviourTest.ChangeBalanceWhenLocked
[       OK ] AccountBehaviourTest.ChangeBalanceWhenLocked (0 ms)
[ RUN      ] AccountBehaviourTest.IdIsCorrect
[       OK ] AccountBehaviourTest.IdIsCorrect (0 ms)
[----------] 4 tests from AccountBehaviourTest (0 ms total)

[----------] 8 tests from TransactionTest
[ RUN      ] TransactionTest.DefaultFee
[       OK ] TransactionTest.DefaultFee (0 ms)
[ RUN      ] TransactionTest.SetFee
[       OK ] TransactionTest.SetFee (0 ms)
[ RUN      ] TransactionTest.SameAccount
[       OK ] TransactionTest.SameAccount (0 ms)
[ RUN      ] TransactionTest.NegativeSum
[       OK ] TransactionTest.NegativeSum (0 ms)
[ RUN      ] TransactionTest.TooSmall
[       OK ] TransactionTest.TooSmall (0 ms)
[ RUN      ] TransactionTest.FeeExceeds
[       OK ] TransactionTest.FeeExceeds (0 ms)
[ RUN      ] TransactionTest.Successful
[       OK ] TransactionTest.Successful (0 ms)
[ RUN      ] TransactionTest.NotEnoughFunds
[       OK ] TransactionTest.NotEnoughFunds (0 ms)
[----------] 8 tests from TransactionTest (0 ms total)

[----------] 1 test from RealTransaction
[ RUN      ] RealTransaction.SaveToDataBaseExecution
[       OK ] RealTransaction.SaveToDataBaseExecution (0 ms)
[----------] 1 test from RealTransaction (0 ms total)

[----------] 4 tests from DifferentBalances/AccountBalanceTest
[ RUN      ] DifferentBalances/AccountBalanceTest.InitialBalance/0
[       OK ] DifferentBalances/AccountBalanceTest.InitialBalance/0 (0 ms)
[ RUN      ] DifferentBalances/AccountBalanceTest.InitialBalance/1
[       OK ] DifferentBalances/AccountBalanceTest.InitialBalance/1 (0 ms)
[ RUN      ] DifferentBalances/AccountBalanceTest.InitialBalance/2
[       OK ] DifferentBalances/AccountBalanceTest.InitialBalance/2 (0 ms)
[ RUN      ] DifferentBalances/AccountBalanceTest.InitialBalance/3
[       OK ] DifferentBalances/AccountBalanceTest.InitialBalance/3 (0 ms)
[----------] 4 tests from DifferentBalances/AccountBalanceTest (0 ms total)

[----------] Global test environment tear-down
[==========] 17 tests from 4 test suites ran. (1 ms total)
[  PASSED  ] 17 tests.



```
`lcov --capture --directory build --output-file coverage_raw.info --ignore-errors mismatch,inconsistent`:

<details>
  <summary>Вывод команды </summary>

  ```sh
  Capturing coverage data from build
Subroutine read_intermediate_text redefined at /usr/bin/geninfo line 2623.
Subroutine read_intermediate_json redefined at /usr/bin/geninfo line 2655.
Subroutine intermediate_text_to_info redefined at /usr/bin/geninfo line 2703.
Subroutine intermediate_json_to_info redefined at /usr/bin/geninfo line 2792.
Subroutine get_output_fd redefined at /usr/bin/geninfo line 2872.
Subroutine print_gcov_warnings redefined at /usr/bin/geninfo line 2900.
Subroutine process_intermediate redefined at /usr/bin/geninfo line 2930.
Found gcov version: 11.4.0
Using intermediate gcov format
Scanning build for .gcda files ...
Found 4 data files in build
Processing CMakeFiles/check.dir/tests/test_account.cpp.gcda
Processing CMakeFiles/check.dir/tests/test_transaction.cpp.gcda
Processing banking/CMakeFiles/banking.dir/Account.cpp.gcda
Processing banking/CMakeFiles/banking.dir/Transaction.cpp.gcda
Finished .info-file creation


  ```
</details>

```sh
lcov --extract coverage_raw.info '*/banking/Account.cpp' '*/banking/Transaction.cpp' --output-file coverage.info
lcov --list coverage.info

```
<details>
  <summary>Вывод команды </summary>

  ```sh
  Reading tracefile coverage_raw.info
Extracting /home/ubuntu/Desktop/artem20308/workspace/projects/banking_project/banking/Account.cpp
Extracting /home/ubuntu/Desktop/artem20308/workspace/projects/banking_project/banking/Transaction.cpp
Extracted 2 files
Writing data to coverage.info
Summary coverage rate:
  lines......: 100.0% (43 of 43 lines)
  functions..: 93.8% (15 of 16 functions)
  branches...: no data found
Reading tracefile coverage.info
                      |Lines       |Functions  |Branches    
Filename              |Rate     Num|Rate    Num|Rate     Num
============================================================
[/home/ubuntu/Desktop/artem20308/workspace/projects/banking_project/banking/]
Account.cpp           | 100%     13| 100%     7|    -      0
Transaction.cpp       | 100%     30|88.9%     9|    -      0
============================================================
                Total:| 100%     43|93.8%    16|    -      0


  ```
</details>






## Вывод
В результате проделанной лабораторной работы мы научились реализовывать тесты используя мок-объекты, кроме того проверяли покрываемость кода на сайте `Coveralls.io`.

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

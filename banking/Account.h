#pragma once

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

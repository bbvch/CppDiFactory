#ifndef FAKEMUTEX_H
#define FAKEMUTEX_H
/// mutex
class FakeMutex
{
public:
  FakeMutex() noexcept = default;
  ~FakeMutex() = default;

  FakeMutex(const FakeMutex&) = delete;
  FakeMutex& operator=(const FakeMutex&) = delete;

  void lock()
  {

  }

  bool try_lock() noexcept
  {
      return true;
  }

  void  unlock()
  {

  }
};

#endif // FAKEMUTEX_H


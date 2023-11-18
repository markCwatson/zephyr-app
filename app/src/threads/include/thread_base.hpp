#ifndef _THREAD_BASE_HPP_
#define _THREAD_BASE_HPP_

/**
 * @brief Base class for threads.
 * 
 * @details Curiously Recurring Template Pattern (CRTP) is used to implement
 *          the thread base class. This allows the derived class to be passed
 *          as a template parameter to the base class. This allows the base
 *          class to call the derived class's methods.
 * 
 * @tparam Derived The derived class.
*/
template<typename Derived>
class ThreadBase {
public:
  void operator()() {
    if (!static_cast<Derived*>(this)->init()) {
      return;
    }

    while (true) {
      static_cast<Derived*>(this)->run();
    }
  }

protected:
  virtual bool init() = 0;
  virtual void run() = 0;
};

#endif // _THREAD_BASE_HPP_
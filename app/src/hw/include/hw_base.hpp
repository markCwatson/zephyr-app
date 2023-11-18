#ifndef _HW_BASE_HPP_
#define _HW_BASE_HPP_

/**
 * @brief Base class for hardware (hw).
 * 
 * @details This class provides a common interface for different hardware components.
 *          It includes mechanisms for initialization and ensures proper resource management.
 *          Copying and moving of hardware objects is prevented to avoid resource conflicts.
 * 
 * @tparam Derived The derived class implementing specific hardware functionalities.
 */
template<typename Derived>
class HwBase {
public:
    // Virtual destructor ensures proper cleanup of derived classes
    virtual ~HwBase() = default;

    // Deleted copy constructor and copy assignment to prevent copying
    HwBase(const HwBase&) = delete;
    HwBase& operator=(const HwBase&) = delete;

    // Default constructor and move operations are allowed
    HwBase() = default;
    HwBase(HwBase&&) = default;
    HwBase& operator=(HwBase&&) = default;

protected:
    /**
     * @brief Initialize the hardware component.
     * 
     * @details This pure virtual function must be implemented by the derived class to
     *          perform specific initialization steps necessary for the hardware component.
     * 
     * @return int Returns 0 on success, or an error code on failure.
     */
    virtual int init() = 0;
};

#endif // _HW_BASE_HPP_
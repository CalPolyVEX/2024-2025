#pragma once
#ifndef NVM_CTRL
#define NVM_CTRL

#include <Arduino.h>

// Concatenate after macro expansion
#define PPCAT_NX(A, B) A##B
#define PPCAT(A, B) PPCAT_NX(A, B)

#define Flash(name, size)                                                   \
    __attribute__((__aligned__(256))) static const uint8_t PPCAT(             \
      _data, name)[(size + 255) / 256 * 256] = {};                            \
    FlashClass name(PPCAT(_data, name), size);

#define FlashStorage(name, T)                                               \
    __attribute__((__aligned__(256))) static const uint8_t PPCAT(             \
      _data, name)[(sizeof(T) + 255) / 256 * 256] = {};                       \
    FlashStorageClass<T> name(PPCAT(_data, name));

class FlashClass
{
public:
  FlashClass(const void* flash_addr = NULL, uint32_t size = 0);

  void write(const void* data) { write(flash_address, data, flash_size); }
  void erase() { erase(flash_address, flash_size); }
  void read(void* data) { read(flash_address, data, flash_size); }

  void write(const volatile void* flash_ptr, const void* data, uint32_t size);
  void erase(const volatile void* flash_ptr, uint32_t size);
  void read(const volatile void* flash_ptr, void* data, uint32_t size);

private:
  void erase(const volatile void* flash_ptr);

  const uint32_t PAGE_SIZE, PAGES, MAX_FLASH, ROW_SIZE;
  const volatile void* flash_address;
  const uint32_t flash_size;
};

class FlashStorageBase
{
  public:

    virtual void read(void* data) { flash.read(data); }

    virtual void write(void* data)
    {
      flash.erase();
      flash.write(&data);
    }


  protected:

    FlashStorageBase(const void* flash_addr, const int size)
      : flash(flash_addr, size){};

    FlashClass flash;
};

template <class T>
class FlashStorageClass : public FlashStorageBase
{
public:
  FlashStorageClass(const void* flash_addr) 
    : FlashStorageBase(flash_addr, sizeof(T)){};
    //: flash(flash_addr, sizeof(T)){};

  // Write data into flash memory.
  // Compiler is able to optimize parameter copy.
  inline void write(T data)
  {
    flash.erase();
    flash.write(&data);
  }

  // Read data from flash into variable.
  inline void read(T* data) { flash.read(data); }

  // Overloaded version of read.
  // Compiler is able to optimize copy-on-return.
  inline T read()
  {
    T data;
    read(&data);
    return data;
  }
};

#endif
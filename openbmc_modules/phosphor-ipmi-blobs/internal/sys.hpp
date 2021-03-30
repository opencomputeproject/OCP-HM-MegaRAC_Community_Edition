#pragma once

namespace blobs
{

namespace internal
{
/**
 * Interface to the dynamic library loader.
 */
class DlSysInterface
{
  public:
    virtual ~DlSysInterface() = default;

    /**
     * obtain error diagnostic for functions in the dlopen API
     *
     * @return the error details
     */
    virtual const char* dlerror() const = 0;

    /**
     * open a shared object
     *
     * @param[in] filename - the path to the shared object or the filename to
     * for searching
     * @param[in] flags - the flags
     * @return a handle to the shared object (null on failure)
     */
    virtual void* dlopen(const char* filename, int flags) const = 0;

    /**
     * obtain address of a symbol in a shared object or executable
     *
     * @param[in] handle - pointer to shared object
     * @param[in] symbol - name of the symbol to find
     * @return the address of the symbol if found (null otherwise)
     */
    virtual void* dlsym(void* handle, const char* symbol) const = 0;
};

class DlSysImpl : public DlSysInterface
{
  public:
    const char* dlerror() const override;
    void* dlopen(const char* filename, int flags) const override;
    void* dlsym(void* handle, const char* symbol) const override;
};

extern DlSysImpl dlsys_impl;

} // namespace internal
} // namespace blobs

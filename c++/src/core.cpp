#include <dfk.hpp>

namespace dfk {

Exception::Exception(Context* context, int code)
  : context_(context)
  , code_(code)
{
}

const char* Exception::what() const throw()
{
  return dfk_strerr(context_->nativeHandle(), code_);
}

int Exception::code() const
{
  return code_;
}

class Context* Exception::context() const
{
  return context_;
}


static void* mallocProxy(dfk_t* dfk, size_t nbytes)
{
  Context* context = static_cast<Context*>(dfk->user.data);
  return context->malloc(nbytes);
}

static void freeProxy(dfk_t* dfk, void* p)
{
  Context* context = static_cast<Context*>(dfk->user.data);
  context->free(p);
}

static void* reallocProxy(dfk_t* dfk, void* p, std::size_t nbytes)
{
  Context* context = static_cast<Context*>(dfk->user.data);
  return context->realloc(p, nbytes);
}

static void logProxy(dfk_t* dfk, int level, const char* msg)
{
  Context* context = static_cast<Context*>(dfk->user.data);
  context->log(static_cast<LogLevel>(level), msg);
}

Context::Context()
{
  dfk_init(&dfk_);
  dfk_.user.data = this;
  dfk_.malloc = mallocProxy;
  dfk_.free = freeProxy;
  dfk_.realloc = reallocProxy;
  dfk_.log = logProxy;
}

Context::~Context()
{
  dfk_free(&dfk_);
}

dfk_t* Context::nativeHandle()
{
  return &dfk_;
}

void Context::setDefaultStackSize(std::size_t stackSize)
{
  dfk_.default_stack_size = stackSize;
}

std::size_t Context::defaultStackSize() const
{
  return dfk_.default_stack_size;
}

void Context::work()
{
  int err = dfk_work(&dfk_);
  if (err != dfk_err_ok) {
    throw Exception(this, err);
  }
}

void Context::stop()
{
  int err = dfk_stop(&dfk_);
  if (err != dfk_err_ok) {
    throw Exception(this, err);
  }
}

void Context::sleep(uint64_t msec)
{
  int err = dfk_sleep(&dfk_, msec);
  if (err != dfk_err_ok) {
    throw Exception(this, err);
  }
}


} // namespace dfk

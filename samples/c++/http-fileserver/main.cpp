/**
 * @copyright
 * Copyright (c) 2016-2017 Stanislav Ivochkin
 * Licensed under the MIT License (see LICENSE)
 */

#include <dfk.hpp>

static void dfkMain(dfk::Coroutine coro, void* arg)
{
  dfk::Context* ctx = static_cast<dfk::Context*>(arg);
  dfk::http::Server srv(ctx);
  dfk::Buffer basepath(".", 1);
  dfk::fileserver::Server fs(ctx, basepath);
  srv.serve("127.0.0.1", 10000, &fs);
}

int main()
{
  dfk::Context ctx;
  ctx.run(dfkMain, &ctx, 0);
  ctx.work();
}

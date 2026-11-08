// Runs the ACTUAL browser api() implementation extracted from PortalPage.h.
// Exercises reboot/stale token, status refresh, failure handling and retry bounds.
import assert from "node:assert/strict";
import { readFileSync } from "node:fs";
import vm from "node:vm";

const source = readFileSync("src/PortalPage.h", "utf8");
const start = source.indexOf("async function api(path,fields,retry=true){");
const end = source.indexOf("\nfunction card(d){", start);
assert(start > 0 && end > start, "Cannot find the portal's actual JS API helper");
const actualApi = source.slice(start, end);

const OLD = "0123456789ABCDEF01234567";
const CURRENT = "89ABCDEF0123456789ABCDEF";

function response(status, data, token) {
  return {
    status,
    ok: status >= 200 && status < 300,
    headers: { get: name => name === "X-Bridge-Token" ? token ?? null : null },
    json: async () => data
  };
}
function browser(fetchImpl) {
  const context = { fetch: fetchImpl, URLSearchParams, Error };
  vm.runInNewContext(
    '"use strict";let CSRF="' + OLD + '";\n' + actualApi +
    "\nglobalThis.portalApi=api;globalThis.currentToken=()=>CSRF;",
    context);
  return context;
}

// An authenticated status request picks up the latest token after reboot.
// No new page reload required.
{
  const page = browser(async () => response(200, { ok: true }, CURRENT));
  await page.portalApi("/api/status");
  assert.equal(page.currentToken(), CURRENT);
}

// A stale POST fails with 403, status GET resynchronizes the secret, and
// retry happens EXACTLY once. No mutation occurs before CSRF validation.
{
  const seen = [];
  const page = browser(async (path, opts) => {
    seen.push({ path, opts });
    if (path === "/api/status") {
      assert.equal(opts.method, undefined);
      return response(200, { ok: true }, CURRENT);
    }
    assert.equal(path, "/api/presets/add");
    assert.equal(opts.method, "POST");
    assert.equal(opts.body.get("aid"), "2");
    assert.equal(opts.body.get("preset"), "5");
    const submitted = opts.headers["X-Bridge-Token"];
    assert.equal(opts.body.get("token"), submitted);
    if (submitted === OLD)
      return response(403, { ok: false, error: "Missing or invalid CSRF token" });
    assert.equal(submitted, CURRENT);
    return response(200, { ok: true, message: "Preset switch saved." });
  });
  const result = await page.portalApi("/api/presets/add", { aid: "2", preset: "5" });
  assert.equal(result.ok, true);
  assert.deepEqual(seen.map(x => x.path), [
    "/api/presets/add", "/api/status", "/api/presets/add"
  ]);
  assert.equal(page.currentToken(), CURRENT);
}

// If the secret did NOT change, the client must not loop/retry.
{
  let posts = 0;
  const page = browser(async (path) => {
    if (path === "/api/status")
      return response(200, { ok: true }, OLD);
    ++posts;
    return response(403, { ok: false, error: "Missing or invalid CSRF token" });
  });
  await assert.rejects(
    () => page.portalApi("/api/presets/add", { aid: "2", preset: "5" }),
    /Missing or invalid CSRF token/);
  assert.equal(posts, 1);
}

// A bad new token is not accepted and cannot bypass CSRF enforcement.
{
  let posts = 0;
  const page = browser(async path => {
    if (path === "/api/status")
      return response(200, { ok: true }, "invalid");
    ++posts;
    return response(403, { ok: false, error: "Missing or invalid CSRF token" });
  });
  await assert.rejects(
    () => page.portalApi("/api/enroll", { ip: "192.168.1.77" }),
    /Missing or invalid CSRF token/);
  assert.equal(posts, 1);
}

// Real application errors must not initiate a CSRF recovery attempt.
{
  let requests = 0;
  const page = browser(async () => {
    ++requests;
    return response(409, { ok: false, error: "Already exists" });
  });
  await assert.rejects(() => page.portalApi("/api/presets/add", {
    aid: "2", preset: "5"
  }), /Already exists/);
  assert.equal(requests, 1);
}

console.log("browser CSRF refresh, reboot recovery and retry-limit tests passed");

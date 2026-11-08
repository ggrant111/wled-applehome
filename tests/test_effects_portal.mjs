// Read-only portal contract verification: every new mutation goes through
// the same authenticated + CSRF-protected authorize(true) gate.
import assert from "node:assert/strict";
import {readFileSync} from "node:fs";
const server=readFileSync("src/PortalServer.cpp","utf8");
const html=readFileSync("web/portal.html","utf8");
for(const endpoint of ["rename","remove-preset","move"]){
  assert(server.includes('"/api/effects/'+endpoint+'"'));
  assert(html.includes('"/api/effects/'+endpoint+'"'));
}
for(const method of ["handleRenameEffect","handleRemoveEffectInput","handleMoveEffectInput"]){
  const start=server.indexOf("void PortalServer::"+method+"() {");
  const end=server.indexOf("\n}\n",start);
  assert(start>=0&&end>start,method);
  const code=server.slice(start,end);
  assert(code.includes("authorize(true)"),method+" lacks CSRF/auth check");
}
assert(html.includes('data-action="moveEffect"'));
assert(html.includes('data-action="removeEffect"'));
assert(html.includes('data-action="renameEffect"'));
assert(html.includes('safe(x.name)'),"Effect labels must be escaped");
console.log("Multi-device effects portal action contract and auth/CSRF passed");

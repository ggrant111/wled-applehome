// Run the actual embedded browser script against a minimal DOM. This catches
// syntax, navigation, HTML escaping, refresh-state and POST/CSRF regressions
// without requiring a browser dependency in firmware CI.
import assert from "node:assert/strict";
import {readFileSync} from "node:fs";
import vm from "node:vm";

const html=readFileSync("web/portal.html","utf8");
const source=html.match(/<script>([\s\S]*?)<\/script>/)?.[1];
assert(source, "Inline local-only dashboard script not found");
new vm.Script(source,{filename:"portal-inline.js"});
const views=["overview","lights","effects","switches","settings"];
for(const name of views){
  assert(html.includes('data-view="'+name+'"'),"Missing view "+name);
  assert(html.includes('data-view-target="'+name+'"'),"Missing nav button "+name);
}
assert.equal((html.match(/<script>/g)||[]).length,1,"Dashboard must stay one self-contained local page");
assert(!html.includes('src="https://'),"No remote JS");
assert(html.includes('id="renameDialog"'),"Accessible rename dialog required");
assert(html.includes('id="restartBanner"'),"Pending HomeKit restart visibility required");
assert(html.includes('id="effectsAdd"'),"Add favorites should use a collapsible section");
assert(html.includes('class="group-tabs"'),"Effects devices should have dedicated tabs");
assert(html.includes("effectsSelectionInitialized"),"Default configured group selection missing");
assert(html.includes('aria-live="polite"'),"Status feedback should be announced");
for(const endpoint of ["/api/scan","/api/enroll","/api/control","/api/effects/add",
  "/api/effects/run","/api/effects/rename","/api/effects/remove-preset",
  "/api/effects/move","/api/effects/remove","/api/presets/add","/api/presets/inspect",
  "/api/presets/remove","/api/restart"])
  assert(html.includes(endpoint),"Dashboard lost endpoint "+endpoint);
assert(html.includes('safe(x.name)'),"Effect names must be HTML-escaped");
assert(html.includes('p.kind==="playlist"'),"Playlist options must be labeled");
assert(html.includes('x.kind==="playlist"'),"Playlist state must be distinct from presets");
assert(html.includes("clearTimeout(noticeTimer)"),"Old notice timers must not erase new notices");

class Element {
  constructor(id=""){
    this.id=id;this.innerHTML="";this.textContent="";this.hidden=id==="restartBanner";
    this.value="";this.dataset={};this.className="";this.disabled=false;this.listeners={};
    this.attrs=new Map();this.open=false;
    this.classList={toggle:()=>{}};
  }
  addEventListener(name,fn){this.listeners[name]=fn;}
  setAttribute(k,v){this.attrs.set(k,v);}
  removeAttribute(k){this.attrs.delete(k);}
  hasAttribute(k){return this.attrs.has(k);}
  contains(){return false;}
  scrollIntoView(){}
  close(){this.open=false;}
  showModal(){this.open=true;}
  focus(){}
}
const elements=new Map();
const get=id=>{
  if(!elements.has(id))elements.set(id,new Element(id));
  return elements.get(id);
};
const viewEls=views.map(name=>{
  const e=get("view-"+name);
  e.dataset.view=name;
  e.hidden=name!=="overview";
  return e;
});
const navEls=views.map(name=>{
  const e=get("nav-"+name);e.dataset.viewTarget=name;return e;
});
const quickEls=[get("quick-effects"),get("quick-add")];
quickEls[0].dataset.openView="effects";
quickEls[1].dataset.openView="lights";
quickEls[1].setAttribute("data-open-discovery","");
const document={
  getElementById:get,
  querySelectorAll:query=>query==="[data-view]"?viewEls:
    query==="[data-view-target]"?navEls:query==="[data-open-view]"?quickEls:[],
  body:new Element("body"),
  activeElement:null
};
const status={
  ok:true,version:"0.9.2-dev",wifi:true,ip:"192.168.1.7",rssi:-54,uptimeSec:4800,
  freeHeap:174000,minFreeHeap:141000,freeSketchSpace:401000,
  devices:[
    {aid:2,name:"Shelf & TV",ip:"192.168.1.80",online:true,on:true,brightness:85,
     presetId:11,playlistId:-1,lastSeenSec:2,pendingCommands:0,networkFailures:0},
    {aid:7,name:"Kitchen <script>",ip:"192.168.1.90",online:true,on:true,brightness:70,
     presetId:15,playlistId:34,lastSeenSec:1,pendingCommands:0,networkFailures:0}
  ],
  presets:[{aid:13,deviceAid:2,name:"Rainbow",presetId:10}],
  effectControllers:[
    {aid:5,deviceAid:2,mode:"grouped",pending:false,inputs:[
      {presetId:11,name:"Mammoth",kind:"preset"}]},
    {aid:9,deviceAid:7,mode:"grouped",pending:false,inputs:[
      {presetId:15,name:"Step preset",kind:"preset"},
      {presetId:34,name:"Kitchen playlist & more",kind:"playlist"}]}
  ]
};
const token="89ABCDEF0123456789ABCDEF";
const requests=[];
function response(data){
  return {ok:true,status:200,headers:{get:key=>key==="X-Bridge-Token"?token:null},
    json:async()=>data};
}
const ctx=vm.createContext({
  document,window:{scrollTo:()=>{}},
  fetch:async(path,opts)=>{
    requests.push({path,opts});
    if(path==="/api/status")return response(status);
    if(path==="/api/effects/run")return response({ok:true,message:"Queued."});
    if(path==="/api/effects/add")return response({ok:true,requiresReboot:true,message:"Saved."});
    if(path.startsWith("/api/catalog"))return response({ok:true,presets:[
      {id:34,name:"Kitchen playlist",kind:"playlist"}]});
    return response({ok:true,message:"Done."});
  },
  URLSearchParams,Date,console,
  clearTimeout:()=>{},setTimeout:()=>0,setInterval:()=>0,
  confirm:()=>true,prompt:()=>null
});
vm.runInContext(source,ctx);
await new Promise(resolve=>setImmediate(resolve)); // initial async status render

assert.equal(get("lightCount").textContent,"2 / 8");
assert.equal(get("groupCount").textContent,"2 / 8");
assert.equal(get("presetCount").textContent,"1 / 12");
assert.equal(get("connection").textContent,"Bridge online");
assert(get("overviewDevices").innerHTML.includes("Shelf &amp; TV"),"Overview name escaped");
assert(!get("overviewDevices").innerHTML.includes("<script>"),"Overview must not inject HTML");
vm.runInContext('showView("effects")',ctx);
assert(!get("view-effects").hidden && get("view-overview").hidden,"Navigation switches visible panel");
assert.equal(get("effectsLight").value,"2","First configured Effects group should be selected initially");
assert(get("effectController").innerHTML.includes("Mammoth"),
  "Existing favorites should be the first content shown");
assert(!get("effectsAdd").open,
  "Add-favorite editor should start collapsed when an Effects group already exists");
assert(get("groupPicker").innerHTML.includes("Shelf &amp; TV"),"Tabs must escape WLED names");
vm.runInContext('switchEffectsLight("7")',ctx);
assert(get("effectController").innerHTML.includes("Kitchen playlist &amp; more"));
assert(get("effectController").innerHTML.includes("Running"),"Playlist must reflect WLED pl status");
assert(!get("effectController").innerHTML.includes("Step preset</div><div class=\"effect-kind\">Preset #15 · Running"),
  "Playlist child presets must not be shown active");
assert(!get("effectController").innerHTML.includes("<script>"),"Device names must be escaped");
assert(get("effectsLight").value==="7","Device selection persisted");
assert(get("effectsPreset").disabled,"Changing WLED light must clear stale saved resources");
assert(get("addEffect").disabled,"Cannot add before this light's catalog has loaded");
await vm.runInContext('loadCatalog("7","effectsPreset",document.getElementById("loadEffectsCatalog"))',ctx);
assert.equal(get("effectsPreset").dataset.loadedAid,"7");
assert(!get("effectsPreset").disabled);
assert(!get("addEffect").disabled);
assert(get("effectsCatalogHint").textContent.includes("1 saved items available"));
vm.runInContext('switchEffectsLight("2")',ctx);
assert(get("effectsPreset").disabled && get("addEffect").disabled,
  "Changing WLED light must invalidate previous device's catalog");
vm.runInContext('switchEffectsLight("7")',ctx);

await vm.runInContext('mutate("/api/effects/run",{aid:"9",preset:"34"})',ctx);
const run=requests.find(r=>r.path==="/api/effects/run");
assert(run,"Run effect API action missing");
assert.equal(run.opts.method,"POST");
assert.equal(run.opts.headers["X-Bridge-Token"],token);
assert.equal(run.opts.body.get("aid"),"9");
assert.equal(run.opts.body.get("preset"),"34");
assert.equal(get("restartBanner").hidden,true,"Run action must not require HomeKit restart");

await vm.runInContext('mutate("/api/effects/add",{aid:"7",preset:"34"})',ctx);
assert.equal(get("restartBanner").hidden,false,"Adding a control must request restart");
quickEls[1].listeners.click();
assert(!get("view-lights").hidden,"Quick add navigation works");
assert(get("addWled").open,"Quick add opens discovery");

console.log("v0.9.3 portal: default configured group, single device tabs, lazy catalog, retained selections and CSRF passed");

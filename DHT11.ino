#include <WiFi.h>
#include <WebServer.h>
#include <time.h>
#include "DHT.h"

// ---- EDIT THESE TWO LINES ----
const char* ssid     = "";
const char* password = "";
// ------------------------------

#define DHTPIN 4
#define DHTTYPE DHT11

#define HISTORY_SIZE  144          // 144 samples
#define SAMPLE_MS     600000UL     // one sample per 10 minutes -> 24 hours of history

DHT dht(DHTPIN, DHTTYPE);
WebServer server(80);

float tempC = 0;
float humidity = 0;
bool  haveReading = false;

float    tHist[HISTORY_SIZE];
float    hHist[HISTORY_SIZE];
uint32_t sHist[HISTORY_SIZE];      // unix timestamp of each sample
int      histCount = 0;
int      histHead  = 0;

unsigned long lastRead   = 0;
unsigned long lastSample = 0;

void readSensor() {
  if (millis() - lastRead < 2000) return;
  lastRead = millis();

  float h = dht.readHumidity();
  float t = dht.readTemperature();
  if (!isnan(h) && !isnan(t)) {
    humidity = h;
    tempC = t;
    haveReading = true;
  }
}

void recordSample() {
  if (millis() - lastSample < SAMPLE_MS) return;
  if (!haveReading) return;                  // don't log until the sensor has answered
  lastSample = millis();

  tHist[histHead] = tempC;
  hHist[histHead] = humidity;
  sHist[histHead] = (uint32_t)time(nullptr); // 0-ish if NTP hasn't synced
  histHead = (histHead + 1) % HISTORY_SIZE;
  if (histCount < HISTORY_SIZE) histCount++;
}

// ---------- JSON endpoint ----------
void handleData() {
  String j = "{";
  j += "\"now_t\":" + String(tempC, 1);
  j += ",\"now_h\":" + String(humidity, 0);
  j += ",\"step\":" + String(SAMPLE_MS / 1000);

  int start = (histCount < HISTORY_SIZE) ? 0 : histHead;

  j += ",\"T\":[";
  for (int i = 0; i < histCount; i++) {
    if (i) j += ",";
    j += String(tHist[(start + i) % HISTORY_SIZE], 1);
  }
  j += "],\"H\":[";
  for (int i = 0; i < histCount; i++) {
    if (i) j += ",";
    j += String(hHist[(start + i) % HISTORY_SIZE], 0);
  }
  j += "],\"S\":[";
  for (int i = 0; i < histCount; i++) {
    if (i) j += ",";
    j += String(sHist[(start + i) % HISTORY_SIZE]);
  }
  j += "]}";

  server.send(200, "application/json", j);
}

// ---------- the page ----------
const char PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESP32 Room Monitor</title>
<style>
  :root{--bg:#111;--panel:#1b1b1b;--fg:#eee;--dim:#888;--temp:#ff7043;--hum:#42a5f5}
  body{font-family:system-ui,sans-serif;background:var(--bg);color:var(--fg);
       margin:0;padding:22px 14px 40px;text-align:center}
  .row{display:flex;justify-content:center;gap:40px;flex-wrap:wrap}
  .l{color:var(--dim);font-size:12px;letter-spacing:2px}
  .v{font-size:46px;font-weight:700;line-height:1.1}
  .v.t{color:var(--temp)} .v.h{color:var(--hum)}
  .wrap{position:relative;max-width:760px;margin:26px auto 0}
  canvas{width:100%;display:block;background:var(--panel);border-radius:10px;touch-action:none}
  #tip{position:absolute;pointer-events:none;background:#000d;border:1px solid #444;
       border-radius:6px;padding:7px 10px;font-size:13px;text-align:left;
       white-space:nowrap;opacity:0;transition:opacity .1s}
  #tip b{font-weight:600}
  .n{color:#666;font-size:12px;margin-top:14px}
</style></head><body>

<div class="row">
  <div><div class="l">TEMPERATURE</div><div class="v t" id="nt">--</div></div>
  <div><div class="l">HUMIDITY</div><div class="v h" id="nh">--</div></div>
</div>

<div class="wrap">
  <canvas id="c"></canvas>
  <div id="tip"></div>
</div>
<div class="n" id="note">loading...</div>

<script>
var D=null, pts=[], M={l:48,r:48,t:14,b:30};
var c=document.getElementById('c'), x=c.getContext('2d'), tip=document.getElementById('tip');

function nice(lo,hi){
  if(hi-lo<1){var m=(lo+hi)/2;lo=m-0.5;hi=m+0.5;}
  var pad=(hi-lo)*0.18; return [lo-pad,hi+pad];
}
function clock(ep,i){
  if(ep>1600000000){var d=new Date(ep*1000);
    return ('0'+d.getHours()).slice(-2)+':'+('0'+d.getMinutes()).slice(-2);}
  var mins=Math.round((D.S.length-1-i)*D.step/60);
  return mins?('-'+mins+'m'):'now';
}

function size(){
  var r=Math.min(window.devicePixelRatio||1,2);
  var w=c.clientWidth, h=Math.max(240,Math.min(340,w*0.5));
  c.style.height=h+'px'; c.width=w*r; c.height=h*r;
  x.setTransform(r,0,0,r,0,0);
  return {w:w,h:h};
}

function draw(){
  var s=size(), W=s.w, H=s.h;
  x.clearRect(0,0,W,H);
  if(!D||D.T.length<1){x.fillStyle='#666';x.font='13px sans-serif';x.textAlign='center';
    x.fillText('waiting for the first samples',W/2,H/2);return;}

  var n=D.T.length;
  var tr=nice(Math.min.apply(null,D.T),Math.max.apply(null,D.T));
  var hr=nice(Math.min.apply(null,D.H),Math.max.apply(null,D.H));
  var x0=M.l,x1=W-M.r,y0=M.t,y1=H-M.b;

  function px(i){return n<2?(x0+x1)/2:x0+i*(x1-x0)/(n-1);}
  function py(v,r){return y1-((v-r[0])/(r[1]-r[0]))*(y1-y0);}

  // gridlines + both axis labels
  x.font='11px sans-serif'; x.lineWidth=1;
  for(var g=0;g<=4;g++){
    var yy=y0+g*(y1-y0)/4;
    x.strokeStyle='#2a2a2a'; x.beginPath(); x.moveTo(x0,yy); x.lineTo(x1,yy); x.stroke();
    var tv=tr[1]-g*(tr[1]-tr[0])/4, hv=hr[1]-g*(hr[1]-hr[0])/4;
    x.fillStyle='#ff7043aa'; x.textAlign='right'; x.fillText(tv.toFixed(1)+'\u00B0',x0-8,yy+4);
    x.fillStyle='#42a5f5aa'; x.textAlign='left';  x.fillText(Math.round(hv)+'%',x1+8,yy+4);
  }

  // time labels along the bottom
  x.fillStyle='#777'; x.textAlign='center';
  var step=Math.max(1,Math.ceil(n/6));
  for(var i=0;i<n;i+=step) x.fillText(clock(D.S[i],i),px(i),H-10);

  function line(arr,r,col){
    if(n<2){x.fillStyle=col;x.beginPath();x.arc(px(0),py(arr[0],r),3,0,7);x.fill();return;}
    x.strokeStyle=col; x.lineWidth=2; x.beginPath();
    for(var i=0;i<n;i++){var X=px(i),Y=py(arr[i],r); i?x.lineTo(X,Y):x.moveTo(X,Y);}
    x.stroke();
  }
  line(D.H,hr,'#42a5f5');
  line(D.T,tr,'#ff7043');

  pts=[]; for(var i=0;i<n;i++) pts.push({x:px(i),ty:py(D.T[i],tr),hy:py(D.H[i],hr),i:i});
  window._plot={x0:x0,x1:x1,y0:y0,y1:y1};
}

function hover(ev){
  if(!pts.length) return;
  var r=c.getBoundingClientRect();
  var mx=(ev.touches?ev.touches[0].clientX:ev.clientX)-r.left;
  var best=pts[0];
  for(var i=1;i<pts.length;i++) if(Math.abs(pts[i].x-mx)<Math.abs(best.x-mx)) best=pts[i];

  draw();
  var p=window._plot;
  x.strokeStyle='#ffffff55'; x.lineWidth=1; x.beginPath();
  x.moveTo(best.x,p.y0); x.lineTo(best.x,p.y1); x.stroke();
  x.fillStyle='#ff7043'; x.beginPath(); x.arc(best.x,best.ty,4,0,7); x.fill();
  x.fillStyle='#42a5f5'; x.beginPath(); x.arc(best.x,best.hy,4,0,7); x.fill();

  tip.innerHTML='<b>'+clock(D.S[best.i],best.i)+'</b><br>'+
                '<span style="color:#ff7043">'+D.T[best.i].toFixed(1)+' &deg;C</span><br>'+
                '<span style="color:#42a5f5">'+D.H[best.i]+' %</span>';
  tip.style.opacity=1;
  var tw=tip.offsetWidth;
  tip.style.left=Math.max(0,Math.min(c.clientWidth-tw,best.x-tw/2))+'px';
  tip.style.top='10px';
}
function leave(){tip.style.opacity=0;draw();}

c.addEventListener('mousemove',hover);
c.addEventListener('mouseleave',leave);
c.addEventListener('touchstart',function(e){e.preventDefault();hover(e);});
c.addEventListener('touchmove',function(e){e.preventDefault();hover(e);});
c.addEventListener('touchend',leave);
window.addEventListener('resize',draw);

function load(){
  fetch('/data').then(function(r){return r.json();}).then(function(d){
    D=d;
    document.getElementById('nt').textContent=d.now_t.toFixed(1)+' \u00B0C';
    document.getElementById('nh').textContent=Math.round(d.now_h)+' %';
    document.getElementById('note').textContent=
      d.T.length+' samples, one every '+Math.round(d.step/60)+' min'+
      (d.T.length?' \u2014 tap or drag across the chart to read a point':'');
    draw();
  }).catch(function(){});
}
load(); setInterval(load,30000);
</script></body></html>
)rawliteral";

void handleRoot() { server.send_P(200, "text/html", PAGE); }

void setup() {
  Serial.begin(115200);
  delay(1000);
  dht.begin();

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }

  Serial.println();
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");   // UTC; browser shows local time

  Serial.print("Connected. Open this address in your browser: http://");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();

  lastSample = millis() - SAMPLE_MS;   // take the first sample as soon as a reading arrives
}

void loop() {
  server.handleClient();
  readSensor();
  recordSample();
}

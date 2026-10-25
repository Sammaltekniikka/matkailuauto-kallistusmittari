// Renderöi kehykset: node gen.js <outdir>
const {chromium}=require('playwright');
(async()=>{
 const out=process.argv[2];
 const b=await chromium.launch();
 const p=await b.newPage({viewport:{width:240,height:160},deviceScaleFactor:2});
 await p.goto('file://'+__dirname+'/frame.html');
 const views=[['pitch','side.svg',176,99,-1],['roll','rear.svg',112,97,1]];
 for(const [name,img,w,h,sign] of views){
  for(let k=-30;k<=30;k++){
   await p.evaluate(([img,w,h,a])=>build(img,w,h,a),[img,w,h,sign*k]);
   await p.waitForFunction(()=>[...document.images].every(i=>i.complete));
   await p.screenshot({path:`${out}/${name}_${String(k+30).padStart(2,'0')}.png`,clip:{x:0,y:0,width:240,height:160}});
  }
 }
 await b.close();
})();

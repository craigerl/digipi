<!-- Version 2 17/10/2018 -->
var Main
var fromleft;

function initialize(mainoffset)
{
	var w=window,d=document,e=d.documentElement,g=d.getElementsByTagName('body')[0];
	x=w.innerWidth; //||e.clientWidth||g.clientWidth;
	y=w.innerHeight; //||e.clientHeight||g.clientHeight; 
	Main = document.getElementById("main");
	w = x;	
	if (w > 920) {w = 920;}
 	fromleft = (x / 2) - (x - 150)/2;
	if (fromleft < 0) {fromleft = 0;}
	Main.style.left = fromleft + "px";
	Main.style.width = x - 150 + "px";
	Main.style.height = y - mainoffset + "px";
}
function newmsg(Key)
{
var param = "toolbar=yes,location=yes,directories=yes,status=yes,menubar=yes,scrollbars=yes,resizable=yes,titlebar=yes,toobar=yes";
window.open("/WebMail/NewMsg?" + Key,"_self",param);
}
function Reply(Num, Key)
{
var param = "toolbar=yes,location=yes,directories=yes,status=yes,menubar=yes,scrollbars=yes,resizable=yes,titlebar=yes,toobar=yes";
window.open("/WebMail/Reply/" + Num + "?" + Key,"_self",param);
}
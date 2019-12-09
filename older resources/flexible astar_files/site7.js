function addEvent(obj, evType, fn) {
if (obj.addEventListener) {
obj.addEventListener(evType, fn, true);return true;} else if (obj.attachEvent) {
var r = obj.attachEvent("on"+evType, fn);return r;} else {
return false;}
}
function get_cookie(Name)
{
var search = Name + "="
var returnvalue = "";if (document.cookie.length > 0)
{
offset = document.cookie.indexOf(search)
if (offset != -1)
{
offset += search.length
end = document.cookie.indexOf(";", offset);if (end == -1) end = document.cookie.length;returnvalue=unescape(document.cookie.substring(offset, end))
}
}
return returnvalue;}
function GetElementsWithClassName(elementName,className)
{
var allElements = document.getElementsByTagName(elementName);var elemColl = new Array();for (i = 0;i< allElements.length;i++) {
if (isClass(allElements[i], className)) {
elemColl[elemColl.length] = allElements[i];}
}
return elemColl;}
function isClass(object, className)
{
return (object.className.search('(^|\\s)' + className + '(\\s|$)') != -1);}
function GetElementsWithClassName(elementName,className)
{
var allElements = document.getElementsByTagName(elementName);var elemColl = new Array();for (i = 0;i< allElements.length;i++) {
if (isClass(allElements[i], className)) {
elemColl[elemColl.length] = allElements[i];}
}
return elemColl;}
function sfHover() {
var sfEls = $$('#nav li');for (var i=0;i<sfEls.length;i++) {
sfEls[i].onmouseover = function() {
this.className+=" sfhover";};sfEls[i].onmouseout = function() {
this.className=this.className.replace(new RegExp(" *sfhover"), "");};}
}
Event.observe(window, 'load', sfHover, false);function openIRC(w, h) {
irc_w = w-80;irc_h = h-140;this.open("/help/help.php?page=ircChat&w="+irc_w+"&h="+irc_h, "IrcWindow", "toolbar=no,menubar=no,location=no,scrollbars=yes,resizable=yes,width="+w+",height="+h);}
function openSlideShow(url, w, h) {
this.open(url, "picWindow", "dependent=yes,toolbar=yes,location=yes,scrollbars=yes,resizable=yes,width="+w+",height="+h);}
HelpWin = -1;function openHelp(url, w, h) {
HelpWin = this.open(url, "AltWindow", "toolbar=no,menubar=no,location=no,scrollbars=yes,resizable=yes,width="+w+",height="+h);if (!HelpWin.opener) HelpWin.opener = self;}
function closeHelp() {
if (HelpWin != -1 && HelpWin.open) HelpWin.close();}function activateLinkTab(obj)
{
var group = $$("div.tabgroup")[obj.getAttribute('group')];var tabs = obj.parentNode.firstChild;while (tabs)
{
tabs.className = 'off';tabs = tabs.nextSibling;}
obj.className = 'on';group.setAttribute('selected', obj.getAttribute('tab'));}
function activateOverlayTab(obj)
{
var group = $$("div.tabgroup")[obj.getAttribute('group')];var tab = group.getElementsByTagName("DIV")[obj.getAttribute('tab')];var divs = group.firstChild;while (divs)
{
if (divs.nodeName == "DIV")
divs.style.display = 'none';divs = divs.nextSibling;}
tab.style.display = 'block';var tabs = obj.parentNode.firstChild;while (tabs)
{
tabs.className = 'off';tabs = tabs.nextSibling;}
obj.className = 'on';group.setAttribute('selected', obj.getAttribute('tab'));}
function processTabGroups()
{
restoreTabState();var group = $$("div.tabgroup");for(var i=0;i < group.length;i++)
{
var ul = group[i].firstChild;while (ul && ul.nodeName != "UL")
ul = ul.nextSibling;if (ul)
{
var selected = group[i].getAttribute('selected') ? group[i].getAttribute('selected') : 0;var type     = group[i].getAttribute('type') ? group[i].getAttribute('type') : 'overlay';var tab = 0;for(var j=0;j < ul.childNodes.length;j++)
{
if (ul.childNodes[j].nodeName == "LI")
{
ul.childNodes[j].setAttribute('group', i);ul.childNodes[j].setAttribute('tab', tab);if (type == "overlay")
ul.childNodes[j].onclick = function() { activateOverlayTab(this) };else
ul.childNodes[j].onclick = function() { activateLinkTab(this) };ul.childNodes[j].onmouseover = function() {
this.className+=" over";};ul.childNodes[j].onmouseout = function() {
this.className=this.className.replace(new RegExp(" *over"), "");};if (tab == selected)
{
if (type == "overlay")
activateOverlayTab(ul.childNodes[j]);else
activateLinkTab(ul.childNodes[j]);}
tab++;}
}
}
}
}
function saveTabState()
{
var data = new Array;var group = $$("div.tabgroup");var save = false;for(var i=0;i < group.length;i++)
{
save |= (group[i].getAttribute('type') == 'overlay');data[i] = group[i].getAttribute('selected');}
if (save)
{
var cookiename=window.location.pathname;document.cookie=cookiename+"="+data.join();}
}
function restoreTabState()
{
var loc = window.location.protocol+"//"+window.location.hostname+window.location.pathname;var state = get_cookie(window.location.pathname);state = state.split(",");var group = $$("div.tabgroup");for(var i=0;i < group.length;i++)
{
if (group[i].getAttribute('type') == 'overlay')
{
if (state[i])
group[i].setAttribute('selected', state[i]);else
group[i].setAttribute('selected', 0);}
else
{
var ul = group[i].getElementsByTagName("UL")[0];var li = ul.getElementsByTagName("LI");group[i].setAttribute('selected', 0);for(var j=0;j < li.length;j++)
{
var a = li[j].getElementsByTagName("A")[0];var pathname = (a.pathname.charAt(0) != "/") ? '/'+a.pathname : a.pathname;if (pathname == window.location.pathname)
{
group[i].setAttribute('selected', j);}
}
}
}
}
Event.observe(window, "load", processTabGroups, true);Event.observe(window, "unload", saveTabState, true);
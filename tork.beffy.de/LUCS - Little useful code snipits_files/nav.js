
function header(title)
{
	document.write(
    "<table width='100%' height='100%' border='0' cellspacing='0' cellpadding='0'> \
	<tr bgcolor=#e0e0e0> \
        <td valign='bottom' width ='*'><h1><nobr>"+title+"</nobr></h1></td> \
        <td align='right'><img src='../common/images/header.gif' width='240' height='60' border='0'></td> \
    </tr> \
    <tr>  \
        <td align='left' bgcolor='#0C3481'> \
        &nbsp;<a class=bar href='http://www.garagegames.com' target='_blank'>GarageGames.com</a> \
        </td> \
        <td align='right' bgcolor='#0C3481'> \
        <span class=bar><nobr>Doc Version 1.0.x&nbsp</nobr></span> \
        </td> \
    </tr> \
	<tr><td colspan='2' height='15'>&nbsp;</td></tr>" 
	);
}


function startBody()
{
	document.write("<tr valign='top'><td colspan='2' height='100%'>");
}

function endBody()
{
	document.write( "</td></tr>");
}



function footer(prev, next, authoremail)
{
	if(authoremail == null){
		authoremail = "";
	}
	else{
		authorname = authoremail.substring(0, authoremail.indexOf("@"));
		authoremail = "Author:<a class=bar href='mailto:"+authoremail+"'>"+authorname+"</a>";
	}
	if (prev == null) prev = "";
	else 			  prev = "<a class=bar href='"+prev+"'><< Previous</a>";
	if (next == null) next = "";
	else 			  next = "<a class=bar href='"+next+"'>Next >></a>";
	document.write(
	"<tr><td colspan='2' height='15'>&nbsp;</td></tr> \
	<tr><td colspan='2'>  \
	<table width='100%' border='0' cellspacing='0' cellpadding='0'> \
	<tr> \
        <td align='left' width='33%' bgcolor='#0C3481'> \
        &nbsp;"+prev+" \
        </td> \
        <td align='center' width='34%' bgcolor='#0C3481'> \
        <a class=bar href='http://tork.zenkel.com/modules.php?op=modload&name=ToRKTutorials&file=index'>Table of Contents</a>&nbsp; \
        </td> \
        <td align='center' width='16%' bgcolor='#0C3481' class=bar> \
        "+authoremail+"&nbsp; \
        </td> \
        <td align='right' width='17%' bgcolor='#0C3481'> \
        "+next+"&nbsp; \
        </td> \
	</tr> \
	</table> \
    </td></tr> \
    </table>"
	);
}


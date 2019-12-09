if( document.images )
{
	img1on = new Image();   img1on.src = "images/home_on.jpg"; 
	img1off = new Image();  img1off.src = "images/home_off.jpg"; 
	
	img2on = new Image();   img2on.src = "images/source_on.jpg"; 
	img2off = new Image();  img2off.src = "images/source_off.jpg"; 
	
	img3on = new Image();   img3on.src = "images/tools_on.jpg"; 
	img3off = new Image();  img3off.src = "images/tools_off.jpg"; 
	
	img4on = new Image();   img4on.src = "images/forums_on.jpg"; 
	img4off = new Image();  img4off.src = "images/forums_off.jpg"; 
	
	img5on = new Image();   img5on.src = "images/links_on.jpg"; 
	img5off = new Image();  img5off.src = "images/links_off.jpg"; 
}

function imgOn( imgName )
{
	if( document.images )
		document[imgName].src = eval( imgName + "on.src" );
}

function imgOff( imgName )
{
	if( document.images )
		document[imgName].src = eval( imgName + "off.src" );
}

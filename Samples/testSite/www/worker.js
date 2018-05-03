function doFrame(){
	
	//item.getStyle().setProperty("fill-opacity", currentValue);
	item.setAttribute("x", currentValue*1);
	item.setAttribute("x", currentValue*1);

	if( currentValue >= 300 ){	
		clearInterval(timerId);
		currentValue = 300;
		dir= 0;
	}

	if( currentValue <=0 ){	
		clearInterval(timerId);
		currentValue = 1;
		dir= 1;
	}

	if(dir == 1)
		currentValue++;
	else
		currentValue--;		
	
}

item = document.getElementById("anirect");

while(1){
	doFrame();
	var a = b;

}

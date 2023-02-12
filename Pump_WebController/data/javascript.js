   var enable = [
    { enableId: 'Enabled', enableName: 'ENABLED' },
    { enableId: 'Disabled', enableName: 'DISABLED' }
];

   var myTimerP1;
   var myTimerP2;
   var myTimerP3;
   var myTimerP4;
   
   var timerP1 = 0;
   var timerP2 = 0;
   var timerP3 = 0;
   var timerP4 = 0;
   
	function clock(e) {
		if (e=="cal1on")
		{
			myTimerP1 = setInterval(myClockP1, 100);
			document.getElementById("cal1off").style.display = "block";
		}
		else if (e=="cal2on")
		{
			myTimerP2 = setInterval(myClockP2, 100);
			document.getElementById("cal2off").style.display = "block";
		}
		else if (e=="cal3on")
		{
			myTimerP3 = setInterval(myClockP3, 100);
			document.getElementById("cal3off").style.display = "block";
		}
		else if (e=="cal4on")
		{
			myTimerP4 = setInterval(myClockP4, 100);
			document.getElementById("cal4off").style.display = "block";
		}
	}

	function myClockP1() {
			++timerP1;
			document.getElementById('captp1').textContent = timerP1/10;
	}
   
	function myClockP2() {
			++timerP2;
			document.getElementById('captp2').textContent = timerP2/10;
	}
	
	function myClockP3() {
			++timerP3;
			document.getElementById('captp3').textContent = timerP3/10;
	}
	
	function myClockP4() {
			++timerP4;
			document.getElementById('captp4').textContent = timerP4/10;
	}

   function toggleCheckbox(x) {
       var xhr = new XMLHttpRequest();
       xhr.open("GET", "/" + x, true);
       xhr.send();
	   if(x=="cal1on")
	   {
		   clock("cal1on");
		   document.getElementById("cal1on").style.display = "none";
		   document.getElementById("save1").style.display = "block";
	   }
	   else if(x=="cal1off")
	   {
		   clearInterval(myTimerP1);
		   document.getElementById("cal1off").style.display = "none";
		   document.getElementById("cal1on").style.display = "block";
	   }
	   else if(x=="cal2on")
	   {
		   clock("cal2on");
		   document.getElementById("cal2on").style.display = "none";
		   document.getElementById("save2").style.display = "block";
	   }
	   else if(x=="cal2off")
	   {
		   clearInterval(myTimerP2);
		   document.getElementById("cal2off").style.display = "none";
		   document.getElementById("cal2on").style.display = "block";
	   }
	   else if(x=="cal3on")
	   {
		   clock("cal3on");
		   document.getElementById("cal3on").style.display = "none";
		   document.getElementById("save3").style.display = "block";
	   }
	   else if(x=="cal3off")
	   {
		   clearInterval(myTimerP3);
		   document.getElementById("cal3off").style.display = "none";
		   document.getElementById("cal3on").style.display = "block";
	   }
	   else if(x=="cal4on")
	   {
		   clock("cal4on");
		   document.getElementById("cal4on").style.display = "none";
		   document.getElementById("save4").style.display = "block";
	   }
	   else if(x=="cal4off")
	   {
		   clearInterval(myTimerP4);
		   document.getElementById("cal4off").style.display = "none";
		   document.getElementById("cal4on").style.display = "block";
	   }
	   else if(x=="cal4off")
	   {
		   clearInterval(myTimerP4);
		   document.getElementById("cal4off").style.display = "none";
		   document.getElementById("cal4on").style.display = "block";
	   }
   };

/*   setInterval(function() {
       var xhttp = new XMLHttpRequest();
       xhttp.onreadystatechange = function() {
           if (this.readyState == 4 && this.status == 200) {
               document.getElementById("pump1runtime").innerHTML = this.responseText;
           }
       };
       xhttp.open("GET", "/pump1runtime", true);
       xhttp.send();
   }, 100);
   
   setInterval(function() {
       var xhttp = new XMLHttpRequest();
       xhttp.onreadystatechange = function() {
           if (this.readyState == 4 && this.status == 200) {
               document.getElementById("pump2runtime").innerHTML = this.responseText;
           }
       };
       xhttp.open("GET", "/pump2runtime", true);
       xhttp.send();
   }, 100);
   
   setInterval(function() {
       var xhttp = new XMLHttpRequest();
       xhttp.onreadystatechange = function() {
           if (this.readyState == 4 && this.status == 200) {
               document.getElementById("pump3runtime").innerHTML = this.responseText;
           }
       };
       xhttp.open("GET", "/pump3runtime", true);
       xhttp.send();
   }, 100);
   
   setInterval(function() {
       var xhttp = new XMLHttpRequest();
       xhttp.onreadystatechange = function() {
           if (this.readyState == 4 && this.status == 200) {
               document.getElementById("pump4runtime").innerHTML = this.responseText;
           }
       };
       xhttp.open("GET", "/pump4runtime", true);
       xhttp.send();
   }, 100);
*/
   function openPage(pageName) {
       var i, tabcontent, tablinks;
       tabcontent = document.getElementsByClassName("tabcontent");
       for (i = 0; i < tabcontent.length; i++) {
           tabcontent[i].style.display = "none";
       }
       tablinks = document.getElementsByClassName("tablink");
       for (i = 0; i < tablinks.length; i++) {
           tablinks[i].style.backgroundColor = "";
       }
       document.getElementById(pageName).style.display = "block";
   };

   if (!!window.EventSource) {
       var source = new EventSource('/events');

       source.addEventListener('open', function(e) {
           console.log("Events Connected");
       }, false);
       source.addEventListener('error', function(e) {
           if (e.target.readyState != EventSource.OPEN) {
               console.log("Events Disconnected");
           }
       }, false);

       source.addEventListener('message', function(e) {
           console.log("message", e.data);
       }, false);

       source.addEventListener('currentTime', function(e) {
           milliseconds = e.data * 1000 // 1575909015000
           dateObject = new Date(milliseconds)
           document.getElementById("currentDatetime").innerHTML = dateObject.toLocaleString("en-GB", { timeZoneName: "short" }) // 12/9/2019, 10:30:15 AM CST
       }, false);

       source.addEventListener('pump1nextDatetime', function(e) {
           milliseconds = e.data * 1000 // 1575909015000
           dateObject = new Date(milliseconds)
           document.getElementById("pump1nextDatetime").innerHTML = "Pump 1 next active: " + dateObject.toLocaleString("en-GB", { timeZoneName: "short" }) // 12/9/2019, 10:30:15 AM CST
       }, false);
       
       source.addEventListener('pump2nextDatetime', function(e) {
           milliseconds = e.data * 1000 
           dateObject = new Date(milliseconds)
           document.getElementById("pump2nextDatetime").innerHTML = "Pump 2 next active: " + dateObject.toLocaleString("en-GB", { timeZoneName: "short" }) // 12/9/2019, 10:30:15 AM CST
       }, false);
       
       source.addEventListener('pump3nextDatetime', function(e) {
           milliseconds = e.data * 1000
           dateObject = new Date(milliseconds)
           document.getElementById("pump3nextDatetime").innerHTML = "Pump 3 next active: " + dateObject.toLocaleString("en-GB", { timeZoneName: "short" }) // 12/9/2019, 10:30:15 AM CST
       }, false);
       
       source.addEventListener('pump4nextDatetime', function(e) {
           milliseconds = e.data * 1000
           dateObject = new Date(milliseconds)
           document.getElementById("pump4nextDatetime").innerHTML = "Pump 4 next active: " + dateObject.toLocaleString("en-GB", { timeZoneName: "short" }) // 12/9/2019, 10:30:15 AM CST
       }, false);
      
   }


   // Get the element with id="defaultOpen" and click on it
   document.getElementById("defaultOpen").click();


   function submitSettingMessage() {
       alert("Time zone settings saved");
       window.location.reload();
   };

   function submitProgramMessageP01() {
       alert("Pump 1 program saved");
       window.location.reload();
   };

   function submitProgramMessageP02() {
       alert("Pump 2 program saved");
       window.location.reload();
   };

   function submitProgramMessageP03() {
       alert("Pump 3 program saved");
       window.location.reload();
   };

   function submitProgramMessageP04() {
       alert("Pump 4 program saved");
       window.location.reload();
   };

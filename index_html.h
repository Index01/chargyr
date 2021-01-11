#include "Arduino.h"


const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
  <head>
  <title>Chargy Charger</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
        body { background-color: #5b6063; padding:10px; color: #97A7B3 }
		h1 { color: blue; }
        p { color: #97A7B3; padding:10px; margin-left:5px }
        .slider { -webkit-appearance: none; margin: 7px; width: 270px; height: 25px; background: #5b6063; outline: none; -webkit-transition: .2s; transition: opacity .2s;}
        .sliderRev { -webkit-appearance: none; direction: rtl; margin: 7px; width: 270px; height: 25px; background: #5b6063; outline: none; -webkit-transition: .2s; transition: opacity .2s;}
        .sparkline { -webkit-appearance: none; margin: 0px; width: 100%; height: 100px; display:block; background: #5b6063; outline: none; -webkit-transition: .2s; transition: opacity .9s;}
  </style>
  <script src="//cdnjs.cloudflare.com/ajax/libs/jquery/1.8.3/jquery.min.js"></script>
  <script src="//cdnjs.cloudflare.com/ajax/libs/jquery-sparklines/2.0.0/jquery.sparkline.min.js"></script>
  </head>
  <body>
  <div class="headHoldaOuta" style="margin:0 auto; width:95%; border-radius:15px;background-color: #474a4c;border:15px;">
    <div class="headHoldaInna" style="margin:0 auto; width: fit-content; border-radius:15px;background-color: #474a4c;border:15px;">
      <p>CHARGY!</p>
    </div>
  </div>
  <div class="formHoldaOuta" style="margin:0 auto; width:95%; border-radius:15px;background-color: #474a4c;border:15px;">
    <div class="formHoldaInna" style="margin:0 auto; width: fit-content; border-radius:15px;background-color: #474a4c;border:15px;">
      <p>
      <form action="/get" style="padding-left:10px">
          Add your network, so your device can do the cool lightup thing.<br>
          WiFi Name:<br> <input type="text" name="inSsid"><br>
          WiFi Password:<br> <input type="text" name="inPass"><br><br>
          Name your Chargyer. <br>Give this name to your friends and they can light up when you charge.<br>
          Device Name: <br><input type="text" name="inUname"><br><br>
          Ask your friends for their Chargy names and assign to colors here. <br>When they charge, you light up!<br>
          Red: <input type="text" name="inLed1"><br>
          Green: <input type="text" name="inLed2"><br>
          Blue: <input type="text" name="inLed3"><br>
          <br><input type="submit" value="Submit">
        </form>
        </p>
    </div>
  </div>
  <div class="slidaHoldaOuta" style="margin:0 auto; width:95%; border-radius:15px;background-color: #474a4c;border:10px;">
    <div class="slidaHoldaInna" style="margin:0 auto; width: fit-content; border-radius:15px;background-color: #474a4c;border:15px;">
        <script>
        function updateSliderPWM(element) {
          var valSlide = document.getElementById(element.id).value;
            // var valAmp = document.getElementById("amplitudeSlider").value;
          //document.getElementById("text_"+element.id).innerHTML = valSlide;
            //document.getElementById("textAmpSlide").innerHTML = valAmp;

          console.log(valSlide);
          var xhr = new XMLHttpRequest();
          xhr.open("POST", "/slider?in_"+element.id+"="+valSlide, true);
          xhr.send();
          upSpark();
        }
        </script>

        <script>
        function upSpark(){
            console.log("spark me up");
            let elem = document.getElementById("sparkline");
            let dataRed = elem.getAttribute("data-red");
            let dataGreen = elem.getAttribute("data-green");
            let dataBlue = elem.getAttribute("data-blue");
            $('.sparkline').sparkline(dataRed.split(','), {lineColor: "red",
                width: '100%',
                height: "100px",
                chartRangeMax: 127,
                chartRangeMaxX: 60,
                fillColor: "rgba(127, 0, 0, 0.3)",
                opacity: "0.2"});
            $('.sparkline').sparkline(dataGreen.split(','), {lineColor: "green",
                fillColor: "rgba(0, 127, 0, 0.3)",
                width: '100%',
                height: "100px",
                chartRangeMax: 127,
                chartRangeMaxX: 60,
                composite: true,
                opacity: "0.5"});
            $('.sparkline').sparkline(dataBlue.split(','), {lineColor: "blue",
                fillColor: "rgba(0, 0, 127, 0.3)",
                width: '100%',
                height: "100px",
                chartRangeMax: 127,
                chartRangeMaxX: 60,
                composite: true,
                opacity: "0.8"});
        }
        </script>

        <script>
        setInterval(function() {
          getLightPoints("data-red");
          getLightPoints("data-green");
          getLightPoints("data-blue");
        }, 3600);
        function getLightPoints(dataColor) {
          var xhttp = new XMLHttpRequest();
          xhttp.onreadystatechange = function() {
            if (this.readyState == 4 && this.status == 200) {
              let elem = document.getElementById("sparkline");
              elem.setAttribute(dataColor, this.responseText);
              upSpark();
            }
          };
          xhttp.open("GET", dataColor, true);
          xhttp.send();
        }
        </script>

        <p>
          <span class="sparkline" id="sparkline",
                data-red=
            "170,134,115,128,168,166,122,81,56,39,97,114,114,130,151,
            184,148,145,134,145,145,145,143,148,224,181,112,111,129,
            151,131,131,131,114,112,112,112,124,187,202,200,203,237,
            263,221,197,184,185,203,290,330,330,226,113,148,169,148,
            78,96,96,96,77,59,22,22,70,110,128",
                data-green=
                        "170,134,115,114,166,170,140,120,100,70,97,114,114,130,151,
            184,148,145,134,145,145,145,143,148,224,181,112,111,129,
            151,131,131,131,114,112,112,112,124,187,199,210,213,237,
            243,251,217,184,185,203,290,330,330,226,113,148,169,148,
            78,96,96,96,77,59,22,22,70,110,128",
                data-blue=
                        "170,134,115,128,168,166,122,81,56,39,97,114,114,130,151,
            184,148,145,134,145,145,145,143,148,224,181,112,111,129,
            151,131,131,131,134,142,152,162,174,187,202,210,223,237,
            253,261,247,224,185,203,290,330,330,226,113,148,169,148,
            78,96,96,96,77,59,62,72,90,110,100">
          </span>
        </p>
        <p>
            Amplitude: <input type="range" onchange="updateSliderPWM(this)" id="slideAmp" min="1" max="64" value="%SLIDEAMP%" step="1" class="slider"><br>
            Offset: <input type="range" onchange="updateSliderPWM(this)" id="slideOffset" min="0" max="64" value="%SLIDEOFFSET%" step="1" class="slider"><br>
            Freq Red: <input type="range" onchange="updateSliderPWM(this)" id="slideFreqRed" min="2" max="102" value="%SLIDEFREQRED%" step="1" class="sliderRev"><br>
            Freq Green: <input type="range" onchange="updateSliderPWM(this)" id="slideFreqGreen" min="2" max="102" value="%SLIDEFREQGREEN%" step="1" class="sliderRev"><br>
            Freq Blue: <input type="range" onchange="updateSliderPWM(this)" id="slideFreqBlue" min="2" max="102" value="%SLIDEFREQBLUE%" step="1" class="sliderRev"><br>

        </p>
        <p><a href="/updateIndex"><br>Update Firmware</a></p>
    </div>
  </div>
  </body></html>


)rawliteral";


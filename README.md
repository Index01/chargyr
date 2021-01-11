

# CHARGY! 

Welcome to the chargy project. Most boring wireless phone chargers light up when your device is charging, BORING. We already know when we are charging our devices. People of the future want to know when other people are charging! Name your chargyr, trade names with friends, get a sweet light show when someone is charging. 

<p align="center">
<img src="https://user-images.githubusercontent.com/3605312/104239526-7ccd3d00-540f-11eb-8e29-a50b94db1b3f.png" width=65%>
</p>




## Setup 

Chargyr has two modes, `APMode` - if the device is unconfigured or cannot join a specified network. `ClientMode` - when credentials are configured and the specified wifi is available
To configure the device for the first time, apply power and watch for three flashes of white light. You should then see a "Charge me up" network, join the network and cruise over to http://chargy.local

<p align="center">
<img src="https://user-images.githubusercontent.com/3605312/104242917-accb0f00-5414-11eb-9a24-f521e1b983ee.png" width=250>
</p>
* Some mobile devices have trouble resolving names. Try a laptop, or find the IP, or wait for the consumer friendly version to release. <br>

After the device is configured it should reboot and attempt to join the network. You can then rejoin your usual wifi, go to chargy.local, play with some light settings and associate friends' ids with RGB colors. When they chargy, you lighty.


## Technical stuffs
This project is based on an esp32 microcontroller and redis on the server side. Client code for the management page is a flat html document with some jquery and js to flavor. Project is in early development, stay tuned for more updates. 

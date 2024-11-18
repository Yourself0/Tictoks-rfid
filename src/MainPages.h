const char Mainpage[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>

<head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title></title>
    <link rel="stylesheet" type="text/css" href="/MainPageCss">
    <link rel="stylesheet" type="text/css" href="/styles.css">
    <link rel="stylesheet" type="text/css" href="/popupCss">
    <link rel="icon" href="data:;base64,iVBORw0KGgo=">
    <script src="/quickSetting.js"></script>
                 

    <style type="text/css" id="stylesheets">
        circle#circle {
            fill: red;
        }

        path#bottomLayer {
            fill: red;
        }

        path#middleLayer {
            fill: red;
        }

        path#topLayer {
            fill: red;
        }

        circle#circles {
            fill: green;
        }

        path#bottomLayers {
            fill: green;
        }

        path#middleLayers {
            fill: green;
        }

        path#topLayers {
            fill: green;
        }

        circle#notavailable {
            fill: white;
            stroke: black;
            stroke-width: 5;
        }

        path#bottomLayern {
            fill: white;
            stroke: black;
            stroke-width: 8;
        }

        path#middleLayern {
            fill: white;
            stroke: black;
            stroke-width: 8;
        }

        path#topLayern {
            fill: white;
            stroke: black;
            stroke-width: 8;
        }

        text#wifisvg {
            fill: rgba(255, 0, 0, 0.904);
            stroke: rgb(0, 0, 0);
            font-weight: bolder;
            stroke-width: 2;
        }
    </style>
</head>

<script>
    function wifiPage() {
        var modal = document.getElementById('myModal');
        modal.style.display = 'block';
        document.getElementById("button1").addEventListener("click", validation);
        function validation() {
            location.replace("/WifiSetting")
        }
    }

    function CompanyPage() {
        var modal = document.getElementById('myModal');
        modal.style.display = 'block';
        document.getElementById("button1").addEventListener("click", validation);
        function validation() {
            location.replace("/CompanySetting")
        }
    }

    function Reset() {
        var modal3 = document.getElementById('myModal3');
        var modal = document.getElementById('myModal');
        modal.style.display = 'block';
        document.getElementById("button1").addEventListener("click", validation);
        function validation() {
            modal3.style.display = 'block';
            modal.style.display = 'none';


        }
    }

    function ResetFetch() {
        fetch('/Resets')
            .then(response => {
                if (!response.ok) {
                    console.error('Network response was not ok');
                }
                return response.text();
            })
            .then(data => {
            })
            .catch(error => {
                console.error('Error triggering Arduino function:', error);
            });
        location.replace("/")
    }

    function RFID() {
        var modal = document.getElementById('myModal');
        modal.style.display = 'block';
        document.getElementById("button1").addEventListener("click", validation);
        function validation() {
            location.replace("/RFID")
        }
    }

    function Update() {

        var modal = document.getElementById('myModal');
        var modal2 = document.getElementById('myModal2');
        modal.style.display = 'block';
        document.getElementById("button1").addEventListener("click", validation);
        function validation() {
            <!--/Update-->
            fetch('/Update')
                .then(response => {
                    if (!response.ok) {
                        throw new Error('Network response was not ok');
                    }
                    return response.text();
                })
                .then(data => {
                })
                .catch(error => {
                    console.error('Error triggering Arduino function:', error);

                });
            modal2.style.display = 'block';
            modal.style.display = 'none';
            window.close();
            // alert("It will take some time for synchronizing");
        }

    }

    function Restart() {

        var modal = document.getElementById('myModal');
        var modal2 = document.getElementById('myModal2');
        modal.style.display = 'block';

        document.getElementById("button1").addEventListener("click", validation);

        function validation() {
            fetch('/Restart')
                .then(response => {
                    if (!response.ok) {
                        throw new Error('Network response was not ok');
                    }
                    return response.text();

                })
                .then(data => {
                })
                .catch(error => {
                    console.error('Error triggering Arduino function:', error);

                });
                location.replace("/")
        }
    }

    function PopupCancel() {
        location.replace("/")
    }

    function HomePage() {
        location.replace("/")
    }

    function PassCheck() {
        var passwordElement = document.getElementById("txtPassword");
        var vpasswordElement = document.getElementById("txtConfirmPassword");
        var submitButton = document.getElementById("button1");

        if (passwordElement && vpasswordElement && submitButton) {
            var password = passwordElement.value;
            var vpassword = vpasswordElement.value;

            submitButton.disabled = password.length === 0 || password !== vpassword;
        }
    }
</script>
<body>
    <div class="toolBar-main">
        <h3 class="toolBar-header-main" onclick="HomePage()">RFID</h3>
    </div>
    <div class="container main-card">
        <div class="card">
            <div class="indicator_wifi">
                <div></div>
                <img src="/TictokLogo" class="LogoImage">
                <div id="svgContainers"></div>
            </div>
            <div class="container card-row">
                <div class="row">
                    <div class="col-md-4">
                        <div class="card1" onclick="QuickSettings()">
                            <img src="/QuickSettings" class="cardImg">
                            <h3>Quick Setup</h3>
                        </div>
                    </div>
                    <div class="col-md-4">
					<div class="card1" onclick="wifiPage()">
						<img src="/wifiImg" class="cardImg">
						<h3>Wifi Setting</h3>
					</div>
				</div>
                    <div class="col-md-4">
                        <div class="card2" onclick="CompanyPage()">
                            <img src="/companyImg" class="cardImg">
                            <h3>Company Setting</h3>
                        </div>
                    </div>
                    <div class="col-md-4">
                        <div class="card3" onclick="Reset()">
                            <img src="/resetImg" class="cardImg">
                            <h3>Reset</h3>
                        </div>
                    </div>
                </div>
            </div>
            <div class="container card-row">
                <div class="row">
                <!-- Commented RFID -->
                    <div class="col-md-4">
                        <div class="card4" onclick="RFID()">
                            <img src="/RFIDImg" class="cardImg">
                            <h3>RFID Register</h3>
                        </div>
                    </div>
                    <div class="col-md-4">
                        <div class="card5" onclick="Update()">
                            <img src="/updateImg" class="cardImg">
                            <h3>Sync</h3>
                        </div>.
                    </div>
                    <div class="col-md-4">
                        <div class="card6" onclick="Restart()">
                            <img src="/restartImg" class="cardImg">
                            <h3>Restart</h3>
                        </div>
                    </div>
                </div>
            </div>
        </div>
    </div>


    <!-- ************POPUP MODAL************** -->

    <div id="myModal" class="modal">
        <div class="modal-content">
            <!-- <span class="close">&times;</span> -->
            <div class="popupTool">
                <h3>Security Code</h3>
            </div>
            <div class="popImg">
                <img src="/popImg" class="LogoImage">
            </div>
            <div class="popupDiv" id="form">
                <label for="password">Password</label>
                <input type="password" placeholder="" id="txtPassword" name="pswd1" class="form-control"
                    onkeyup="PassCheck()"></input></br>
                <label for="password">Confirm Password</label>
                <span style="color:red;" id="error1"></span>
                <input type="password" placeholder="" id="txtConfirmPassword" name="pswd2" class="form-control"
                    onkeyup="PassCheck()"></input>
                <div class="buttonBtn">
                    <button type="button" id="button1" class="btn btn-primary" id="btSubmit" disabled>Submit</button>
                    <button type="reset" class="btn btn-primary btn_cancel" id="cancelbutton" value="Reset"
                        onclick="PopupCancel()">Cancel</button>
                </div>
            </div>

        </div>
    </div>


    <!-- ************RESET MODAL************** -->
    <div id="myModal3" class="modal3">
        <div class="modal-content">
            <!-- <span class="close" onclick="PopupCancel()">&times;</span> -->
            <div class="resetTool">
                <h3>Confirmation</h3>
            </div>
            <div class="popImg">
                <img src="/deleteImg" class="ResetImage">
            </div>
            <div class="popupDiv" id="form">
                <h3 class="resetText">Are you sure, Do you want to reset?</h3>
            </div>
            <div class="buttonClass">
                <button type="button" id="button1" class="btn btn-primary btn-submit ResetBtn" id="btSubmit"
                    onclick="ResetFetch()">Reset</button>
                <button type="reset" class="btn btn-primary btn_cancel ResetBtn2" id="cancelbutton" value="Reset"
                    onclick="PopupCancel()">Cancel</button>
            </div>
        </div>
    </div>


    <!-- ***********UPDATE SUCCESS MODAL************* -->
     <div id="myModal2" class=" modal2">
        <div class="modal-content_main">
               

            <div class="UpdateTool">
 <span class="close" onclick="PopupCancel()"   >&times;</span>
                <h3>Update</h3>
            </div>
            <div class="popImg">
                <img src="/successImg" class="SuccessImg">
            </div>
            <div class="popupDiv" id="form">
                <h3 class="UpdateText">Please wait.. It will take time for synchronizing</h3>
            </div>
        </div>
    </div>
    <script>
        var WifiStatus = new WebSocket('ws://' + window.location.hostname + '/Status');
        const wifiNotConnected = `
                                    <svg fill="#000000" height="10px" width="20px" version="1.1" id="Capa_1" viewBox="0 0 365.892 365.892" xml:space="preserve">
                                        <g>
                                            <circle  id = "circle" cx="182.945" cy="286.681" r="41.494"/>
                                           <path id='bottomLayer' d="M182.946,176.029c-35.658,0-69.337,17.345-90.09,46.398c-5.921,8.288-4.001,19.806,4.286,25.726
                                                c3.249,2.321,6.994,3.438,10.704,3.438c5.754,0,11.423-2.686,15.021-7.724c13.846-19.383,36.305-30.954,60.078-30.954
                                                c23.775,0,46.233,11.571,60.077,30.953c5.919,8.286,17.437,10.209,25.726,4.288c8.288-5.92,10.208-17.438,4.288-25.726
                                                C252.285,193.373,218.606,176.029,182.946,176.029z"/>
                                            <path id='middleLayer' d="M182.946,106.873c-50.938,0-99.694,21.749-133.77,59.67c-6.807,7.576-6.185,19.236,1.392,26.044
                                                c3.523,3.166,7.929,4.725,12.32,4.725c5.051-0.001,10.082-2.063,13.723-6.116c27.091-30.148,65.849-47.439,106.336-47.439
                                                s79.246,17.291,106.338,47.438c6.808,7.576,18.468,8.198,26.043,1.391c7.576-6.808,8.198-18.468,1.391-26.043
                                                C282.641,128.621,233.883,106.873,182.946,106.873z"/>
                                            <path id='topLayer' d="M360.611,112.293c-47.209-48.092-110.305-74.577-177.665-74.577c-67.357,0-130.453,26.485-177.664,74.579
                                                c-7.135,7.269-7.027,18.944,0.241,26.079c3.59,3.524,8.255,5.282,12.918,5.281c4.776,0,9.551-1.845,13.161-5.522
                                                c40.22-40.971,93.968-63.534,151.344-63.534c57.379,0,111.127,22.563,151.343,63.532c7.136,7.269,18.812,7.376,26.08,0.242
                                                C367.637,131.238,367.745,119.562,360.611,112.293z"/>                                            
                                            <!-- Diagonal line -->
                                            <svg width="450" height="330" xmlns="http://www.w3.org/2000/svg">
                                                <line x1="0" y1="0" x2="400" y2="400" stroke="black" stroke-width="20px" id='lineStroke'/>
                                            </svg>
                                        </g>
                                    </svg>
                                    `;

        const wifiConnected = `
        <svg fill="#000000" height="10px" width="20px" version="1.1" id="Capa_1" viewBox="0 0 365.892 365.892" xml:space="preserve">
                                        <g>
                                            <circle id='circles' cx="182.945" cy="286.681" r="41.494"/>
                                            <path id='bottomLayers' d="M182.946,176.029c-35.658,0-69.337,17.345-90.09,46.398c-5.921,8.288-4.001,19.806,4.286,25.726
                                                c3.249,2.321,6.994,3.438,10.704,3.438c5.754,0,11.423-2.686,15.021-7.724c13.846-19.383,36.305-30.954,60.078-30.954
                                                c23.775,0,46.233,11.571,60.077,30.953c5.919,8.286,17.437,10.209,25.726,4.288c8.288-5.92,10.208-17.438,4.288-25.726
                                                C252.285,193.373,218.606,176.029,182.946,176.029z"/>
                                            <path id='middleLayers' d="M182.946,106.873c-50.938,0-99.694,21.749-133.77,59.67c-6.807,7.576-6.185,19.236,1.392,26.044
                                                c3.523,3.166,7.929,4.725,12.32,4.725c5.051-0.001,10.082-2.063,13.723-6.116c27.091-30.148,65.849-47.439,106.336-47.439
                                                s79.246,17.291,106.338,47.438c6.808,7.576,18.468,8.198,26.043,1.391c7.576-6.808,8.198-18.468,1.391-26.043
                                                C282.641,128.621,233.883,106.873,182.946,106.873z"/>
                                            <path id='topLayers' d="M360.611,112.293c-47.209-48.092-110.305-74.577-177.665-74.577c-67.357,0-130.453,26.485-177.664,74.579
                                                c-7.135,7.269-7.027,18.944,0.241,26.079c3.59,3.524,8.255,5.282,12.918,5.281c4.776,0,9.551-1.845,13.161-5.522
                                                c40.22-40.971,93.968-63.534,151.344-63.534c57.379,0,111.127,22.563,151.343,63.532c7.136,7.269,18.812,7.376,26.08,0.242
                                                C367.637,131.238,367.745,119.562,360.611,112.293z"/>
                                        </g>
                                    </svg>
                                    `;
        const InternetNotAvailable = `
       <svg class='topLayer'fill="#000000" height="10px" width="20px" version="1.1" id="Capa_1" viewBox="0 0 365.892 365.892" xml:space="preserve">
                                        <g>
                                            <circle cx="182.945" cy="286.681" r="41.494"/>
                                            <path id='bottomLayern' d="M182.946,176.029c-35.658,0-69.337,17.345-90.09,46.398c-5.921,8.288-4.001,19.806,4.286,25.726
                                                c3.249,2.321,6.994,3.438,10.704,3.438c5.754,0,11.423-2.686,15.021-7.724c13.846-19.383,36.305-30.954,60.078-30.954
                                                c23.775,0,46.233,11.571,60.077,30.953c5.919,8.286,17.437,10.209,25.726,4.288c8.288-5.92,10.208-17.438,4.288-25.726
                                                C252.285,193.373,218.606,176.029,182.946,176.029z"/>
                                            <path id='middleLayern' d="M182.946,106.873c-50.938,0-99.694,21.749-133.77,59.67c-6.807,7.576-6.185,19.236,1.392,26.044
                                                c3.523,3.166,7.929,4.725,12.32,4.725c5.051-0.001,10.082-2.063,13.723-6.116c27.091-30.148,65.849-47.439,106.336-47.439
                                                s79.246,17.291,106.338,47.438c6.808,7.576,18.468,8.198,26.043,1.391c7.576-6.808,8.198-18.468,1.391-26.043
                                                C282.641,128.621,233.883,106.873,182.946,106.873z"/>
                                            <path id='topLayern' d="M360.611,112.293c-47.209-48.092-110.305-74.577-177.665-74.577c-67.357,0-130.453,26.485-177.664,74.579
                                                c-7.135,7.269-7.027,18.944,0.241,26.079c3.59,3.524,8.255,5.282,12.918,5.281c4.776,0,9.551-1.845,13.161-5.522
                                                c40.22-40.971,93.968-63.534,151.344-63.534c57.379,0,111.127,22.563,151.343,63.532c7.136,7.269,18.812,7.376,26.08,0.242
                                                C367.637,131.238,367.745,119.562,360.611,112.293z"/>
                                        </g>
                                        <text id="wifisvg"x="500" y="100" font-family="Arial" font-size="300" rotate="180"text-anchor="middle">i</text>
                                    </svg>
                                    `;
        WifiStatus.onmessage = function (event) {
            var data = event.data;
            const svgContainer = document.getElementById('svgContainers');
            if (data === 'Connected') {
                if (data === 'Connected' && data !== 'Internet Not Available') {
                    svgContainer.innerHTML = wifiConnected;
                }

            }
            if (data === 'Internet Not Available') {
                svgContainer.innerHTML = InternetNotAvailable;
            }
            if (data === 'Not Connected') {
                svgContainer.innerHTML = wifiNotConnected;
            }
        }
        WifiStatus.onclose = function (event) {
            window.location.reload(); // Reload the page
        }
        

        function QuickSettings() {

        var modal = document.getElementById('myModal');
        modal.style.display = 'block';
        document.getElementById("button1").addEventListener("click", validation);
        function validation() {
            quickSettings();


        }

    }
    </script>
</body>


</html>
)=====";

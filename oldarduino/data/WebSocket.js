
//var connection = new WebSocket('ws://'+location.hostname+':81/', ['arduino']);
var connection = new WebSocket('ws://192.168.10.35:81/', ['arduino']); // Debug from local computer
connection.onopen = function () {
    connection.send('Connect ' + new Date());
};
connection.onerror = function (error) {
    console.log('WebSocket Error ', error);
};
connection.onmessage = function (e) {  
    console.log('Server: ', e.data);
    //console.log('Test:', e.data.charCodeAt(5));
    if ( e.data.charAt(0) == 'S' ) {
      if (e.data == "SaveOk") {
        console.log("Save was succesfull.");
      } else {
        console.log("Save fail.");
      }
    }
    if ( e.data.charAt(5) == ':' ) {  
      var timeInit = e.data.substr(6);
      timerManModeOut.value = timeInit;
      timerManMode.value = timeInit;
      console.log('Timer init:', timeInit);
    }
};
connection.onclose = function(){
    console.log('WebSocket connection closed');
};

function saveEEPROM() {
    var timerManMode = document.getElementById('timerManMode').value;
    var saveStr = 'save' + timerManMode.toString()
    console.log(saveStr);	
    connection.send(saveStr);
    console.log('SaveEEPROM send');	
}

function search() {
  connection.send('scan');
  console.log("Search send"); 
}


function restart() {
  connection.send('restart');
  console.log("Send restart request"); 
}